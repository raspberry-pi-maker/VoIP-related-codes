
#include <unistd.h>
#include <osipparser2/internal.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_message.h>
#include <osipparser2/osip_parser.h>

#include <osipparser2/osip_body.h>



const char *msg0 =
{
"SIP/2.0 100 Trying\r\n"
"From: <sip:3622@192.168.88.24>;tag=60c59f5d\r\n"
"To: <sip:901090224672@192.168.88.24>\r\n"
"Call-ID: gLJYCfALUrXQJ3SlL5M7dg..\r\n"
"CSeq: 1 INVITE\r\n"
"Supported: sec_agree,100rel\r\n"
"Via: SIP/2.0/UDP 192.168.232.2:13354;received=192.168.183.22;rport=53857;branch=z9hG4bK-524287-1---514818181cd10618\r\n"
"Contact: <sip:901090224672@192.168.88.24;transport=UDP>\r\n"
"Content-Length: 0\r\n"
"\r\n"
};

const char *msg1 =
{
"SIP/2.0 200 OK\r\n"
"From: <sip:3622@192.168.88.24>;tag=60c59f5d\r\n"
"To: <sip:901090224672@192.168.88.24>;tag=7ff6d52f4ac8-1858a8c0-13c4-45026-76b9886-53ab6edf-76b9886\r\n"
"Call-ID: gLJYCfALUrXQJ3SlL5M7dg..\r\n"
"CSeq: 1 INVITE\r\n"
"Session-Expires: 1800;refresher=uas\r\n"
"Min-SE: 90\r\n"
"Supported: sec_agree,100rel,timer\r\n"
"IPRON-Info: uuid=45E08737-7D0C-43C8-ACEB-1DCFBFD64FA0;hop=0\r\n"
"Via: SIP/2.0/UDP 192.168.232.2:13354;received=192.168.183.22;rport=53857;branch=z9hG4bK-524287-1---514818181cd10618\r\n"
"Contact: <sip:901090224672@192.168.88.24;transport=UDP>\r\n"
"Content-Type: application/sdp\r\n"
"Content-Length: 257\r\n"
"\r\n"
"v=0\r\n"
"o=AudiocodesGW 941448041 941448012 IN IP4 192.168.88.27\r\n"
"s=Phone-Call\r\n"
"c=IN IP4 192.168.88.27\r\n"
"t=0 0\r\n"
"m=audio 6850 RTP/AVP 8 101\r\n"
"c=IN IP4 192.168.88.27\r\n"
"a=sendrecv\r\n"
"a=ptime:20\r\n"
"a=rtpmap:8 PCMA/8000\r\n"
"a=rtpmap:101 telephone-event/8000\r\n"
"a=fmtp:101 0-15\r\n"
"\r\n"
};
const char* msg2 = {
"INVITE sip:901090224672@192.168.88.24 SIP/2.0\r\n"
"Via: SIP/2.0/UDP 192.168.232.2:13354;branch=z9hG4bK-524287-1---514818181cd10618;rport\r\n"
"Max-Forwards: 70\r\n"
"Contact: <sip:3622@192.168.183.22:53857>;+sip.instance=\"<urn:uuid:13c3d016-8c0e-47b9-98fc-6a1c09290e21>\"\r\n"
"To: <sip:901090224672@192.168.88.24>\r\n"
"From: <sip:3622@192.168.88.24>;tag=60c59f5d\r\n"
"Call-ID: gLJYCfALUrXQJ3SlL5M7dg..\r\n"
"CSeq: 1 INVITE\r\n"
"Allow: INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, REGISTER, SUBSCRIBE, INFO, PUBLISH\r\n"
"Content-Type: application/sdp\r\n"
"Supported: replaces, answermode, eventlist, outbound, path\r\n"
"User-Agent: BluebayNetworks SDK for Android\r\n"
"Allow-Events: hold, talk, conference\r\n"
"Content-Length: 267\r\n"
"\r\n"
"v=0\r\n"
"o=- 1605674498 1 IN IP4 192.168.232.2\r\n"
"s=ps\r\n"
"c=IN IP4 192.168.232.2\r\n"
"t=0 0\r\n"
"m=audio 20000 RTP/AVP 8 0 18 101\r\n"
"a=rtpmap:8 PCMA/8000\r\n"
"a=rtpmap:0 PCMU/8000\r\n"
"a=rtpmap:18 G729/8000\r\n"
"a=fmtp:18 annexb=no\r\n"
"a=rtpmap:101 telephone-event/8000\r\n"
"a=fmtp:101 0-16\r\n"
"a=sendrecv\r\n"
"\r\n"
};

