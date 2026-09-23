# SIPp Overview

<br>

* SIPp is an open‑source test tool and traffic generator for the SIP (Session Initiation Protocol) used in VoIP systems.
* It can simulate thousands of concurrent SIP calls to test performance, scalability, and reliability of SIP servers, proxies, and gateways.

* Features include:

* * Predefined call scenarios (e.g., UAC, UAS, registration, etc.)

* * Custom XML scenario scripting for flexible test cases

* * Support for TCP, UDP, TLS, and IPv6

* * Ability to measure response times, call setup rates, and stress test SIP infrastructure

* Widely used by developers, QA engineers, and telecom operators to validate VoIP solutions under real‑world traffic conditions.

<br><br>

# SIPp install

<br>

```bash
sudo dnf update -y
sudo dnf install -y epel-release
sudo dnf config-manager --set-enabled crb

# Install essential build tools and libraries.
sudo dnf install -y git cmake make gcc-c++ \
    ncurses-devel \
    openssl-devel \
    libpcap-devel \
    lksctp-tools-devel \
    gsl-devel

# Change working directory and clone source
cd /usr/local/src
sudo git clone https://github.com/SIPp/sipp.git
cd sipp

# Initialize submodules (synchronize necessary files such as GTest)
sudo git submodule update --init

# TGenerate a Makefile including TLS, PCAP, SCTP, and GSL functionality.
sudo cmake . -DUSE_SSL=1 -DUSE_PCAP=1 -DUSE_SCTP=1 -DUSE_GSL=1
# compile
sudo make

# Copy to the /usr/local/bin directory so that it can be used regardless of the path.
cp sipp /usr/local/bin
```

<br><br>

# Testing SIPp in a NAT environment

<br>

Most VoIP devices today operate in NAT environments. Therefore, we will create a scenario in which a client (PhonerLite) located in a NAT environment places a call to SIPp, which is also situated in a NAT environment.

Unlike softswitches such as FreeSWITCH and Asterisk, SIPp does not fully support NAT environments. Therefore, it is advisable to configure the endpoint (PhonerLite) located behind NAT to use a STUN server to identify and utilize its public IP address.

<br>

![config](./image/1.png)

*Figure 1: Test Call Flow*


<br>

## PhonerLite in a NAT environment

<br>

Configure the Google STUN server in the PhonerLite configuration settings under "Server," as shown in the image below.
When a STUN server is configured, the softphone no longer uses a local private IP address but instead uses the public IP address obtained from the STUN server.

<br>

![config](./image/2.png)

*Figure 2: Configuring a STUN server*

<br>

### make a PhonerLite phonebook item

<br>

To facilitate SIPp connection testing, add an entry to the PhoneBook as follows.
SIPp will listen on port 5080, and the server address (X.X.139.21) is the public IP of the host where SIPp is running.

<br>

![config](./image/3.png)


<br>

## SIPp in a NAT environment

<br>

<br>

### Preparing Audio

<br>

SIPp lacks an audio streaming engine and codecs.
The reason for this is easily inferred: SIPp is designed for high-volume call processing tests. Consequently, it maximizes processing performance by focusing on high-volume signaling handling and avoiding the use of codecs or audio streaming engines.

To stream audio, SIPp requires a pcap file containing captured RTP network packets.
The RTP pcap file holds pre-encoded audio, and the stored RTP packets include timestamps indicating when they were sent. Streaming is performed by utilizing these time intervals. It is an intelligent architecture that processes RTP while minimizing CPU usage.
Therefore, it is advisable to pre-generate the audio source to be played—in the form of a pcap file—for use when the call connects.

If you have audio files such as MP3 or WAV, you can create a pcap file using ffmpeg and tcpdump as follows.


```bash
# First console. 
# It stores data arriving on the local interface via UDP port 10000.
sudo tcpdump -i lo -n -w music_pcmu.pcap udp port 10000

# Second console
# Play the `bg.wav` file using ffmpeg, but fix the sample size to 160 bytes to match the packet size.
# 160 bytes corresponds to the audio size for a `ptime` of 20 when using the G.711 codec.
ffmpeg -re -i bg.wav -vn -af asetnsamples=160 -acodec pcm_mulaw -ar 8000 -ac 1 -f rtp rtp://127.0.0.1:10000

```

