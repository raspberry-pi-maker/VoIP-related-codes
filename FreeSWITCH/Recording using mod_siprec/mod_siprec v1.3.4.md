# 

<br>

# # mod_siprec

<br>

📌 **As of today (May 18, 2026), the latest version of mod_siprec is 1.34. After downloading, building, installing, and testing this version, I found a few issues. I hope this will be improved in version 1.4.**

<br>

## mod_siprec 1.3.4 problems

<br>

## Codec Issue

The payload value in the sdp of the INVITE message sent by mdo_siprec was set to 0 (ulaw). Additionally, when the SRS (Siprec Server) sent the OK response, it also sent a payload value of 0 (ulaw). However, upon examining the actual RTP header values, the payload value was A law.

<br>

## RX, TX separation problem:

<br>

To separate RX and TX, it is correct that there are two media entries in the sdp of the INVITE message.
According to SIPREC standards (RFC 7245, 7865), the Recording Server (SRS) is designed to process each stream independently.

* Single stream (m= 1): The SRC (PBX or IVR) already mixes the voices of two people into one and sends it. In this case, since the Recording Server receives the already combined sound, it cannot separate RX and TX later.

* Dual stream (m= 2): The SRC generates and sends a separate media stream for each participant. Since the Recording Server receives data through two separate ports, it is easy to save them to separate files or separate them into stereo (left/right) channels.

<br>

The SDP of the separate recording INVITE has approximately the following structure.

```
v=0
o=SRC-Server 12345 12346 IN IP4 1.1.1.1
...
c=IN IP4 1.1.1.1

m=audio 56376 RTP/AVP 0 101
a=label:1
a=sendonly

m=audio 45344 RTP/AVP 0 101
a=label:2
a=sendonly
```

<br>

However, in version 1.34, the INVITE message is sent via a single stream (m=1). Looking at the SIPRERC call flow in the latter part, the siprec client sends an INVITE again (reinvite) at the time the actual call is made.
The second INVITE should send an SDP in Dual stream (m=2) format to reflect that the call has been connected, but in v1.34, all data except the CSeq is sent as an INVITE identical to the first one.
This part must be corrected.

<br>

📌 **SDP samples in INVITE, (reINVITE)**

**First INVITE**

```bash
Content-Type: application/sdp
v=0
o=- 171 213 IN IP4 10.0.0.2
s=-
c=IN IP4 10.0.0.1
t=0 0
m=audio 6000 RTP/AVP 0
a=rtpmap:0 PCMU/8000
a=label:1

Content-Type: application/rs-metadata+xml
Content-Disposition: recording-session
<?xml version='1.0' encoding='UTF-8'?>
<recording xmlns='urn:ietf:params:xml:ns:recording'>
        <dataMode>complete</dataMode>
        <session id="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <start-time>2011-06-27T17:03:57</start-time>
        </session>
        <participant id="urn:uuid:10ac9063-76b7-40bb-4587-08ba290d7327" session="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <aor>sip:sipp@168.192.24.40</aor>
                <name>sipp </name>
                <send>urn:uuid:07868c77-ef8e-4d6f-6dd5-a02ff53a1329</send>
                <start-time>2011-06-27T17:03:57</start-time>
        </participant>
        <participant id="urn:uuid:797c45f5-e765-4b12-52b0-d9be31138529" session="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <aor>sip:service@168.192.24.60</aor>
                <name>sut </name>
        </participant>
        <stream id="urn:uuid:4a72a1ed-abb2-4d7c-5f4d-6d4c36e2d4ec" session="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <mode>separate</mode>
                <start-time>2011-06-27T17:03:57</start-time>
                <label>1</label>
        </stream>
</recording>
```

<br>

**Second INVITE**

```bash
Content-Type: application/sdp
v=0
o=- 171 213 IN IP4 10.0.0.2
s=-
c=IN IP4 10.0.0.1
t=0 0
m=audio 6000 RTP/AVP 0
a=rtpmap:0 PCMU/8000
a=label:1
m=audio 6002 RTP/AVP 0
a=rtpmap:0 PCMU/8000
a=label:2

Content-Type: application/rs-metadata+xml
Content-Disposition: recording-session
<?xml version='1.0' encoding='UTF-8'?>
<recording xmlns='urn:ietf:params:xml:ns:recording'>
        <dataMode>partial</dataMode>
        <session id="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <start-time>2011-06-27T17:03:57</start-time>
        </session>
        <participant id="urn:uuid:797c45f5-e765-4b12-52b0-d9be31138529" session="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <aor>sip:service@168.192.24.60</aor>
                <name>sut </name>
                <send>urn:uuid:4a72a1ed-abb2-4d7c-5f4d-6d4c36e2d4ec</send>
                <start-time>2011-06-27T17:03:58</start-time>
        </participant>
        <stream id="urn:uuid:07868c77-ef8e-4d6f-6dd5-a02ff53a1329" session="urn:uuid:79b2fcd8-5c7f-455c-783f-db334e5d57d0">
                <mode>separate</mode>
                <start-time>2011-06-27T17:03:58</start-time>
                <label>2</label>
        </stream>
</recording>
```

