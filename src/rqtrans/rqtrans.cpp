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
**/

#define DEFAULT_PORT 9819
#define DEFAULT_PATH "."

struct Info {
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

int main(int argc, char *argv[]){
	struct Info info;
	if (!argParser(argc, argv, info)){
		exit(1);
	}
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

	while (idx < argc){
		char *s = argv[idx];

		// option arguments
		if (strcmp(s, "-l") == 0){
			if (info.text_mode){
				fprintf(stderr, "\'-t\' and \'-l\' can not be used simultaneously!\n");
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
	return true;
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