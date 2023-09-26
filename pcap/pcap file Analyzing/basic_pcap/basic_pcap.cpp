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

using namespace std;

typedef struct _RTP_HEADER
{
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  
    unsigned char cc:4;          /* CSRC count */
    unsigned char extension:1;   /* header extension flag */
    unsigned char padding:1;     /* padding flag */
    unsigned char version:2;     /* protocol version */

    unsigned char type:7;          /* payload type */
    unsigned char marker:1;      /* marker bit */
    uint16_t seq:16;            /* sequence number */
  #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    unsigned char version:2;     /* protocol version */
    unsigned char padding:1;     /* padding flag */
    unsigned char extension:1;   /* header extension flag */
    unsigned char cc:4;          /* CSRC count */
    unsigned char marker:1;      /* marker bit */
    unsigned char type:7;          /* payload type */
    uint16_t seq:16;            /* sequence number */
  #else
    #error Byte Order Error
  #endif    
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
} RTP_HEADER;


inline uint32_t get_ssrc(RTP_HEADER *ph){ return ntohl(ph->ssrc);}
inline uint32_t get_timestamp(RTP_HEADER *ph){ return ntohl(ph->ts);}
inline uint16_t get_seq(RTP_HEADER *ph){ return  ntohs(ph->seq);}
inline int get_payload(RTP_HEADER *ph) { return ph->type;}
inline int get_version(RTP_HEADER *ph) { return ph->version;}
const char *ip_to_str(const u_char *ip, char *pstr)
{
  sprintf(pstr, "%d.%d.%d.%d", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
  return(const char *)pstr;
}


void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void rtp_packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet, const struct udphdr *udph);

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
    if (0x00 == pcapfile[0])
    {    
        cout << "Usage: basic_pcap -f \"pcap filename\" " <<endl;
        exit(0);
    }
    
    cap = pcap_open_offline(pcapfile, errbuf);
    if(NULL == cap){
        cout <<  "invalid pcap file" << pcapfile << endl;
        exit(0);
    }

    //We just want to parce RTP packets here
    string filter_exp("(ip proto \\udp) and (greater 213 and less 295)"); /* ptime 20 , 30 RTP only */
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

/*
G.711  rtp packet size
ptime:20  -> frame:214 ip:200, udp:180(udp header:8 , rtp header:12, g.711:160)
ptime:30  -> frame:294 ip:280, udp:260(udp header:8 , rtp header:12, g.711:240)
ptime:40  -> frame:374 ip:360, udp:340(udp header:8 , rtp header:12, g.711:320)
*/

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
      udph = (struct udphdr *)(packet + iph->ip_hl * 4); // UDP header --> 8byte
      rtp_packetHandler(userData, pkthdr, packet, udph);
    }
  
}    

/*
Analyzing RTP Packet here !!
*/
void rtp_packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet, const struct udphdr *udph)
{
    struct ip *iph;
    const u_char *pdata;
    int udp_size = 0;
    char from_ip[64], to_ip[64];
    
    iph = (struct ip *)packet;
    pdata = packet + sizeof(struct ip) + sizeof(struct udphdr);
    udp_size = ntohs(udph->len);
    uint16_t src_port = ntohs(udph->source);
    uint16_t dst_port = ntohs(udph->dest);
    uint32_t rtp_ssrc = get_ssrc((RTP_HEADER *)pdata);
    uint32_t rtp_ts = get_timestamp((RTP_HEADER *)pdata);
    uint32_t rtp_seq = get_seq((RTP_HEADER *)pdata);
    int rtp_payload = get_payload((RTP_HEADER *)pdata);
    int rtp_version = get_version((RTP_HEADER *)pdata);

    if (rtp_version != 2 || (rtp_payload != 0 && rtp_payload != 8)) //I'm only interested in PCMU,PCMA payload
    {   //RTP validation check error
        // fprintf(stderr, "ip size:%d, udp_size[%d] RTP version:%d payload:%d =>skip processing", ip_size, udp_size, rtp_version, (int)payload);
        return;
    }

    ip_to_str((const u_char*)&iph->ip_src, from_ip);
    ip_to_str((const u_char*)&iph->ip_dst, to_ip);
    printf("\nver:%d payload:%d, seq:%d, stamp:%ld ssrc:%0X\n", rtp_version, rtp_payload, rtp_seq, rtp_ts, rtp_ssrc);
    printf("from[%s:%d] to[%s:%d]\n", from_ip, src_port, to_ip, dst_port);
}
