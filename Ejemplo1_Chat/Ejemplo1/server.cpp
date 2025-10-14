#include "utils.h"
#include "clientManager.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

static std::atomic<bool> shutting_down{false};

static void sigint_handler(int){
    std::cout<<"\n[SERVIDOR] Recibida señal de interrupción (Ctrl+C)\n";
    shutting_down = true;
    clientManager::cierreDePrograma = true;
}

static void broadcast_shutdown(){
    std::cout<<"[SERVIDOR] Enviando mensaje de apagado a todos los clientes...\n";
    std::vector<unsigned char> buf;
    pack<int>(buf, clientManager::shutdown_);
    // mensaje de cortesía
    std::string from="server", txt="El servidor se está apagando. Desconectando...";
    pack<long int>(buf, (long int)from.size());
    if(!from.empty()) packv(buf, from.data(), (int)from.size());
    pack<long int>(buf, (long int)txt.size());
    if(!txt.empty())  packv(buf, txt.data(),  (int)txt.size());

    // Copiar el mapa para evitar problemas de concurrencia
    std::map<std::string, int> clients_copy = clientManager::connectionIds;
    for (auto& kv : clients_copy){
        if(kv.second >= 0) {
            sendMSG(kv.second, buf);
            std::cout<<"[SERVIDOR] Mensaje de apagado enviado a " << kv.first << "\n";
        }
    }
}

int main(int argc, char** argv){
    // Uso: server [host ignored] <port>
    int port = 1250;
    if (argc>=2) port = std::stoi(argv[argc-1]);

    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);

    std::cout<<"[SERVIDOR] Servidor escuchando en 0.0.0.0:"<<port<<"\n";
    std::cout<<"[SERVIDOR] Presiona Ctrl+C para apagar el servidor de forma segura\n";

    int serverID = initServer(port);
    if (serverID<0){
        std::cerr<<"[SERVIDOR] No se pudo iniciar servidor\n";
        return 1;
    }

    while(!shutting_down){
        while(!checkClient() && !shutting_down) usleep(100000); // 100ms
        if (shutting_down) break;

        int clientID = getLastClientID();
        std::cout<<"[SERVIDOR] Cliente conectado con ID: "<<clientID<<"\n";
        std::thread(clientManager::atiendeCliente, clientID).detach();
    }

    // Ctrl+C: notificar y cierre cuando no queden clientes
    std::cout<<"[SERVIDOR] Iniciando secuencia de apagado...\n";
    broadcast_shutdown();

    // Espera a que los clientes se desconecten (máximo 10 segundos)
    std::cout<<"[SERVIDOR] Esperando a que los clientes se desconecten...\n";
    for (int i=0; i<100 && getNumClients()>0; i++) {
        usleep(100000); // 100ms
        if(i % 10 == 0 && getNumClients() > 0) {
            std::cout<<"[SERVIDOR] Esperando... Clientes restantes: " << getNumClients() << "\n";
        }
    }

    if(getNumClients() > 0) {
        std::cout<<"[SERVIDOR] Forzando cierre con " << getNumClients() << " clientes aún conectados\n";
    }

    std::cout<<"[SERVIDOR] Cerrando servidor\n";
    close(serverID);
    return 0;
}
