# Freeswitch Screen Popup

Most call center solutions provide CTI (Computer Telephony Integration) function.

CTI's important features include:
* Customer call distribution function
* Call-related statistics function
* Real-time phone control from agent computer
* Provides real-time call information when entering a call (Screen Popup)

Among these functions, call distribution can use mod_callcenter or mod_fifo. How to use mod_fifo
explained in [Freeswitch Callcenter using mod_fifo](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/Freeswitch%20Calltcenter%20using%20mod_fifo)

In this article, we will see how to implement a screen popup. 

To implement screen pop-up in Freeswitch, you need to know the event system provided by Freeswitch. To handle events in Freeswitch, develop a program using ESL (Event Socket Library).
The fs_cli program provided by Freeswitch was also developed using ESL for C language. ESL is available in various languages such as Javascript, Ruby, C/C++, and Python. I'll try a simple implementation using Python.

<br/><br/>

# Python ESL(Event Socket Library)
<br>

In Freeswitch 1.6, which I use mostly, ESL for Python is only provided for Python 2. So some work is required to get it to work in Python 3. I hope you don't use Python 2 anymore.
The following work was done on Ubuntu 20.04.
<br><br>

## Python3 ESL build
<br>
python3, python3-dev (devel for centOS) must be installed in advance. In the case of centOS, if there is no symbolic link to the /usr/lib64/libpython3.6m.so file, it is created.
<br><br>

``` bash
$ sudo ln -s /usr/lib64/libpython3.6m.so /usr/lib64/libpython3.6.so
```
<br><br>

### __Install swig__
<br>
The Simplified Wrapper and Interface Generator (SWIG) is an open-source software tool used to connect computer programs or libraries written in C or C++ with scripting languages such as Lua, Perl, PHP, Python, R, Ruby, Tcl, and other languages like C#, Java, JavaScript, Go, D, OCaml, Octave, Scilab and Scheme. Output can also be in the form of XML. <br><br>

Freeswitch is basically developed in C language. Therefore, ESL is also made in c language. In addition, it was implemented so that other languages could use the ESL function made in C language by using swig.

Makefile's swig uses the version checked at the time of ./configure. You can check the contents of /usr/local/src/freeswitch-XXX/libs/esl/python/Makefile. If necessary, install swig as follows.


<br><br>

``` bash
$ sudo apt -y update
$ sudo apt -y dist-upgrade
# install swig 
$ sudo apt-get install -y swig
```

<br><br>

### __Create a Python 3 working directory__
<br>
The ESL directory for Python is /usr/local/src/freeswitch-XXX/libs/esl/python. Create directory /usr/local/src/freeswitch-XXX/libs/esl/python3 to keep the existing ESL for Python 2. Copy necessary files from /usr/local/src/freeswitch-XXX/libs/esl/python directory.

<br><br>

``` bash
$ mkdir -p /usr/local/src/freeswitch-XXX/libs/esl/python3
$ cd /usr/local/src/freeswitch-XXX/libs/esl/python3
$ cp ../python/Makefile ../python/python-config ./
```
<br><br>

### __Modify python-config__
<br>
This file simply functions to output environment variables. Therefore, you can change the print function of Python 2 syntax to Python 3.


``` python
#! /usr/bin/python3
import sys
import os
import getopt
from distutils import sysconfig

valid_opts = ['prefix', 'exec-prefix', 'includes', 'libs', 'cflags', 
              'ldflags', 'help']

def exit_with_usage(code=1):
    print ( "Usage: %s [%s]" % (sys.argv[0], 
                                            '|'.join('--'+opt for opt in valid_opts)))
    sys.exit(code)

try:
    opts, args = getopt.getopt(sys.argv[1:], '', valid_opts)
except getopt.error:
    exit_with_usage()

if not opts:
    exit_with_usage()

opt = opts[0][0]

pyver = sysconfig.get_config_var('VERSION')
getvar = sysconfig.get_config_var

if opt == '--help':
    exit_with_usage(0)

elif opt == '--prefix':
    print (sysconfig.PREFIX)

elif opt == '--exec-prefix':
    print (sysconfig.EXEC_PREFIX)

elif opt in ('--includes', '--cflags'):
    flags = ['-I' + sysconfig.get_python_inc(),
             '-I' + sysconfig.get_python_inc(plat_specific=True)]
    if opt == '--cflags':
        flags.extend(getvar('CFLAGS').split())
    print (' '.join(flags))

elif opt in ('--libs', '--ldflags'):
    libs = getvar('LIBS').split() + getvar('SYSLIBS').split()
    libs.append('-lpython'+pyver)
    # add the prefix/lib/pythonX.Y/config dir, but only if there is no
    # shared library in prefix/lib/.
    if opt == '--ldflags' and not getvar('Py_ENABLE_SHARED'):
        libs.insert(0, '-L' + getvar('LIBPL'))
    print (' '.join(libs))
```

