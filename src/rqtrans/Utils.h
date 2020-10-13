#include <sys/stat.h>
#include <string>
#include <vector>

#ifndef UTILS_H
#define UTILS_H
namespace Utils
{
    bool isPathExist(const std::string &s);
	bool isPathDir(const std::string &s);
	std::string pathAppend(const std::string &base, const std::string &fname);
	std::string extractFilename(const std::string &path);
	void listDir(const std::string dpath, const std::string dname, std::vector<std::string> &nls, std::vector<std::string> &pls, std::vector<char> &tls);	
	bool deleteDir(const std::string dpath);
}


#endif