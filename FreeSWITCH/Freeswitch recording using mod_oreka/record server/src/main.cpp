/*
mod_oreka 개선 사항

INVITE PACKET : SDP 순서 및 항목 "t"가 생략되면 에러 발생. => 현재는 osip으로 SDP 파싱 안함.
oreka_send_sip_message 함수에서 수정 가능

v=0
o=freeswitch 70116f1c-30da-434a-ae1f-8b72bbacb6dd 1 IN IP4 192.168.150.128
c=IN IP4 192.168.150.129
s=Phone Recording (TX)
i=FreeSWITCH Oreka Recorder (pid=1401)
m=audio 21446 RTP/AVP 0
a=rtpmap:0 PCMU/8000

====> 

v=0
o=freeswitch 70116f1c-30da-434a-ae1f-8b72bbacb6dd 1 IN IP4 192.168.150.128
s=Phone Recording (RX)
i=FreeSWITCH Oreka Recorder (pid=1401)
c=IN IP4 192.168.150.129
t=0 0
m=audio 21008 RTP/AVP 0
a=rtpmap:0 PCMU/8000



BYE PACKET : sip 주소에서  뛰어쓰기 없애기. oreka_send_sip_message 함수 마지막 부분에서 수정 가능
"BYE sip:Outbound Call@192.168.150.128:5060 SIP/2.0\r\n" =>
"BYE sip:OutboundCall@192.168.150.128:5060 SIP/2.0\r\n"

RTP 포트 변경 : 현재 Freeswich에서 사용하는 포트와 같은 번호로  mod_oreka에서 전달. 따라서 같은 장비에서는 포트 바인딩 에러 발생하며
패킷 전송 불가능

/src/mod/applications/mod_oreka/mod_oreka.c의 다음 함수 수정 필요
static int oreka_setup_rtp(oreka_session_t *oreka, oreka_stream_type_t type)
가운데 부분에 switch_rtp_new 함수를 수정하면 될 듯


*/

#include"global.h"
#include"epoll.h"
#include"channel.h"

int socks[MAX_CLIENT];
//SIP message receiving port and socket
int g_sip_port = 15060;
int g_sip_sock;

void usage();

int main(int argc, char * argv[])
{
    int x;
    int opt;

    while ((opt = getopt(argc, argv, "hnf:i:d:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            usage();
            exit(1);
        case 'p':
            g_sip_port = atoi(optarg);
            break;
        case 'd':
            if(0 == strcmp( optarg, "debug")){
                g_log_level = debug;
            }
            else if(0 == strcmp( optarg, "info")){
                g_log_level = info;
            }
            else if(0 == strcmp( optarg, "notice")){
                g_log_level = notice;
            }
            else if(0 == strcmp( optarg, "warning")){
                g_log_level = warning;
            }
            else if(0 == strcmp( optarg, "err")){
                g_log_level = err;
            }
            else if(0 == strcmp( optarg, "critical")){
                g_log_level = critical;
            }
            else if(0 == strcmp( optarg, "file_only")){
                g_log_level = file_only;
            }
            break;
            
        }
    }
    //Init oSip
    parser_init(); //Very Important. Instead of osip_init, use parser_init. if you ommit this call, segmentation fault occurs.

    init_epoll();
    //First add SIP message socket to the epoll
    g_sip_sock = buildsocket(g_sip_port);

	pthread_t p_thread;
	pthread_create(&p_thread, NULL, epoll_thread, (void *)NULL); 


    for(int y = 0; y < 1000; y++){
        for(x = 18000; x < 19000; x++){
            socks[x - 18000] = buildsocket(x);
        }
        usleep(1000);
        for(x = 18000; x < 19000; x++){
            close(socks[x - 18000]);
        }
        
    }

    for(x = 18000; x < 19000; x++){
        socks[x - 18000] = buildsocket( x);
    }
    
    cout << "Create UDP sockets success" <<endl;
    while(1){
        sleep(1);
    }        
    return 0;
}


void usage()
{
    fprintf(stderr, "===== Usage =====\n");
    fprintf(stderr, "-p: port number to communicate with mod_oreka  \n");
    fprintf(stderr, "-d: debug mode (debug, info, notice, warning, err, critical, file_only) default info \n");
    fprintf(stderr, "./rtp_call -p 15060  \n");
}
