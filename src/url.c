#include "url.h"

static char* process_until_char(char* str, char chr);

void init_url(url* url)
{
    // fill with zero
    memset(url->user, 0, sizeof(url_content));
    memset(url->password, 0, sizeof(url_content));
    memset(url->host, 0, sizeof(url_content));
    memset(url->path, 0, sizeof(url_content));
    memset(url->filename, 0, sizeof(url_content));
    // default port
    url->port = 21;
}

/* ftp://user:pass@ftp.up.pt:/abc */
const char* USER_PW_REGEX =
  "ftp://([A-Za-z0-9])+:([A-Za-z0-9])+@([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";
/* ftp://ftp.up.pt/abc */
const char* ANONYMOUS_REGEX =
  "ftp://([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";

int parse_url(url* url, const char* URLSTR)
{
    const char USER_SEPARATOR = '@';

    /* copy url string to temporary */
    char* tempURL = (char*) malloc(strlen(URLSTR));
    memcpy(tempURL, URLSTR, strlen(URLSTR));

    /* Use password? */
    int use_password;
    char* active_regex;
    if (strchr(tempURL, USER_SEPARATOR) != NULL) { // find separator
        use_password = 1;
        active_regex = (char*) USER_PW_REGEX;
    } else {
        use_password = 0;
        active_regex = (char*) ANONYMOUS_REGEX;
    }

    /* Check validity of URL against regex */
    regex_t* regex = (regex_t*) malloc(sizeof(regex_t));
    int reti = regcomp(regex,active_regex,REG_EXTENDED); // compile regex
    if (reti) {
        perror("URL regex error");
        return 1;
    }
    size_t nmatch = strlen(URLSTR);
    regmatch_t pmatch[nmatch];
    if ((reti = regexec(regex,tempURL,nmatch,pmatch,REG_EXTENDED)) != 0) {
        perror("URL regex mismatch");
        return 1;
    }
    free(regex);

    // removing ftp:// from string
    strcpy(tempURL, tempURL + 6);

    /*
     * Write to URL struct:
     */

    char* element = (char*) malloc(strlen(URLSTR));
    if (use_password) {
        // saving username
        strcpy(element, process_until_char(tempURL, ':'));
        memcpy(url->user,element,strlen(element));

        // saving password
        strcpy(element, process_until_char(tempURL, '@'));
        memcpy(url->password, element, strlen(element));
    }

    // Setting host
    strcpy(element, process_until_char(tempURL, '/'));
    memcpy(url->host, element, strlen(element));

    // Setting URL path
    char* path = (char*) malloc(strlen(tempURL));
    int startPath = 1;
    while (strchr(tempURL, '/')) {
        element = process_until_char(tempURL, '/');

        if (startPath) {
            startPath = 0;
            strcpy(path, element);
        } else {
            strcat(path, element);
        }

        strcat(path, "/");
    }

    strcpy(url->path, path);

    // Setting filename
    strcpy(url->filename, tempURL);

    free(tempURL);
    free(element);

//	fprintf(stdout, "\n%s\n%s\n%s\n%s\n%s\n", url->user, url->password,
//			url->host, url->path, url->filename);

	return 0;
}

int get_host_ip(url* url)
{
    struct hostent* h;

    if ((h = gethostbyname(url->host)) == NULL) {
        perror("get_host_ip");
        return 1;
    }

//	fprintf(stdout, "Host name  : %s\n", h->h_name);
//	fprintf(stdout, "IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));

    /* "inet_ntoa()" converts a numeric address (in network byte order) to the
     * IPv4 numbers-and-dots representation.  */
    char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
    strcpy(url->ip, ip);
    return 0;
}

static char* process_until_char(char* str, char chr)
{
    // using temporary string to process substrings
    char* tempStr = (char*) malloc(strlen(str));

    // calculating length to copy element
    // eg, copy @pass/abc, compute length, subtract from length of string
    int index = strlen(str) - strlen(strcpy(tempStr, strchr(str, chr)));

    tempStr[index] = '\0'; // termination char in the end of string
    strncpy(tempStr, str, index);

    // delete from the beginning of string
    strcpy(str, str + strlen(tempStr) + 1);

    return tempStr;
}
