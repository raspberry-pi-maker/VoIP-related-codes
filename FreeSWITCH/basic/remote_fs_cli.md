# Remote access using fs_cli

<br>
Perhaps the most convenient tool for monitoring FreeSWITCH is fs_cli. Usually, fs_cli is run on a local computer where FreeSWITCH is installed. However, sometimes you need to access remotely. Several tasks are required to use fs_cli remotely.



<br>

## Increasing security

<br>
When using fs_cli locally, use it without any separate settings. However, when connecting remotely, you must change the connection port, connection account password, and network ACL (Access Control List) values.

<br>
The parameters of fs_cli can be easily checked using --help.
<br>

```bash
[root@localhost freeswitch]# fs_cli --help

Usage: fs_cli [-H <host>] [-P <port>] [-p <secret>] [-d <level>] [-x command] [-t <timeout_ms>] [profile]

  -?,-h --help                    Usage Information
  -H, --host=hostname             Host to connect
  -P, --port=port                 Port to connect (1 - 65535)
  -u, --user=user@domain          user@domain
  -p, --password=password         Password
  -i, --interrupt                 Allow Control-c to interrupt
  -x, --execute=command           Execute Command and Exit
  -l, --loglevel=command          Log Level
  -U, --log-uuid                  Include UUID in log output
  -S, --log-uuid-short            Include shortened UUID in log output
  -q, --quiet                     Disable logging
  -r, --retry                     Retry connection on failure
  -R, --reconnect                 Reconnect if disconnected
  -d, --debug=level               Debug Level (0 - 7)
  -b, --batchmode                 Batch mode
  -t, --timeout                   Timeout for API commands (in milliseconds)
  -T, --connect-timeout           Timeout for socket connection (in milliseconds)
  -n, --no-color                  Disable color

```
<br>

And the setting that controls the connection of ESL clients, including fs_cli, in FreeSWITCH is the event_socket.conf.xml file.

<br>

```xml
<configuration name="event_socket.conf" description="Socket Client">
  <settings>
    <param name="nat-map" value="false"/>
    <param name="listen-ip" value="0.0.0.0"/>   <!-- You shoud set this value as 0.0.0.0 for IPV4-->
    <param name="listen-port" value="8021"/>
    <param name="password" value="ClueCon"/>    <!-- You shoud change this value-->
    <param name="apply-inbound-acl" value="loopback.auto"/> <!-- You shoud change this value-->
    <!--<param name="stop-on-bind-error" value="true"/>-->
  </settings>
</configuration>
```

<br>

|Key value|Description|
|------|---|
|apply-inbound-acl|Use the value defined in acl.conf.xml. If you want to allow access from users using the same subnet, change this value to LAN. "loopback.auto" is the default value and only loopback interface on localhost. Therefore, you must change the apply-inbound-acl value to enable remote access.|
|password|Be sure to change the remote password.|
|listen-port|In IPV4, "0.0.0.0" is used; in IPV6, "::" is used. Then, when binding the event socket, any client IP is allowed to connect.|
|listen-port|It is not necessary to change it, but it is safer to change it if remote access is allowed.|
|nat-map|Whether to allow access when connecting from an external terminal with a private IP through a router. If possible, maintain flase so as not to allow access from private terminal IPs.|
|||


<br>
Let's assume you set it up like this:
<br>

```xml
<configuration name="event_socket.conf" description="Socket Client">
  <settings>
    <param name="nat-map" value="false"/>
    <param name="listen-ip" value="0.0.0.0"/>   
    <param name="listen-port" value="8055"/>
    <param name="password" value="ClueConPassword150"/>    
    <param name="apply-inbound-acl" value="lan"/> 
    <!--<param name="stop-on-bind-error" value="true"/>-->
  </settings>
</configuration>
```

<br>
Then, you can connect FreeSWITCH(192.168.0.100) using fs_cli from another computer as follows.

<br>


```bash
fs_cli --host=192.168.0.100 --port=8055 --password=ClueConPassword150
```
