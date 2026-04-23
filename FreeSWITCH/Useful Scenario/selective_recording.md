# Selective Recording

There are two methods for recording calls in call center systems. One is the full recording method, which records the entire call, and the other is the selective recording method, which records only the desired sections of the call when necessary.

For the initial test, I created a dial plan and performed selective recording using the bind_meta_app application, which I had previously used in attended_transfer. However, I soon discovered that there was a problem with my choice. 
The reason is that with bind_meta_app, the other party's leg changes to a hold state when they press DTMF.
Selective recording should proceed naturally without interrupting the flow of the call, but a situation occurred where the other party suddenly had to listen to hold music.
Therefore, I discovered that the bind_digit_action application is more suitable than bind_meta_app.

The following link contains an implementation of attended transfer using FreeSWITCH's bind_meta_app application.
If you are interested in attended transfer, please read this article. A detailed explanation of bind_meta_app is also available in this article.

* [attended transfer](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/FreeSWITCH/Useful%20Scenario/attended_transfer.md)

<br>

# Dialplan

<br>

## Selective Recording

<br>

The recording function is, of course, only possible when a call is actually made. For testing purposes, I will first create a dial plan that receives an incoming external call on an internal line.

To do this, first create a dial plan as follows.
The explanation of how to create a sip profile for a domestic dial plan is omitted.

To test a call transfer, you must first create a call.

1. Let's create the following scenario first.
2. Call FreeSWITCH line 07047378800 from external trunk 01090224600.
2. Extension 5007 answers a call from external line 01090224600.
3. 5007 press *2 to perform an selective recording.
4. The recording guide message plays on both a and b Legs, and recording begins.
5. Press *4 to end the recording. However, it is not mandatory to press it. If you do not press it, the recording will automatically end when the call ends.

To do this, first create a dial plan as follows.


```xml
<!--trunk xml conf/dialplan/***.xml-->
<extension name="TRUNK_CUSTOMER_BLUE1">
  <condition field="${sip_to_user}" expression="^(07047378301)$">
            <action application="set" data="continue_on_fail=true"/>
            <action application="export" data="hold_music=$${base_dir}/sounds/common/elise.wav" />            

            <action application="set" data="bridge_export_vars=bind_digit_action_status,digit_action_set_realm"/>
            <action application="set" data="bind_digit_action_status=activated"/>
            <action application="set" data="digit_action_set_realm=recording"/>
            <action application="bind_digit_action" data="recording,*2,exec:execute_extension,88 XML features,both,both"/>
            <action application="bind_digit_action" data="recording,*4,exec:execute_extension,89 XML features,both,both"/>
           
            <action application="bridge" data="USER/5007@$${domain}"/>
  </condition>
</extension>
```
<br>

You can modify above dialplan to suit your environment.You just need to create a dial plan that allows a connection from the external line to the internal line.

**Note that the above dial plan uses the bind_digit_action application. This application provides the ability to launch a new dial plan during a call by pressing *2.**

<br>

### bridge_export_vars

<br>

* Role: Defines a list of variables to be copied to the other party's session (B-Leg) when a call is connected by the bridge application.

* Detailed Description:
```
By default, variables set with `set` in FreeSWITCH remain only in the current channel (A-Leg).
However, to detect the button pressed by 5007 (B-Leg) after the call is connected, the B-Leg session must also contain the information that "digit watch is currently turned on."
This line is a command that reserves the instruction to "make sure to pass the variable values ​​of `bind_digit_action_status` and `digit_action_set_realm` to the other party when the call is connected later."
```

<br>

### bind_digit_action_status

<br>

* Role: Activates the digit watch engine (digit_action).

* Description:
```
The value of this variable must be set to activated for FreeSWITCH's switch_ivr_digit_helper to intercept and process incoming DTMF (digit button signals) during a call.
Without this setting, the engine will not operate and will be ignored, no matter how bind_digit_action is configured below.
```
<br>

### digit_action_set_realm

<br>

* Role: Specifies the name of the pattern area (Realm) that the current channel will monitor.

* Detailed Description:

```
bind_digit_action can have multiple groups (Realms). (e.g., recording group, transmission group, etc.)
When a user makes a call, the system recognizes, "This call only needs to monitor the number rules defined within the pocket (Realm) called 'recording'!"
In the <action application="bind_digit_action" data="recording,*2,..."/> code you wrote earlier, the value of this variable must match the first argument, 'recording', for *2 to be matched correctly.
```

<br>

📌 **Summary: Why is it written this way?**

* A standard set is information known only to **me (A)**. However, to process a button pressed by **the other party (B)**, **the other party (B)** must also know the rule.

* A decides, "I am going to use the 'recording' rule" while making a call. (digit_action_set_realm)

* A commands, "Activate this rule." (bind_digit_action_status)

* When A connects with B, A sends a bundle saying, "I will give you this rule information I have, B." (bridge_export_vars)

As a result, when **number 5007 (B-Leg)** presses *2, based on the information received within their session, they determine, "Ah, *2 is to start recording (call 88)!" and execute it.




```xml
<include>
  <context name="features">
  <extension name="record_start">
    <condition field="destination_number" expression="^88$">
      <action application="log" data="ALERT [RECORDING_START]"/>
      <action application="displace_session" data="$${base_dir}/sounds/common/recording_started.wav mux"/>
      <!--<action application="playback" data="$${base_dir}/sounds/common/recording_started.wav"/>-->
      <action application="set" data="rec_file=$${base_dir}/recordings/${strftime(%Y-%m-%d-%H-%M-%S)}_${caller_id_number}.wav"/>
      <action application="record_session" data="${rec_file}"/>
    </condition>
  </extension>

  <extension name="record_stop">
    <condition field="destination_number" expression="^89$">
      <action application="log" data="ALERT [RECORDING_STOP]"/>
      <action application="stop_record_session" data="${rec_file}"/>
      <!--<action application="playback" data="$${base_dir}/sounds/custom/recording_stopped.wav"/>-->
    </condition>
  </extension>  
  
  </context>
</include>
```

<br>



### displace_session

<br>

Initially, I intended to use playback, but a slight issue was discovered. It plays normally in Leg A, but the beginning of Leg B is cut off. 
To resolve this issue, I used the displace_session application instead of playback.

<br>

📌 **Why displace_session?**

* Prevents call disconnection: While playback can disconnect the call if it fails to take ownership of the channel's media, displace is safe because it simply 'adds' sound to the audio stream.

* Relieves clipping: Since it injects sound while maintaining the call's connection (Bridge), the clipping that occurs when the audio device reopens in B-Leg is significantly reduced.