At first, I assumed that RX and TX were being sent as a single mixed stream. However, after examining the packets using tcpdump, I confirmed that 100 RTP packets were arriving per second. At ptime=20, 50 RX and 50 TX packets arrive. If they were mixed, only 50 should be arriving.

Upon further investigation, I discovered that the RTP transmission port for mod_siprec was different.
However, this port information has not yet been reflected in the INVITE message.

<br>

## mod_siprec build

<br>

mod_siprec is not a module officially included in the FreeSWITCH source code. Therefore, you must download and build it separately.

* Download the FreeSWITCH source code. I will assume that it has been downloaded to the /usr/locla/src/freeswitch directory.
* Download the mod_siprec source code from https://github.com/voicetel/mod_siprec. The location of the mod_siprec source code is /usr/locla/src/freeswitch/src/mod/applications.

<br>

There is a slight issue with the current mod_siprec.c code.
According to C90/C99 rules, variable declarations must be placed at the beginning of a block. However, since variables are declared in the middle of code execution within mod_siprec.c, compiling it as is results in an error.
While changing compilation options is an option, in my personal opinion, declaring variables in the middle of a function in a C source file is not a good practice.

However, building the downloaded source code requires a bit of a trick.

<br>

**The original build command of the voicetel homepage**

```bash
cd /usr/local/src
git clone https://github.com/voicetel/mod_siprec.git
git checkout v1.3.4
cd mod_siprec
ln -sf $PWD src/mod/applications/mod_siprec
echo 'applications/mod_siprec' >> build/modules.conf.in
./bootstrap.sh && ./configure
make mod_siprec-install
```

<br>

**Instead of creating a symbolic link, I will download the source code from the freeswitch source code directory.**
**CC="gcc -Wno-error=declaration-after-statement" The command "CC="gcc -Wno-error=declaration-after-statement" prevents errors from occurring even when declaring variables in the middle of a function, just like in C++.**

<br>

```bash
cd /usr/local/src/freeswitch/src/mod/applications
git clone https://github.com/voicetel/mod_siprec.git
cd src/mod/applications/mod_siprec

gcc -fPIC -DPIC -I/usr/include/uuid -I/usr/local/src/freeswitch/src/include \
    -I/usr/local/src/freeswitch/libs/libteletone/src -g -O2 \
    -Wno-error=declaration-after-statement \
    -c mod_siprec.c siprec_invite.c recording_session.c siprec_sdp.c siprec_metadata.c siprec_media.c

gcc -shared -fPIC \
    -o mod_siprec.so \
    mod_siprec.o siprec_invite.o recording_session.o siprec_sdp.o siprec_metadata.o siprec_media.o -luuid
cp mod_siprec.so  /usr/local/freeswitch/mod/
```

<br>

If registered in the autoload_configs/modules.conf.xml file, the module will be automatically loaded when FreeSWITCH is restarted.

## Setting up mod_siprec

<br>

So, add mod_siprec to the conf/autoload_configs/modules.conf.xml file so that FreeSWITCH loads mod_siprec when it starts.

```xml
<load module="mod_siprec"/>
```

<br>

### siprec.conf.xml

Then, create the mod_siprec configuration file, /usr/local/freeswitch/conf/autoload_configs/siprec.conf.xml, as follows.

```xml
<configuration name="siprec.conf" description="SIPREC (RFC 7866) module config">
  <settings>
    <param name="src-enabled" value="true"/>
    <param name="srs-enabled" value="false"/>
  </settings>
  <recording-servers>
    <recording-server name="default">
      <settings>
        <param name="host" value="127.0.0.1"/>
        <param name="port" value="5070"/>
        <param name="register" value="false"/>
        <param name="username" value=""/>
        <param name="password" value=""/>
      </settings>
    </recording-server>
  </recording-servers>
</configuration>
```

<br>

The src-enabled and srs-enabled keys are visible in settings.

#### SRC (Session Recording Client)

* Role: The entity that initiates recording at the PBX/media server/gateway where the actual call takes place.
* Operation: Monitors the call session and transmits the RTP stream and metadata (XML) to the recording server (SRS).
* Meaning in FreeSWITCH: FreeSWITCH acts as a SIPREC client to transmit call content to an external recording server.

<br>

#### SRS (Session Recording Server)

* Role: A server that receives, stores, and manages SIPREC streams sent by the SRC.
* Operation: Receives RTP streams, parses SIPREC metadata (XML), and manages recording files and session information.
* Meaning in FreeSWITCH: While FreeSWITCH can act as a recording server, external recording servers such as NICE, Verint, or VoIPmonitor generally handle the role of the SRS.

