#include "utils.h"
#include <iostream>
#include <string>
#include <map> 
#include <vector>

#define ERRLOG(msg) cout<<"Error "<<__FILE__<<":"<<__LINE__<<" "<<msg<<" \n"

using namespace std;


class clientManager{
	
public:	

	typedef enum{
		texto=0, 
		exit=1,
		login=2,
		ack=3
	}msgTypes;

	//variable de cierre de programa:
	static inline bool cierreDePrograma=false;
	//recepción asincrona de paquetes en cliente
	static inline mutex cerrojoBuffers;
	static inline vector<unsigned char> bufferAcks; 
	static inline vector<unsigned char> bufferTxt;

	static inline map<string,int> connectionIds;
	static void enviaMensaje(int id, string mensaje);
	static string desempaquetaTipoTexto(vector<unsigned char> &buffer);
	static void enviaLogin(int id, string userName);
	static void atiendeCliente(int clientId);
	static string recibeMensaje(int serverId);
	static void reenviaTexto(string userName, string msg);

};

