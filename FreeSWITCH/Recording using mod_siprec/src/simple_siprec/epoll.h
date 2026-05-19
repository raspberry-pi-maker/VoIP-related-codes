#pragma once
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

int init_epoll();
int buildsocket(int port);
void *epoll_thread(void *data);