Since FreeSWITCH operates as a recording client and not as a recording server, set src-enabled=true and srs-enabled=false.

<br>

#### recording-server

Enter the address and information of the siprec server into the recording server.
The following siprec servers for recording can be used.

| Server                               | Installation difficulty                               | recording format                | Scope of application     |
| ------------------------------------ | ----------------------------------------------------- | ------------------------------- | ------------------------ |
| **drachtio-siprec-recording-server** | Easy (Docker/Node.js)                                 | pcap (Post-processing required) | Development/Testing, PoC |
| **Oreka (OrecX)**                    | Intermediate (SIPREC+RTPProxy configuration required) | wav/mp3                         | Enterprise, Call Center  |

<br>

## Dialplan

<br>

```xml
  <!-- Activate SIPREC while connecting to extension 5007 -->
  <extension name="TRUNK_SIPREC">
    <condition field="${sip_to_user}" expression="^(07047008888)$">
          <action application="set" data="continue_on_fail=true"/>
          <action application="export" data="hold_music=$${base_dir}/sounds/common/elise.wav" />
          <action application="siprec" data="default"/>
          <action application="bridge" data="USER/5007@$${domain}"/>
    </condition>
  </extension>
```

<br>

## Loading mod_siprec

<br>

If the setup process is finished, load mod_siprec in fs_cli.

```bash
freeswitch@blueivr> load mod_siprec
+OK Reloading XML
+OK

2026-05-08 17:14:52.382041 48.87% [INFO] switch_time.c:1436 Timezone reloaded 0 definitions
2026-05-08 17:14:52.382041 48.87% [CONSOLE] switch_loadable_module.c:1772 Successfully Loaded [mod_siprec]
2026-05-08 17:14:52.382041 48.87% [NOTICE] switch_loadable_module.c:329 Adding Application 'siprec'
2026-05-08 17:14:52.382041 48.87% [NOTICE] switch_loadable_module.c:329 Adding Application 'siprec_pause'
2026-05-08 17:14:52.382041 48.87% [NOTICE] switch_loadable_module.c:329 Adding Application 'siprec_resume'
error: tport_logging: capture socket error: Success
```

The last line in the fs_cli output above can be ignored.

<br><br>

# About simple_srs

<br><br>

simple_srs is a simple srs program created to test mod_siprec.

It operates through the following process.

![status](./image/1.png)<br/>

<div align="center">
<a href="https://translate.google.co.kr">siprec Call flow from ORACLE</a>
</div>

<br>

* When simple_srs receives an INVITE message from mod_siprec, it sends an OK response containing port information to receive rtp.

* In mod_siprec V1.34, RX and TX packets are sent from two ports. However, it is not possible to distinguish which one is the caller. simple_srs distinguishes between the two ports and creates two recording files to save the packets. The file names are based on the call ID included in mod_siprec's INVITE message.

* When the call ends, mod_siprec sends BYE. simple_srs sends OK and cleans up resources.Then, the recording file is converted to a wav file using the external program sox.

<br>

## simple_srs build

<br>

No special work is required for the build.

* mkdir build
* cd build
* cmake ..
* make

<br>

## run simple_srs

<br>

Run simple_srs.

```bash
[root@dev1 build]# ./simple_srs
====== Simple siprec server(for mod_siprec V1.34) Start ======
SIPREC socket port[5070] bind success
Initialize epoll success epoll siprec sock[4]
Create UDP sockets success
```

When receiving an INVITE

```bash
INVITE media:audio 20872 RTP/AVP 0  port:20872  codec:0
SIPREC socket port[21872] bind success
```

When receiving an BYE

```bash
End of recording
End of recording
CID:cc6489ed-ce3b-123f-8abb-d20d227d4ce8 Call termination completed
```

And you can see that two wav files were created as follows. rx and tx are stored in independent wav files.

```bash
[root@dev1 build]# ll
total 2276
-rw-r--r-- 1 root    root    151084 May 19 22:49 52a0faef-ce2c-123f-8abb-d20d227d4ce8_0_0.raw.wav
-rw-r--r-- 1 root    root    151084 May 19 22:49 52a0faef-ce2c-123f-8abb-d20d227d4ce8_0_1.raw.wav
-rw-r--r-- 1 root    root     14445 May 19 16:31 CMakeCache.txt
drwxr-xr-x 6 root    root      4096 May 20 00:39 CMakeFiles
-rw-r--r-- 1 root    root      1652 May 19 16:31 cmake_install.cmake
-rw-r--r-- 1 root    root      8774 May 19 16:31 Makefile
-rwxr-xr-x 1 root    root    873904 May 20 00:39 simple_srs
```

<br><br>

# Wrapping up

<br>
As mentioned earlier, mod_siprec v1.34 is an unfinished version. I will update the content once 1.4 is released.
