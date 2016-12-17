#ifndef _FTP_H_
#define _FTP_H_

#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct FTP {
	int control_socket_fd; // file descriptor to control socket
	int data_socket_fd; // file descriptor to data socket
} ftp;

int connect_ftp(ftp* ftp, const char* ip, int port);
int login_ftp(ftp* ftp, const char* user, const char* password);
int cwd_ftp(ftp* ftp, const char* path);
int passive_ftp(ftp* ftp);
int retr_ftp(ftp* ftp, const char* filename);
int download_ftp(ftp* ftp, const char* filename);
int disconnect_ftp(ftp* ftp);
int send_ftp(ftp* ftp, const char* str, size_t size);
int read_ftp(ftp* ftp, char* str, size_t size);

#endif // _FTP_H_
