#include <string>

#ifndef RQTRANS_SERVER_H
#define RQTRANS_SERVER_H
class RQTransServer{
private:
	static const int MAX_CONN = 5;
	int sockfd;
	int port;
	std::string path;

	int establish_socket();
	int get_connection(const int sockfd);
	void TransCommunication(const int sockfd);
public:
	RQTransServer(const int port);
	void run(const std::string path);
};
#endif