#include <string>

#ifndef RQTRANS_CLIENT_H
#define RQTRANS_CLIENT_H
class RQTransClient{
private:
	int port;
	std::string host;
	int sockfd;
public:
	RQTransClient(const std::string host, const int port);
	void transferText(const std::string &text);
	void transferFile(const std::string &filepath, const bool force);
	void transferDir(const std::string &dirpath, const bool force);
};
#endif