void debug_msg(osip_message_t *sip);

int main(int argc, const char * argv[])
{
        char *dest=NULL;
        size_t length=0;
        int i;
  const char *pstr[3] ;
  pstr[0] = msg0;
  pstr[1] = msg1;
  pstr[2] = msg2;

        osip_message_t *sip;
        //fprintf(stdout, "input message len:%d\n", strlen(temp_data));

        parser_init(); //아주 중요함. osip_init 대신 이 함수로 충분함. 이 함수를 생략하면 segmentation fault 발생
        for(int x = 0; x < 3; x++){
    fprintf(stderr, "input message:\n====================================\n%s\n=================================\n\n", pstr[x]);
    i = osip_message_init(&sip);
    if (i!=0) { fprintf(stderr, "cannot allocate 1\n"); return -1; }
    else{ fprintf(stderr, "osip_messsage_init success\n");}
    fflush(stderr);
    i = osip_message_parse(sip, pstr[x], strlen(pstr[x]));
    //if (i!=0) { fprintf(stderr, "cannot allocate 2\n"); return -1; }  //에러가 발생하면 메시지 포맷에 에러가 있는 경우가  대부분
    //else{ fprintf(stderr, "osip_messsage_parse success\n");}
    fflush(stderr);


    i = osip_message_to_str(sip, &dest, &length);
    if (i!=0) { fprintf(stderr, "cannot allocate 3\n"); return -1; }

    //fprintf(stdout, "message:\n%s\n", dest);
    osip_free(dest);

    debug_msg(sip);
    fprintf(stderr, "\n\n");

    osip_message_free(sip);
    usleep(0);

        }
        return 0;
}

// https://www.antisip.com/doc/osip2/structosip__message.html 참조
void debug_msg(osip_message_t *sip)
{
        char *buf;
        int size, x;
        size_t length;

        fprintf(stderr, "sip_version      : %s\n", sip->sip_version);
  if(sip->sip_method){  //Request
    fprintf(stderr, "sip_method       : %s\n", sip->sip_method);
  }
  else{
    fprintf(stderr, "sip_method       : %s\n", "++++++++++++++++ Response");
    fprintf(stderr, "reason_phrase    : %s\n", sip->reason_phrase);
  }

  //fprintf(stderr, "sip_method       : %s\n", osip_message_get_method(sip));
  fprintf(stderr, "status code      : %d\n", sip->status_code);
        //fprintf(stderr, "message property : %d\n", sip->message_property);

        osip_call_id_to_str(sip->call_id, &buf );
        fprintf(stderr, "call_id          : %s\n",buf);
        osip_free(buf);

        osip_from_to_str(sip->from, &buf );
        fprintf(stderr, "from             : %s\n", buf);
        osip_free(buf);

        osip_to_to_str(sip->to, &buf ) ;
        fprintf(stderr, "to               : %s\n", buf);
        osip_free(buf);

        osip_cseq_to_str(sip->cseq, &buf ) ;
        fprintf(stderr, "cseq             : %s\n", buf);
        osip_free(buf);

        size = osip_list_size(&sip->bodies);
        for(x = 0; x < size; x++){
                osip_body_t *body = (osip_body_t *)osip_list_get((const osip_list_t *)&sip->bodies, x);
                osip_body_to_str(body, &buf, &length) ;
                fprintf(stderr, "body             : %s\n", buf);
                osip_free(buf);

        }

        size = osip_list_size(&sip->headers);
        for(x = 0; x < size; x++){
                osip_header_t *head = (osip_header_t *)osip_list_get((const osip_list_t *)&sip->headers, x);
                osip_header_to_str(head, &buf) ;
                fprintf(stderr, "head             : %s\n", buf);
                osip_free(buf);

        }

        size = osip_list_size(&sip->contacts);
        for(x = 0; x < size; x++){
                osip_contact_t *contact = (osip_contact_t *)osip_list_get((const osip_list_t *)&sip->contacts, x);
                osip_contact_to_str(contact, &buf) ;
                fprintf(stderr, "contact          : %s\n", buf);
                osip_free(buf);

        }




}


