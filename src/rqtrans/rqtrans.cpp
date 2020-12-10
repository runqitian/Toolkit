#include "RQTransServer.h"
#include "RQTransClient.h"
#include "Utils.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <climits>

/**
accepted format:
client:
	// non option argument for file transfer: host filepath
	rqtrans host filepath/text
	// options
	// -f means force overwrite
	rqtrans	-f -p [port] host filepath

	// transfer text format:
	rqtrans -t host text
	// options
	rqtrans -p [port] -t [text]


server:
	// default use 9819 port and current dir
	rqtrans -l
	// options for file transfering
	rqtrans -l -p [port] -o [target path]

	// If data is text, rqtrans -l will output result to stdout
	// If data is file, rqtrans -l will write file to target path

It can accept file, directory or text.

UPDATE:
add upload/download mode:
server:
	rqtrans server -p <port>

client:
	rqtrans config <host> <port>
	rqtrans upload <path>
	rqtrans download -o <path>

**/

#define DEFAULT_PORT 9819
#define DEFAULT_PATH "."
#define DEDAULT_CONFIG_FILE "/.rqtrans"

struct Info {
	bool up_down_mode;
	bool download;
	bool config_mode;
	bool server_mode;
	bool text_mode;
	bool force;
	std::string text;
	std::string host;
	std::string filepath;
	std::string path;
	int port;
	Info(): server_mode(false), text_mode(false), force(false), path(DEFAULT_PATH), port(DEFAULT_PORT) {}
};

int strToPort(char* str);
bool argParser(int argc, char *argv[], struct Info &info);
int write_config(const std::string host, const int port);
int read_config(struct Info &info);

int main(int argc, char *argv[]){
	struct Info info;
	if (!argParser(argc, argv, info)){
		exit(1);
	}

	// config mode
	if (info.config_mode){
		int success = write_config(info.host, info.port);
		if (success){
			printf("record successfully!\n");
			return 0;
		}else{
			fprintf(stderr, "failed, please check ~/.rqtrans\n");
			return 1;
		}
	}

	// upload/download mode
	if (info.up_down_mode){
		if (info.server_mode){
			RQTransServer server(info.port);
			server.upload_download_run(info.path);
			return 0;
		}
		// client mode
		int success = read_config(info);
		if (!success){
			fprintf(stderr, "failed reading config, please check ~/.rqtrans or config first\n");
			return 1;
		}
		RQTransClient client(info.host, info.port);
		if (info.download) {
			client.download(info.path);
		}else{
			client.upload(info.filepath);
		}
		return 0;
	}

	// not upload/download mode
	if (info.server_mode){
		RQTransServer server(info.port);
		server.run(info.path);
	}else{
		RQTransClient client(info.host, info.port);
		if (info.text_mode){
			client.transferText(info.text);
		}else{
			if (Utils::isPathDir(info.filepath)){
				client.transferDir(info.filepath, info.force);
			}else{
				client.transferFile(info.filepath, info.force);
			}
		}
	}
	return 0;
}


