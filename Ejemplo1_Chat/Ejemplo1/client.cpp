#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define INVALID_SOCKET -1
#endif

#ifdef _WIN32
void init_sockets(){ WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); }
void cleanup_sockets(){ WSACleanup(); }
void close_socket(sock_t s){ closesocket(s); }
#else
void init_sockets(){}
void cleanup_sockets(){}
void close_socket(sock_t s){ close(s); }
#endif

ssize_t sock_recv(sock_t s, char* buf, size_t len){
#ifdef _WIN32
  return recv(s, buf, (int)len, 0);
#else
  return recv(s, buf, len, 0);
#endif
}

ssize_t sock_send(sock_t s, const char* buf, size_t len){
#ifdef _WIN32
  return send(s, buf, (int)len, 0);
#else
  return send(s, buf, len, 0);
#endif
}

int main(int argc, char** argv){
    if (argc < 3){ std::cerr << "Uso: client <host> <port>\n"; return 1; }
    std::string host = argv[1];
    int port = std::stoi(argv[2]);

    init_sockets();

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET){ std::cerr << "No se pudo crear socket\n"; return 1; }

    sockaddr_in serv;
    std::memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serv.sin_addr);

    if (connect(s, (sockaddr*)&serv, sizeof(serv)) != 0){
        std::cerr << "No se pudo conectar\n";
        close_socket(s);
        return 1;
    }

    std::atomic<bool> running(true);

    std::thread reader([&](){
        char buf[1024];
        while(running){
            ssize_t r = sock_recv(s, buf, sizeof(buf));
            if (r <= 0) break;
            std::string msg(buf, buf + r);
            std::cout << msg;
        }
        running = false;
    });

    std::string line;
    while(running && std::getline(std::cin, line)){
        if (line.empty()) continue;
        std::string tosend = line + "\n";
        sock_send(s, tosend.c_str(), tosend.size());
        if (line == "exit") break;
    }

    running = false;
    close_socket(s);
    if (reader.joinable()) reader.join();
    cleanup_sockets();
    return 0;
}

