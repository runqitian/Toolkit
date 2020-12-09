#include "RQTransProtocol.h"
#include "Utils.h"

#include <cstdio>
#include <dirent.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


std::string RQTransProtocol::encodingName(const std::string &name) {
	std::string encoded;
	for (const char c: name){
		encoded += c == ' ' ? "\\s" : c == '\\' ? "\\\\" : c == '\n' ? "\\n" : std::string(1, c);
	}
	return encoded;
}

std::string RQTransProtocol::decodingName(const std::string &name){
	std::string decoded;
	int i = 0;
	while (i < name.size()){
		if (name[i] == '\\'){
			++i;
			switch (name[i]){
				case 's':
					decoded += ' ';
					break;
				case 'n':
					decoded += '\n';
					break;
				case '\\':
					decoded += '\\';
					break;
				default:
					fprintf(stderr, "file name %s encoded invalid!\n", name.c_str());
			}
		}else{
			decoded += name[i];
		}
		++i;
	}
	return decoded;
}

RQTransProtocol::RQTransProtocol(const int sockfd): sockfd(sockfd) {
	sbuff = (char *)malloc(sizeof(char) * BUFF_SIZE);
	sbuff_size = 0;
}

bool RQTransProtocol::SetConnection(){
	char buff[BUFF_SIZE] = {0};
	sprintf(buff, "s %c %c\n", type, force ? '1' : '0');
	sendBytes(sockfd, buff, strlen(buff));
	std::string resp = readline(sockfd);
	if (resp == "200 OK: Ready\n"){
		return true;
	}
	return false;
}

bool RQTransProtocol::AcceptConnection(){
	std::string req = readline(sockfd);
	char f_flag;
	// printf("|%s|", req.c_str());
	if (sscanf(req.c_str(), "s %c %c\n", &type, &f_flag) == 2){
		if ((type == 't' || type == 'd' || type == 'f') && (f_flag == '0' || f_flag == '1')){
			force = f_flag == '0' ? false : true;
			std::string res = "200 OK: Ready\n";
			sendBytes(sockfd, res.c_str(), res.size());
			return true;
		}
	}
	std::string res = "400 ERROR: Invalid Connection Request\n";
	sendBytes(sockfd, res.c_str(), res.size());
	return false;
}

bool RQTransProtocol::TransferText(const std::string &input){
	std::string msg = "t ";
	msg += std::to_string(input.size());
	msg += "\n";
	msg += input;
	sendBytes(sockfd, msg.c_str(), msg.size());
	std::string resp = readline(sockfd);
	if (resp == "200 OK: Received\n"){
		return true;
	}else{
		fprintf(stderr, "Message: %s", resp.c_str());
	}
	return false;
}


bool RQTransProtocol::ReceiveText(std::string &output){

	std::string header = readline(sockfd);
	int len;
	if (sscanf(header.c_str(), "t %d\n", &len) == 1) {
		char *text = (char *)malloc(sizeof(char) * len);
		readBytes(sockfd, text, len);
		output = std::string(text, len);
		free(text);
		std::string res = "200 OK: Received\n";
		sendBytes(sockfd, res.c_str(), res.size());
		return true;
	}
	std::string res = "400 ERROR: Invalid Header\n";
	sendBytes(sockfd, res.c_str(), res.size());
	return false;
}


bool RQTransProtocol::TransferFile(const std::string &fpath){
	FILE *fp;
	if ((fp = fopen(fpath.c_str(), "rb")) == NULL){
		return false;
	}
	fflush(stdout);
	fseek(fp, 0L, SEEK_END);
    long int fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

	char *buffer = (char *)malloc(sizeof(char) * BUFF_SIZE);
	int pre_len;
	if ((pre_len = sprintf(buffer, "t %s %ld\n", encodingName(Utils::extractFilename(fpath)).c_str(), fsize)) < 0) {
		return false;
	}
	sendBytes(sockfd, buffer, pre_len);

	int bnum;
	int bcount = 0;
	while((bnum = fread(buffer, sizeof(char), BUFF_SIZE, fp)) == BUFF_SIZE){
		sendBytes(sockfd, buffer, BUFF_SIZE);
		bcount += bnum;
		std::string bar = Utils::getProcessBar((float)bcount / fsize);
		fprintf(stdout, "\r%s %s", fpath.c_str(), bar.c_str());
	}
	if (bnum > 0){
		sendBytes(sockfd, buffer, bnum);
		bcount += bnum;
		std::string bar = Utils::getProcessBar((float)bcount / fsize);
		fprintf(stdout, "\r%s %s", fpath.c_str(), bar.c_str());
	}
	fprintf(stdout, "\n");
	free(buffer);
	fclose(fp);

	std::string resp = readline(sockfd);
	if (resp == "200 OK: Received\n"){
		return true;
	}else{
		fprintf(stderr, "Message: %s", resp.c_str());
	}
	return false;
}