bool argParser(int argc, char *argv[], struct Info &info){
	int idx = 1;

	if (argc == 1){
		fprintf(stderr, "need arguments!\n");
		return false;
	}

	if (strcmp(argv[idx], "config") == 0){
		if (argc != 4){
			fprintf(stderr, "--configure usage not correct!\n");
			return false;
		}
		info.config_mode = true;
		info.host = std::string(argv[2]);
		info.port = strToPort(argv[3]);
		if (info.port == -1){
			fprintf(stderr, "port %s invalid\n", argv[idx]);
			return false;
		}
		return true;
	}
	else if (strcmp(argv[idx], "server") == 0){
		info.up_down_mode = true;
		info.server_mode = true;
		++idx;
	}
	else if (strcmp(argv[idx], "upload") == 0){
		info.up_down_mode = true;
		info.download = false;
		++idx;
	}
	else if (strcmp(argv[idx], "download") == 0){
		info.up_down_mode = true;
		info.download = true;
		++idx;
	}


	while (idx < argc){
		char *s = argv[idx];

		// option arguments
		if (strcmp(s, "-l") == 0){
			if (info.text_mode){
				fprintf(stderr, "\'-t\' and \'-l\' can not be used simultaneously!\n");
				return false;
			}
			if (info.up_down_mode){
				fprintf(stderr, "\'-l\' can not be in upload-download mode!\n");
				return false;
			}
			info.server_mode = true;
			++idx;
			continue;
		}
		if (strcmp(s, "-t") == 0){
			if (info.server_mode){
				fprintf(stderr, "\'-t\' and \'-l\' can not be used simultaneously!\n");
				return false;
			}
			if (info.up_down_mode){
				fprintf(stderr, "\'-t\' can not be in upload-download mode!\n");
				return false;
			}
			info.text_mode = true;
			++idx;
			continue;
		}
		if (s[0] == '-'){
			if (strlen(s) != 2){
				fprintf(stderr, "argument \'%s\' incorrect!\n", s);
				return false;
			}
			switch(s[1]){
				case 'p':
					if (++idx == argc){
						fprintf(stderr, "no port following \'-p\' option!\n");
						return false;
					}
					info.port = strToPort(argv[idx]);
					if (info.port == -1){
						fprintf(stderr, "port %s invalid\n", argv[idx]);
						return false;
					}
					++idx;
					break;
				case 'o':
					if (++idx == argc){
						fprintf(stderr, "no path following \'-o\' option!\n");
						return false;
					}
					info.path = std::string(argv[idx]);
					++idx;
					break;
				case 'f':
					info.force = true;
					++idx;
					break;
				default:
					fprintf(stderr, "argument \'%s\' incorrect!\n", s);
					return false;
			}
			continue;
		}

		// non option arguments
		if (info.up_down_mode){
			if (info.server_mode || info.download){
				fprintf(stderr, "argument \'%s\' invalid in server mode!\n", s);
				return false;
			}
			// upload param
			if (idx == argc - 1){
				info.filepath = std::string(argv[idx]);
				// will quit loop
				++idx;
			}else{
				fprintf(stderr, "non option argument incorrect\n");
				return false;
			}
		}
		else{
			if (info.server_mode){
				fprintf(stderr, "argument \'%s\' invalid in server mode!\n", s);
				return false;
			}
			info.host = std::string(s);
			if (++idx == argc - 1){
				if (info.text_mode){
					info.text = std::string(argv[idx]);
				}else{
					info.filepath = std::string(argv[idx]);
				}
				// will quit loop
				++idx;
			}else{
				fprintf(stderr, "non option argument incorrect\n");
				return false;
			}
		}
	}
	return true;
}

int write_config(const std::string host, const int port){
	FILE *wf;
	wf = fopen((std::string(getenv("HOME")) + DEDAULT_CONFIG_FILE).c_str(), "w");
	if (wf == NULL){
		return 0;
	}
	fprintf(wf, "host:%s\nport:%d\n", host.c_str(), port);
	fclose(wf);
	return 1;
}

int read_config(struct Info &info){
	FILE *rf;
	rf = fopen((std::string(getenv("HOME")) + DEDAULT_CONFIG_FILE).c_str(), "r");
	if (rf == NULL){
		return 0;
	}
	char *temp = new char[200];
	int port;
	int success = 0;
	if (fscanf(rf, "host:%s\nport:%d\n", temp, &port) == 2){
		info.port = port;
		info.host = std::string(temp);
		success = 1;
	}
	delete[] temp;
	fclose(rf);
	return success;
}

int strToPort(char* str){
	char *endptr;
	long int port = strtol(str, &endptr, 10);
	if(port == LONG_MAX || port == LONG_MIN){
		return -1;
	}
	if (*endptr != 0){
		return -1;
	}
	if (port > 65535 || port < 0){
		return -1;
	}
	return (int)port;
}