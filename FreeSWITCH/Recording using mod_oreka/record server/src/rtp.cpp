#include"global.h"
#include"epoll.h"
#include"util.h"
#include"channel.h"
#include"sip_parser.h"
#include"rtp.h"

int do_rtp(const unsigned char *msg_buf, int count, int sock)
{
    rtp_direction dir = dir_rx;
    int payload;
    if(count != 172) return -1;     //ptime 20,  G.711 only (RTP :160 + RTP Header 12)
    CHANNEL *ch = find_channel_by_sock(sock);
    if(NULL == ch) return -1;
    if(ch->tx_sock == sock){
        dir = dir_tx;
        payload = ch->tx_payload;
    }
    else{
        payload = ch->rx_payload;
    }

    u_char r;
    int16_t w;  
    int size = count - RTP_HEADER_OFFSET;    
    
    int16_t *pcm = (int16_t *)malloc(sizeof(int16_t) * size);
    for(int x = 0; x < size; x++){
        r = msg_buf[RTP_HEADER_OFFSET + x];
        if(0 == payload) w = ulaw2linear(r);
        else w = alaw2linear(r);
        pcm[x] = w;
    }

    
    if(dir_tx == dir){
        fwrite(pcm, size , sizeof(int16_t), ch->tx_file);
    }
    else{
        fwrite(pcm, size , sizeof(int16_t), ch->rx_file);
    }
    free(pcm);  
    return 0;
}
