#include"global.h"
#include"epoll.h"
#include"util.h"
#include"channel.h"
#include"sip_parser.h"

int do_sip_message(const char *msg_buf)
{
    //First check whether this is rx or tx INVITE
    osip_message_t *sip;
    int i, x, size;
    size_t    length;
    i = osip_message_init(&sip);
    if (i != 0) { 
        LogToFile(err, "osip_message_init err. cannot allocate 1 ");
        return -1;
    }
    i = osip_message_parse(sip, msg_buf, strlen(msg_buf));
    if (i != 0) { 
        LogToFile(err, "osip_message_parse err. cannot allocate 1 ");
        osip_message_free(sip);
        return -1;
    }    

    bool invite = false;
    bool bye = false;
    
    if(sip->sip_method){  //Request
        if(0 == strcmp(sip->sip_method, "INVITE")){
            invite = true;
        }
        else if(0 == strcmp(sip->sip_method, "BYE")){
            bye = true;
        }
        else{
            LogToFile(err, "NOT IMPLEMENTED SIP Message");
            osip_message_free(sip);
            return -1;
        }
    }
    else{
        LogToFile(err, "Not Implemented SIP Message");
        osip_message_free(sip);
        return -1;
    }
    
    char *call_id = NULL, *from = NULL, *to = NULL, *buf = NULL;
    rtp_direction dir = dir_tx;
    osip_call_id_to_str(sip->call_id, &call_id );
    osip_from_to_str(sip->from, &from );
    osip_to_to_str(sip->to, &to ) ;

    char *psdp = NULL;
    size = osip_list_size(&sip->bodies);
    for(x = 0; x < size; x++){  //size must be 1
        osip_body_t *body = (osip_body_t *)osip_list_get((const osip_list_t *)&sip->bodies, 0);
        osip_body_to_str(body, &psdp, &length) ;
    }

    size = osip_list_size(&sip->headers);
    for(x = 0; x < size; x++){
        osip_header_t *head = (osip_header_t *)osip_list_get((const osip_list_t *)&sip->headers, x);
        osip_header_to_str(head, &buf) ;
        if(strstr(buf, "Subject")){
            if(strstr(buf, "RX")){
                dir = dir_rx;
            }
            else if(strstr(buf, "TX")){
                dir = dir_tx;
            }
        }
        osip_free(buf);
    }

    if(invite && psdp){
        int port = 0, codec = -1;

        get_audioinfo(psdp, port, codec);
        if(dir_tx == dir){
            LogToFile(info, "INVITE  port[%d], codec[%d] dir[tx] name[%s]" , port, codec, sip->from->url->username);
            update_invite_channel(dir, call_id, port, codec, sip->from->url->username);
        }
        else{
            LogToFile(info, "INVITE  port[%d], codec[%d] dir[rx] name[%s]" , port, codec, sip->to->url->username);
            update_invite_channel(dir, call_id, port, codec, sip->to->url->username);
        }
    }

    if(bye){
        LogToFile(info, "BYE dir[%d] call id[%s]" , dir, call_id);
        update_bye_channel(dir, call_id);
    }


    if(call_id) osip_free(call_id);
    if(from) osip_free(from);
    if(to) osip_free(to);
    if(psdp) osip_free(psdp);

    osip_message_free(sip);
    return 0;
    
}


/*
m=audio 21008 RTP/AVP 0
This function only gets one codec (First codec number string),
but this is enough for mod_oreka, because the mod_oreka always send 1 codec in the SDP
*/
int get_audioinfo(const char *pSDP, int &port, int &codec)
{
	const char *szTok = "m=audio";
	char str[MAX_PATH];
    port = 0;
    codec = -1;
    
	int ret = getline(pSDP, szTok, str);
    
	if(-1 == ret) return -1;

    char *token = strtok(str, " ");
    if(!token) return -1;   //token maybe "m=audio"

    token = strtok(NULL, " ");
    if(!token) return -1;   //token maybe "21008"
    port = atoi(token);

    token = strtok(NULL, " ");
    if(!token) return -1;   //token maybe "RTP/AVP"

    token = strtok(NULL, " ");
    if(!token) return -1;   //token maybe "0"
    codec = atoi(token);
    
    return 0;
    
}
