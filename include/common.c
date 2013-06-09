/*
 * diskio.c
 *
 *  Created on: Jun 3, 2013
 *      Author: jarwel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "common.h"

config conf;

char * getCurTime()
{
	char static buffer[SIZE];
	struct tm *loctime;
	time_t curtime;
	if(time(&curtime) == -1)
	{
		perror("time() in get_time.c");
		return NULL;
	}
	loctime = localtime(&curtime);
	strftime(buffer, SIZE, "%Y-%m-%d %H:%M:%S", loctime);
	return buffer;
}

char * getCurDate()
{
	char static date_buf[20];
	time_t	now_sec;
	struct tm *time_now;

	if(time(&now_sec) == -1)
	{
		perror("time() in get_time.c");
		return NULL;
	}

	time_now = localtime(&now_sec);
	strftime(date_buf, 20, "%Y-%m-%d", time_now);

	return date_buf;
}

config getConfig(char *filename)
{

	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	config conf;

	char * delimiter = "=\n";
	char * token;

	fp = fopen(filename, "r");
	if (fp == NULL)	exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fp)) != -1)
	{
		token = strtok(line, delimiter);

		if (strcmp(token, "port") == 0) {
			token = strtok(NULL, delimiter);
			conf.port = atoi(token);
		} else if (strcmp(token, "backlog") == 0) {
			token = strtok(NULL, delimiter);
			conf.backlog = atoi(token);
		} else if (strcmp(token, "documentRoot") == 0) {
			token = strtok(NULL, delimiter);
			strcpy(conf.web_dir, token);
		} else if (strcmp(token, "logRoot") == 0) {
			token = strtok(NULL, delimiter);
			strcpy(conf.log_dir, token);
		} else if (strcmp(token, "index") == 0) {
			token = strtok(NULL, delimiter);
			strcpy(conf.index, token);
		}
	}

	if (line) free(line);

	fclose(fp);

	return conf;
}

char * get_time_str(char *time_buf)
{
	time_t	now_sec;
	struct tm	*time_now;
	if(	time(&now_sec) == -1)
	{
		perror("time() in get_time.c");
		return NULL;
	}
	if((time_now = gmtime(&now_sec)) == NULL)
	{
		perror("localtime in get_time.c");
		return NULL;
	}
	char *str_ptr = NULL;
	if((str_ptr = asctime(time_now)) == NULL)
	{
		perror("asctime in get_time.c");
		return NULL;
	}
	strcat(time_buf, str_ptr);
	return time_buf;
}

char * str_replace( const char *string, const char *substr, const char *replacement )
{
    char * pch = NULL;
    char * newstr = NULL;
    int len = strlen(substr);

    if (strlen(string) == 0) return NULL;

    newstr = (char *) malloc(strlen(string) * sizeof(char));
    if( newstr == NULL ) return NULL;

    memcpy(newstr, string, strlen(string));

    char ssub[len];

    pch = strchr(newstr, substr[0]);

    while (pch!=NULL)
    {
        memcpy(ssub, pch, len);
        if (strcmp(ssub, substr) == 0)
            strncpy (pch, replacement , len);

        pch=strchr(pch+1, substr[0]);
    }

    return newstr;
}

char * squeeze(char *s, int c)
{
	char *ss = NULL;
    int i,j;
    ss = (char *) malloc(strlen(s) * sizeof(char));
    for (i = 0, j = 0; s[i] != '\0'; i++)
    {
        if (s[i] != c)
        {
            ss[j++] = s[i];
        }
    }
    ss[j] = '\0';
    return ss;
}

char * strtoupper(const char *str)
{
    int i;
    char *p = (char*) malloc((strlen(str) + 1) * sizeof(char));
    for(i = 0; str[i] != '\0'; ++i)
    {
        if((str[i] >= 'a') && (str[i] <= 'z'))
            p[i] = str[i] + 'A' - 'a';
        else
            p[i] = str[i];
    }
    p[i] = '\0';

    return p;
}

char * strtolower(const char *str)
{
    int i;
    char *p = (char*) malloc((strlen(str) + 1) * sizeof(char));
    for(i = 0; str[i] != '\0'; ++i)
    {
        if((str[i] >= 'A') && (str[i] <= 'Z'))
            p[i] = str[i] - 'A' + 'a';
        else
            p[i] = str[i];
    }
    p[i] = '\0';

    return p;
}

int writefile(const char *filename, const char *cons)
{
    FILE *f;

    f = fopen(filename, "a+");
    if (f != NULL) {
    	fprintf(f, cons);
    	fclose(f);
    } else {
    	perror("CAN'T OPEN FILE");
    }

    return 0;
}

/* Converts a hex character to its integer value */
char from_hex(char ch)
{
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code)
{
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str)
{
	char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;

	int i = 0;
	while (*pstr)
	{
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
			*pbuf++ = *pstr;
		else if (*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);

		pstr++;
		i++;
	}
	*pbuf = '\0';

	return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str)
{
	char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
	while (*pstr) {
		if (*pstr == '%') {
			if (pstr[1] && pstr[2]) {
				*pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
				pstr += 2;
			}
		} else if (*pstr == '+') {
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *pstr;
		}
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