<br><br>

### __Modify Makefile__
<br>
Modify swig to use the latest version. Modify the python command to the python3 command. And remove the classic option, which has been removed from the latest swig. The install option was removed because it could conflict with Python 2. After building, manually copy the necessary files.

<br/><br/>

``` bash
LOCAL_CFLAGS=`python3 ./python-config --includes`
LOCAL_LDFLAGS=`python3 ./python-config --ldflags`

all: _ESL.so

esl_wrap.cpp:
	swig -module ESL -python -c++ -DMULTIPLICITY -threads -I../src/include -o esl_wrap.cpp ../ESL.i

esl_wrap.o: esl_wrap.cpp
	$(CXX) $(CXX_CFLAGS) $(CXXFLAGS) $(LOCAL_CFLAGS) -c esl_wrap.cpp -o esl_wrap.o

_ESL.so: esl_wrap.o
	$(CXX) $(SOLINK) esl_wrap.o $(MYLIB) $(LOCAL_LDFLAGS) -o _ESL.so -L. $(LIBS)


clean:
	rm -f *.o *.so *~

swigclean:
	rm -f esl_wrap.* ESL.so

reswig:	swigclean esl_wrap.cpp
```
<br><br>

### __Build__
<br>

Looking at the Makefile
Values such as (MYLIB) and $(LIBS) are inherited from the Makefile in the parent directory. So you shouldn't build in the python3 directory. Build by modifying the Makefile in the parent directory.

The following shows only the modified parts of the Makefile in the esl directory. This Makefile contains environment variables for Python 2, and these values are automatically created at the time of the first ./configure. These values do not need to be changed, just add the following for python3.

<br/><br/>

``` Makefile
py3mod: $(MYLIB)
	$(MAKE) MYLIB="../$(MYLIB)" SOLINK="$(SOLINK)" CFLAGS="-I$(switch_srcdir)/libs/esl/src/include $(SWITCH_AM_CFLAGS)" CXXFLAGS="-I$(switch_srcdir)/libs/esl/src/include $(SWITCH_AM_CXXFLAGS)" CXX_CFLAGS="$(CXX_CFLAGS)" -C python3
```
<br/><br/>

``` bash
root@ubuntusrv:/usr/local/src/freeswitch-1.10.3.-release/libs/esl# make py3mod
make MYLIB=".././.libs/libesl.a" SOLINK="-shared -Xlinker -x" CFLAGS="-I/usr/local/src/freeswitch-1.10.3.-release/libs/esl/src/include -I/usr/include/uuid -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/libs/libteletone/src -fPIC -ffast-math -Werror -Wno-unused-result -Wno-misleading-indentation -fvisibility=hidden -DSWITCH_API_VISIBILITY=1 -DCJSON_API_VISIBILITY=1 -DHAVE_VISIBILITY=1 -g -ggdb -DHAVE_OPENSSL" CXXFLAGS="-I/usr/local/src/freeswitch-1.10.3.-release/libs/esl/src/include -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/libs/libteletone/src -fPIC -ffast-math -fvisibility=hidden -DSWITCH_API_VISIBILITY=1 -DCJSON_API_VISIBILITY=1 -DHAVE_VISIBILITY=1" CXX_CFLAGS="" -C python3
make[1]: Entering directory '/usr/local/src/freeswitch-1.10.3.-release/libs/esl/python3'
swig -module ESL -python -c++ -DMULTIPLICITY -threads -I../src/include -o esl_wrap.cpp ../ESL.i
g++  -I/usr/local/src/freeswitch-1.10.3.-release/libs/esl/src/include -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/src/include -I/usr/local/src/freeswitch-1.10.3.-release/libs/libteletone/src -fPIC -ffast-math -fvisibility=hidden -DSWITCH_API_VISIBILITY=1 -DCJSON_API_VISIBILITY=1 -DHAVE_VISIBILITY=1 `python3 ./python-config --includes` -c esl_wrap.cpp -o esl_wrap.o
g++ -shared -Xlinker -x esl_wrap.o .././.libs/libesl.a `python3 ./python-config --ldflags` -o _ESL.so -L.
make[1]: Leaving directory '/usr/local/src/freeswitch-1.10.3.-release/libs/esl/python3'
```

