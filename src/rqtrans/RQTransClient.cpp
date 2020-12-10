#include "RQTransClient.h"
#include "RQTransProtocol.h"
#include "Utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

RQTransClient::RQTransClient(const std::string host, const int port): host(host), port(port) {
	struct hostent* ip = gethostbyname(host.c_str());
	if (ip == NULL){
		fprintf(stderr, "host name invalid!\n");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket creation failed!\n");
		exit(1);
	}
	struct sockaddr_in sa;
	memcpy(&sa.sin_addr, ip->h_addr_list[0], ip->h_length);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0){
		close(sockfd);
		fprintf(stderr, "connection failed!\n");
		exit(1);
	}
}

void RQTransClient::transferText(const std::string &text){
	RQTransProtocol prot(sockfd);
	prot.execClient('t', text);
	close(sockfd);
}

void RQTransClient::transferFile(const std::string &filepath, const bool force){
	fflush(stdout);
	RQTransProtocol prot(sockfd);
	prot.execClient('f', filepath, force);
	close(sockfd);
}

void RQTransClient::transferDir(const std::string &dirpath, const bool force){
	fflush(stdout);
	RQTransProtocol prot(sockfd);
	prot.execClient('d', dirpath, force);
	close(sockfd);
}

void RQTransClient::download(std::string path){
	RQTransProtocol prot(sockfd);
	char buff[RQTransProtocol::BUFF_SIZE] = {0};
	sprintf(buff, "download\n");
	prot.sendBytes(sockfd, buff, strlen(buff));
	std::string resp = prot.readline(sockfd);
	if (resp == "200 OK: Ready\n"){
		std::string arg1_temp;
		char arg2_temp;
		prot.execServer(path, arg1_temp, arg2_temp);
		close(sockfd);
	}else{
		fprintf(stderr, "can not download from server\n");
		close(sockfd);
	}
}

void RQTransClient::upload(std::string filepath){
	RQTransProtocol prot(sockfd);
	char buff[RQTransProtocol::BUFF_SIZE] = {0};
	sprintf(buff, "upload\n");
	prot.sendBytes(sockfd, buff, strlen(buff));
	std::string resp = prot.readline(sockfd);
	if (resp == "200 OK: Ready\n"){
		if (Utils::isPathDir(filepath)){
			transferDir(filepath, true);
		}else{
			transferFile(filepath, true);
		}
		close(sockfd);
	}else{
		fprintf(stderr, "can not upload to server\n");
		close(sockfd);
	}
}