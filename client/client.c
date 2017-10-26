#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common.h"

#define SERVER_NAME "anbox"
#define PORT 16767

static int do_work(struct msdk_message *msdk_msg)
{
	int ret;
	
	if(!strlen(msdk_msg->cmd))
		return -1;
	ret = system(msdk_msg->cmd);
	if(ret < 0)
	{
		perror("system fork");
		return -1;
	}
	return ret;
}

static char* get_server_addr(const char *name)
{
	char **pptr, *ip;
	struct hostent *hptr;

	ip = malloc(16);
	if (ip == NULL)
		return NULL;
	hptr = gethostbyname(name);
	if (NULL == hptr)
		return NULL;
	for(pptr = hptr->h_addr_list ; *pptr != NULL; pptr++)
	{
		if (NULL != inet_ntop(hptr->h_addrtype, *pptr, ip, 16))
			return ip;
	}
	return NULL;
}

static void fill_send_msg(struct msdk_message *msdk_msg)
{
	msdk_msg->type = MSDK_DATA;
	msdk_msg->cmd[0] = 'G';
}

static void clear_send_msg(struct msdk_message *msdk_msg)
{
	int len;
	len = sizeof(msdk_msg->cmd);
	memset(msdk_msg->cmd, 0, len);
}

static void parse_msg(struct msdk_message *msdk_msg, msg_type *type)
{
	msg_type t;

	t = msdk_msg->type;
	*type = t;
}

int main()
{
	int conn, sock, ret, msg_len;
	msg_type type;
	char *ip;
	ssize_t n;
	struct msdk_message *msdk_msg;
	
	log_level_init();	
	msdk_msg = (struct msdk_message *)malloc(sizeof(struct msdk_message));
	if (!msdk_msg)
	{
		msdk_log(ERROR, "msdk_message malloc error!\n");
		return -1;
	}
	msg_len = sizeof(struct msdk_message);
	ip = get_server_addr(SERVER_NAME);
	if (!ip)
	{
		msdk_log(ERROR, "ERROR:get server ip error");
		return -1;
	}
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		msdk_log(DEBUG, "ERROR:create socket error\n");
		free(ip);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(ip);

	conn = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (conn < 0)
	{
		perror("connect");
		exit(-1);
	}

	msdk_log(DEBUG, "DEBUG: connect success\n");
	clear_send_msg(msdk_msg);
	for(;;) {
		//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
		fill_send_msg(msdk_msg);
		n = send(sock, msdk_msg, msg_len, 0);
		if (n < 0)
		{
			perror("send");
			goto out;
		}
		clear_send_msg(msdk_msg);
		n = recv(sock, msdk_msg, msg_len, 0);
		if (n < 0)
		{
			perror("recv");
			goto out;
		} else if (n == 0) 
		{
			msdk_log(DEBUG, "Server die\n");
			goto out;
		} else
		{
			parse_msg(msdk_msg, &type);
			switch(type)
			{
				case MSDK_DATA:
					ret = do_work(msdk_msg);
					msdk_log(DEBUG, "%s\n", strerror(ret));
					break;
				case MSDK_EXIT:
					goto out;
				default:
					break;
			}
		}
	}
out:
	free(ip);
	close(sock);
	return 0;
}
