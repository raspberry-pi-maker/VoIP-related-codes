# Pcap file SIP Protocol Analyzing

This is an example of analyzing a SIP packet stored in a pcap file. It is also possible to analyze packets in real time instead of pcap files with some modifications.
<br><br>


## prerequisite knowledge

* basic_pcap : https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/pcap%20file%20Analyzing/basic_pcap
* Sip protocol Analyzing using osip : https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/SIP%20Protocol/SIP%20Protocol%20Alalyzing%20using%20osip

<br>

## Build

<br>

``` bash
# build command (modify the include path to  your osip installation path)
g++ pcap_sip_analyzing.cpp -I ~/src/study/osip_test/libosip2-5.3.1/include -lpcap -losip2 -losipparser2 -o pcap_sip_analyzing

#Run the command and you might see the results like this. You can see two newly created pcm files and two wav files.
./pcap_sip_analyzing -f SIP_CALL_RTP_G711.pcap

......
......

++++++++++++++++++ rcv +++++++++++++++++++++++++
SIP/2.0 180 Ringing
Via: SIP/2.0/UDP 200.57.7.195;branch=z9hG4bK9a86043a5daf59f4e9afd83eff9fc6e8
Via: SIP/2.0/UDP 200.57.7.195:55061;branch=z9hG4bKf3abad1ad86c86fc9f3fae7f49548564
From: "Ivan Alizade" <sip:5514540002@200.57.7.195:55061;user=phone>;tag=GR52RWG346-34
To: "francisco@bestel.com" <sip:francisco@bestel.com:55060>;tag=4098209679
Contact: <sip:francisco@200.57.7.204:5061>
Call-ID: 12015624@200.57.7.195
CSeq: 1 INVITE
Server: X-Lite release 1103m
Content-Length: 0


++++++++++++++++++++++++++++++++++++++++++++++++

+++++++++++++++++ debug ++++++++++++++++++++++++
sip_version      : SIP/2.0
sip_method       : ++++++++++++++++ Response
reason_phrase    : Ringing
status code      : 180
call_id          : 12015624@200.57.7.195
from             : "Ivan Alizade" <sip:5514540002@200.57.7.195:55061;user=phone>;tag=GR52RWG346-34
to               : "francisco@bestel.com" <sip:francisco@bestel.com:55060>;tag=4098209679
cseq             : 1 INVITE
head             : Server: X-Lite release 1103m
contact          : <sip:francisco@200.57.7.204:5061>
++++++++++++++++++++++++++++++++++++++++++++++++

++++++++++++++++++ rcv +++++++++++++++++++++++++


++++++++++++++++++++++++++++++++++++++++++++++++

| ERROR | 2022-11-20 00:38:29.0856 <osip_messa:  886> Could not parse start line of message.
osip_message_parse error:

pcap file : SIP_CALL_RTP_G711.pcap end

```
<br><br>