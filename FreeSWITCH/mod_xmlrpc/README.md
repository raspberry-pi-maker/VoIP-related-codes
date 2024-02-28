# Freeswitch mod_xmlrpc

xmlrpc provides the ability to call APIs from outside FreeSWITCH, similar to esl (event socket library).
If esl provides the ability to control FreeSWITCH using various languages such as python, c/c++, lua, and ruby, xmlrpc can control FreeSWITCH through http requests.<br/><br/>

## Configuration
<br>
The xmlrpc service is provided through mod_xmlrpc. Therefore, the module must be registered in the conf/autoload_configs/modules.conf.xml file as follows.<br/><br/>

``` xml
<configuration name="modules.conf" description="Modules">
  <modules>
    ......
    <!-- XML Interfaces -->
    <load module="mod_xml_rpc"/>
    ......
  </modules>
</configuration>
```

<br>
And the module must be confirmed in fs_cli as follows.

```bash
freeswitch@blueivr> module_exists mod_xmlrpc
false
```

<br/>

Next, create or modify the conf/autoload_configs/xml_rpc.conf.xml file that configures xmlrpc as follows.

``` xml
<configuration name="xml_rpc.conf" description="XML RPC">
  <settings>
    <!-- The port where you want to run the http service (default 18080) -->
    <param name="http-port" value="18081"/>
    <!-- if all 3 of the following params exist all http traffic will require auth -->
    <param name="auth-realm" value="freeswitch"/>
    <param name="auth-user" value="blueivr"/>
    <param name="auth-pass" value="bluebaynetworks"/>

    <!-- regex pattern to match against commands called against this service.
                  If a command with arguments matches, it will be logged at INFO level -->
    <param name="commands-to-log" value="lua"/>

  </settings>
</configuration>
```
<br>

## Run lua script using xmlrpc synchronously

<br>

Once the settings are complete, you can call the lua script as follows. Testing can be done using curl or a browser. First, create a lua script to be used for testing as follows. 

<br/>


```lua script
--- test_rpc.lua
freeswitch.consoleLog("WARNING", "XML RPC Test parameter count:" ..tostring(#argv) .."\n")

if #argv > 0 then
    freeswitch.consoleLog("WARNING", "XML RPC Parameter:" .. argv[1] .."\n")
end

function sleep(n)
    os.execute("sleep " ..tostring(n))
end

sleep(5)
freeswitch.consoleLog("WARNING", "XML RPC Test End\n") 
stream:write("Ok World\n")
```

<br/>

And you can use curl on the console of the same host to call cmlrpc as follows.

<br/>

```bash
[root@tmoney_pbx scripts]# curl --verbose http://blueivr:bluebaynetworks@127.0.0.1:18081/webapi/lua?test_rpc.lua%20B
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 18081 (#0)
* Server auth using Basic with user 'blueivr'
> GET /webapi/lua?test_rpc.lua%20B HTTP/1.1
> Host: 127.0.0.1:18081
> Authorization: Basic Ymx1ZWl2cjpibHVlYmF5bmV0d29ya3M=
> User-Agent: curl/7.61.1
> Accept: */*
>
< HTTP/1.1 200 OK
< freeswitch-user: blueivr
< freeswitch-domain: 127.0.0.1
< Connection: close
< Date: Wed, 28 Feb 2024 02:42:43 GMT
< Server: FreeSWITCH-1.10.6-release~64bit-mod_xml_rpc
< Content-Type: text/html
<
Ok World
* Closing connection 0
```

<br/>

The curl process returns after about 5 seconds. And you can see that "Hello World" sent from the lua script is received and printed. **Please note that the curl process that called xmlrpc does not terminate until the lua script terminates.**

<br>

## Run lua script using xmlrpc asynchronously

<br>

This time, let's operate xmlrpc in an asynchronous manner. curl that calls xmlrpc will immediately receive a return value and the process will terminate, and the lua script will continue to operate in FreeSWITCH even after curl is terminated.
**There is one thing to note. Because the xmlrpc call returns immediately in an asynchronous manner, the "stream:write("Ok World\n")" used earlier cannot be used. This is because in the asynchronous method, the stream no longer exists in the lua script.**

<br>


