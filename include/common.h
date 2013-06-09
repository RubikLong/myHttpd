/*
 * common.h
 *
 *  Created on: Jun 3, 2013
 *      Author: jarwel
 */

#ifndef COMMON_H_
#define COMMON_H_

#define NOFILE 3

#define SIZE 32

#define MAXLINE 80

#define TIME_BUFFER_SIZE	40

typedef struct {
	int port;
	int backlog;
	char web_dir[32];
	char log_dir[32];
	char index[16];
} config ;

extern config getConfig(char *);

extern char * getCurTime();

extern char * getCurDate();

extern char * get_time_str(char *);

extern char * str_replace(const char *, const char *, const char *);

extern char * squeeze(char *, int);

extern char * strtoupper(const char *);

extern char * strtolower(const char *);

extern int writefile(const char *, const char *);

extern char *url_encode(char *str);

extern char *url_decode(char *str);

extern config conf;

#endif /* COMMON_H_ */
