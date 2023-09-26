/*
This program reads only ptime 20, 30 ms RTP packets from pcap file.
And analyze the RTP packets
*/
#include "global.h"


//I'll save call info here!
list<CALL *> g_calls;
char g_outputdir[256];

void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void rtp_packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet, const struct udphdr *udph);

int main(int argc, char *argv[]){


    char *dev, errbuf[PCAP_ERRBUF_SIZE], pcapfile[PCAP_ERRBUF_SIZE];
    int opt;
    pcap_t *cap = NULL;

    pcapfile[0] = 0x00;
    while ((opt = getopt(argc, argv, "f:o:")) != -1)
    {
      switch (opt)
      {
        case 'f':
            strncpy(pcapfile, optarg, PCAP_ERRBUF_SIZE - 1);
            break;
        case 'o':
            strncpy(g_outputdir, optarg, PCAP_ERRBUF_SIZE - 1);
            break;
      }
    }
    if (0x00 == pcapfile[0])
    {    
        cout << "Usage: basic_pcap -f \"pcap filename\" " <<endl;
        exit(0);
    }
    if (0x00 == g_outputdir[0])
    {    
        strcpy(g_outputdir, "./");  //default => current directory
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
    finalize_calls();
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
    int offset = RTP_HEADER_OFFSET;
    iph = (struct ip *)packet;
    pdata = packet + sizeof(struct ip) + sizeof(struct udphdr);
    udp_size = ntohs(udph->len);
    uint16_t src_port = ntohs(udph->source);
    uint16_t dst_port = ntohs(udph->dest);
    uint32_t rtp_ssrc = get_ssrc((RTP_HEADER *)pdata);
    uint32_t rtp_ts = get_timestamp((RTP_HEADER *)pdata);
    uint32_t rtp_seq = get_seq((RTP_HEADER *)pdata);
    int rtp_extension = get_extension((RTP_HEADER *)pdata);
    int rtp_cc = get_cc((RTP_HEADER *)pdata);
    
    int rtp_payload = get_payload((RTP_HEADER *)pdata);
    int rtp_version = get_version((RTP_HEADER *)pdata);

    if (rtp_version != 2 || (rtp_payload != 0 && rtp_payload != 8)) //I'm only interested in PCMU,PCMA payload
    {   //RTP validation check error
        // fprintf(stderr, "ip size:%d, udp_size[%d] RTP version:%d payload:%d =>skip processing", ip_size, udp_size, rtp_version, (int)payload);
        return;
    }
    //I've never seen an rtp header with cc and header extensions.
    offset += (4 * rtp_cc); //cc(Count of CSRC)  CSRC (Contributing Source) Identifiers : 32 bit
    if(rtp_extension){
        uint16_t extension_header_ID, extension_header_length;
        memcpy(&extension_header_ID, pdata + offset, sizeof(uint16_t));
        memcpy(&extension_header_length, pdata + offset + sizeof(uint16_t), sizeof(uint16_t));
        extension_header_ID = ntohs(extension_header_ID);
        extension_header_length = ntohs(extension_header_length);
        offset += (sizeof(uint16_t) * 2 + extension_header_length);
    }
    
    u_char src_ip[4], dst_ip[4];
    memcpy(&src_ip, &iph->ip_src, 4);
    memcpy(&dst_ip, &iph->ip_dst, 4);
    
    CALL *call = findcall(src_ip, dst_ip, rtp_ssrc);
    if(NULL == call) call = add_call(src_ip, dst_ip, rtp_ssrc);

    add_rtp(call, pdata + offset, udp_size -20, rtp_payload);
}