bool RQTransProtocol::ReceiveFile(const std::string &baseDir){
	std::string header = readline(sockfd);
	char fname[100];
	long int fsize;
	if (sscanf(header.c_str(), "t %s %ld\n", fname, &fsize) != 2){
		const char *resp = "400 ERROR: Request format invalid\n";
		sendBytes(sockfd, resp, strlen(resp));
		fprintf(stderr, "%s", resp);
		return false;
	}

	// file exist or not? overwirte?
	std::string path = Utils::pathAppend(baseDir, decodingName(std::string(fname)));
	if (Utils::isPathDir(std::string(path))){
		if (!force){
			const char *resp = "400 ERROR: Same name directory on server side and not forced.\n";
			sendBytes(sockfd, resp, strlen(resp));
			fprintf(stderr, "%s", resp);
			return false;
		}
		if (!Utils::deleteDir(path)){
			const char *resp = "400 ERROR: Directory on server side can not be overwritten.\n";
			sendBytes(sockfd, resp, strlen(resp));
			fprintf(stderr, "%s", resp);
			return false;
		}
	}
	if (Utils::isPathExist(std::string(path))){
		if (!force){
			const char *resp = "400 ERROR: Same name file on server side and not forced.\n";
			sendBytes(sockfd, resp, strlen(resp));
			fprintf(stderr, "%s", resp);
			return false;
		}
		if (unlink(path.c_str()) != 0){
			const char *resp = "400 ERROR: File on server side can not be overwritten.\n";
			sendBytes(sockfd, resp, strlen(resp));
			fprintf(stderr, "%s", resp);
			return false;
		}
	}

	// now we can write the file.
	FILE *fp;
	if ((fp = fopen(path.c_str(), "wb")) == NULL){
		const char *resp = "400 ERROR: File written error\n";
		sendBytes(sockfd, resp, strlen(resp));
		fprintf(stderr, "%s", resp);
		return false;
	}
	char *fbuffer = (char *)malloc(sizeof(char) * BUFF_SIZE);
	unsigned int unread = fsize;
	while (BUFF_SIZE < unread){
		readBytes(sockfd, fbuffer, BUFF_SIZE);
		fwrite(fbuffer, sizeof(char), BUFF_SIZE, fp);
		unread -= BUFF_SIZE;
		std::string bar = Utils::getProcessBar(1 - (float)unread / fsize);
		fprintf(stdout, "\r%s %s", path.c_str(), bar.c_str());
	}
	readBytes(sockfd, fbuffer, unread);
	fwrite(fbuffer, sizeof(char), unread, fp);
	fprintf(stdout, "\r%s %s\n", path.c_str(), Utils::getProcessBar(1).c_str());
	fclose(fp);
	const char *resp = "200 OK: Received\n";
	sendBytes(sockfd, resp, strlen(resp));
	return true;
}

bool RQTransProtocol::TransferDir(const std::string &dpath) {
	printf("transfer file");
	fflush(stdout);
	std::vector<std::string> nls;
	std::vector<std::string> pls;
	std::vector<char> tls;
	nls.push_back(Utils::extractFilename(dpath));
	pls.push_back(dpath);
	tls.push_back('d');
	Utils::listDir(dpath, Utils::extractFilename(dpath), nls, pls, tls);
	int expectedNum = nls.size();
	std::string req;
	req += "t ";
	req += std::to_string(expectedNum);
	req += "\n";
	sendBytes(sockfd, req.c_str(), req.size());
	printf("before alloc");
	fflush(stdout);
	char *buffer = (char *)malloc(sizeof(char) * BUFF_SIZE);
	printf("afeter alloc");
	fflush(stdout);
	for (int i = 0; i < nls.size(); i++){
		if(tls[i] == 'f'){
			FILE *fp;
			if ((fp = fopen(pls[i].c_str(), "rb")) == NULL){
				return false;
			}
			fseek(fp, 0L, SEEK_END);
		    long int fsize = ftell(fp);
		    fseek(fp, 0L, SEEK_SET);

			int pre_len;
			if ((pre_len = sprintf(buffer, "t %c %s %ld\n", tls[i], encodingName(nls[i]).c_str(), fsize)) < 0) {
				return false;
			}
			sendBytes(sockfd, buffer, pre_len);

			int bnum;
			int bcount = 0;
			while((bnum = fread(buffer, sizeof(char), BUFF_SIZE, fp)) == BUFF_SIZE){
				sendBytes(sockfd, buffer, BUFF_SIZE);
				bcount += bnum;
				std::string bar = Utils::getProcessBar((float)bcount / fsize);
				fprintf(stdout, "\r%s %s", pls[i].c_str(), bar.c_str());
			}
			if (bnum > 0){
				sendBytes(sockfd, buffer, bnum);
				bcount += bnum;
				std::string bar = Utils::getProcessBar((float)bcount / fsize);
				fprintf(stdout, "\r%s %s", pls[i].c_str(), bar.c_str());
			}
			fclose(fp);
		}else{
			int pre_len = sprintf(buffer, "t %c %s %ld\n", tls[i], encodingName(nls[i]).c_str(), 0L);
			sendBytes(sockfd, buffer, pre_len);
		}
	}
	fprintf(stdout, "\n");
	std::string resp = readline(sockfd);
	if (resp == "200 OK: Received\n"){
		return true;
	}else{
		fprintf(stderr, "Message: %s", resp.c_str());
	}
	return false;
}

