# SIP Protocol Analyzing

There are many libraries those can be used to analyze SIP protocol.
I use Sofia Library made by Nokia a lot in actual SIP project. But this time, I will make a protocol analysis program using the OSIP library.
<br><br>


##  OSIP Library

oSIP is a free software library for VoIP applications implementing lower layers of Session Initiation Protocol (SIP). The library includes the minimal codebase required by any SIP application and offers enough flexibility to implement any SIP extension or behavior. Started in September 2000 and published in April 2001, oSIP is among the oldest SIP open source stack still being developed and maintained. The project was made part of the GNU Project as GNU oSIP in 2002. (From Wikipedia)
<br><br>


##  Download OSIP Library

You can download it from  [OSIP Library Source codes download](http://ftp.gnu.org/gnu/osip/)  .

As of November 2022, the latest version is 5.3.1.
``` bash
$ wget http://ftp.gnu.org/gnu/osip/libosip2-5.3.1.tar.gz
$ tar -xvzf libosip2-5.3.1.tar.gz
$ cd libosip2-5.3.1
```
<br>

##  Build OSIP Library
then build the source codes to make binaries(so files)

``` bash
  $> mkdir build
  $> cd build
  $> ../configure
  $> make
  $> sudo make install
```
<br>

##  Set library path
If you run the last commands(make install), the so files are copied to "/usr/local/lib".
However, many Linux distributions do not include this directory in shared library path


Add this file into /etc/ld.so.conf.d directory and run ldconfig command
``` bash
[spypiggy@localhost bin]$ cat /etc/ld.so.conf.d/osip.conf
/usr/local/lib/
[spypiggy@localhost bin]$ sudo ldconfig
```
<br>


##  Build test program
Please edit the Makefile before building. This file contains information about the directory where the osip library is stored. Modify this route in advance to suit your preferences.

``` bash
  $> make
  $> ./osip_test
```   
<br>

If there are no problems, you should see the following output.
<br>

``` bash
[spypiggy@localhost sip_broker]$ ./osip_test
input message:
====================================
SIP/2.0 100 Trying
From: <sip:3622@192.168.88.24>;tag=60c59f5d
To: <sip:901090224672@192.168.88.24>
Call-ID: gLJYCfALUrXQJ3SlL5M7dg..
CSeq: 1 INVITE
Supported: sec_agree,100rel
Via: SIP/2.0/UDP 192.168.232.2:13354;received=192.168.183.22;rport=53857;branch=z9hG4bK-524287-1---514818181cd10618
Contact: <sip:901090224672@192.168.88.24;transport=UDP>
Content-Length: 0


=================================

osip_messsage_init success
sip_version      : SIP/2.0
sip_method       : ++++++++++++++++ Response
reason_phrase    : Trying
status code      : 100
call_id          : gLJYCfALUrXQJ3SlL5M7dg..
from             : <sip:3622@192.168.88.24>;tag=60c59f5d
to               : <sip:901090224672@192.168.88.24>
cseq             : 1 INVITE
head             : Supported: sec_agree
head             : Supported: 100rel
contact          : <sip:901090224672@192.168.88.24;transport=UDP>

......
......

```  


 
