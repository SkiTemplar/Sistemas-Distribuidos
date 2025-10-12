#include "clientManager.h"
#include "utils.h"




void clientManager::enviaMensaje(int id, string mensaje)
{
	vector<unsigned char> buffer; //para crear un paquete de datos
	//empaquetar tipo
	pack(buffer,texto);
	//empaquetar datos
		//empaquetar tamaño de string
	pack(buffer,mensaje.size());
			//empaquetar datos de string
	packv(buffer,mensaje.data(),mensaje.size());
		//enviar datos
	sendMSG(id,buffer);
	//recibir respuesta
		//limpiar buffer
	//consultar ack
	while(bufferAcks.size()==0) usleep(100); //espera semidurmiente
	//leer ack
	cerrojoBuffers.lock();
	if(unpack<int>(bufferAcks)!= ack)
		cout<<"Error enviando mensaje\n";
	cerrojoBuffers.unlock();
}

string clientManager::desempaquetaTipoTexto(vector<unsigned char> &buffer){

	string mensaje;
	//para crear un paquete de datos
	mensaje.resize(unpack<long int>(buffer));
	unpackv(buffer,(char*)mensaje.data(),mensaje.size());
	return mensaje;
}

void clientManager::enviaLogin(int id, string userName){

	//buffer datos
	vector<unsigned char> buffer;
	//empaquetar tipo de mensaje
	pack(buffer,login);
	//empaquetar metadato
	pack(buffer,userName.size());
	//dato
	packv(buffer, userName.data(),userName.size());
	//enviar
	sendMSG(id,buffer);
	//consultar ack
	while(bufferAcks.size()==0) usleep(100); //espera semidurmiente
	//leer ack
	cerrojoBuffers.lock();
	if(unpack<int>(bufferAcks)!= ack)
		cout<<"Error enviando mensaje\n";
	cerrojoBuffers.unlock();

}


void clientManager::atiendeCliente(int clientID)
{
	vector<unsigned char> bufferIn;
	bool salir=false;
	string userName="default";
	while(!salir){
		//recibe paquete datos
		recvMSG(clientID,bufferIn);
		//desempaquetar tipo paquete
		msgTypes type=unpack<msgTypes>(bufferIn);
		//dependiendo de tipo
		switch(type){
			//tipo texto
			case texto:{
				//desempaquetar mensaje
				string msg=desempaquetaTipoTexto(bufferIn);
				//reenviar
				reenviaTexto(userName,msg);
			}break;
			//tipo exit
			case exit:{
				//eliminar usuario
				connectionIds.erase(userName);
				//cerrar conexión
				salir=true;
			}break;//tipo login
			case login:{
				//desempaquetar usuario
				userName=desempaquetaTipoTexto(bufferIn);
				//añadir si no existe
				if(connectionIds.find(userName)==connectionIds.end())
					connectionIds[userName]=clientID;
				else
					salir=true;
			}break;
			default:{
			//cualquier otro tipo
				ERRLOG ("tipo mensaje no válido");
				//eliminar usuario
				connectionIds.erase(userName);
				//cerrar conexión
				salir=true;
			}break;
		};

		//limpiar buffer
		bufferIn.clear();
		//enviar ack
		pack(bufferIn,ack);
		sendMSG(clientID,bufferIn);
	}
	closeConnection(clientID);
}

void clientManager::reenviaTexto(string userName, string msg)
{
	//empaquetar mensaje
	vector<unsigned char> bufferOut;
	pack(bufferOut,texto); //tipo
	pack(bufferOut,userName.size());
	packv(bufferOut,userName.data(),userName.size());
	pack(bufferOut,msg.size());
	packv(bufferOut,msg.data(),msg.size());

	//por cada cliente conectado
	for(  auto client  : connectionIds){
		//reenviar paquete
			//si no soy el emisor
		if(client.first!=userName)
			sendMSG(client.second,bufferOut);
	}
	bufferOut.clear();//opcional

}

string clientManager::recibeMensaje(int serverId){

	//recibir mensaje
	string userName;
	string mensaje;
    vector<unsigned char> buffer;
 
	recvMSG(serverId,buffer);
	//desempaquetar mensaje reenviado
		//desepaquetar tipo
	msgTypes type=unpack<msgTypes>(buffer);
		//username
	userName=desempaquetaTipoTexto(buffer);
		//mensaje
	mensaje=desempaquetaTipoTexto(buffer);
		
	return userName+":"+mensaje;
	
}//recibir un mensaje
