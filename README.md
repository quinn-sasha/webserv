This project has been created as part of the 42 curriculum by squinn, ikota, skimura.

## Description

Webserv is a high-performance HTTP server built from scratch in C++98. The goal of this project is to deeply understand the mechanics of network protocols and the client-server model by implementing a functional web server capable of handling real-world browser requests.

**The server is fully compliant with HTTP/1.0 and provides partial support for HTTP/1.1**, focusing on core features required for efficient data communication. By utilizing non-blocking I/O with `poll()` (or similar multiplexing), the architecture is designed to manage multiple concurrent connections without performance degradation.

## Instructions

Clone the repository.

```bash
git clone https://github.com/quinn-sasha/webserv.git
cd webserv
```

Compile and run the server.

```bash
make
./webserv config/default.conf
```

Server’s IP address is [localhost](http://localhost) and port number is 8080.

Start sending request after entering telnet:

```bash
telnet localhost 8080
```

## Resources

- https://nafuka.hatenablog.com/entry/2022/04/14/194200
- https://qiita.com/ryhara/items/c46fe320332b237b5c0d
- https://www.ohmsha.co.jp/book/9784274065194/
- https://christina04.hatenablog.com/entry/io-multiplexing
- https://www.oreilly.co.jp//books/9784873115856/
- https://tex2e.github.io/rfc-translater/html/rfc9112.html
- https://tex2e.github.io/rfc-translater/html/rfc3875.html
- https://www.rfc-editor.org/rfc/rfc9110
- https://gihyo.jp/admin/serial/01/eng_knowhow/0018
- https://maku.blog/p/ugkqy8z/
