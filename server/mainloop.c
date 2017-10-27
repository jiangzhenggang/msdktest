#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "common.h"

#define TEST_ITEMS 1000
#define MAX_LEN 100
#define MAX_EVENTS 100

#define CONFIG_PATH "/var/run.sh"

static char *contents[TEST_ITEMS];
static int sub;
static int totallen;

static int readline(char *file, char *readerbuf[])
{
	int fd, i, j, ishead;
	ssize_t n, n1;
	char tmp;

	fd = open(file, O_RDONLY);
	ishead = 1;
	i = -1;
	j = 0;
	while (1)
	{
		if ((i + 1) > TEST_ITEMS)
		{
			msdk_log(ERROR, "ERROR: Panic, config file too large!!!\n ");
			close(fd);
			return -1;
		}
		n = read(fd, &tmp, 1);
		if (n < 0)
			return -1;
		if (n > 0 && ishead)
		{
			while (tmp == ' ' || tmp == '	')
			{
				n1 = read(fd, &tmp, 1);
				if (n1 == 0)
				{
					totallen = i + 1;
					close(fd);
					return 0;
				}
			}
			j = 0;
			ishead = 0;
			if (tmp != '#' && tmp != '\n')
			{
				i++;
				*(readerbuf[i] + j) = tmp;
			}
			else if (tmp == '#')
			{
				while (1)
				{
					n1 = read(fd, &tmp, 1);
					if (tmp == '\n')
					{
						ishead = 1;
						break;
					}
				}
			}
			else if (tmp == '\n')
				ishead = 1;
		}
		else if (n > 0)
		{
			++j;
			*(readerbuf[i] + j) = tmp;
			if (tmp == '\n')
			{
				ishead = 1;
				*(readerbuf[i] + j) = '\0';
			}
		}
		else if (n == 0)
		{
			//end
			close(fd);
			totallen = i + 1;
			break;
		}
	}
	return 0;
}

void init_data(void)
{
	int i, ret;

	for (i = 0; i < TEST_ITEMS; ++i)
	{
		contents[i] = malloc(MAX_LEN);
		if (!contents[i])
		{
			msdk_log(ERROR, "ERROR:Contents malloc!\n");
			exit(1);
		}
	}
	if (access(CONFIG_PATH, F_OK) == 0) {
		ret = readline(CONFIG_PATH, contents);
		if (ret < 0)
		{
			msdk_log(ERROR, "ERROR: Read %s failed!\n", CONFIG_PATH);
			exit(1);
		}
		sub = 0;
	}
	else {
		msdk_log(ERROR, "ERROR: Cannot find config file!\n");
		exit(1);
	}
}

//int setnonblocking(int sock)
//{
//	int opts;
//	opts = fcntl(sock, F_GETFL);
//	if(opts < 0)
//	{
//		perror("fcntl(sock, GETFL)");
//		return -1;
//	}
//	opts = opts | O_NONBLOCK;
//	if(fcntl(sock, F_SETFL, opts) < 0)
//	{
//		perror("fcntl(sock,SETFL,opts)");
//		return -1;
//	}
//}

static int mainloop_read(int fd, struct msdk_message *recvbuf)
{
	int len, ret;

	len = sizeof(struct msdk_message);
	ret = recv(fd, recvbuf, len, 0);

	msdk_log(DEBUG, "DEBUG,type: %d\n", recvbuf->type);
	return ret;
}

static int mainloop_handler(int fd, struct msdk_message *sendbuf)
{
	int len, ret;
	char *p;

	len = sizeof(struct msdk_message);
	memset(sendbuf, 0, len);
	if (sub < totallen)
	{
		sendbuf->type = MSDK_DATA;
		p = strncpy(sendbuf->cmd, contents[sub], strlen(contents[sub]));
		if (p == NULL)
		{
			perror("strncpy");
			return -1;
		}
		sub++;
		ret = send(fd, sendbuf, len, 0);
	}
	else {
		sendbuf->type = MSDK_EXIT;
		ret = send(fd, sendbuf, len, 0);
	}

	return ret;
}

int abstract_inet_open(int port)
{
	int fd, ret;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0)
		return -1;

	ret = listen(fd, 200);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int mainloop(int efd, int sfd, int timeout_ms)
{
	int i, clifd, conn, nfds, clientfds;
	ssize_t n;
	socklen_t clilen;
	struct epoll_event ev, events[MAX_EVENTS];
	struct sockaddr_in cliaddr;
	struct msdk_message recvbuf;

	//init clientfds;
	clientfds = 12345678;

	memset((void *)&cliaddr, 0, sizeof(cliaddr));
	for (;;) {
		nfds = epoll_wait(efd, events, MAX_EVENTS, timeout_ms);
		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == sfd) {
				clifd = accept(sfd, (struct sockaddr *)&cliaddr, &clilen);
				if (clifd < 0) {
					msdk_log(ERROR, "accept error!\n");
					continue;
				}
				if (clientfds == 12345678)
					clientfds = 0;
				++clientfds;
				msdk_log(DEBUG, "DEBUG: accept success!\n");
				//setnonblocking(client);
				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				ev.data.fd = clifd;
				if (epoll_ctl(efd, EPOLL_CTL_ADD, clifd, &ev) < 0) {
					msdk_log(ERROR, "epoll set insertion error: fd=%d\n",
						clifd);
					return -1;
				}
			}
			else if (events[i].events & EPOLLIN) {
				//read
				conn = events[i].data.fd;
				//n = read(cli_sock, recvbuf, sizeof(recvbuf));
				memset(&recvbuf, 0, sizeof(struct msdk_message));
				n = mainloop_read(conn, &recvbuf);
				if (n == -1) {
					printf("read client sock error!\n");
					//fix me!
				}
				if (n == 0) {
					close(conn);
					epoll_ctl(efd, EPOLL_CTL_DEL, conn, &events[i]);
					--clientfds;
				}
				if (n > 0) {
					mainloop_handler(conn, &recvbuf);
				}
			}
		}
		if (clientfds == 0)
		{
			for (i = 0; i < TEST_ITEMS; ++i)
			{
				free(contents[i]);
			}
			return 0;
		}
		msdk_log(DEBUG, "DEBUG: running... \n");
		//to be continue
	}
}

