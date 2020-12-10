#include <string>

/**
<separator>: space
<phase>: s, t, c

Connection Setup Phase:
s <type> <force>\n
<type>: 1 char  f d t
<force>: 1 char y n
f for file
d for directory
t for text

Transfer Phase:
text:
	t <length>\n<text>

file:
	t <fname> <size>\n<file bytes>
	<size> is unsigned long int

dir:
	t <expected number>\n
	t <isDir> <name> <size>\n<file bytes>
	t <isDir> <name> <size>\n<file bytes>
	...
	<isDir>: 'd' or 'f', 'd' represents directory, 'f' represents file
	for example:
	t f hello 209\nfadsfsdalfjlsdjf....
	t d dir1 0\n

Close Connection Phase:
c\n

Response:
200 OK: <msg>\n
400 ERROR: <msg>\n

Some special case:
case 1: there exists space in file name
solution:
	replace " " with "\s"
	replace "\" with "\\"
	replace '\n' with "\n"

**/

#ifndef RQTRANS_PROTOCOL_H
#define RQTRANS_PROTOCOL_H
class RQTransProtocol{
private:
	int sockfd;
	char * sbuff;
	int sbuff_size;
	char type;
	bool force;

public:
	static const int BUFF_SIZE = 64 * 1024;

	RQTransProtocol(const int sockfd);

	std::string encodingName(const std::string &name);
	std::string decodingName(const std::string &name);
	void sendBytes(int s, const char *msg, const unsigned int len);
	void readBytes(int s, char * dest, int len);
	std::string readline(int s);

	bool SetConnection();
	bool AcceptConnection();
	bool TransferText(const std::string &input);
	bool ReceiveText(std::string &output);
	bool TransferFile(const std::string &fpath);
	bool ReceiveFile(const std::string &baseDir, std::string &last_received);
	bool TransferDir(const std::string &dpath);
	bool ReceiveDir(const std::string &baseDir, std::string &last_received);
	bool CloseConnection();
	bool WaitClose();
	void execClient(const char type, const std::string &arg, bool force=false);
	bool execServer(const std::string baseDir, std::string &last_received, char &last_received_type);
};
#endif