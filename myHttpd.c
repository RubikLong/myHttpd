/*
 * myHttpd.c
 *
 *  Created on: Jun 3, 2013
 *      Author: jarwel
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "include/common.h"
#include "include/http.h"

#define CONFIG "etc/myHttpd.conf"

void init_daemon();
int init_socket(int *, struct sockaddr_in *);

int main(int argc, char * argv[])
{
	int listen_fd, connect_fd;

	struct sockaddr_in server_addr, client_addr;

	pid_t ppid;

	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	conf = getConfig(CONFIG);

	if(init_socket(&listen_fd, &server_addr) == -1) {
		perror("init_socket() error. in myHttpd.c");
	    exit(EXIT_FAILURE);
	}

	socklen_t addrlen = sizeof(struct sockaddr_in);


    signal(SIGCHLD, SIG_IGN);   /* 忽略子进程结束信号，防止出现僵尸进程 */

    init_daemon();

    //printf("Accepting connections ...\n");

    while (1)
    {
        if((connect_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
            perror("accept() error. in myHttpd.c");
            continue;
        }

        if( (ppid = fork()) > 0) {
            close(connect_fd);
        } else if(ppid == 0) {
            close(listen_fd);
            //printf("pid %d process http from %s:%d\n", getpid(), inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
			if(http_session(&connect_fd, &client_addr) == -1) {
				printf("pid %d loss connection to %s\n", getpid(), inet_ntoa(client_addr.sin_addr));
				shutdown(connect_fd, SHUT_RDWR);
				exit(EXIT_FAILURE);     /* exit from child process, stop this http session  */
			}

			//printf("pid %d close connection to %s\n", getpid(), inet_ntoa(client_addr.sin_addr));
			shutdown(connect_fd, SHUT_RDWR);
			exit(EXIT_SUCCESS);
        } else {
            perror("fork() error. in myHttpd.c");
            exit(EXIT_FAILURE);
        }
	}

    return 0;
}

void init_daemon()
{
    int i;
    pid_t pid = fork();              //fork a process

    if (pid < 0)	exit(1);         //fork error
    if (pid > 0)	exit(0);         //father process exit

    //子进程继续执行

    setsid(); //创建新的会话组，子进程成为组长，并与控制终端分离

    //第二子进程继续执行 , 第二子进程不再是会会话组组长
    for (i = 0; i < NOFILE; i++) {   //关闭打开的文件描述符
        close(i);
    }

    return;
}


int init_socket(int *listen_fd, struct sockaddr_in *server_addr)
{
    if((*listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error. in myHttpd.c");
        return -1;
    }

    /* set reuse the port on server machine  */
    int opt = 1;

    if(setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt() error.  in myHttpd.c");
        return -1;
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(conf.port);
    server_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(*listen_fd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind() error.  in myHttpd.c");
        return -1;
    }

    if(listen(*listen_fd, conf.backlog) == -1) {
        perror("listen() error.  in myHttpd.c");
        return -1;
    }

    return 0;
}
