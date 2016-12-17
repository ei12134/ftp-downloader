#include "url.h"

void init_url(url* url)
{
	memset(url->user, 0, sizeof(url_content));
	memset(url->password, 0, sizeof(url_content));
	memset(url->host, 0, sizeof(url_content));
	memset(url->path, 0, sizeof(url_content));
	memset(url->filename, 0, sizeof(url_content));
	url->port = 21;
}

const char* user_pw_regex =
		"ftp://([([A-Za-z0-9])*:([A-Za-z0-9])*@])*([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";
const char* anonymous_regex = "ftp://([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";

int parse_url(url* url, const char* urlStr)
{
	char* tempURL, *element, *activeExpression;
	char target = '@';
	regex_t* regex;
	size_t nmatch = strlen(urlStr);
	regmatch_t pmatch[nmatch];
	int user_password_mode;

	element = (char*) malloc(strlen(urlStr));
	tempURL = (char*) malloc(strlen(urlStr));

	memcpy(tempURL, urlStr, strlen(urlStr));

	if (strchr(tempURL, target) != NULL) {
		user_password_mode = 1;
		activeExpression = (char*) user_pw_regex;
	} else {
		user_password_mode = 0;
		activeExpression = (char*) anonymous_regex;
	}

	regex = (regex_t*) malloc(sizeof(regex_t));

	int reti;
	if ((reti = regcomp(regex, activeExpression, REG_EXTENDED)) != 0) {
		perror("URL format is wrong.");
		return 1;
	}

	if ((reti = regexec(regex, tempURL, nmatch, pmatch, REG_EXTENDED))
			!= 0) {
		perror("URL could not execute.");
		return 1;
	}

	free(regex);

	// removing ftp:// from string
	strcpy(tempURL, tempURL + 6);

	if (user_password_mode) {
		// saving username
		strcpy(element, process_until_char(tempURL, ':'));
		memcpy(url->user, element, strlen(element));

		// saving password
		strcpy(element, process_until_char(tempURL, '@'));
		memcpy(url->password, element, strlen(element));
	}

	//saving host
	strcpy(element, process_until_char(tempURL, '/'));
	memcpy(url->host, element, strlen(element));

	//saving url path
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

	// saving filename
	strcpy(url->filename, tempURL);

	free(tempURL);
	free(element);

//	fprintf(stdout, "\n%s\n%s\n%s\n%s\n%s\n", url->user, url->password,
//			url->host, url->path, url->filename);

	return 0;
}

int get_ip_by_hostname(url* url)
{
	struct hostent* h;

	if ((h = gethostbyname(url->host)) == NULL) {
		herror("gethostbyname");
		return 1;
	}

//	fprintf(stdout, "Host name  : %s\n", h->h_name);
//	fprintf(stdout, "IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

	return 0;
}

char* process_until_char(char* str, char chr)
{
	// using temporary string to process substrings
	char* tempStr = (char*) malloc(strlen(str));

	// calculating length to copy element
	int index = strlen(str) - strlen(strcpy(tempStr, strchr(str, chr)));

	tempStr[index] = '\0'; // termination char in the end of string
	strncpy(tempStr, str, index);
	strcpy(str, str + strlen(tempStr) + 1);

	return tempStr;
}
