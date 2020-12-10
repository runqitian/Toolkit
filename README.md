# Toolkit
Some useful tools.
Finished:
* rqtrans

## rqtrans
### Updates:
* Add Upload-Download mode, see usage for details
### Intro
Implement a TCP based application protocol, which support text, file and directory transfer.
Protocol Design:
```
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
```
### Environment
Unix based system.
### Build
Execute 'make'
### Usage
* This tool can be used to transfer **text**, **file** and **directory**. 
* Run server mode on the destination side and run client mode on the source side.
* Add Upload-download mode, run the server mode on remote server. Any client config this server can upload and download files or directories.
```
Client Mode:
File/Directory Transfer:
    rqtrans <destination host> <file/directory>

    Options:
        -f: force overwrite target file, only valid for file transfer.
        -p: destination port, default is 9819 
        
        rqtrans -f -p <port> <destination host> <file/directory>

Text Transfer:
    rqtrans -t <destination host> <text>

    Options:
        -p: destination port, default is 9819
        
        rqtrans -p <port> -t <destination host> <text>


Server Mode:
    rqtrans -l

    Options:
        -o: write files to which directory, default is current directory.
        -p: listening port, default is 9819
        
        rqtrans -l -p <port> -o <target directory>


New mode: Upload-Download
client:
    # firstly, config the server
    rqtrans config <host> <port>
    # then you can download and upload files or directories easily.
    rqtrans download -o <dest path>
    rqtrans upload <file/directory>

server:
    # set up a server
    rqtrans server -p <port>
```
