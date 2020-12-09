#include "Utils.h"

#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <unistd.h>

std::string processBar[] {
    "00%: [                     ]",
    "05%: [#                    ]",
    "10%: [##                   ]",
    "15%: [###                  ]",
    "20%: [####                 ]",
    "25%: [#####                ]",
    "30%: [######               ]",
    "35%: [#######              ]",
    "40%: [########             ]",
    "45%: [#########            ]",
    "50%: [##########           ]",
    "55%: [###########          ]",
    "60%: [############         ]",
    "65%: [#############        ]",
    "70%: [##############       ]",
    "75%: [###############      ]",
    "80%: [################     ]",
    "85%: [#################    ]",
    "90%: [###################  ]",
    "95%: [#################### ]",
    "100%: [#####################]",
};

std::string Utils::getProcessBar(const float percent){
    if(percent > 1 || percent < 0){
        throw std::runtime_error("Utils::getProcessBar\tinput percent invalid");
    }
	int idx = (int)(percent * 20);
	return processBar[idx];
}

bool Utils::isPathExist(const std::string &s){
	struct stat buffer;
  	return (stat (s.c_str(), &buffer) == 0);
}

bool Utils::isPathDir(const std::string &s){
	struct stat buffer;
  	if (stat (s.c_str(), &buffer) == 0){
  		if (buffer.st_mode & S_IFDIR){
  			return true;
  		}
  	}
  	return false;
}

std::string Utils::pathAppend(const std::string &base, const std::string &fname){
	char sep = '/';
	
	std::string tmp = base;

	#ifdef _WIN32
	sep = '\\';
	#endif

	if (tmp[tmp.size() - 1] != sep){
		tmp += sep;
	}
	return tmp + fname;
}

std::string Utils::extractFilename(const std::string &path){
	char sep = '/';

	#ifdef _WIN32
	sep = '\\';
	#endif

	const char *p;
	if ((p =strrchr(path.c_str(), sep)) == NULL){
		return path;
	}
	return std::string(p + 1);
}

void Utils::listDir(const std::string dpath, const std::string dname, std::vector<std::string> &nls, std::vector<std::string> &pls, std::vector<char> &tls){
	DIR *dp;
	struct dirent *de;
	std::string sep = "/";
	#ifdef _WIN32
	sep = "\\";
	#endif

	if ((dp = opendir(dpath.c_str())) == NULL) {
		return;
	}
	while ((de = readdir(dp)) != NULL){
		if (strcmp(de -> d_name, "..") == 0 || strcmp(de -> d_name, ".") == 0){
			continue;
		}
		if (de->d_type == DT_DIR) {
			std::string subdname = dname + sep + std::string(de->d_name);
			std::string subdpath = dpath + sep + std::string(de->d_name);
			nls.push_back(subdname);
			pls.push_back(subdpath);
			tls.push_back('d');
			listDir(subdpath, subdname, nls, pls, tls);
		}else{
			nls.push_back(dname + sep + std::string(de->d_name));
			pls.push_back(dpath + sep + std::string(de->d_name));
			tls.push_back('f');
		}
	}
}


bool Utils::deleteDir(const std::string dpath){
    DIR *dp;
    struct dirent *de;
    std::string sep = "/";
    #ifdef _WIN32
    sep = "\\";
    #endif

    if ((dp = opendir(dpath.c_str())) == NULL) {
    	fprintf(stderr, "can not open directory %s\n", dpath.c_str());
        return false;
    }
    while ((de = readdir(dp)) != NULL){
        if (strcmp(de -> d_name, "..") == 0 || strcmp(de -> d_name, ".") == 0){
            continue;
        }
        if (de->d_type == DT_DIR) {
            std::string subdpath = dpath + sep + std::string(de->d_name);
            if (!deleteDir(subdpath)){
            	return false;
            }
        }else{
            std::string subfpath = dpath + sep + std::string(de->d_name);
            if (unlink(subfpath.c_str()) != 0){
            	fprintf(stderr, "can not remove file %s\n", subfpath.c_str());
        		return false;
            }
        }
    }
    if (rmdir(dpath.c_str()) != 0){
    	fprintf(stderr, "can not remove directory %s\n", dpath.c_str());
        return false;
    }
    return true;
}