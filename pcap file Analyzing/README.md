# pcap file related codes

Root privileges are required to capture network packets using libpcap.
<br><br>


## basic_pcap
<br>
Example of filtering only RTP (G.711 payload) in a pcap file to show RTP information

``` bash
# build command
g++ basic_pcap.cpp  -lpcap -o basic_pcap

#run command
./basic_pcap -f rtp_example.pcap
```

