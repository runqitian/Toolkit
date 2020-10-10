#include "RQTransServer.h"
#include "Utils.h"
#include "RQTransProtocol.h"

#include <string>
#include <cstdio>
#include <thread>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

RQTransServer::RQTransServer(const int port)
:port(port) {}

int RQTransServer::establish_socket(){
	// struct sockaddr_in sa = {AF_INET, htons(port), INADDR_ANY};
	struct sockaddr_in sa;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}
	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0){
		close(sockfd);
		return -1;
	}
	listen(sockfd, MAX_CONN);
	return sockfd;
}

int RQTransServer::get_connection(int sockfd){
	int t;
	if ((t = accept(sockfd, NULL, NULL)) < 0){
		return -1;
	}
	return t;
}

void RQTransServer::run(const std::string path){
	if (!Utils::isPathDir(path)){
		fprintf(stderr, "target path \'%s\' is not a directory\n", path.c_str());
		exit(1);
	}
	this -> path = path;
	if ((sockfd = establish_socket()) == -1){
		fprintf(stderr, "socket establishment failed!\n");
		exit(1);
	}
	fprintf(stdout, "server listening...\n");
	fflush(stdout);
	while(true){
		int t;
		if ((t = get_connection(sockfd)) == -1){
			printf("connection failed!\n");
			continue;
		}
		std::thread t1(&RQTransServer::TransCommunication, this, t);
		t1.detach();
	}
}

void RQTransServer::TransCommunication(const int sockfd){
	RQTransProtocol prot(sockfd);
	prot.execServer(path);
}