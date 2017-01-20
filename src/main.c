#include "ftp.h"
#include "url.h"

#include <stdio.h>
#include <stdlib.h>

#define INVALID_PORT -1
#define USE_IPV6 0

void print_usage(char* program) {
    fprintf(stdout,
            "Usage: %s ftp://[<user>:<password>@]<host>/<url-path> \
            [-p PORT]\n",
            program);
}

int get_url_from_args(int argc,char** argv,url* dest_url)
{
    int port_option = INVALID_PORT;
    char* url_option = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcasecmp(argv[i],"-p") == 0) {
            if (argc != 4 || i==argc-1) {
                print_usage(argv[0]);
                return 1;
            }
            i += 1;
            for (char* s = argv[i]; *s != '\0'; ++s) {
                if (!isdigit(*s)) {
                    print_usage(argv[0]);
                    return 2;
                }
            }
            port_option = atoi(argv[i]);
        } else {
            url_option = argv[i];
        }
    }

    // Uniform resource locator parser
    url url;
    init_url(&url);
    if (parse_url(&url, url_option))
        return -1;

    if (USE_IPV6) {
        if (get_host_ipv6(&url)) {
            printf("Error: Cannot find ip to hostname %s.\n", url.host);
            return -1;
        }
    } else {
        if (get_host_ipv4_new(&url)) {
            printf("Error: Cannot find ip to hostname %s.\n", url.host);
            return -1;
        }
    }

    if (port_option != INVALID_PORT) {
        url.port = port_option;
    }
    *dest_url = url;

    return 0;
}

int main(int argc, char** argv)
{
    url url;
    int get_url_ret = get_url_from_args(argc,argv,&url);
    if (get_url_ret != 0) { return get_url_ret; }
    printf("The IP received to %s was %s:%d\n", url.host,url.ip,url.port);

    // File transfer protocol client
    ftp ftp;
    if (connect_ftp(&ftp, url.ip, url.port)) {
        return 2;
    }

    const char* user = strlen(url.user) ? url.user : "anonymous";
    const char* password = strlen(url.password) ? url.password : "";

	// Sending credentials to server
	if (login_ftp(&ftp, user, password)) {
		printf("Error: Cannot login user %s\n", user);
		return -1;
	}

	// Changing directory
	if (cwd_ftp(&ftp, url.path)) {
		printf("Error: Cannot change directory to the folder of %s\n",
				url.filename);
		return -1;
	}

	// Entry in passive mode
	if (passive_ftp(&ftp)) {
		printf("Error: Cannot entry in passive mode\n");
		return -1;
	}

	// Begins transmission of a file from the remote host
    retr_ftp(&ftp, url.filename);

    // Starting file transfer
    download_ftp(&ftp, url.filename);

    // Disconnecting from server
    disconnect_ftp(&ftp);

    return 0;
}
