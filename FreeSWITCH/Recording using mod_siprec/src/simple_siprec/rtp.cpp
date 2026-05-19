#include"global.h"
#include"util.h"



/*
Currently, freeswitch mod_siprec sends ulaw to the INVITE, but alaw is actually received via the RTP.
Therefore, it utilizes packet information in real time.
*/
int do_rtp(const unsigned char *msg_buf, int count, int sock, int remote_port) 
{
  MediaInfo *minfo = g_call_factory.find_mediainfo(sock);
  if(!minfo) return 0;
  
  int index = 0;
  if(!minfo->src_port[0] && !minfo->src_port[1]){
    minfo->src_port[0] = remote_port;
    return 0; //Recording starts when packets from both sides arrive.
  }
  else if(minfo->src_port[0] && !minfo->src_port[1]){
    if(minfo->src_port[0] != remote_port){
      minfo->src_port[1] = remote_port;
      index = 1;
    }
    else return 0; //Recording starts when packets from both sides arrive.
  }
  else{
    if(minfo->src_port[1] == remote_port) index = 1;
  }
  
  //cout << remote_port << "  " << index << endl;
  RTP_HEADER header;
  memcpy(&header, msg_buf, RTP_HEADER_OFFSET);
  minfo->codec = int(header.type);
  if(minfo->fh[index]){
    fwrite(msg_buf + RTP_HEADER_OFFSET, sizeof(u_char), count - RTP_HEADER_OFFSET, minfo->fh[index]);    
  }
  return 0;
}