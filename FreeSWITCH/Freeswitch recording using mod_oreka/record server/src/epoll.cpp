#include"global.h"
#include"epoll.h"
#include"sip_parser.h"
#include"rtp.h"

epoll_event *events;
int efd;
int create_and_bind (int port);
int make_socket_non_blocking (int sfd);

int init_epoll()
{
	efd = epoll_create1 (0);
	if (efd == -1){
		LogToFile(err, "socket epoll create error");
		return -1;
	}

	events = (epoll_event *)calloc (MAXEVENTS, sizeof(epoll_event));
    return 0;
}

int buildsocket(int port)
{
	int sfd = create_and_bind (port);
	if (sfd == -1){
		LogToFile(err, "socket bind error : port[%d]", port);
		return -1;
	}

	int s = make_socket_non_blocking (sfd);
	if (s == -1){
		LogToFile(err, "socket make nonblocking error");
		return -1;
	}
    epoll_event event;
    
	event.data.fd = sfd;
	event.events = EPOLLIN ;	//level trigger mode is more safe for me
	int rc = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
	if (rc == -1){
		LogToFile(err, "socket epoll_ctl error");
		return -1;
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

	//int clilen;
	int err_val;
	int s;
	
	struct sockaddr_in serveraddr;	
	
	s = socket(AF_INET, SOCK_DGRAM, 0);	
	if(-1 == s){
		err_val = errno;
		printf("Err:socket creation Error ->Code[%d]\n", err_val);
		return -1;
	}
	//접속 종료시 빨리 재사용 가능하게 함.
	int option;
	option = 1;
	if( 0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) )
	{
		LogToFile(err, "err, Error(%d): create_socket: setsockopt failure!: %s", errno, strerror(errno));
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);		
	err_val = bind(s, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	if(-1 == err_val){
		err_val = errno;
		printf("socket bind port[%d] Error ->Code[%d]\n", port, err_val);
		return -1;
	}
	return s;

}



void *epoll_thread(void *data)
{
  /* The event loop */
	while (1)
	{
		int n, i;

		n = epoll_wait (efd, events, MAXEVENTS, -1);
		for (i = 0; i < n; i++)
		{
			if ((events[i].events & EPOLLERR) || 	(events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
			{
				/* An error has occured on this fd, or the socket is not
				ready for reading (why were we notified then?) */
				fprintf (stderr, "epoll error -> client socket closed\n");
				continue;
			}

			else
			{
				/* We have data on the fd waiting to be read. Read it. 
                We must read whatever data is available completely, 
                as we are running in edge-triggered mode
				and won't get a notification again for the same	data. */
                
				ssize_t count;
				char buf[SIP_BUFF];

                count = read(events[i].data.fd, buf, SIP_BUFF);
                if (count == -1)
                {
                    /* If errno == EAGAIN, that means we have read all data. So go back to the main loop. */
                    if (errno != EAGAIN)
                    {
                        cout<< "Err " <<endl;
                    }
                    break;
                }
                else if (count == 0)
                {
                    /* End of file. The remote has closed the connection. */
                    LogToFile(info, "socket[%d] closed", events[i].data.fd);
                    break;
                }
                else {
                    if(g_sip_sock == events[i].data.fd){    //SIP Message from mod_oreka
                        buf[count] = 0x00;
                        LogToFile(info, "%s", buf);
                        do_sip_message(buf);
                    }
                    else{   //might be RTP 
                        do_rtp((const unsigned char *)buf, count, events[i].data.fd);
                    }
                }
			}
		}
	}

	free (events);
    return NULL;
}


