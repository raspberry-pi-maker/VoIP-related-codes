/*
This program reads only ptime 20, 30 ms RTP packets from pcap file.
And analyze the RTP packets
*/
#include <stdio.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <pcap.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>
#include <time.h>
#include <unistd.h>

#include <osipparser2/internal.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/osip_message.h>
#include <osipparser2/osip_parser.h>
#include <osipparser2/osip_body.h>


using namespace std;

const char *ip_to_str(const u_char *ip, char *pstr)
{
  sprintf(pstr, "%d.%d.%d.%d", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
  return(const char *)pstr;
}


void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet);
bool parse_msg(const char *pmsg);
void debug_msg(osip_message_t *sip);

int main(int argc, char *argv[]){


    char *dev, errbuf[PCAP_ERRBUF_SIZE], pcapfile[PCAP_ERRBUF_SIZE];
    int opt;
    pcap_t *cap = NULL;

    pcapfile[0] = 0x00;
    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
      switch (opt)
      {
        case 'f':
            strncpy(pcapfile, optarg, PCAP_ERRBUF_SIZE - 1);
            break;
      }
    }
    
    parser_init(); //Very Important. Instead of osip_init, use parser_init. if you ommit this call, segmentation fault occurs.
    
    if (0x00 == pcapfile[0])
    {    
        cout << "Usage: pcap_sip_analyzing -f \"pcap filename\" " <<endl;
        exit(0);
    }
    
    cap = pcap_open_offline(pcapfile, errbuf);
    if(NULL == cap){
        cout <<  "invalid pcap file" << pcapfile << endl;
        exit(0);
    }

    //We just want to parce SIP packets here
    string filter_exp("(ip proto \\udp) and (port 5060 or port 5080)"); /* 5060, 5080 port data only */
    struct bpf_program fp;                                                /* The compiled filter expression */
    int capture_mode = 1;                                                 // 1:promiscuous mode(capture all packets)  0:non-promiscuous (capture packets only for this HOST)
    bpf_u_int32 net = 0;  /* The IP of our sniffing device */

    if (pcap_compile(cap, &fp, filter_exp.c_str(), 0, net) == -1)
    {
        printf( "Couldn't parse filter %s: %s", filter_exp.c_str(), pcap_geterr(cap));
        exit(0);
    }
    if (pcap_setfilter(cap, &fp) == -1)
    {
        printf( "Couldn't install filter %s: %s", filter_exp.c_str(), pcap_geterr(cap));
        exit(0);
    }

    if (NULL == cap && pcapfile[0])
    {
        printf( "Couldn't open pcap file[%s], ERR:%s", pcapfile, pcap_geterr(cap));
        exit(1);
    }
    
    struct pcap_pkthdr *pktH;
    const u_char *pkt_data;
    int pkt_ret;

    while(true){
        pkt_ret = pcap_next_ex(cap, &pktH, &pkt_data);
        if (pkt_ret > 0)  //read packet
        {
            packetHandler(NULL, pktH, pkt_data);
        }
        else if (pkt_ret == 0) //timeout. I don't use this case here.
        { 

        }
        else if (pkt_ret == -2) //offline file read end
        { 
          break;
        }
    }
    cout << "pcap file : "<< pcapfile << " end" <<endl;
    
    return 0;
}


void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    struct ether_header *ep;
    unsigned short ether_type;
    // IP header struct
    struct ip *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    int ip_size = 0;
    uint16_t src_port, dst_port;

    // get ehternet header information ---> 8 byte
    ep = (struct ether_header *)packet;
    // Get protocol of packets
    ether_type = ntohs(ep->ether_type);  

    // for get IP header move pointer for ehternet header size
    packet += sizeof(struct ether_header);

    //If not IP packet, just return;
    if (ether_type != ETHERTYPE_IP) return;

    // get IP header information
    iph = (struct ip *)packet;
    ip_size = ntohs(iph->ip_len);      

    //If TCP packet, just return;
    if(iph->ip_p == IPPROTO_TCP) return;
    
    else if(iph->ip_p == IPPROTO_UDP){
        const u_char *pdata;
        int udp_size = 0, packet_size;
        udph = (struct udphdr *)(packet + iph->ip_hl * 4); // UDP header --> 8byte
        src_port = ntohs(udph->source);
        dst_port = ntohs(udph->dest);
        pdata = packet + sizeof(struct ip) + sizeof(struct udphdr);
        udp_size = ntohs(udph->len);
        packet_size = udp_size - sizeof(struct udphdr);

        char *sip_packet = (char *)malloc(packet_size + 4);
        bzero(sip_packet, packet_size + 4);
        memcpy(sip_packet, pdata, packet_size);
        
        fprintf(stderr, "++++++++++++++++++ rcv +++++++++++++++++++++++++ \n");
        fprintf(stderr, "%s\n", sip_packet);
        fprintf(stderr, "++++++++++++++++++++++++++++++++++++++++++++++++ \n\n");

        parse_msg(sip_packet);
        free (sip_packet);

    }
  
}    

bool parse_msg(const char *pmsg)
{
    int i;
    char *dest=NULL;
    size_t length=0;
    osip_message_t *sip;

    i = osip_message_init(&sip);
    if (i!=0) { 
        cout <<  "Error! init_sip_lib" << endl;
        return false;
    }

    
    i = osip_message_parse(sip, pmsg, strlen(pmsg));
    if (i != 0) { 
        cout <<  "osip_message_parse error:" << pmsg << endl;
        osip_message_free(sip);
        return false;
    }
    
    debug_msg(sip);
    osip_message_free(sip);
    return true;
}

void debug_msg(osip_message_t *sip)
{
    char *buf;
    int size, x;
    size_t length;

    fprintf(stderr, "+++++++++++++++++ debug ++++++++++++++++++++++++ \n");
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
    fprintf(stderr, "++++++++++++++++++++++++++++++++++++++++++++++++ \n\n");
    
}


