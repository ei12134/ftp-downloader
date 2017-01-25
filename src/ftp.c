#include "ftp.h"
#include <time.h>

#define NO_NUMBER 0

/* Call connect() on ip and port (which are in host byte order.) */
static int connect_socket(const char* ip, int port)
{
    // open a TCP socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // server address handling
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // 32 bit Internet address network byte ordered:
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // server TCP port must be network byte ordered:
    server_addr.sin_port = htons(port);

    // connect to the FTP server
    if (connect(
            sockfd,
            (struct sockaddr*)&server_addr,
            sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }
    return sockfd;
}

int connect_ftp(ftp* ftp, const char* ip, int port)
{
    printf("Connecting to %s:%d...\n", ip, port);
    int socketfd;
    if ((socketfd = connect_socket(ip, port)) < 0) {
        fprintf(stderr, "Error: Cannot connect socket.\n");
        return 1;
    }

    ftp->control_socket_fd = socketfd;
    ftp->data_socket_fd = 0;

    /* read first message */
    char rd[1024];
    if (read_ftp(ftp, rd, sizeof(rd))) {
        fprintf(stderr, "Error: read_ftp failure.\n");
        return 1;
    }

    return 0;
}

int ftp_command(ftp* ftp, const char* cmd, const char* args, char* rep, const int reply1, const int reply2)
{
    char s[1024];

    if (args != NULL) {
        sprintf(s, "%s %s\r\n", cmd, args);
    } else {
        sprintf(s, "%s\r\n", cmd);
    }
    if (send_ftp(ftp, s, strlen(s))) {
        fprintf(stderr, "Error: send_ftp failure.\n");
        return 1;
    }
    if (read_ftp(ftp, s, sizeof(s))) {
        fprintf(stderr, "Error: read_ftp failure.\n");
        return 1;
    }
    if (rep != NULL) {
        strcpy(rep, s);
    }

    char num[4];
    strncpy(num, s, 3);
    num[3] = '\0';
    if (atoi(num) != reply1 && atoi(num) != reply2) {
        fprintf(stderr, "Error: Reply code %s != %d || %d\n", num, reply1, reply2);
        return 2;
    }
    return 0;
}

int login_ftp(ftp* ftp, const char* user, const char* password)
{
    // send the user
    if (ftp_command(ftp, "USER", user, NULL, 331, NO_NUMBER)) {
        return 1;
    }
    // send the password
    if (ftp_command(ftp, "PASS", password, NULL, 230, NO_NUMBER)) {
        return 2;
    }
    return 0;
}

int list_ftp(ftp* ftp, char* path)
{
    path = strlen(path) == 0 ? path : ".";
    if (ftp_command(ftp, "LIST", NULL, NULL, 125, 150)) {
        return 1;
    }
    return 0;
}

int cwd_ftp(ftp* ftp, const char* path)
{
    if (strlen(path) == 0) {
        fprintf(stderr, "CWD: Empty path.\n");
        return 0;
    }
    if (ftp_command(ftp, "CWD", path, NULL, 250, NO_NUMBER)) {
        return 1;
    }
    return 0;
}

int passive_ftp(ftp* ftp)
{
    char reply[256];

    if (ftp_command(ftp, "PASV", "", reply, 227, NO_NUMBER)) {
        return 1;
    }

    // starting process information
    int ipPart1, ipPart2, ipPart3, ipPart4;
    int port1, port2;
    if ((sscanf(reply, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                &ipPart1, &ipPart2, &ipPart3, &ipPart4, &port1, &port2))
        < 0) {
        fprintf(stderr,
                "Error: Cannot process information to calculating port.\n");
        return 1;
    }

    char ipstr[256];

    // format IP address
    if ((sprintf(ipstr, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4))
        < 0) {
        fprintf(stderr, "Error: Cannot form IP address.\n");
        return 1;
    }

    // calculating new port
    int portResult = port1 * 256 + port2;

    fprintf(stderr, "IP: %s\n", ipstr);
    fprintf(stderr, "PORT: %d\n", portResult);

    if ((ftp->data_socket_fd = connect_socket(ipstr, portResult)) < 0) {
        fprintf(stderr,
                "Error: Incorrect file descriptor associated to ftp data socket fd.\n");
        return 1;
    }

    return 0;
}

int retr_ftp(ftp* ftp, const char* filename)
{
    return ftp_command(ftp, "RETR", filename, NULL, 125, 150);
}

int download_ftp(ftp* ftp, const char* filename)
{
    FILE* file;
    if (!(file = fopen(filename, "w"))) {
        fprintf(stderr, "Error: Cannot open file.\n");
        return 1;
    }

    char buf[1024];
    int bytes;
    long progress = 0;
    time_t t = time(NULL);
    while ((bytes = read(ftp->data_socket_fd, buf, sizeof(buf)))) {
        progress += bytes;
        if (time(NULL) - t > 1) {
            printf("Downloaded %ld B\n", progress);
            t = time(NULL);
        }
        if (bytes < 0) {
            fprintf(stderr,
                    "Error: Nothing was received from data socket fd.\n");
            return 1;
        }

        if ((bytes = fwrite(buf, bytes, 1, file)) < 0) {
            fprintf(stderr, "Error: Cannot write data in file.\n");
            return 2;
        }
    }

    char s[1024];
    if (read_ftp(ftp, s, sizeof(s))) {
        fprintf(stderr, "Error: read_ftp failure.\n");
        return 1;
    }
    printf("%s\n", s);

    fclose(file);
    close(ftp->data_socket_fd);

    return 0;
}

int disconnect_ftp(ftp* ftp)
{
    if (ftp_command(ftp, "QUIT", NULL, NULL, 221, 226)) {
        return 1;
    }
    if (ftp->control_socket_fd) {
        return close(ftp->control_socket_fd);
    }
    return 0;
}

int send_ftp(ftp* ftp, const char* msg, size_t size)
{
    int bytes;

    if ((bytes = write(ftp->control_socket_fd, msg, size)) <= 0) {
        fprintf(stderr, "Warning: Nothing was sent.\n");
        return 1;
    }

    fprintf(stderr, "Message (%d bytes): %.*s\n", bytes, (int)size - 1, msg);

    return 0;
}

int read_ftp(ftp* ftp, char* str, size_t size)
{
    FILE* fp = fdopen(ftp->control_socket_fd, "r");

    do {
        memset(str, 0, size);
        str = fgets(str, size, fp);
        if (str == NULL) {
            return 1;
        }
        fprintf(stdout, "%s\n", str);
    } while (!('1' <= str[0] && str[0] <= '5') || str[3] != ' ');

    return 0;
}
