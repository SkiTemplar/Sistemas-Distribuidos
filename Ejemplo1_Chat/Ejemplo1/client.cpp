#include "utils.h"
#include <string>
#include <iostream>
#include <string>
#include <vector>
#include "clientManager.h"

using namespace std;

//thread que muestra mensajes de otros usuarios
void leerMensajeTextoExternos(int id)
{
	string txt="";
	string user="";
	//mientras no cerrar program
	while(!clientManager::cierreDePrograma){
	
		//mirar buffer mensajes de texto
		while(clientManager::bufferTxt.size()==0) usleep(100); //espera semidurmiente
		//si hay mensaje
			//desempaquetar
			//mostrarlo
		clientManager::cerrojoBuffers.lock();
		user=clientManager::desempaquetaTipoTexto(clientManager::bufferTxt);
		txt=clientManager::desempaquetaTipoTexto(clientManager::bufferTxt);
		clientManager::cerrojoBuffers.unlock();
		cout<<user<<" dice:"<<txt<<"\n";
	}
	cout<<"Cierre de hilo recepcion de mensajes de clientes\n";
}


void recibePaquetesAsync(int id)
{
	vector<unsigned char> buffer;
	//mientras no cerrar program
	while(!clientManager::cierreDePrograma){
	
	//recibir paquete
		recvMSG(id,buffer);
		clientManager::msgTypes tipo=unpack<clientManager::msgTypes>(buffer);
		clientManager::cerrojoBuffers.lock(); //cerrar para rellenar buffers en privado
		//almacenar paquete
		switch(tipo){
			case clientManager::ack:
				pack(clientManager::bufferAcks,clientManager::ack);
				break;
			case clientManager::texto:
			{
				string user=clientManager::desempaquetaTipoTexto(buffer);
				string txt=clientManager::desempaquetaTipoTexto(buffer);
				pack(clientManager::bufferTxt,user.size());
				packv(clientManager::bufferTxt,user.data(),user.size());
				pack(clientManager::bufferTxt,txt.size());
				packv(clientManager::bufferTxt,txt.data(),txt.size());
			}break;
			default:
			{
				ERRLOG("Mensaje recibido no válido\n");
				
			}break;
		}
		clientManager::cerrojoBuffers.unlock();
		
	//repetir
	}
	cout<<"Cierre de hilo recepcion de paquetes\n";
}

 

void chat(int serverId, string userName)
{
 bool salir= false;
 string mensajeLeido;
 string mensajesRecibidos;
 //pedir nombre de usuario
 cout<<"Cliente conectado, introduzca usuario \n";
 cin>>userName;	
 //mientras no salir
 //enviar login
 clientManager::enviaLogin(serverId, userName);

 while(!salir){
	 //pedir mensaje a usuario
	cout<<"Introduzca el mensaje para el servidor\n";
	//leerlo
	getline(cin,mensajeLeido);
	salir=(mensajeLeido=="salir");//TODO
	 //si no es salir
	 
	if(!salir){		 
		//enviar mensaje
		clientManager::enviaMensaje(serverId,mensajeLeido);
	}
 }
 
}
 
int main(int argc,char** argv)
{
	string nombreUsuario="";
	cout<<"Inicio conexión cliente\n";
	auto serverConnID=initClient("127.0.0.1",1250);

	//crear hilos de gestión de mensajes:
	thread* th=new thread(leerMensajeTextoExternos,serverConnID.serverId);
	thread* th2=new thread(recibePaquetesAsync,serverConnID.serverId);

	chat(serverConnID.serverId, nombreUsuario);
	
	closeConnection(serverConnID.serverId);
	return 0;
}
