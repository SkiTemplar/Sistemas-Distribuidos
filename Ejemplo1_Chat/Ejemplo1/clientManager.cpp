#include "clientManager.h"
#include <iostream>
#include <thread>

// ===== util común =====
static void packString(std::vector<unsigned char>& buf, const std::string& s){
    pack<long int>(buf, (long int)s.size());
    if (!s.empty()) packv(buf, s.data(), (int)s.size());
}
static std::string unpackString(std::vector<unsigned char>& buf){
    std::string s;
    s.resize(unpack<long int>(buf));
    if (!s.empty()) unpackv(buf, (char*)s.data(), (int)s.size());
    return s;
}
std::string clientManager::desempaquetaTipoTexto(std::vector<unsigned char>& b){ return unpackString(b); }

// ===== CLIENTE =====
void clientManager::enviaLogin(int id, const std::string& user){
    std::vector<unsigned char> buf;
    pack<int>(buf, login);
    packString(buf, user);
    sendMSG(id, buf);
    // Esperar ACK
    for(int i = 0; i < 100 && bufferAcks.size() == 0; i++) usleep(10000);
    if(bufferAcks.size() > 0){
        std::lock_guard<std::mutex> lk(cerrojoBuffers);
        (void)unpack<int>(bufferAcks);
        bufferAcks.clear();
    }
}

void clientManager::enviaLogout(int id, const std::string& user){
    std::vector<unsigned char> buf;
    pack<int>(buf, exit_);
    packString(buf, user);
    sendMSG(id, buf);
    // Esperar ACK del logout
    for(int i = 0; i < 50 && bufferAcks.size() == 0; i++) usleep(10000);
    if(bufferAcks.size() > 0){
        std::lock_guard<std::mutex> lk(cerrojoBuffers);
        (void)unpack<int>(bufferAcks);
        bufferAcks.clear();
    }
}

void clientManager::enviaMensajePublico(int id, const std::string& user, const std::string& msg){
    std::vector<unsigned char> buf;
    pack<int>(buf, texto);
    packString(buf, user);
    packString(buf, msg);
    sendMSG(id, buf);
    // Esperar ACK
    for(int i = 0; i < 100 && bufferAcks.size() == 0; i++) usleep(10000);
    if(bufferAcks.size() > 0){
        std::lock_guard<std::mutex> lk(cerrojoBuffers);
        (void)unpack<int>(bufferAcks);
        bufferAcks.clear();
    }
}

void clientManager::enviaMensajePrivado(int id, const std::string& from, const std::string& to, const std::string& msg){
    std::vector<unsigned char> buf;
    pack<int>(buf, priv_);
    packString(buf, from);
    packString(buf, to);
    packString(buf, msg);
    sendMSG(id, buf);
    // Esperar ACK
    for(int i = 0; i < 100 && bufferAcks.size() == 0; i++) usleep(10000);
    if(bufferAcks.size() > 0){
        std::lock_guard<std::mutex> lk(cerrojoBuffers);
        (void)unpack<int>(bufferAcks);
        bufferAcks.clear();
    }
}

// ===== SERVIDOR =====
void clientManager::reenviaPublico(const std::string& from, const std::string& msg){
    std::vector<unsigned char> out;
    pack<int>(out, texto);
    packString(out, from);
    packString(out, msg);
    for (auto& kv : connectionIds) {
        if(kv.second >= 0) sendMSG(kv.second, out);
    }
}

void clientManager::reenviaPrivado(const std::string& to, const std::string& from, const std::string& msg){
    auto it = connectionIds.find(to);
    if (it == connectionIds.end()) {
        std::cout<<"[SERVIDOR] Usuario " << to << " no encontrado para mensaje privado de " << from << "\n";
        return;
    }
    std::vector<unsigned char> out;
    pack<int>(out, texto);
    packString(out, from + " (privado)");
    packString(out, msg);
    sendMSG(it->second, out);
    std::cout<<"[SERVIDOR] Mensaje privado de " << from << " a " << to << "\n";
}

void clientManager::atiendeCliente(int clientID){
    std::vector<unsigned char> in;
    bool salir=false;
    std::string user="desconocido";

    while(!salir && !cierreDePrograma){
        recvMSG<unsigned char>(clientID, in);
        if (in.empty()){
            usleep(10000);
            continue;
        }

        int type = unpack<int>(in);

        switch(type){
            case login:{
                user = unpackString(in);
                std::cout<<"[SERVIDOR] Login de usuario: " << user << "\n";
                if (!connectionIds.count(user)) {
                    connectionIds[user] = clientID;
                    std::cout<<"[SERVIDOR] Usuario " << user << " conectado (ID: " << clientID << ")\n";
                } else {
                    std::cout<<"[SERVIDOR] Usuario " << user << " ya existe, rechazando conexión\n";
                    salir = true;
                }
            }break;

            case texto:{
                std::string from = unpackString(in);
                std::string msg  = unpackString(in);
                std::cout<<"[SERVIDOR] Mensaje público de " << from << ": " << msg << "\n";
                reenviaPublico(from, msg);
            }break;

            case priv_:{
                std::string from = unpackString(in);
                std::string to   = unpackString(in);
                std::string msg  = unpackString(in);
                std::cout<<"[SERVIDOR] Mensaje privado de " << from << " a " << to << ": " << msg << "\n";
                reenviaPrivado(to, from, msg);
            }break;

            case exit_:{
                std::string exitUser = unpackString(in);
                std::cout<<"[SERVIDOR] Logout de usuario: " << exitUser << "\n";
                connectionIds.erase(exitUser);
                salir = true;
            }break;

            default:{
                std::cout<<"[SERVIDOR] Tipo de mensaje desconocido: " << type << "\n";
                connectionIds.erase(user);
                salir = true;
            }break;
        }

        in.clear();
        // Enviar ACK
        std::vector<unsigned char> ackbuf;
        pack<int>(ackbuf, ack);
        sendMSG(clientID, ackbuf);
    }

    // Limpiar al salir
    connectionIds.erase(user);
    std::cout<<"[SERVIDOR] Cliente " << user << " (ID: " << clientID << ") desconectado\n";
    closeConnection(clientID);
}