<br/><br/>
You can see that it built successfully.  <br/>
Now you can find ESL.py and _ESL.so files. We will now import and use ESL in our Python code. And ESL.py will call the functions of _ESL.so using swig.
<br/><br/>

## Testing in Python 3
<br/><br/>
You can test it as simply as:
<br/><br/>

``` python
root@ubuntusrv:/usr/local/src/freeswitch-1.10.3.-release/libs/esl/python3# python3
Python 3.8.5 (default, Jan 27 2021, 15:41:15)
[GCC 9.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import ESL
>>> con = ESL.ESLconnection('localhost', '8021', 'ClueCon')
>>> if con.connected(): print('connect success')
...
connect success

```

<br/><br/>
To use in another directory, copy the ESL.py file and _ESL.so file to the working directory and use them together.
<br/><br/>

## Example in Python 3

<br/><br/>
The following Python program receives various events that occur in Freeswitch installed on the same computer and displays them on the screen. <br/>
And if __filter = 'all'__ , all events are received. By modifying the filter value, only desired events can be selectively received.

``` python
#-*- coding: utf-8 -*-
import ESL
import json
import threading
import time, sys
import sqlite3
import pprint

filter = 'all'

# In Freeswitch 1.6, sometimes connection errors occur when using localhost. So use '127.0.0.1'
con = ESL.ESLconnection('127.0.0.1', '8021', 'ClueCon') 


if con.connected():
    con.events('json', filter)
    while 1:
        e = con.recvEvent()
        if e:
            j = json.loads(e.serialize('json'))
            print(e.serialize('json'))
            '''
            if "variable_sofia_profile_name" in j:
                if("internal" == j["variable_sofia_profile_name"]):
                    print(e.serialize('json'))
            '''
else:
    print('esl connect failed')
print('esl programm end')
```
<br/><br/>

# Event Receiving

<br/><br/>
For event reception, I will make an example of processing the events of extensions 1001 and 1002 receiving inbound calls in [Freeswitch Callcenter using mod_fifo](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/Freeswitch%20Calltcenter%20using%20mod_fifo)'s FIFO. <br/>

This Python program receives only two events, CHANNEL_ORIGINATE and CHANNEL_ANSWER from Freeswitch.
<br/><br/>

``` python
import ESL
import json
import os
from datetime import datetime

#filter = 'CHANNEL_ORIGINATE CHANNEL_ANSWER CHANNEL_HANGUP CHANNEL_HANGUP_COMPLETE '
filter = 'CHANNEL_ORIGINATE  CHANNEL_ANSWER '


def display(j):
    now = datetime.now()
    print("\n\nEvent-Time:", now)
    print("Event-Name:" + j["Event-Name"])

    if "Call-Direction" in j:
        print("Call-Direction:" + j["Call-Direction"])      
    if "Channel-Call-UUID" in j:
        print("Channel-Call-UUID:" + j["Channel-Call-UUID"])

    if "Channel-Name" in j:
        print("Channel-Name:" + j["Channel-Name"])          #full name
    if "Caller-ANI" in j:
        print("Caller-ANI:" + j["Caller-ANI"])
    if "Caller-Destination-Number" in j:
        print("Caller-Destination-Number:" + j["Caller-Destination-Number"])
    if "Channel-Call-State" in j:
        print("Channel-Call-State:" + j["Channel-Call-State"])
    if "variable_uuid" in j:
        print("variable_uuid:" + j["variable_uuid"])



def event_job():
    con = ESL.ESLconnection('127.0.0.1', '8021', 'ClueCon')
    print('Connecting to ESL ....')
    if con.connected():
        print('Connecting to ESL SUCCESS')
        con.events('json', filter)
        while 1:
            e = con.recvEvent()
            if e:
                j = json.loads(e.serialize('json'))
                if j:
                    #print(json.dumps(j))
                    display(j)                
            if con.connected() == False:
                print("error:", "ESL connection broken")
                os.kill(os.getpid(), signal.SIGINT)

def main():
    event_job()

if __name__ == "__main__":
    main()
```

