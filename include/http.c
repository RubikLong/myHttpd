/*
 * http.c
 *
 *  Created on: Jun 4, 2013
 *      Author: jarwel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <wait.h>

#include "http.h"

int http_session(int *connect_fd, struct sockaddr_in *client_addr)
{
	char recv_buf[RECV_BUFFER_SIZE + 1]; /* server socket receive buffer */
	unsigned char send_buf[SEND_BUFFER_SIZE + 1]; /* server socket send bufrer */
	char file_buf[FILE_MAX_SIZE + 1];
	char uri_buf[URI_SIZE + 1]; /* store the the uri request from client */

	char uri_get_buf[RECV_BUFFER_SIZE];

	memset(recv_buf, 0, sizeof(recv_buf));
	memset(send_buf, 0, sizeof(send_buf));
	memset(file_buf, 0, sizeof(file_buf));
	memset(uri_buf, 0, sizeof(uri_buf));
	memset(uri_get_buf, 0, sizeof(uri_get_buf));

	int maxfd = *connect_fd + 1;

	fd_set read_set;

	FD_ZERO(&read_set);

	struct timeval timeout;
	timeout.tv_sec = TIME_OUT_SEC;
	timeout.tv_usec = TIME_OUT_USEC;

	int res = 0;
	int read_bytes = 0;
	int send_bytes = 0;
	int file_size = 0;
	char *mime_type;
	int uri_status;
	FD_SET(*connect_fd, &read_set);

	res = select(maxfd, &read_set, NULL, NULL, &timeout);
	if (res == 1)
	{
		if(FD_ISSET(*connect_fd, &read_set))
		{
			memset(recv_buf, 0, sizeof(recv_buf));
			if((read_bytes = recv(*connect_fd, recv_buf, RECV_BUFFER_SIZE, 0)) == 0)
			{
				return 0; //client close the connection
			}
			else if(read_bytes > 0)  //there are some data from client
			{
				if(is_http_protocol(recv_buf) == 0)  //check is it HTTP protocol
				{
					fprintf(stderr, "Not http protocol.\n");
					close(*connect_fd);
					return -1;
				} else { //http protocol
					memset(uri_buf, 0, sizeof(uri_buf));
					if(get_uri(recv_buf, uri_buf, uri_get_buf) == NULL)  //get the uri from http request head
						uri_status = URI_TOO_LONG;
					else
						uri_status = get_uri_status(uri_buf);

					write_httpd_log(uri_buf, uri_get_buf, uri_status);
					switch(uri_status)
					{
						case FILE_OK:
							if (strcmp(get_file_prifix(uri_buf), "sh") == 0) {
								exec_shell(uri_buf, uri_get_buf, file_buf);
								file_size = strlen(file_buf);
							} else {
								file_size = get_file_disk(uri_buf, file_buf);
							}
							mime_type = get_mime_type(uri_buf);
							send_bytes = reply_normal_information(send_buf, file_buf, file_size, mime_type);
							break;
						case FILE_NOT_FOUND:  //file not found on server
							send_bytes = set_error_information(send_buf, FILE_NOT_FOUND);
							break;
						case FILE_FORBIDEN:  //server have no permission to read the request file
							break;
						default:
							break;
					}
					if (send_bytes > 0) {
						send(*connect_fd, send_buf, send_bytes, 0);
					} else {
						fprintf(stderr, "REQEUST SEND BYTES IS ZERO.\n");
						close(*connect_fd);
						return -1;
					}
				}
			}
		} else {
			perror("select() error. in http_sesseion.c");
			close(*connect_fd);
			return -1;
		}
	}
	return 0;
}

void write_httpd_log(const char *uri_buf, const char *uri_get_buf, int uri_status)
{
	char filename[128];
	char cons[512];
	memset(filename, 0, sizeof(filename));
	memset(cons, 0, sizeof(cons));
	sprintf(filename, "%s/%s.log", conf.log_dir, squeeze(getCurDate(), '-'));
	sprintf(cons, "%s\t%s\t[%d]\t%s\n", getCurTime(), uri_buf, uri_status, uri_get_buf);

	char *url_params = (char *) malloc(strlen(uri_get_buf) + 1);
	strcpy(url_params, uri_get_buf);

	writefile(filename, cons);
}