bool RQTransProtocol::ReceiveDir(const std::string &baseDir) {
	long int expectedNum;
	std::string req = readline(sockfd);
	if (sscanf(req.c_str(), "t %ld\n", &expectedNum) != 1){
		const char *resp = "400 ERROR: expected number record invalid\n";
		sendBytes(sockfd, resp, strlen(resp));
		fprintf(stderr, "%s", resp);
		return false;
	}

	bool first_loop = true;
	for (int i = 0; i < expectedNum; i++){
		char fname[500];
		char ftype;
		long int fsize;

		std::string header = readline(sockfd);
		if (sscanf(header.c_str(), "t %c %s %ld\n", &ftype, fname, &fsize) != 3){
			const char *resp = "400 ERROR: file format invalid\n";
			sendBytes(sockfd, resp, strlen(resp));
			fprintf(stderr, "%s", resp);
			return false;
		}

		// file exist or not? overwirte?
		std::string path = Utils::pathAppend(baseDir, decodingName(std::string(fname)));
		if (first_loop){

			// file exist or not? overwirte?
			if (Utils::isPathDir(std::string(path))){
				if (!force){
					const char *resp = "400 ERROR: Same name directory on server side and not forced.\n";
					sendBytes(sockfd, resp, strlen(resp));
					fprintf(stderr, "%s", resp);
					return false;
				}
				if (!Utils::deleteDir(path)){
					const char *resp = "400 ERROR: Directory on server side can not be overwritten.\n";
					sendBytes(sockfd, resp, strlen(resp));
					fprintf(stderr, "%s", resp);
					return false;
				}
			}
			if (Utils::isPathExist(std::string(path))){
				if (!force){
					const char *resp = "400 ERROR: Same name file on server side and not forced.\n";
					sendBytes(sockfd, resp, strlen(resp));
					fprintf(stderr, "%s", resp);
					return false;
				}
				if (unlink(path.c_str()) != 0){
					const char *resp = "400 ERROR: File on server side can not be overwritten.\n";
					sendBytes(sockfd, resp, strlen(resp));
					fprintf(stderr, "%s", resp);
					return false;
				} 
			}
			first_loop = false;
		}


		// now we can write the file.
		if (ftype == 'd'){
			 mkdir(path.c_str(), 0755);
		}else{
			FILE *fp;
			if ((fp = fopen(path.c_str(), "wb")) == NULL){
				const char *resp = "400 ERROR: File written error\n";
				sendBytes(sockfd, resp, strlen(resp));
				fprintf(stderr, "%s", resp);
				return false;
			}

			char *fbuffer = (char *)malloc(sizeof(char) * BUFF_SIZE);
			unsigned int unread = fsize;
			while(BUFF_SIZE < unread){
				readBytes(sockfd, fbuffer, BUFF_SIZE);
				fwrite(fbuffer, sizeof(char), BUFF_SIZE, fp);
				unread -= BUFF_SIZE;
				std::string bar = Utils::getProcessBar(1 - (float)unread / fsize);
				fprintf(stdout, "\r%s %s", path.c_str(), bar.c_str());
			}
			readBytes(sockfd, fbuffer, unread);
			fwrite(fbuffer, sizeof(char), unread, fp);
			free(fbuffer);
			fprintf(stdout, "\r%s %s", path.c_str(), Utils::getProcessBar(1).c_str());
			fclose(fp);
		}
		fflush(stdout);
	}
	fprintf(stdout, "\n");
	const char *resp = "200 OK: Received\n";
	sendBytes(sockfd, resp, strlen(resp));
	return true;
}


