# Voice Conference with Recording Example

__This document is applicable to FreeSWITCH 1.6 or higher. We recommend using 1.10 if possible.__

<br>

## Prerequisites

<br>

* [mod_conference](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/mod_conference.md). 
* [Very Simple Voice Conference Examples](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/voice_conference_example.md). 

<br>

Recording conference content is very simple. Just add “auto-record” to the profile in the conference.conf.xml file.

## conference.conf.xml

<br>

"auto-record" has been added to the following xml file, and year, month, and day directories are automatically created under the /usr/local/freeswitch/recordings directory to save conference contents as a wav file.

<br>


```xml
    <profile name="simpleconf2">
      <!-- Domain (for presence) -->
      <param name="domain" value="$${domain}"/>
      <!-- Sample Rate-->
      <param name="rate" value="8000"/>
      <!-- Number of milliseconds per frame -->
      <param name="interval" value="20"/>
      <!-- Energy level required for audio to be sent to the other users -->
      <param name="energy-level" value="100"/>

      <param name="muted-sound" value="conference/conf-muted.wav"/>
      <!-- File to play to acknowledge unmuted -->
      <param name="unmuted-sound" value="conference/conf-unmuted.wav"/>
      <!-- File to play if you are alone in the conference -->
      <param name="alone-sound" value="conference/conf-alone.wav"/>
      <!-- File to play endlessly (nobody will ever be able to talk) -->
      <!-- <param name="perpetual-sound" value="perpetual.wav"/> -->
      <!-- File to play when you're alone (music on hold)-->
      <param name="moh-sound" value="conference/conf-music.wav"/>
      <!-- File to play when you join the conference -->
      <!--<param name="enter-sound" value="tone_stream://%(200,0,500,600,700)"/>-->
      <param name="enter-sound" value="conference/conf-welcome.wav"/>
      <!-- File to play when you leave the conference -->
      <param name="exit-sound" value="conference/conf-exit.wav"/>
      <!-- File to play when you are ejected from the conference -->
      <param name="kicked-sound" value="conference/conf-kicked.wav"/>
      <!-- File to play to prompt for a pin -->
      <param name="pin-sound" value="conference/conf-pin.wav"/>
      <!-- File to play to when the pin is invalid -->
      <param name="bad-pin-sound" value="conference/conf-bad-pin.wav"/>
      <!-- Default Caller ID Name for outbound calls -->
      <param name="caller-id-name" value="$${outbound_caller_name}"/>
      <!-- Default Caller ID Number for outbound calls -->
      <param name="caller-id-number" value="$${outbound_caller_id}"/>
      <!-- enable comfort noise generation -->
      <param name="comfort-noise" value="true"/>
      <param name="auto-record" value="/usr/local/freeswitch/recordings/${strftime(%Y/%m/%d)}/${conference_name}_${strftime(%Y%m%d%H%M%S)}.mp3"/>
    </profile>
```
<br>

## dialplan

```xml
    <extension name="SIMPLE_CONFERENCE2">
        <condition field="destination_number" expression="^(2001)$">
            <action application="log" data="ALERT ==== Simple Conference2 CALL From ${caller_id_number}======"/>
	    <action application="set" data="continue_on_fail=true"/>
            <!--<action application="answer"/>-->
            <action application="conference" data="test@simpleconf2+7536+flags{mute}"/>
        </condition>
    </extension>
```
<br>

If you check after finishing the conference, you can see that the file was created as follows.

<br>

```bash
root@debian:/usr/local/freeswitch/recordings# tree
.
└── 2023
    └── 12
        └── 30
            └── test_20231230223631.wav

4 directories, 1 file

```
<br><br>

# Wrapping up
If you want to save the recording file as mp3, activate mod_shout and change the recording file extension to mp3.

```bash
root@debian:/usr/local/freeswitch/conf/autoload_configs# tree ../../recordings/
../../recordings/
└── 2023
    └── 12
        └── 30
            ├── test_20231230223631.wav
            └── test_20231230225130.mp3

4 directories, 2 files
```