void exec_shell(char *uri_buf, char *uri_get_buf, char *file_buf)
{
	FILE *stream;
	char buf[FILE_MAX_SIZE + 1];
	char cmd[FILE_NAME_SIZE];

	memset(buf, 0, sizeof(buf));
	memset(cmd, 0, sizeof(cmd));
	char *uri_get = str_replace(uri_get_buf, "&", " ");
	if (uri_get != NULL)
		sprintf(cmd, "%s %s", uri_buf, url_decode(uri_get));
	else
		sprintf(cmd, "%s", uri_buf);

	stream = popen(cmd, "r"); //将命令的输出 通过管道读取（“r”参数）到FILE* stream
	fread( buf, sizeof(char), sizeof(buf), stream); //将刚刚FILE* stream的数据流读取到buf中
	pclose( stream );

	strcpy(file_buf, buf);
	return ;
}

int is_http_protocol(char *msg_from_client)
{
    int index = 0;

    while(msg_from_client[index] != '\0' && msg_from_client[index] != '\n')
    	index++;

    if(strncmp(msg_from_client + index - 9, "HTTP/", 5) == 0)
        return 1;

    return 0;
}

char * get_uri(char *req_header, char *uri_buf, char *uri_get_buf)
{
	char uri[RECV_BUFFER_SIZE];
	char filename[32];
	int index = 0, flag = 0;
	char * pch;

	memset(uri, '\0', sizeof(uri));
	int base = 0;
	while((req_header[index] != '\0'))
	{
		if (req_header[index] == ' ')
		{
			if (flag == 0) base = index;
			flag++;
		}
		if (flag > 1) break;
		index++;
	}

	strncpy(uri, req_header + base + 1, index - base - 1);

	pch=strchr(uri,'?');

	if (pch!=NULL) {
		strncpy(filename, uri, pch-uri);
		strcpy(uri_get_buf, uri + (pch-uri+1));
	} else {
		strcpy(filename, uri);
	}

    if((req_header[index - 1] == '/') && (req_header[index] == ' '))
    {
        sprintf(filename, "%s%s", filename, conf.index);
    }

    sprintf(uri_buf, "%s%s", conf.web_dir, filename);

    return uri_buf;
}

int get_uri_status(char *uri)
{
    if(access(uri, F_OK) == -1)
    {
        fprintf(stderr, "File: %s not found.\n", uri);
        return FILE_NOT_FOUND;
    }

    if(access(uri, R_OK) == -1)
    {
        fprintf(stderr, "File: %s can not read.\n", uri);
        return FILE_FORBIDEN;
    }

    return FILE_OK;
}

char *get_file_prifix(char *uri)
{
	int len = strlen(uri);
	int dot = len - 1;
	while( dot >= 0 && uri[dot] != '.')
		dot--;

	if(dot <=  0)		/* if the uri begain with a dot and the dot is the last one, then it is a bad uri request,so return NULL  */
		return NULL;

	dot++;

	char *type_off = uri + dot;

	return type_off;
}

char *get_mime_type(char *uri)
{
	char *type_off = strtolower(get_file_prifix(uri));

	if(type_off == NULL)			/* the uri is '/',so default type text/html returns */
		return "text/html";

	int type_len = strlen(type_off);

	switch(type_len)
	{
		case 4:
			if (!strcmp(type_off, "html"))	return "text/html";

			if (!strcmp(type_off, "jpeg"))	return "image/jpeg";

			break;
		case 3:
			if (!strcmp(type_off, "htm"))	return "text/html";

			if (!strcmp(type_off, "css"))	return "text/css";

			if (!strcmp(type_off, "png"))	return "image/png";

			if (!strcmp(type_off, "jpg"))	return "image/jpeg";

			if (!strcmp(type_off, "gif"))	return "image/gif";

			if (!strcmp(type_off, "txt"))	return "text/plain";

			break;
		case 2:
			if (!strcmp(type_off, "js"))	return "text/javascript";

			if (!strcmp(type_off, "sh"))	return "text/html";

			break;
		default: /* unknown mime type or server do not support type now*/
			return "NULL";
			break;
	}

	return NULL;
}

