#ifndef _URL_H_
#define _URL_H_

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef char url_content[256];

#define h_addr h_addr_list[0] /* for backward compatibility */

typedef struct URL {
	url_content user; // string to user
	url_content password; // string to password
	url_content host; // string to host
	url_content ip; // string to IP
	url_content path; // string to path
	url_content filename; // string to filename
	int port; // integer to port
} url;

void init_url(url* url);
int parse_url(url* url, const char* str); // Parse a string with the url to create the URL structure
int get_host_ip(url* url); // gets an IP by host name
char* process_until_char(char* str, char chr);

#endif // _URL_H_
