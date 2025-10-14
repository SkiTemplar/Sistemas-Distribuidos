#include "utils.h"
#include "clientManager.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

static std::atomic<bool> running{true};
static int g_sock = -1;
static std::string g_user;

static void reader_thread(int id){
    while(running){
        std::vector<unsigned char> buf;
        recvMSG<unsigned char>(id, buf);
        if (buf.empty()) { usleep(10000); continue; }
        int type = unpack<int>(buf);

        if (type == clientManager::texto){
            std::string from = clientManager::desempaquetaTipoTexto(buf);
            std::string txt  = clientManager::desempaquetaTipoTexto(buf);
            std::cout<<from<<" dice: "<<txt<<"\n";
        } else if (type == clientManager::shutdown_){
            (void)clientManager::desempaquetaTipoTexto(buf); // from
            (void)clientManager::desempaquetaTipoTexto(buf); // txt
            std::cout<<"[Servidor apagándose]\n";
            running = false;
        } else if (type == clientManager::ack){
            // Almacenar el ACK para que las funciones de envío puedan procesarlo
            std::lock_guard<std::mutex> lk(clientManager::cerrojoBuffers);
            clientManager::bufferAcks.clear();
            pack<int>(clientManager::bufferAcks, type);
        }
        // ACK implícito del servidor ya gestionado por él
    }
}

static void sigint_handler(int){
    if (g_sock>=0){
        std::cout<<"\n[Enviando logout...]\n";
        clientManager::enviaLogout(g_sock, g_user);
    }
    running = false;
}

int main(int argc, char** argv){
    // Uso: client <host> <port>
    if (argc<3){ std::cerr<<"Uso: client <host> <port>\n"; return 1; }
    std::string host = argv[1]; int port = std::stoi(argv[2]);

    std::signal(SIGINT, sigint_handler);

    std::cout<<"Cliente conectado, introduzca usuario:\n";
    std::getline(std::cin, g_user);
    if (g_user.empty()) g_user = "user";

    auto conn = initClient(host, port);
    if (conn.socket<0){ std::cerr<<"No se pudo conectar\n"; return 1; }
    g_sock = conn.id;

    clientManager::enviaLogin(conn.id, g_user);

    std::thread th(reader_thread, conn.id);

    std::cout<<"Comandos disponibles:\n";
    std::cout<<"  /pm <usuario> <mensaje> - Enviar mensaje privado\n";
    std::cout<<"  /logout - Desconectarse\n";
    std::cout<<"  Cualquier otro texto - Mensaje público\n\n";

    std::string line;
    while(running && std::getline(std::cin, line)){
        if (line.empty()) continue;

        if (line == "/logout"){
            std::cout<<"Desconectando...\n";
            clientManager::enviaLogout(conn.id, g_user);
            running = false;
            break;
        } else if (line.rfind("/pm ",0)==0){
            auto sp = line.find(' ',4);
            if (sp==std::string::npos){
                std::cout<<"Uso: /pm <destinatario> <mensaje>\n";
                continue;
            }
            std::string to = line.substr(4, sp-4);
            std::string txt = line.substr(sp+1);
            if (to.empty() || txt.empty()){
                std::cout<<"Error: destinatario o mensaje vacío\n";
                continue;
            }
            std::cout<<"[Mensaje privado a " << to << "]\n";
            clientManager::enviaMensajePrivado(conn.id, g_user, to, txt);
        } else {
            clientManager::enviaMensajePublico(conn.id, g_user, line);
        }
    }

    running = false;
    if (th.joinable()) th.join();
    closeConnection(conn.id);
    return 0;
}
