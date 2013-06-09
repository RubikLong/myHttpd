/*
 * http.h
 *
 *  Created on: Jun 4, 2013
 *      Author: jarwel
 */

#ifndef HTTP_H_
#define HTTP_H_

#include "common.h"

#define RECV_BUFFER_SIZE	1024		/* 1KB of receive buffer  */
#define	SEND_BUFFER_SIZE	1050000		/* 1.xMB of send buffer  */
#define	URI_SIZE			128			/* length of uri request from client browse */
#define FILE_NAME_SIZE		1024

#define TIME_OUT_SEC		10			/* select timeout of secend */
#define TIME_OUT_USEC		0			/* select timeout of usecend */

#define	FILE_OK				200
#define	FILE_FORBIDEN		403			/* there are no access permission*/
#define FILE_NOT_FOUND		404			/* file not found on server */
#define	UNALLOW_METHOD		405			/* un allow http request method*/
#define FILE_TOO_LARGE		413			/* file is too large */
#define	URI_TOO_LONG		414			/*  */
#define	UNSUPPORT_MIME_TYPE	415
#define	UNSUPPORT_HTTP_VERSION	505
#define	FILE_MAX_SIZE		1048576		/* 1MB the max siee of file read from hard disk */

#define ALLOW				"Allow:GET"	/* the server allow GET request method*/
#define	SERVER				"Server:Juezhong Long(1.0.0 Alpha)/Linux"

extern int http_session(int *, struct sockaddr_in *);

void exec_shell(char *, char *, char *);

void write_httpd_log(const char *, const char *, int);

int is_http_protocol(char *);

char * get_uri(char *, char *, char *);

int get_uri_status(char *);

char *get_file_prifix(char *);

char *get_mime_type(char *);

int get_file_disk(char *, char *);

int reply_normal_information(unsigned char *, char *, int ,  char *);

int set_error_information(unsigned char *, int);

#endif /* HTTP_H_ */
