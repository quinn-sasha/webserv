#include "CgiEnv.hpp"

#include <cstring>

CgiEnv::CgiEnv() {}
CgiEnv::~CgiEnv() {}

CgiEnv::CgiEnv(const CgiEnv& other) : env_(other.env_) {}

CgiEnv& CgiEnv::operator=(const CgiEnv& other) {
  if (this != &other) {
    env_ = other.env_;
  }
  return *this;
}

void CgiEnv::set(const std::string& key, const std::string& value) {
  env_[key] = value;
}

std::string CgiEnv::get(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = env_.find(key);
  if (it == env_.end()) return "";
  return it->second;
}

bool CgiEnv::has(const std::string& key) const {
  return env_.find(key) != env_.end();
}

char** CgiEnv::to_envp() const {
  char** envp = new char*[env_.size() + 1];
  std::size_t i = 0;

  for (std::map<std::string, std::string>::const_iterator it = env_.begin();
       it != env_.end(); ++it) {
    std::string entry = it->first + "=" + it->second;
    envp[i] = new char[entry.size() + 1];
    std::strcpy(envp[i], entry.c_str());
    ++i;
  }
  envp[i] = NULL;
  return envp;
}

void CgiEnv::free_envp(char** envp) {
  for (std::size_t i = 0; envp[i] != NULL; ++i) {
    delete[] envp[i];
  }
  delete[] envp;
}