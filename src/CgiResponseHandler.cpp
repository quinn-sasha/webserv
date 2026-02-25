#include "CgiResponseHandler.hpp"

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cctype>
#include <map>

#include "Server.hpp"

CgiResponseHandler::CgiResponseHandler(int cgi_fd, int client_fd, 
                                       pid_t cgi_pid)
    : cgi_fd_(cgi_fd),
      client_fd_(client_fd),
      cgi_pid_(cgi_pid),
      bytes_sent_(0),
      cgi_finished_(false)
      {}

CgiResponseHandler::~CgiResponseHandler() {
  if (cgi_fd_ != -1) {
    close(cgi_fd_);
  }
  
  // CGIプロセスが生きていたら終了待機
  if (cgi_pid_ > 0) {
    int status;
    if (waitpid(cgi_pid_, &status, WNOHANG) == 0) {
      waitpid(cgi_pid_, &status, 0);
    }
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      std::cerr << "CGI exited with status " << WEXITSTATUS(status) << "\n";
    }
    if (WIFSIGNALED(status)) {
      std::cerr << "CGI killed by signal " << WTERMSIG(status) << "\n";
    }
  }
}

HandlerStatus CgiResponseHandler::handle_input() {
  char buffer[4096];
  ssize_t n = read(cgi_fd_, buffer, sizeof(buffer));
  
  if (n == -1) {
    return kClosed;
  }
  
  if (n == 0) {
    // CGI出力受信完了
    cgi_finished_ = true;
    
    // CGIプロセスの終了コードを確認
    bool cgi_error = false;
    if (cgi_pid_ > 0) {
      int status;
      pid_t wpid = waitpid(cgi_pid_, &status, 0);
      if (wpid == cgi_pid_) {
        if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) || WIFSIGNALED(status)) {
          cgi_error = true;
          std::cerr << "CGI Error: Process terminated abnormally.\n";
        }
        cgi_pid_ = -1; 
      }
    }

    if (cgi_error) {
      send_buffer_ = "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<h1>502 Bad Gateway</h1>";
    } else {
      send_buffer_ = parse_cgi_output(cgi_output_);
    }
    
    bytes_sent_ = 0;
    return kCgiReceived;
  }
  
  cgi_output_.append(buffer, n);
  return kContinue;
}

HandlerStatus CgiResponseHandler::handle_output() {
  if (cgi_fd_ != -1) {
    close(cgi_fd_);
    cgi_fd_ = -1;
  }
  
  if (send_buffer_.empty()) {
    return kClosed;
  }
  
  ssize_t remaining = send_buffer_.size() - bytes_sent_;
  ssize_t n = send(client_fd_, send_buffer_.data() + bytes_sent_, 
                   remaining, 0);
  
  if (n == -1) {
    return kClosed;
  }
  
  bytes_sent_ += n;
  
  if (bytes_sent_ < static_cast<ssize_t>(send_buffer_.size())) {
    return kContinue;
  }
  
  return kAllSent;
}

HandlerStatus CgiResponseHandler::handle_poll_error() {
  std::cerr << "Error: poll error on CGI fd\n";
  return kClosed;
}

static std::string key_to_lower(const std::string& str) {
  std::string lower_str = str;
  for (std::size_t i = 0; i < lower_str.size(); ++i) {
    lower_str[i] = std::tolower(static_cast<unsigned char>(lower_str[i]));
  }
  return lower_str;
}

std::string CgiResponseHandler::parse_cgi_output(const std::string& cgi_output) {
  // ヘッダーとボディを分離
  std::size_t body_start = cgi_output.find("\r\n\r\n");
  if (body_start == std::string::npos) {
    body_start = cgi_output.find("\n\n");
    if (body_start == std::string::npos) {
      return "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<h1>502 Bad Gateway</h1>";
    }
    body_start += 2;
  } else {
    body_start += 4;
  }
  
  std::string headers_str = cgi_output.substr(0, body_start);
  std::string body = cgi_output.substr(body_start);
  
  std::map<std::string, std::string> headers_map;
  std::istringstream headers_stream(headers_str);
  std::string line;
  std::string status_line = ""; // CGIが明示的に指定したStatusを保持する
  
  while (std::getline(headers_stream, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') {
      line.erase(line.size() - 1);
    }
    if (line.empty()) continue;
    
    std::size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);
      
      // Trim leading spaces from value
      std::size_t value_start = value.find_first_not_of(" \t");
      if (value_start != std::string::npos) {
        value = value.substr(value_start);
      } else {
        value = "";
      }
      
      std::string lower_key = key_to_lower(key);
      if (lower_key == "status") {
        status_line = value;
      } else {
        // Location ヘッダーの値を保存（後でリダイレクト判定に使用）
        headers_map[lower_key] = value;
      }
    }
  }
  
  bool has_content_type = headers_map.count("content-type");
  bool has_location = headers_map.count("location");

  // Content-Type または Location のどちらかは必須
  if (!has_content_type && !has_location) {
    std::cerr << "CGI Error: Missing both Content-Type and Location headers.\n";
    return "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<h1>502 Bad Gateway</h1>";
  }

  // ステータスラインの決定
  if (status_line.empty()) {
    if (has_location) {
      // Client Redirect / Local Redirect Response
      status_line = "302 Found";
    } else {
      // Document Response
      status_line = "200 OK";
    }
  }

  std::ostringstream response;
  response << "HTTP/1.1 " << status_line << "\r\n";
  
  // 保存したヘッダーを出力
  for (std::map<std::string, std::string>::iterator it = headers_map.begin(); 
       it != headers_map.end(); ++it) {
    // mapのキーは小文字なので、出力時は適切な形式にする必要があるが
    // ここでは簡易的に元のキーを復元せず、一般的な形式で出力する
    // (本来は元のキーを保存しておくのが理想)
    std::string key = it->first;
    // 先頭とハイフンの後を大文字にする簡易的な整形
    if (!key.empty()) {
      key[0] = std::toupper(key[0]);
      for (size_t i = 1; i < key.size(); ++i) {
        if (key[i-1] == '-') key[i] = std::toupper(key[i]);
      }
    }
    response << key << ": " << it->second << "\r\n";
  }
  
  // Content-Length や Transfer-Encoding が指定されていなければ自動付与
  if (headers_map.find("content-length") == headers_map.end() &&
      headers_map.find("transfer-encoding") == headers_map.end()) {
    response << "Content-Length: " << body.size() << "\r\n";
  }
  
  response << "\r\n" << body;
  return response.str();
}
