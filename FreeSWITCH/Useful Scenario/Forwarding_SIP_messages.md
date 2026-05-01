Occasionally, it is necessary to forward SIP messages from IP PBXs, including FreeSWITCH, to other servers.
Currently, there are several ways to consider for forwarding SIP messages from a PBX to another server.

* Packet capture using tcpdump or sngrep.

* Direct real-time packet capture using the pcap library.

However, these two methods have weaknesses.

The first method has the disadvantage that packets cannot be transmitted in real time because packets must be saved and then transmitted using a third-party program.

And the second method also has the following weaknesses.

* SIP signals can use UDP or TCP. In particular, since TCP packets are stream-based, packets can become fragmented, and in this case, there is the difficulty of having to reassemble the packets.

* However, a bigger problem arises when using TLS/SRTP. In particular, for WebRTC, which is widely used these days, the use of TLS/SRTP is mandatory. When using TLS/SRTP, decrypting SIP packets is very difficult because they are encrypted. Specifically, Perfect Forward Secrecy (PFS, e.g., ECDHE) based TLS sessions cannot be decrypted using only the server key. In this case, decryption is nearly impossible because the session key must be extracted directly.

Most SIP servers (FreeSWITCH, Asterisk, Kamailio) support the Host Event Protocol (HEP) to easily solve this problem. Using HEP, decrypted SIP messages can be sent to another server over a single protocol (e.g., UDP), regardless of whether the messages are transmitted via UDP, TCP, or TLS.

<br><br>

# HEP

<br>

Host Event Protocol (HEP) is an encapsulation protocol used to forward SIP and VoIP traffic to external capture servers (such as HOMER). It bundles SIP messages and associated metadata (IP, port, timestamp, etc.) into a Type-Length-Value (TLV) structure and transmits them via UDP, TCP, or SCTP. The most widely used version currently is HEPv3.

<br>

## HEP Protocol Overview

<br>

* Purpose: Captures various network packets, such as SIP, RTP, RTCP, and DNS, and delivers them to a central server (e.g., HOMER).
* Structure: Encapsulates headers and payloads into multiple chunks without altering the original packet.
* Transmission Methods: Supports UDP, TCP, and SCTP.
* Versions: HEPv1, v2, v3 (v3 is currently the standard).

<br>

## HEPv3 Packet Structure

<br>

HEPv3 consists of multiple chunks, and each chunk contains the following fields:

* Vendor ID (2 bytes): Provider identifier
* Type ID (2 bytes): Data type
* Length (2 bytes): Payload length
* Payload (variable): Actual data

<br>

| Type ID | meaning             | example                   |
|---------|---------------------|---------------------------|
| 0x0001  | Protocol Family     | IPv4 / IPv6               |
| 0x0002  | Protocol ID         | UDP / TCP                 |
| 0x0003  | Source IP Address   | 192.168.0.10              |
| 0x0004  | Destination IP Addr | 192.168.0.20              |
| 0x0005  | Source Port         | 5060                      |
| 0x0006  | Destination Port    | 5060                      |
| 0x0007  | Timestamp (Seconds) | 1714550000                |
| 0x0008  | Timestamp (Usec)    | 123456                    |
| 0x0009  | Protocol Type       | SIP / RTP / RTCP / DNS    |
| 0x000a  | Capture ID          | 1                         |
| 0x000b  | Node ID             | 1001                      |
| 0x000c  | Keep-Alive          | Boolean                   |
| 0x000f  | Captured Payload    | SIP body, RTP packet      |


<br><br>

# Implementing HEP in FreeSWITCH

<br>

The module that handles SIP messages in FreeSWITCH is mod_sofia.
Therefore, modify the following files related to the mod_sofia configuration file.

<br>

## sofia.conf.xml

<br>

The sofia.conf.xml file in the conf/autoload_configs directory is responsible for the overall configuration of mod_sofia operation. Add capture-server information to this file. Note that 9060 is the default port value for HEP.

```xml
  <global_settings>
    <param name="capture-server" value="udp:127.0.0.1:9060"/>
  </global_settings>
```

<br>

## internal.xml

<br>

When transmitting extension SIP messages generated in the internal profile using HEP, add the following.
In the example, we assumed that a SIP message would be sent to the local computer.

```xml
  <settings>
    <!-- hep protocol-->
    <param name="sip-capture" value="yes"/>
    <param name="capture-server" value="udp:127.0.0.1:9060;hep=3"/>
  </settings>  

```

<br>

📌 **If you also want to send SIP messages for the external line, you can add them to the external line profile as shown above**

<br>

## restart mod_sofia

<br>

Restart mod_sofia or restart only the profile. Execute only one of the commands below.

```bash
freeswitch@blueivr> relaod mod_sofia
freeswitch@blueivr> sofia profile internal restart
```


<br><br>

# HEP Client

<br>

The SIP messages transmitted in this way can be received and processed directly by the program you created, or processed using an existing monitoring program such as Homer.

<br>

## HOMER

<br>

HOMER is an open-source platform for centrally collecting and analyzing SIP and VoIP traffic, and SIPCAPTURE is its core module/project. When packets captured via HEP (Host Event Protocol) from SIP servers such as FreeSWITCH, Kamailio, and OpenSIPS are transmitted to a HOMER server, HOMER becomes able to store, search, and analyze them.
You can find a lot of information about HOMER by searching its GitHub page and YouTube videos.

By using HOMER, you can receive SIP packets on a remote monitoring server and perform various monitoring tasks without burdening the SIP server.

