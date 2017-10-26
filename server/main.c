#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"

#define PORT 16767

static int msdktest_init_socket(void)
{
	int fd;

	fd = abstract_inet_open(PORT);
	if(fd < 0) {
		perror("create server socket");
		return -1;
	}
	
	return fd;
}

static int msdktest_init_epoll(int fd)
{
	int epfd;
	struct epoll_event ev;

	epfd = epoll_create1(0);
	if (epfd < 0)
		return -1;

	if (fcntl(epfd, F_SETFD, FD_CLOEXEC)) {
		close(epfd);
		return -1;
	}

	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev))
		goto out;
	return epfd;
out:
	close(epfd);
	return -1;
}

int main(void)
{
	int sfd, epfd;

	log_level_init();
	init_data();

	sfd = msdktest_init_socket();
	if (sfd < 0) {
		msdk_log(ERROR, "server create socket error");
		return -1;
	}
	epfd = msdktest_init_epoll(sfd);
	if (epfd < 0) {
		close(sfd);
		return -1;
	}

	if(!mainloop(epfd, sfd, 500))
	{
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
		ev.data.fd = sfd;
		if (epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, &ev))
		{
			perror("epoll_ctl del server fd!");
			return -1;
		}
		close(sfd);
		close(epfd);
	}
	msdk_log(DEBUG, "DEBUG: Server Exit Success!\n");
	return 0;
}

