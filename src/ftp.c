#include "ftp.h"

static int connect_socket(const char* ip, int port)
{
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); // 32 bit Internet address network byte ordered
	server_addr.sin_port = htons(port); // server TCP port must be network byte ordered

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr,
			sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int connect_ftp(ftp* ftp, const char* ip, int port)
{
	int socketfd;
	char rd[1024];

	if ((socketfd = connect_socket(ip, port)) < 0) {
		fprintf(stderr, "Error: Cannot connect socket.\n");
		return 1;
	}

	ftp->control_socket_fd = socketfd;
	ftp->data_socket_fd = 0;

	if (read_ftp(ftp, rd, sizeof(rd))) {
		fprintf(stderr, "Error: read_ftp failure.\n");
		return 1;
	}

	return 0;
}

int login_ftp(ftp* ftp, const char* user, const char* password)
{
	char sd[1024];

	// username
	sprintf(sd, "USER %s\r\n", user);
	if (send_ftp(ftp, sd, strlen(sd))) {
		fprintf(stderr, "Error: send_ftp failure.\n");
		return 1;
	}

	if (read_ftp(ftp, sd, sizeof(sd))) {
		fprintf(stderr,
				"Error: Access denied reading username response.\nftpRead failure.\n");
		return 1;
	}

	// cleaning buffer
	memset(sd, 0, sizeof(sd));

	// password
	sprintf(sd, "PASS %s\r\n", password);
	if (send_ftp(ftp, sd, strlen(sd))) {
		fprintf(stderr, "Error: send_ftp failure.\n");
		return 1;
	}

	if (read_ftp(ftp, sd, sizeof(sd))) {
		fprintf(stderr,
				"Error: Access denied reading password response.\nread_ftp failure.\n");
		return 1;
	}

	return 0;
}

int cwd_ftp(ftp* ftp, const char* path)
{
	char cwd[1024];

	sprintf(cwd, "CWD %s\r\n", path);
	if (send_ftp(ftp, cwd, strlen(cwd))) {
		fprintf(stderr, "Error: Cannot send path to CWD.\n");
		return 1;
	}

	if (read_ftp(ftp, cwd, sizeof(cwd))) {
		fprintf(stderr,
				"Error: Cannot send path to change directory.\n");
		return 1;
	}

	return 0;
}

int passive_ftp(ftp* ftp)
{
	char pasv[1024] = "PASV\r\n";
	if (send_ftp(ftp, pasv, strlen(pasv))) {
		fprintf(stderr, "Error: Cannot enter in passive mode.\n");
		return 1;
	}

	if (read_ftp(ftp, pasv, sizeof(pasv))) {
		fprintf(stderr,
				"Error: None information received to enter in passive mode.\n");
		return 1;
	}

	// starting process information
	int ipPart1, ipPart2, ipPart3, ipPart4;
	int port1, port2;
	if ((sscanf(pasv, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
			&ipPart1, &ipPart2, &ipPart3, &ipPart4, &port1, &port2))
			< 0) {
		fprintf(stderr,
				"Error: Cannot process information to calculating port.\n");
		return 1;
	}

	// cleaning buffer
	memset(pasv, 0, sizeof(pasv));

	// forming ip
	if ((sprintf(pasv, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4))
			< 0) {
		fprintf(stderr, "Error: Cannot form ip address.\n");
		return 1;
	}

	// calculating new port
	int portResult = port1 * 256 + port2;

	fprintf(stderr, "IP: %s\n", pasv);
	fprintf(stderr, "PORT: %d\n", portResult);

	if ((ftp->data_socket_fd = connect_socket(pasv, portResult)) < 0) {
		fprintf(stderr,
				"Error: Incorrect file descriptor associated to ftp data socket fd.\n");
		return 1;
	}

	return 0;
}

int retr_ftp(ftp* ftp, const char* filename)
{
	char retr[1024];

	sprintf(retr, "RETR %s\r\n", filename);
	if (send_ftp(ftp, retr, strlen(retr))) {
		fprintf(stderr, "Error: Cannot send filename.\n");
		return 1;
	}

	if (read_ftp(ftp, retr, sizeof(retr))) {
		fprintf(stderr, "Error: None information received.\n");
		return 1;
	}

	return 0;
}

int download_ftp(ftp* ftp, const char* filename)
{
	FILE* file;
	int bytes;

	if (!(file = fopen(filename, "w"))) {
		fprintf(stderr, "Error: Cannot open file.\n");
		return 1;
	}

	char buf[1024];
	while ((bytes = read(ftp->data_socket_fd, buf, sizeof(buf)))) {
		if (bytes < 0) {
			fprintf(stderr,
					"Error: Nothing was received from data socket fd.\n");
			return 1;
		}

		if ((bytes = fwrite(buf, bytes, 1, file)) < 0) {
			fprintf(stderr, "Error: Cannot write data in file.\n");
			return 1;
		}
	}

	fclose(file);
	close(ftp->data_socket_fd);

	return 0;
}

int disconnect_ftp(ftp* ftp)
{
	char disc[1024];

	if (read_ftp(ftp, disc, sizeof(disc))) {
		fprintf(stderr, "Error: Cannot disconnect account.\n");
		return 1;
	}

	sprintf(disc, "QUIT\r\n");
	if (send_ftp(ftp, disc, strlen(disc))) {
		fprintf(stderr, "Error: Cannot send QUIT command.\n");
		return 1;
	}

	if (ftp->control_socket_fd)
		close(ftp->control_socket_fd);

	return 0;
}

int send_ftp(ftp* ftp, const char* str, size_t size)
{
	int bytes;

	if ((bytes = write(ftp->control_socket_fd, str, size)) <= 0) {
		fprintf(stderr, "Warning: Nothing was send.\n");
		return 1;
	}

	fprintf(stderr, "Bytes send: %d\nInfo: %s\n", bytes, str);

	return 0;
}

int read_ftp(ftp* ftp, char* str, size_t size)
{
	FILE* fp = fdopen(ftp->control_socket_fd, "r");

	do {
		memset(str, 0, size);
		str = fgets(str, size, fp);
		fprintf(stdout, "%s", str);
	} while (!('1' <= str[0] && str[0] <= '5') || str[3] != ' ');

	return 0;
}
