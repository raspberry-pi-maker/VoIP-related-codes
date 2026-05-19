#include "global.h"
#include "epoll.h"
#include "sip_util.h"
#include "rtp.h"

#include <iostream>
#include <queue>
#include <string>

epoll_event *events;
int efd;
int create_and_bind (int port);
int make_socket_non_blocking (int sfd);

int init_epoll()
{
    efd = epoll_create1 (0);
    if (efd == -1){
        std::cout << "socket epoll create error" << std::endl;
        exit(0);
    }

    events = (epoll_event *)calloc (MAXEVENTS, sizeof(epoll_event));
    return 0;
}

int buildsocket(int port)
{
    int sfd = create_and_bind (port);
    if (sfd == -1){
        std::cout << "socket bind error : port[" << port << "]" << std::endl;
        return 0;
    }

    int s = make_socket_non_blocking (sfd);
    if (s == -1){
        std::cout << "socket make nonblocking error" << std::endl;
        return 0;
    }
    std::cout << "SIPREC socket port[" << port << "] bind success" << std::endl;
    epoll_event event;
    
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;
    event.events = EPOLLIN;
    int rc = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
    if (rc == -1){
        std::cout << "socket epoll_ctl error" << std::endl;
        return 0;
    }
    return sfd;
}

int make_socket_non_blocking (int sfd)
{
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

/*
Create UDP socket
*/
int create_and_bind (int port)
{
    int err_val;
    int s;
    
    struct sockaddr_in serveraddr{};    
    
    s = socket(AF_INET, SOCK_DGRAM, 0);    
    if(-1 == s){
        err_val = errno;
        std::cout << "Err:socket creation Error ->Code[" << err_val << "]" << std::endl;
        return -1;
    }
    int option = 1;
    if( 0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) )
    {
        std::cout << "Error(" << errno << "): create_socket: setsockopt failure!: " 
                  << strerror(errno) << std::endl;
    }

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);        
    err_val = bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

    if(-1 == err_val){
        err_val = errno;
        std::cout << "socket bind port[" << port << "] Error ->Code[" << err_val << "]" << std::endl;
        return -1;
    }
    return s;
}

void *epoll_thread(void *data)
{
  
    sockaddr_in cliaddr{};
    socklen_t addr_len = sizeof(cliaddr);
  
    while (1)
    {
        int n, i;
        n = epoll_wait (efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
            {
                std::cerr << "epoll error -> client socket closed" << std::endl;
                continue;
            }
            else
            {
                ssize_t count;
                char buf[SIP_BUFF];

                count = recvfrom(events[i].data.fd, buf, SIP_BUFF, 0, (struct sockaddr*)&cliaddr, &addr_len);        
                if (count == -1)
                {
                    if (errno != EAGAIN)
                    {
                        std::cout << "Err " << std::endl;
                    }
                    break;
                }
                else if (count == 0)
                {
                    std::cout << "socket[" << events[i].data.fd << "] closed" << std::endl;
                    break;
                }
                else {
                    if(g_siprec_sock == events[i].data.fd){
                        if(count >= SIP_BUFF || count < 0){ 
                            std::cout << "invalid siprec sock read size[" << count << "]" << std::endl;
                            buf[SIP_BUFF - 1] = 0x00;
                            std::cout << "invalid siprec sock read [" << buf << "]" << std::endl;
                            continue;
                        }
                        g_siprec_addr = cliaddr;
                        g_siprec_addr_len = addr_len;
                        
                        buf[count] = 0x00;
                        std::string data(buf);
                        //std::cout << buf << std::endl;
                        {
                          std::lock_guard<std::mutex> lock(g_sip_mutex);
                          g_siprec_queue.push(data);
                        }
                        g_cv_siprec.notify_one(); // Consumers(main.cpp) wake up only when called.
                    }
                    else{
                      int remote_port = ntohs(cliaddr.sin_port);
                      do_rtp((const unsigned char *)buf, count, events[i].data.fd, remote_port);
                    }
                }
            }
        }
    }  
    free (events);
    return NULL;
}
