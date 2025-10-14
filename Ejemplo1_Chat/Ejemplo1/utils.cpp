#include "utils.h"
#include <list>
#include <mutex>

std::map<unsigned int,connection_t> clientList;
static unsigned int g_counter = 0;
static std::atomic<bool> g_exit{false};
static std::list<unsigned int> g_waiting;
static std::mutex g_mx;

// Función para permitir el cierre del servidor desde el exterior
void setServerExit() { g_exit = true; }

int initServer(int port){
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv{}; serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(port);

    if (bind(sock_fd, (sockaddr*)&serv, sizeof(serv)) < 0){ perror("bind"); return -1; }
    if (listen(sock_fd, 16) < 0){ perror("listen"); return -1; }

    std::thread(waitForConnectionsAsync, sock_fd).detach();
    return sock_fd;
}

void waitForConnectionsAsync(int server_fd){
    while(!g_exit){
        int result = waitForConnections(server_fd);
        if(result < 0) {
            usleep(100000); // Esperar si hay error de accept()
        }
    }
}

int waitForConnections(int sock_fd){
    sockaddr_in cli{}; socklen_t len = sizeof(cli);

    // Hacer accept no bloqueante o con timeout
    struct timeval tv;
    tv.tv_sec = 1;  // 1 segundo de timeout
    tv.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int newsock = accept(sock_fd, (sockaddr*)&cli, &len);
    if (newsock < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return -1; // Timeout, no hay conexión pendiente
        }
        return -1;
    }

    connection_t c{};
    {
        std::lock_guard<std::mutex> lk(g_mx);
        c.id = g_counter++;
    }
    c.alive = true; c.socket = newsock; c.buffer = new std::list<msg_t*>();
    clientList[c.id] = c;

    {
        std::lock_guard<std::mutex> lk(g_mx);
        g_waiting.push_back(c.id);
    }
    return newsock;
}

bool checkClient(){
    std::lock_guard<std::mutex> lk(g_mx);
    return !g_waiting.empty();
}

int getLastClientID(){
    std::lock_guard<std::mutex> lk(g_mx);
    if(g_waiting.empty()) return -1;
    int id = g_waiting.back();
    g_waiting.pop_back();
    return id;
}

int getNumClients(){
    std::lock_guard<std::mutex> lk(g_mx);
    return (int)clientList.size();
}

void closeConnection(int clientID){
    std::lock_guard<std::mutex> lk(g_mx);
    if (!clientList.count(clientID)) return;
    connection_t c = clientList[clientID];
    if (c.socket >= 0) close(c.socket);
    c.alive = false;
    if (c.buffer){
        for (auto* m : *c.buffer){ delete[] m->data; delete m; }
        delete c.buffer;
    }
    clientList.erase(clientID);
}

// ===== raw framed I/O =====
template<typename T>
void recvMSG(int clientID, std::vector<T>& data){
    std::lock_guard<std::mutex> lk(g_mx);
    if (!clientList.count(clientID) || !clientList[clientID].alive) {
        data.clear();
        return;
    }

    int sock = clientList[clientID].socket;
    if (sock < 0) {
        data.clear();
        return;
    }

    int bytes = 0;
    int r = read(sock, &bytes, sizeof(int));
    if (r<=0 || bytes<=0){
        data.clear();
        return;
    }

    int n = bytes / (int)sizeof(T);
    data.resize(n);
    int remaining = bytes;
    char* dst = (char*)data.data();
    while(remaining>0){
        int got = read(sock, dst + (bytes-remaining), remaining);
        if (got<=0) break;
        remaining -= got;
    }
    if (remaining!=0) data.clear();
}

template<typename T>
void sendMSG(int clientID, std::vector<T>& data){
    std::lock_guard<std::mutex> lk(g_mx);
    if (!clientList.count(clientID) || !clientList[clientID].alive) {
        return;
    }

    int sock = clientList[clientID].socket;
    if (sock < 0) return;

    int bytes = (int)(data.size()*sizeof(T));
    int sent = write(sock, &bytes, sizeof(int));
    if (sent <= 0) return;

    if (bytes>0) {
        sent = write(sock, data.data(), bytes);
        if (sent <= 0) return;
    }
}

// explicit instantiations
template void recvMSG<unsigned char>(int, std::vector<unsigned char>&);
template void sendMSG<unsigned char>(int, std::vector<unsigned char>&);

template<typename T>
void getMSG(int, std::vector<T>&) { /* no usado en esta versión */ }