void RQTransProtocol::sendBytes(int s, const char *msg, const unsigned int len){
	unsigned int remain = len;
	const char *p = msg;
	while(remain > BUFF_SIZE){
		int num = send(s, p, BUFF_SIZE, 0);
		if (num == -1){
			fprintf(stderr, "Socket Bytes Sending Error\n");
			close(s);
			exit(1);
		}
		// fprintf(stdout, "sent %d bytes\n", num);
		fflush(stdout);
		p += num;
		remain -= num;
	}
	if (remain > 0){
		int num = send(s, p, remain, 0);
		if (num == -1){
			fprintf(stderr, "Socket Bytes Sending Error\n");
			close(s);
			exit(1);
		}
		fflush(stdout);
	}
}

void RQTransProtocol::readBytes(int s, char * dest, int len) {
	if (len == 0){
		return;
	}
	if (sbuff_size == 0){
		if ((sbuff_size = recv(s, sbuff, BUFF_SIZE, 0)) <= 0) {
			fprintf(stderr, "Connection closed unexpectedly!\n");
			close(s);
			exit(1);
		}
	}
	int need = len;
	char *p = dest;
	while (sbuff_size < need){
		memcpy(p, sbuff, sbuff_size);
		need -= sbuff_size;
		p += sbuff_size;
		if ((sbuff_size = recv(s, sbuff, BUFF_SIZE, 0)) <= 0){
			fprintf(stderr, "Connection closed unexpectedly!\n");
			close(s);
			exit(1);
		}
	}
	memcpy(p, sbuff, need);
	memmove(sbuff, sbuff + need, sbuff_size - need);
	sbuff_size = sbuff_size - need;
}

std::string RQTransProtocol::readline(int s){
	if (sbuff_size == 0){
		if ((sbuff_size = recv(s, sbuff, BUFF_SIZE, 0)) <= 0) {
			fprintf(stderr, "Connection closed unexpectedly!\n");
			close(s);
			exit(1);
		}
	}
	std::string msg;
	bool lf = false;
	int idx = 0;
	while(!lf) {
		while(idx < sbuff_size){
			if (sbuff[idx] == '\n'){
				msg += sbuff[idx];
				++idx;
				lf = true;
				break;
			}
			msg += sbuff[idx];
			++idx;
		}
		if (!lf){
			if ((sbuff_size = recv(s, sbuff, BUFF_SIZE, 0)) <= 0){
				fprintf(stderr, "Connection closed unexpectedly!\n");
				close(s);
				exit(1);
			}
			idx = 0;
		}
	}
	memmove(sbuff, sbuff + idx, sbuff_size - idx);
	sbuff_size = sbuff_size - idx;
	return msg;
}


void RQTransProtocol::execClient(const char type, const std::string &arg, bool force){
	this -> type = type;
	this -> force = force;
	if (!SetConnection()){
		fprintf(stderr, "Client: Setting up connetion failed\n");
		close(sockfd);
		exit(1);
	}
	if (type == 't'){
		if (!TransferText(arg)){
			fprintf(stderr, "Client: Text Transfer failed\n");
			close(sockfd);
			exit(1);
		}
		fprintf(stdout, "Text sent successfully!\n");
	}else if (type == 'f'){
		if (!TransferFile(arg)){
			fprintf(stderr, "Client: File Transfer failed\n");
			close(sockfd);
			exit(1);
		}
		fprintf(stdout, "File %s sent successfully!\n", arg.c_str());
	}
	else if (type == 'd'){
		if (!TransferDir(arg)){
			fprintf(stderr, "Client: Directory Transfer failed\n");
			close(sockfd);
			exit(1);
		}
		fprintf(stdout, "Directory %s sent successfully!\n", arg.c_str());
	}
}

void RQTransProtocol::execServer(const std::string baseDir){
	if (!AcceptConnection()){
		fprintf(stderr, "Server: Set up connetion failed\n");
		close(sockfd);
		return;
	}
	if (type == 't'){
		std::string output;
		if (!ReceiveText(output)){
			fprintf(stderr, "Server: Text Reception failed\n");
			fflush(stderr);
			close(sockfd);
			return;
		}
		fprintf(stdout, ">>>>>>>>\n%s\n<<<<<<<<\n", output.c_str());
		fflush(stdout);
	}
	else if (type == 'f'){
		if (!ReceiveFile(baseDir)){
			fprintf(stderr, "Server: File Reception failed\n");
			fflush(stderr);
			close(sockfd);
			return;
		}
		fprintf(stdout, "File received\n");
		fflush(stdout);
	}
	else if (type == 'd'){
		if (!ReceiveDir(baseDir)){
			fprintf(stderr, "Server: Directory Reception failed\n");
			fflush(stderr);
			close(sockfd);
			return;
		}
		fprintf(stdout, "Directory received\n");
		fflush(stdout);
	}
}