For example, even using only sngrep inevitably places a load on the server because the PBX must capture and analyze packets. Therefore, in the case of PBXs with high traffic, this can cause performance issues. However, by using HOMER, you can conveniently check SIP call flows and other data on a GUI screen without causing such problems.

<br>

![homer](https://user-images.githubusercontent.com/1423657/73536888-5513dd80-4427-11ea-82aa-b2ce53192a63.png "HOMER")


<br>

<br>

### Structure Overview

<br>

* HEP Protocol: Encapsulates and transmits various packets, such as SIP, RTP, RTCP, and DNS, within a TLV structure.

* Capture Agents: Captagent, heplify, and SIP server modules (siptrace, mod_sofia, etc.) generate HEP packets.

* HOMER Server: Receives HEP packets, stores them in a database, and provides search and analysis via a web UI.

<br>

### Key Components

<br>

* SIPCAPTURE Module: Transmits SIP messages from Kamailio/OpenSIPS to HEP.

* Captagent: A standalone capture agent. It captures packets directly from network interfaces and forwards them to HEP for HOMER.

* Heplify-Server: A lightweight Go-based HEP receiving server. It integrates with the HOMER database.

* HOMER UI: A web-based interface for searching and analyzing SIP messages, RTP streams, QoS metrics, and more.


<br>

## Your own HEP CLient

<br>

For general monitoring tasks, using well-established tools like HOMER is a safe bet. However, if you need to receive SIP messages and perform specific operations, you can create your own HEP ​​client. HEP does not simply send the SIP message; it also includes information about the message in a small header. Therefore, you must analyze the HEP header first before processing the SIP message.

Since the HEPv3 Packet Structure was explained earlier, we will only look at how to implement it in a Python example.

<br>

```python
import socket
import struct

def hep3_receiver():
    listen_address = ('0.0.0.0', 9060)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(listen_address)
    print("--- HEP3 SIP Receiver (FreeSWITCH Fix) ---")

    while True:
        data, addr = sock.recvfrom(65535)
        
        # Keep printing received bytes
        print(f"Received {len(data)} bytes from {addr}")

        # Check for minimum length and HEP3 signature
        if len(data) < 6 or data[:4] != b'\x48\x45\x50\x33':
            continue

        offset = 6 
        sip_payload = b""
        src_ip = ""
        dst_ip = ""

        while offset < len(data):
            if offset + 6 > len(data): break

            # Read VendorID, TypeID, Length
            v_id, t_id, length = struct.unpack("!HHH", data[offset:offset+6])
            
            p_len = length - 6
            offset += 6
            
            if offset + p_len > len(data):
                p_len = len(data) - offset

            chunk_data = data[offset : offset + p_len]
            
            # --- Updated parsing logic ---
            # 1. IP addresses (Type 3, 4)
            if t_id == 3:
                src_ip = ".".join(map(str, chunk_data))
            elif t_id == 4:
                dst_ip = ".".join(map(str, chunk_data))
            
            # 2. SIP Payload (standard Type 11 or FreeSWITCH/Homer Type 15)
            # Even if Vendor ID is not 0, capture payload if TypeID matches
            elif t_id in [11, 15]:
                sip_payload = chunk_data
            
            offset += p_len

        # Print results
        if sip_payload:
            print("\n")
            print(f"SRC: {src_ip} | DST: {dst_ip}")
            print(f"SIP Payload Size: {len(sip_payload)} bytes")
            print("\n")
            print(sip_payload.decode('utf-8', errors='ignore').strip())
            print("\n")
        else:
            print("   -> No SIP Payload found in chunks.")

if __name__ == "__main__":
    hep3_receiver()

```
<br>

If your settings are correct and you have restarted mos_sofia, you will be able to see the following Python output.

<br>

```bash
SRC: 175.xxx.xxx.Xxx | DST: 10.9.xxx.xxx
SIP Payload Size: 800 bytes


REGISTER sip:dev1.hello.co.kr SIP/2.0
Via: SIP/2.0/UDP 175.xxx.xxx.Xxx:5060;branch=z9hG4bK284119613178911388;rport
From: 5008 <sip:5008@dev1.hello.co.kr>;tag=2780954026
To: 5008 <sip:5008@dev1.hello.co.kr>
Call-ID: 59935776818786-44401316132310@192.168.1.13
CSeq: 103176 REGISTER
Contact: <sip:5008@175.xxx.xxx.Xxx:5060>
Authorization: Digest username="5008", realm="dev1.hello.co.kr", nonce="bc012256-ed76-4078-b191-e05c4777f28b", uri="sip:dev1.hello.co.kr", response="6ed582e003cc032e32d63c6388017732", algorithm=MD5, cnonce="ebcb82d2", qop=auth, nc=00000001
Max-Forwards: 70
Expires: 30
Supported: path
User-Agent: Fanvil X303 2.12.18.9 0c383e63963b
Mac: 0c:38:3e:63:96:3b
Allow: INVITE, ACK, OPTIONS, BYE, CANCEL, REFER, NOTIFY, INFO, PRACK, UPDATE, MESSAGE
Content-Length: 0

```

<br><br>

# Wrapping up

HEP is supported by most open-source PBXs, not just FreeSWITCH.

It is also supported by many commercial PBXs. If you are using a commercial PBX, please contact the manufacturer to check for HEP support and how to configure it.

If you wish to remotely access SIP messages exchanged between your PBXs, you can utilize HEP.