<br/><br/>
If you run this program and call 2000 from an external phone in the environment where the FIFO is tested, you can see the following events occur.

``` bash
root@ubuntusrv:/usr/local/src/esl# python3 fifo_event.py 
Connecting to ESL ....
Connecting to ESL SUCCESS


Event-Time: 2023-06-14 21:20:41.643748
Event-Name:CHANNEL_ANSWER
Call-Direction:inbound
Channel-Call-UUID:bab2bedf-fdb4-4301-8ca3-962ba2dbe15a
Channel-Name:sofia/blueivr/6208@192.168.150.1:5066
Caller-ANI:6208
Caller-Destination-Number:2000
Channel-Call-State:EARLY
variable_uuid:bab2bedf-fdb4-4301-8ca3-962ba2dbe15a
Answer-State:answered




Event-Time: 2023-06-14 21:20:41.929757
Event-Name:CHANNEL_ORIGINATE
Call-Direction:outbound
Channel-Call-UUID:bde95592-ae9f-44a6-a46d-72fb6e44f00a
Channel-Name:sofia/internal/1001@192.168.150.1:59813
Caller-ANI:6208
Caller-Destination-Number:1001
Channel-Call-State:DOWN
variable_uuid:bde95592-ae9f-44a6-a46d-72fb6e44f00a
Answer-State:ringing




Event-Time: 2023-06-14 21:20:41.930040
Event-Name:CHANNEL_ORIGINATE
Call-Direction:outbound
Channel-Call-UUID:1391b830-b436-4095-9b8b-009edb3a17af
Channel-Name:sofia/internal/97825064@192.168.150.1:59801
Caller-ANI:6208
Caller-Destination-Number:97825064
Channel-Call-State:DOWN
variable_uuid:1391b830-b436-4095-9b8b-009edb3a17af
Answer-State:ringing




Event-Time: 2023-06-14 21:20:45.225359
Event-Name:CHANNEL_ANSWER
Call-Direction:outbound
Channel-Call-UUID:bde95592-ae9f-44a6-a46d-72fb6e44f00a
Channel-Name:sofia/internal/1001@192.168.150.1:59813
Caller-ANI:6208
Caller-Destination-Number:1001
Channel-Call-State:RINGING
variable_uuid:bde95592-ae9f-44a6-a46d-72fb6e44f00a
Answer-State:answered
```
<br/><br/>
CHANNEL_ORIGINATE event corresponds to ringing, and CHANNEL_ANSWER occurs when a call is connected. <br/>
Among these events, if you filter only those whose Caller-Destination-Number is 1001 or 1002, you can receive ringing and answer events from stations 1001 and 1002. <br/>
The json data contains much more information than the print statement above. You can find and use the necessary part in the json data.

<br/><br/>


## Event Distribution
<br/><br/>
Event distribution is the process of sending previously received events to clients that want to receive them.
After setting the data format such as json or xml, convert the previously received event to suit the format of json or xml before sending it.
I will not dwell on this point further as it is beyond the scope of this article.
I prefer to use a messaging system rather than developing directly with a client/server architecture.
Using a messaging system such as MQTT or RabbitMQ, events can be delivered without much difficulty. Client programs can also be developed in various languages such as Javascript, python, java, c#, and python.