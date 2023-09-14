# Freeswitch recording with agent information

Call center systems often record customer service calls.
Recently, I created a call center system using mod_callcenter, but I ran into a problem creating recording files. In the dial plan, the customer's consultation call is transferred to a call center queue. This call is then distributed to the appropriate agent according to mod_callcenter's call distribution rules.
However, in the dial plan, it is impossible to know which agent the call will be distributed to, so it is impossible to include agent (or extension phone number) information in the recording file.

Below is the first dial plan. The following dial plan uses mod_callcenter.

``` xml
<extension name="CALLCENTER_MAIN">
  <condition field="destination_number" expression="^(07070067777)$">
    <action application="set" data="continue_on_fail=true"/>
    <action application="set" data="caller_id_name=${destination_number}" />
    <action application="set" data="record_session=$${base_dir}/recordings/${strftime(%Y%m%d%H%M%S)}_${caller_id_number}_${destination_number}.wav" />
    <action application="callcenter" data="myqueue@default" />
  </condition>
</extension>
```
When I called the number "07070067777" using the dial plan above, I was able to confirm that the call was properly routed to the agents registered in myqueue. You can also see that recording files are created in the recordings directory as follows. This is a list of recording files of calls received by a customer service representative via the call center "myqueue" queue by calling from 01058400500 to 07070067777.

``` bash
[root@tmoney_pbx ~]# ls -al /usr/local/freeswitch/recordings/
total 872
drwxrws---  2 root root   4096 Sep 14 12:57 .
drwxr-xr-x 20 root root   4096 Sep  6  2021 ..
-rw-r--r--  1 root root 219244 Sep 14 00:46 20230914004619_01058400500_07070067777.wav
-rw-r--r--  1 root root 111724 Sep 14 10:45 20230914104500_01058400500_07070067777.wav
-rw-r--r--  1 root root  87404 Sep 14 10:45 20230914104521_01058400500_07070067777.wav
-rw-r--r--  1 root root  74604 Sep 14 10:46 20230914104610_01058400500_07070067777.wav
```
However, this dial plan has one drawback. That is, there is no information about the agent who answered the phone in the recording file.

To solve this problem, the dial plan must be modified. At the time of setting record_session, it is not yet determined which agent the call will be distributed to, so this problem must be solved through post-processing after recording is completed.
In FreeSWITCH, set one of the values of "record_post_process_exec_api" and "record_post_process_exec_app" to enable post-processing after recording is finished.
I will use "record_post_process_exec_app".

```xml
<extension name="CALLCENTER_MAIN">
  <condition field="destination_number" expression="^(07070067777)$">
    <action application="set" data="continue_on_fail=true"/>
    <action application="set" data="caller_id_name=${destination_number}" />

    <action application="set" data="cc_recordfile=$${base_dir}/recordings/${strftime(%Y%m%d%H%M%S)}_${caller_id_number}_${destination_number}"/>
    <action application="set" data="record_post_process_exec_app=lua:after_record.lua ${cc_recordfile}"/>

    <action application="set" data="record_session=${cc_recordfile}.wav" />
    <action application="callcenter" data="myqueue@default" />
  </condition>
</extension>
```

A Lua script that can be executed after recording is added to the dial plan. Now, when recording is finished, this script will run automatically. Now let’s create the after_record.lua file as follows:

```lua
recFilePath = argv[1]	--"/usr/local/freeswitch/recordings/****"
if nil == recFilePath then
    freeswitch.consoleLog("warning", "==== Invalid Parameter\n")
    return
end

freeswitch.consoleLog("warning", "==== After Recording Process\n")
freeswitch.consoleLog("warning", "==== Recording FileName:" ..recFilePath .. "\n")
agent = session:getVariable("cc_agent")
if nil ~= agent then
    freeswitch.consoleLog("warning", "==== Agent:" ..agent .."\n")
    os.rename(recFilePath ..".wav", recFilePath .."_" ..agent ..".wav")
end
```

And if you test again, you can see that after_record.lua is executed as follows. You can also check agent information stored in the “cc_agent” variable. Now we can use this information to change the name of the recorded file to the format we want.

```bash
EXECUTE [depth=0] sofia/blueivr/01058400592@1.228.34.170 lua(after_record.lua /usr/local/freeswitch/recordings/20230914131834_01058400500_07070067777.wav /usr/local/freeswitch/recordings/20230914131834_01058400500_07070067777)
2023-09-14 13:18:39.839776 [WARNING] switch_cpp.cpp:1465 ==== After Recording Process
2023-09-14 13:18:39.839776 [WARNING] switch_cpp.cpp:1465 ==== Recording FileName:/usr/local/freeswitch/recordings/20230914131834_01058400500_07070067777
2023-09-14 13:18:39.839776 [WARNING] switch_cpp.cpp:1465 ==== Agent:1001
```
<br>
Now you can create a recording file containing agent information as follows.<br>

```bash
[root@tmoney_pbx ~]# ls -al /usr/local/freeswitch/recordings/
total 872
drwxrws---  2 root root   4096 Sep 14 12:57 .
drwxr-xr-x 20 root root   4096 Sep  6  2021 ..
-rw-r--r--  1 root root 219244 Sep 14 00:46 20230914004619_01058400500_07070067777.wav
-rw-r--r--  1 root root 111724 Sep 14 10:45 20230914104500_01058400500_07070067777.wav
-rw-r--r--  1 root root  87404 Sep 14 10:45 20230914104521_01058400500_07070067777.wav
-rw-r--r--  1 root root  74604 Sep 14 10:46 20230914104610_01058400500_07070067777.wav
-rw-r--r--  1 root root  73964 Sep 14 12:39 20230914123918_01058400500_07070067777_1001.wav
-rw-r--r--  1 root root  96364 Sep 14 13:11 20230914131135_01058400500_07070067777_1001.wav
```