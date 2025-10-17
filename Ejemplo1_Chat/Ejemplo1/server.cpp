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

static std::atomic<bool> running(true);

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

void handle_client(sock_t client){
    char buf[1024];
    while(running){
        ssize_t r = sock_recv(client, buf, sizeof(buf));
        if (r <= 0) break;
        std::string msg(buf, buf + r);
        while(!msg.empty() && (msg.back()=='\n' || msg.back()=='\r')) msg.pop_back();
        if (msg == "exit") break;
        std::string out = "Server: " + msg + "\n";
        sock_send(client, out.c_str(), out.size());
    }
    close_socket(client);
    std::cout << "[Cliente desconectado]\n";
}

int main(int argc, char** argv){
    std::string host = "0.0.0.0";
    int port = 54000;

    // Uso: server <puerto> o server <host> <puerto>
    if (argc == 2) {
        port = std::stoi(argv[1]);
    } else if (argc >= 3) {
        host = argv[1];
        port = std::stoi(argv[2]);
    }

    init_sockets();

    sock_t listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET){
        std::cerr << "No se pudo crear socket\n";
        return 1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(listen_sock, (sockaddr*)&addr, sizeof(addr));
    listen(listen_sock, 5);

    std::cout << "Servidor escuchando en puerto " << port << "\n";

    while(running){
        sockaddr_in client_addr;
        socklen_t ca_len = sizeof(client_addr);
        sock_t client = accept(listen_sock, (sockaddr*)&client_addr, &ca_len);
        if (client == INVALID_SOCKET) continue;
        std::cout << "Cliente conectado\n";
        std::thread t(handle_client, client);
        t.detach();
    }

    close_socket(listen_sock);
    cleanup_sockets();
    return 0;
}
