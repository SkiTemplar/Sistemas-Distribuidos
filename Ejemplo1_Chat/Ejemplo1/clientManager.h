#include "utils.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>

#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

struct Packet {
	int type;              // ver MsgType
	std::string from;
	std::string to;        // vacío si público
	std::string text;      // cuerpo
};

class clientManager{
public:
	enum MsgType { texto=0, exit_=1, login=2, ack=3, priv_=4, shutdown_=5 };

	static inline bool cierreDePrograma=false;

	// buffers cliente
	static inline std::mutex cerrojoBuffers;
	static inline std::vector<unsigned char> bufferAcks;
	static inline std::vector<unsigned char> bufferTxt;

	// directorio usuarios en servidor
	static inline std::map<std::string,int> connectionIds;

	// cliente
	static void enviaLogin(int id, const std::string& user);
	static void enviaLogout(int id, const std::string& user);
	static void enviaMensajePublico(int id, const std::string& user, const std::string& msg);
	static void enviaMensajePrivado(int id, const std::string& from, const std::string& to, const std::string& msg);
	static std::string desempaquetaTipoTexto(std::vector<unsigned char>& buffer);

	// servidor
	static void atiendeCliente(int clientId);
	static void reenviaPublico(const std::string& from, const std::string& msg);
	static void reenviaPrivado(const std::string& to, const std::string& from, const std::string& msg);
};

#endif
