#include <stdio.h>

#include "ftp.h"
#include "url.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stdout,
				"Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n",
				argv[0]);
		return 1;
	}

	///////////// URL PROCESS /////////////
	url url;
	init_url(&url);

	// start parsing argv[1] to URL components
	if (parse_url(&url, argv[1]))
		return -1;

	// edit url ip by hostname
	if (get_ip_by_hostname(&url)) {
		printf("ERROR: Cannot find ip to hostname %s.\n", url.host);
		return -1;
	}

	printf("\nThe IP received to %s was %s\n", url.host, url.ip);

	///////////// FTP CLIENT PROCESS /////////////

	ftp ftp;
	connect_ftp(&ftp, url.ip, url.port);

	// Verifying username
	const char* user = strlen(url.user) ? url.user : "anonymous";

	// Verifying password
	char* password;
	if (strlen(url.password)) {
		password = url.password;
	} else {
		char buf[100];
		printf("You are now entering in anonymous mode.\n");
		printf("Please insert your college email as password: ");
		while (strlen(fgets(buf, 100, stdin)) < 14)
			printf("\nIncorrect input, please try again: ");
		password = (char*) malloc(strlen(buf));
		strncat(password, buf, strlen(buf) - 1);
	}

	// Sending credentials to server
	if (login_ftp(&ftp, user, password)) {
		printf("ERROR: Cannot login user %s\n", user);
		return -1;
	}

	// Changing directory
	if (cwd_ftp(&ftp, url.path)) {
		printf("ERROR: Cannot change directory to the folder of %s\n",
				url.filename);
		return -1;
	}

	// Entry in passive mode
	if (passive_ftp(&ftp)) {
		printf("ERROR: Cannot entry in passive mode\n");
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