int get_file_disk(char *uri, char *file_buf)
{
	int read_count = 0;
	int fd = open(uri, O_RDONLY);
	if(fd == -1)
	{
		perror("open() in get_file_disk http_session.c");
		return -1;
	}
	unsigned long st_size;
	struct stat st;
	if(fstat(fd, &st) == -1)
	{
		perror("stat() in get_file_disk http_session.c");
		return -1;
	}
	st_size = st.st_size;
	if(st_size > FILE_MAX_SIZE)
	{
		fprintf(stderr, "the file %s is too large.\n", uri);
		return -1;
	}
	if((read_count = read(fd, file_buf, FILE_MAX_SIZE)) == -1)
	{
		perror("read() in get_file_disk http_session.c");
		return -1;
	}
	return read_count;
}

int reply_normal_information(unsigned char *send_buf, char *file_buf, int file_size,  char *mime_type)
{
	char *str =  "HTTP/1.1 200 OK\r\nServer: Juezhong Long/Linux(1.0.0)\r\nDate:";
	register int index = strlen(str);
	memcpy(send_buf, str, index);

	char time_buf[TIME_BUFFER_SIZE];
	memset(time_buf, '\0', sizeof(time_buf));
	str = get_time_str(time_buf);
	int len = strlen(time_buf);
	memcpy(send_buf + index, time_buf, len);
	index += len;

	len = strlen(ALLOW);
	memcpy(send_buf + index, ALLOW, len);
	index += len;

	memcpy(send_buf + index, "\r\nContent-Type:", 15);
	index += 15;
	len = strlen(mime_type);
	memcpy(send_buf + index, mime_type, len);
	index += strlen(mime_type);

	memcpy(send_buf + index, "\r\nContent-Length:", 17);
	index += 17;
	char num_len[8];
	memset(num_len, '\0', sizeof(num_len));
	sprintf(num_len, "%d", file_size);
	len = strlen(num_len);
	memcpy(send_buf + index, num_len, len);
	index += len;

	memcpy(send_buf + index, "\r\n\r\n", 4);
	index += 4;

	memcpy(send_buf + index, file_buf, file_size);
	index += file_size;
	return index;

}

int set_error_information(unsigned char *send_buf, int errorno)
{
	register int index = 0;
	register int len = 0;
	char *str = NULL;
	switch(errorno)
	{
		case FILE_NOT_FOUND:
			//printf("In set_error_information FILE_NOT_FOUND case\n");
			str = "HTTP/1.1 404 File Not Found\r\nServer: Juezhong Long/Linux(1.0.0)\r\nDate:";
			len = strlen(str);
			memcpy(send_buf + index, str, len);
			index += len;

			char time_buf[TIME_BUFFER_SIZE];
			memset(time_buf, '\0', sizeof(time_buf));
			get_time_str(time_buf);
			len = strlen(time_buf);
			memcpy(send_buf + index, time_buf, len);
			index += len;

			str = "Content-Type:text/html\r\n";
			len = strlen(str);
			memcpy(send_buf + index, str, len);
			index += len;

			memcpy(send_buf + index, "Content-Length:", 15);
			index += 15;

			str = "<html><head></head><body>404 File not found<br/>Please check your url,and try it again!</body></html>";
			len = strlen(str);
			int htmllen = len;
			char num_len[8];
			memset(num_len, '\0', sizeof(num_len));
			sprintf(num_len, "%d", len);
			len = strlen(num_len);
			memcpy(send_buf + index, num_len, len);
			index += len;

			memcpy(send_buf + index, "\r\n\r\n", 4);
			index += 4;

			memcpy(send_buf + index, str, htmllen);
			index += htmllen;

			break;
		default:
			break;

	}
	return index;
}
