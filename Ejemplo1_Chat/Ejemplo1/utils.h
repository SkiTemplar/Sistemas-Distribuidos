#ifndef  _UTILS_H_
#define  _UTILS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

typedef struct msg_t { int size; unsigned char* data; } msg_t;

typedef struct connection_t{
    unsigned int id;            // id interno
    unsigned int serverId;      // no usado fuera
    int socket;                 // fd del socket
    std::list<msg_t*>* buffer;  // cola opcional
    bool alive;                 // vivo
} connection_t;

// === API servidor/cliente ===
int initServer(int port);
int waitForConnections(int sock_fd);
void waitForConnectionsAsync(int server_fd);
bool checkClient();
int getLastClientID();
int getNumClients();
void closeConnection(int clientID);
void setServerExit(); // Nueva función para permitir el cierre del servidor

connection_t initClient(std::string host, int port);

// === envío/recepción cruda por tamaño + payload ===
template<typename T> void sendMSG(int clientID, std::vector<T>& data);
template<typename T> void recvMSG(int clientID, std::vector<T>& data);
template<typename T> void getMSG(int clientID, std::vector<T>& data);

// === utilidades pack/unpack ===
template<typename T> inline void pack(std::vector<unsigned char>& packet, T data){
    int size = packet.size();
    unsigned char* ptr = (unsigned char*)&data;
    packet.resize(size + sizeof(T));
    std::copy(ptr, ptr + sizeof(T), packet.begin() + size);
}
template<typename T> inline void packv(std::vector<unsigned char>& p, T* data, int n){
    for(int i=0;i<n;i++) pack(p, data[i]);
}
template<typename T> inline T unpack(std::vector<unsigned char>& packet){
    T data; long int n = sizeof(T); int ps = packet.size();
    memcpy(&data, packet.data(), n);
    memcpy(packet.data(), packet.data()+n, ps-n);
    packet.resize(ps-n);
    return data;
}
template<typename T> inline void unpackv(std::vector<unsigned char>& p, T* data, int n){
    int bytes = n*sizeof(T); int ps = p.size();
    memcpy(data, p.data(), bytes);
    memcpy(p.data(), p.data()+bytes, ps-bytes);
    p.resize(ps-bytes);
}

// === estado global conexiones (simple) ===
extern std::map<unsigned int,connection_t> clientList;

#endif
