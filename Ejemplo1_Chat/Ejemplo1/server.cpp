#include "utils.h"
#include <iostream>
#include <string>
#include <thread>
#include <list>
#include "clientManager.h"
using namespace std;

int main(int argc, char** argv)
{
	bool salir=false;
	cout<<"Hola mundo desde server\n";

	int serverID=initServer(1250);//dirección localhost
	//por cada cliente
	while(!salir){
		//esperar conexion
		while(!checkClient()) usleep(100);
		cout<<"Cliente conectado\n";
		int clientID=getLastClientID();
		thread* th=new thread(clientManager::atiendeCliente,clientID);
	    //atiendeCliente(clientID);
	}
	close(serverID);
	return 0;
}