By stopping tcpdump after the ffmpeg playback finishes, you can create a pcap file containing the audio data. While `pcm_mulaw` is specified as the codec for ffmpeg playback, specifying `pcm_alaw` instead allows you to create a pcap file encoded in A-law.


<br>

### Preparing Scenarion Xml 

<br>

SIPp uses XML for its default scenarios. Create the XML as shown below.
This XML includes information on the audio file to be played upon connection (the file created earlier) and codec details.
It also contains the public IP address (X.X.139.21) of the host where SIPp is running. When SIPp operates in a NAT environment, private IP information is not required.
If SIPp interacts with a device located on the same host or within the internal network, use a private IP address instead of a public IP address.

<br>

```xml
<?xml version="1.0" encoding="ISO-8859-1" ?>
<scenario name="UAS Loop Media">

  <!-- 1. Receive initial INVITE -->
  <recv request="INVITE" />

  <!-- 2. Send initial 200 OK -->
  <send retrans="500">
    <![CDATA[
      SIP/2.0 200 OK
      [last_Via:]
      [last_From:]
      [last_To:];tag=[pid]SIPpTag[call_number]
      [last_Call-ID:]
      [last_CSeq:]
      Contact: <sip:X.X.139.21:[local_port]>
      Content-Type: application/sdp
      Content-Length: [len]

      v=0
      o=user1 53655765 2353687637 IN IP4 X.X.139.21
      s=SIPp
      c=IN IP4 X.X.139.21
      t=0 0
      m=audio [media_port] RTP/AVP 0
      a=rtpmap:0 PCMU/8000
    ]]>
  </send>

  <!-- 3. Receive initial ACK -->
  <recv request="ACK" />

  <!-- Start media playback loop -->
  <label id="play_media" />
  <nop>
    <action>
      <exec play_pcap_audio="/usr/local/src/sipp/pcap/music_pcmu.pcap"/>
    </action>

```


<br>

### Preparing SIPp command

<br>

Prepare the SIPp command as follows.

<br>

``` bash
# sipp -sf uas_media.xml -i [local ip] -mi [local ip] -p [sip port] -mp [local media port]

sipp -sf uas_media.xml -i 10.10.10.10 -mi l10.10.10.10 -p 5080 -mp 10000

```


<br>

## Test

<br>

Everything is now ready. Run SIPp as follows. Then, using PhoneLite on your PC, place a call to the address in the phonebook you created earlier.

<br>

```bash
sipp -sf uas_media.xml -i 10.10.10.10 -mi 10.10.10.10 -p 5080 -mp 100001:10000

```

If there are no issues with settings such as the firewall, the audio file you created (bg.wav) will play on your PC. And the call information will appear on the SIPp screen as follows.

```bash
------------------------------ Scenario Screen -------- [1-9]: Change Screen --
  Port   Total-time  Total-calls  Transport
  5080      91.15 s            2  UDP

  0 new calls during 1.001 s period	  1 ms scheduler resolution
  1 calls                                 Peak was 1 calls, after 53 s
  0 Running, 3 Paused, 3 Woken up
  0 dead call msg (discarded)
  3 open sockets                          0/0/0 UDP errors (send/recv/cong)
  768 Total RTP pckts sent                0.000 last period RTP rate (kB/s)

                                 Messages  Retrans   Timeout   Unexpected-Msg
0 :  ----------> INVITE             2         0         0         0
1 :  <---------- 200                2         4         0
2 :  ----------> ACK                1         0         0         0
3 :              [ NOP ]
4 :  ----------> INVITE             0         0         0         0
5 :  ----------> BYE                1         0         0         0
6 :  <---------- 200                1         0
7 :  <---------- 200                0         0
8 :  ----------> ACK                0         0         0         0
------------------------------ SIPp Server Mode -------------------------------

```


<br>