```lua script
--- test_rpc.lua
freeswitch.consoleLog("WARNING", "XML RPC Async Test parameter count:" ..tostring(#argv) .."\n")

if #argv > 0 then
    freeswitch.consoleLog("WARNING", "XML RPC Parameter:" .. argv[1] .."\n")
end

function sleep(n)
    os.execute("sleep " ..tostring(n))
end

sleep(5)
freeswitch.consoleLog("WARNING", "XML RPC Async Test End\n") 
--stream:write("Ok World\n")
```

<br>
Use luarun? instead of lua? in API calls.
<br>


```bash
[root@tmoney_pbx scripts]# curl --verbose http://blueivr:bluebaynetworks@127.0.0.1:18081/webapi/luarun?test_rpc.lua%20B
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 18081 (#0)
* Server auth using Basic with user 'blueivr'
> GET /webapi/luarun?test_rpc.lua%20B HTTP/1.1
> Host: 127.0.0.1:18081
> Authorization: Basic Ymx1ZWl2cjpibHVlYmF5bmV0d29ya3M=
> User-Agent: curl/7.61.1
> Accept: */*
>
< HTTP/1.1 200 OK
< freeswitch-user: blueivr
< freeswitch-domain: 127.0.0.1
< Connection: close
< Date: Wed, 28 Feb 2024 02:54:17 GMT
< Server: FreeSWITCH-1.10.6-release~64bit-mod_xml_rpc
< Content-Type: text/html
<
+OK
* Closing connection 0
```
<br>

However, if you monitor with fs_cli, you can see that the following message is output after 5 seconds.

<br>

```bash
2024-02-28 11:54:17.017649 [INFO] mod_xml_rpc.c:1085 Executed HTTP request command: [luarun test_rpc.lua B].
2024-02-28 11:54:17.037645 [WARNING] switch_cpp.cpp:1465 XML RPC Async Test parameter count:1
2024-02-28 11:54:17.037645 [WARNING] switch_cpp.cpp:1465 XML RPC Parameter:B
2024-02-28 11:54:22.037647 [WARNING] switch_cpp.cpp:1465 XML RPC Async Test End

```


<br>

## Run API using xmlrpc

<br>

```bash
[root@tmoney_pbx scripts]# curl --verbose  http://blueivr:bluebaynetworks@127.0.0.1:18081/webapi/status
*   Trying 127.0.0.1...
* TCP_NODELAY set
* Connected to 127.0.0.1 (127.0.0.1) port 18081 (#0)
* Server auth using Basic with user 'blueivr'
> GET /webapi/status HTTP/1.1
> Host: 127.0.0.1:18081
> Authorization: Basic Ymx1ZWl2cjpibHVlYmF5bmV0d29ya3M=
> User-Agent: curl/7.61.1
> Accept: */*
>
< HTTP/1.1 200 OK
< freeswitch-user: blueivr
< freeswitch-domain: 127.0.0.1
< Connection: close
< Date: Wed, 28 Feb 2024 03:08:54 GMT
< Server: FreeSWITCH-1.10.6-release~64bit-mod_xml_rpc
< Content-Type: text/html
<
<h1>FreeSWITCH Status</h1>
2024-02-28 12:08:54<br>
UP 0 years, 0 days, 19 hours, 23 minutes, 8 seconds, 93 milliseconds, 520 microseconds<br>
FreeSWITCH (Version 1.10.6 -release 64bit) is ready<br>
0 session(s) since startup<br>
0 session(s) - peak 0, last 5min 0 <br>
0 session(s) per Sec out of max 30, peak 0, last 5min 0 <br>
31 session(s) max<br>
min idle cpu 15.00/0.00<br>
Current Stack Size/Max 240K/8192K
* Closing connection 0
```
<br>


```bash
[root@tmoney_pbx scripts]# curl -s -S -G -X GET --user blueivr:bluebaynetworks --data-urlencode "codec as xml as xml" http://127.0.0.1:18081/xmlapi/show
<result row_count="26">
  <row row_id="1">
    <type>codec</type>
    <name>ADPCM (IMA)</name>
    <ikey>mod_spandsp</ikey>
  </row>
  <row row_id="2">
    <type>codec</type>
    <name>G.711 alaw</name>
    <ikey>CORE_PCM_MODULE</ikey>
  </row>
  ......
  <row row_id="25">
    <type>codec</type>
    <name>VP8 Video</name>
    <ikey>CORE_VPX_MODULE</ikey>
  </row>
  <row row_id="26">
    <type>codec</type>
    <name>VP9 Video</name>
    <ikey>CORE_VPX_MODULE</ikey>
  </row>
</result>
```
<br>
