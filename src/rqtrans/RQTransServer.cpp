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

void RQTransServer::upload_download_run(const std::string path){
	if (!Utils::isPathDir(path)){
		fprintf(stderr, "target path \'%s\' is not a directory\n", path.c_str());
		exit(1);
	}
	this -> path = path;
	this -> last_received = "";
	if ((sockfd = establish_socket()) == -1){
		fprintf(stderr, "socket establishment failed!\n");
		exit(1);
	}
	fprintf(stdout, "upload-download-server listening...\n");
	fflush(stdout);
	while(true){
		int t;
		if ((t = get_connection(sockfd)) == -1){
			printf("connection failed!\n");
			continue;
		}
		std::thread t1(&RQTransServer::UploadDownloadCommunication, this, t);
		t1.join();
	}
}

bool RQTransServer::upload_download_prot(int sockfd, bool &download){
	RQTransProtocol prot(sockfd);
	std::string req = prot.readline(sockfd);
	if (req == "download\n" && this -> last_received != ""){
		download = true;
		std::string res = "200 OK: Ready\n";
		prot.sendBytes(sockfd, res.c_str(), res.size());
		return true;
	}else if (req == "upload\n"){
		download = false;
		std::string res = "200 OK: Ready\n";
		prot.sendBytes(sockfd, res.c_str(), res.size());
		return true;
	}
	std::string res = "400 ERROR: Invalid Request\n";
	prot.sendBytes(sockfd, res.c_str(), res.size());
	return false;
}

void RQTransServer::UploadDownloadCommunication(const int sockfd){
	try{
		bool download = false;
		if (!upload_download_prot(sockfd, download)){
			close(sockfd);
			return;
		}
		RQTransProtocol prot(sockfd);
		if (download){
			prot.execClient(this -> last_received_type, this -> last_received, false);
		}else{
			bool success = prot.execServer(path, this -> last_received, this -> last_received_type);
			if (!success){
				last_received = "";
			}
		}
	}catch(int e){
		printf("Error happened\n");
	}
	close(sockfd);
	return;
}

void RQTransServer::TransCommunication(const int sockfd){
	RQTransProtocol prot(sockfd);
	prot.execServer(path, this -> last_received, this -> last_received_type);
	close(sockfd);
}