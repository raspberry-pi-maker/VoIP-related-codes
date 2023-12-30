# mod_conference

__This document is applicable to FreeSWITCH 1.6 or higher. We recommend using 1.10 if possible.__

mod_conference provides both inbound and outbound conference bridge service for FreeSWITCH™. It can process multiple bit rates, load various profiles that specify DTMF controls, play prompt sounds and tones, and many other functions. 

<br>

## Important concepts
<br>
The following are important concepts used in mod_conference's configuration file(autoload_configs/conference.conf.xml).

The following is the basic structure of the conference.conf.xml file.
<br>

```xml
<configuration name="conference.conf" description="Standard Conference configuration">
  <advertise>
        [... config here ...]
  </advertise>

  <caller-controls>
    <group name="default">
        [... config here ...]
    </group>
  </caller-controls>

  <chat-permissions>
    <profile name="default">
        [... config here ...]
    </profile>
  </chat-permissions>    

  <profiles>
    <profile name="888">
        [... config here ...]
    </profile>
  </profiles>

</configuration>
```

<br/><br/>

### advertise
<br>

This block specifies whether empty conferences should be advertised in presence, allowing you to see status of empty conferences. Any endpoint that supports presence, such as mod_sofia, can subscribe to these presence notifications.

```xml
  <advertise>
    <room name="888@$${subdomain}" status="FreeSWITCH"/>
  </advertise>
```
The conference name (888 in this case) should be the profile name that you specified in <profiles> section, $${subdomain} will be replaced with the subdomain that you specified in freeswitch.xml.

"status" is advertised as whatever you pass to it (identifier) or "Available" if none is passed.

__This block has the function of informing endpoints that support the sip presence function of the currently empty conference. However, it is not a necessary block at a conference.__

<br>

### caller-controls
<br>

Caller controls are used to modify the state of the conference, such as lowering the volume, mute a participant, and such. Below are the commands that can be assigned to digits and executed during a conference. The "moderator-controls" group provides additional controls for participants who enter the conference with the moderator flag set. See below.

<br>

__Reserved Control Group Names__

|Name|Description|
|------|---|
|None|Use this name to prevent installing caller-controls for callers to a conference.|
|default|This group of settings will be assigned if no "caller-controls" is specified. You can also assign it explicitly. This group is defined in vanilla config, thus removing it from the configurations will make no caller controls at all|

<br>

__action__
<br>
Each of these actions can be configured in caller-controls to be invoked by a conference member who enters the conference with this caller-controls group. These are instances of the conference API.

|Action|Description|
|------|---|
|mute|Toggle audio from this member into the conference|
|mute on|Disable audio from this member into the conference|
|mute off|Enable audio from this member into the conference|
|deaf mute|Block audio from conference to this member as well as mute, in one action|
|energy up|Increase minimum energy threshold by 1 unit above which sound will be admitted into conference (noise gate)|
|energy equ|Reset minimum energy threshold to default|
|energy dn|Decrease minimum energy threshold by 1 unit|
|vol talk up|Increase member talk (mic) volume into conference by 1 unit|
|vol talk zero|Reset talk volume to default setting|
|vol talk dn|Decrease talk volume by 1 unit|
|vol listen up|Increase member receive (earpiece) volume by 1 unit|
|vol listen zero|Reset member receive volume to default setting|
|vol listen dn|Decrease member receive volume by 1 unit|
|hangup|Leave the conference|
|event|Send the DTMF event via CUSTOM conference::maintenance subclass to the event system (even to event socket)|
|lock|Toggle the conference lock state (no new members can enter when locked)|
|transfer|Transfer member to a given extension in a dialplan context|
|execute_application|Execute a dialplan application|
|floor|Toggle yourself on and off of talking floor, as long as no one else has floor status.|
|vid-floor|Video floor. If video floor is currently locked, it will revert to auto; if there is no current holder, you become video floor holder|
|vid-floor-force|	Video floor. If video floor is currently locked, it will revert to auto, otherwise you become the locked video floor holder|
|vmute|Video mute. Toggle video from this member into the conference|
|vmute on|Disable video from this member into the conference|
|vmute off|Enable video from this member into the conference|
|vmute snap|Take a video snapshot for this user to be used when in vmute|
|vmute snapoff|Discard the vmute video snapshot|
|||


<br>
This is the default controls dtmf values.

<br>

```xml
   <group name="default">
      <control action="vol talk dn"      digits="1"/>
      <control action="vol talk zero"    digits="2"/>
      <control action="vol talk up"      digits="3"/>
      <control action="vol listen dn"    digits="4"/>
      <control action="vol listen zero"  digits="5"/>
      <control action="vol listen up"    digits="6"/>
      <control action="energy dn"        digits="7"/>
      <control action="energy equ"       digits="8"/>
      <control action="energy up"        digits="9"/>
      <control action="mute"             digits="0"/>
      <control action="deaf mute"        digits="*"/>
      <control action="hangup"           digits="#"/>
   </group>
```

<br>

This is the controls example.

```xml
  <caller-controls>
    <group name="somekeys">
      <control action="vol talk dn"         digits="1"/>
      <control action="vol talk zero"       digits="2"/>
      <control action="vol talk up"         digits="3"/>
      <control action="transfer"            digits="5"   data="100 XML default"/>
      <control action="execute_application" digits="0"   data="playback conf_help.wav"/>
      <control action="event" digits="#" />
    </group>
  </caller-controls>
```

<br><br/>

__Be aware that the caller-controls are applied across the entire conference session. You cannot apply one group of caller-controls to one member and then a second group of caller-controls to a second member in the same conference session.__

<br/><br/>


### profiles
<br>

You can specify a number of different profiles in the profiles section, these will let you easily apply a number of settings to a conference. Please note that the profiles are not conference rooms, but define settings that are later applied to conference rooms. The dialplan section in this document will describe how you create conference rooms and apply these profile settings. <br>
You can have any number of profile tags, and each profile can have any number of param tags up to the entire set of parameters.

<br><br>

__Conference Profile Parameters__

|Name|Description|Default Value|
|------|---|-|
|announce-count|Requires TTS. If the number of members equals or exceeds this value, the conference will speak the count to the conference after a new member joins|0|
|auto-gain-level|Enables Automatic Gain Control (AGC). If the parameter is set to 'true', then the default AGC value is used. If set to a number it will override the default value|1100|
|auto-record|Set a filename or stream value for this parameter in order to enable recording every conference call. Within mod_conference there is a special parameter named ${conference_name} that can be used to form the record filename. All channel variables are accessible as well for generating a unique filename. NOTE: auto-record doesn't begin recording until the number of conferees specified in min-required-recording-participants have joined.| |
|broadcast-chat-message|Message to send in response to chat messages.| |
|caller-controls|Name of the caller control group to use for this profile. It must be one of those specified in the <caller-controls> section|default|
|moderator-controls|Name of the moderator control group to use for this profile. It must be one of those specified in the <caller-controls> section|default|
|cdr-log-dir|	Target directory for conference CDRs to be written. Use "auto" to store in $PREFIX/log/conference_cdr. An absolute path is acceptable as is a relative path. A relative path will yield $PREFIX/log/<value> for the conference CDR directory||
|cdr-event-mode|Include full cdr or path to file in conference cdr event|none|
|caller-id-name|Default Caller ID Name for outbound calls originated by mod_conference|conference|
|caller-id-number|Default Caller ID Number for outbound calls originated by mod_conference|0000000000|
|channels|The number of audio channels. Special value "auto" sets this based on the channels of the first member to enter|auto|
|comfort-noise|Sets the volume level of background white noise to add to the conference. Special value "true" sets to default value. 0 for no CN (total silence).|1400|
|conference-flags|Can be any combination of allowed values separated by "|" (pipe character).||
|description|Description of the conference that is included in some events||
|domain|The domain name used for presence events from conferences||
|energy-level|Noise gate. Energy level required for audio to be sent to the other users. The energy level is a minimum threshold of 'voice energy' that must be present before audio is bridged into the conference. Useful if a participant is in a noisy environment, so their background noise is heard only when they speak. 0 disables the noise gate and will bridge all packets even if they are only background noise.||
|interval|Number of milliseconds per frame. Which may be different from ptime in SIP SDP, or driver with TDM. Higher numbers require less CPU but can degrade conversation quality, so experimentation with your setup is best. The default is good for conversation quality. Special "auto" value sets interval based on interval of the first member who enters.||
|max-members|Sets a maximum number of participants in conferences with this setting in its profile.||
|member-flags|Can be any combination of allowed values separated by "|". See table below for descriptions.||
|pin|Pin code that must be entered before user is allowed to enter the conference.||
|moderator-pin|Pin code that must be entered before moderator is allowed to enter the conference.||
|pin-retries|Max number of times the user or moderator will be asked for PIN||
|rate|Audio sample rate. Special value "auto" sets this based on the rate of the first member to enter.||
|sound-prefix|Set a default path here so you can use relative paths in the other sound params.||
|suppress-events|For use with the event socket. This parameter is a comma delimited string that specifies which events will NOT be sent to the socket when getting CUSTOM conference::maintenance events.||
|terminate-on-silence|Specifies the number of contiguous seconds of silence after which the conference will automatically terminate and disconnect all members.||
|timer-name|Name of the timer interface in freeswitch to use for timing the conference|soft|
|tts-engine|Text-To-Speech (TTS) Engine to use||
|tts-voice|	TTS Voice to use||
|verbose-events|Events related to the conference will send more data. Specifically the events related to members will include all the channel variables on each event|false|
|alone-sound|File to play if you are alone in the conference||
|bad-pin-sound|File to play to when the pin is invalid||
|enter-sound|File to play when you join the conference||
|exit-sound|File to play when you leave the conference||
|is-locked-sound|File to play when the conference is locked during the call to the members in the conference||
|is-unlocked-sound|File to play when the conference is unlocked during the call||
|join-only-sound|File to play when member with join-only flag tries to create the conference (join as the first).||
|kicked-sound|File to play when you are kicked from the conference||
|ocked-sound|File to play when the conference is locked and someone goes to join||
|max-members-sound|If max-members has been reached, this sound plays instead of allowing new users to the conference||
|moh-sound|he given sound file/resource will be played only when the conference size is 1 member. It will loop over and over until the member count is 2 or more. When the conference goes back to 1 member it will play again||
|muted-sound|File to play when member is muted||
|mute-detect-sound|If the mute-detect member-flag has been set, this sound plays when the user talks while muted||
|perpetual-sound|The given sound file/resource will be played on a loop forever. This can be used to broadcast sales or emergency messages to callers.||
|pin-sound|File to prompt the user to enter the conference pin code.||
|unmuted-sound|File to play when member is unmuted.||
|ivr-dtmf-timeout|Inter-digit timeout between DTMF digits in milliseconds|500|
|ivr-input-timeout|Time to wait for the first DTMF in milliseconds, zero = forever|0|
|endconf-grace-time|Defines how much time all members have before the conference is terminated when the last member with endconf leaves in seconds|0|
|min-required-recording-participants|Minimum number of conference participants required for their audio to be heard in a recording and for auto recording to start. This can be either 1 (the default) or 2.|1|
|outcall-templ|Template to use for outcall URL||
|video-mode|The mode to run video conferencing in. passthrough is non transcoded video follow audio. transcode allows for better switching and multiple codecs. mux allows for multiple parties on the video canvas at the same time|passthrough|
|video-layout-name|The layout name from conference_layouts.conf.xml or group prefixed by "group:". Setting this setting will enable the video mux. Not setting this will switch video presentation based on floor.||
|video-canvas-bgcolor|Overall Canvas color for video mux canvas as an HTML HEX color code|#333333|
|video-letterbox-bgcolor|Color to use for bars on the caller video if their aspect ratio doesn’t fit the member square in the layout as an HTML HEX color code|#000000|
|video-canvas-size|Pixel dimensions of the video mux canvas||
|video-fps|Frames per second to run the video mux at|15|
|video-codec-bandwidth|The bandwidth maximum for the conference output video stream, This setting can be auto, our defined in KB (kilobytes), mb (megabits) or MB (megabytes)|<integer>mb|
|video-no-video-avatar|Path to PNG file for member without video to display.||
|video-canvas-count|Number of canvases to use in mux mode|1|
|video-quality|Motion factor of the video, this is used to adjust the video bitrate. From 1 to 4, low motion to high motion video. (ref. Kush Gauge formula for determining bitrate).|1|
|video-border-color|Border color around each video feed|#000000|
|video-border-size|Border size (in pixels) around each video feed|0|
|video-mute-banner|Sets the video mute banner text, font, font size, font color, and background color for the member. font_scale valid values 5-50, fg/bg hex color code, all settings besides text are optional.||
|video-super-canvas-bgcolor|Background color of the supercanvas|#068DF3|
|video-super-canvas-label-layers|Label canvases in supercanvas|false|
|video-super-canvas-show-all-layers|Display all canvases, even empty ones|false|
|video-auto-floor-msec|Milliseconds of speaking before a speaker gets the floor|0|
|video-kps-debounce|Milliseconds between sending packets to the client to request a client bitrate adjustment (video only). Note that FreeSWITCH may decide to force sending more frequently under certain circumstances.|30000|
|auto-record-canvas-id|Which canvas on the supercanvas to auto record, (range is the )|1|
|video-layout-conf|Specify an alternate conference layout config file.|conference_layouts.conf|
||||


<br><br>

### Conference Flags

These flags control the operation of the conference session and apply to all members.

|Conference Flag|Description|
|------|---|
|wait-mod|Members will wait (with music) until a member with the 'moderator' flag set enters the conference|
|audio-always|Do not use energy detection to choose which participants to mix; instead always mix audio from all members|
|restart-auto-record|If the auto-record conference param is set, and recording is stopped, auto recording will continue in a new file|
|rfc-4579|Enable support for RFC-4579 SIP–based call control for conferences|
|livearray-sync|Add support for livearray advertisement for the conference status|
|auto-3d-position|Enable 3d positioned audio support|
|json-events|Send event-channel JSON events when members enter and leave|
|video-floor-only|Only video members can get floor. REMOVED in 1.6|
|minimize-video-encoding|Use a single video encoder per output codec|
|livearray-json-status|Machine parseable version of display for conference live array. Example: {"audio":{"muted":false,"onHold":false,"talking":true,"floor":true,"energyScore":639},"video":{"avatarPresented":false,"mediaFlow":"sendRecv","muted":false,"floor":true,"reservationID":null,"videoLayerID":0}}|
|video-bridge-first-two|In mux mode, If there are only 2 people in conference, you will see only the other member|
|video-muxing-personal-canvas|In mux mode, each member will get their own canvas and they will not see themselves|
|video-required-for-canvas|Only video participants will be shown on the canvas (no avatars)|
|video-mute-exit-canvas|	Don't display video muted users on the canvas|
|manage-inbound-video-bitrate|If calling client supports TMMBR, on each change of layout position FreeSWITCH will instruct the client to increase/reduce the video bit rate appropriately.|
|||



<br><br>

### Conference Events

These events are available to event consumers.

|Event Name|Description|
|------|---|
|add-member|Member added to a conference|
|del-member|Member removed from a conference|
|energy-level|Conference default energy level changed|
|volume-level|Conference default volume level changed|
|gain-level|Conference default gain level changed|
|dtmf|Key bound to transfer, event, or execute_application is hit by member|
|stop-talking|Member stopped talking (as detected by energy level)|
|start-talking|Member started talking (as detected by energy level)|
|mute-detect|Detected member speaking (as detected by energy level) while muted|
|mute-member|Member became muted|
|unmute-member|Member became unmuted|
|kick-member|Member kicked|
|dtmf-member|Key bound to DTMF is hit by member|
|energy-level-member|Member energy level changed|
|volume-in-member|Member gain level changed|
|volume-out-member|Member volume level changed|
|play-file|Conference–level play file started|
|play-file-done|Conference–level play file ended|
|play-file-member|Member–level play file started|
|lock|Conference locked, no one else can enter|
|unlock|Conference unlocked|
|transfer|Member transferred to another conference|
|bgdial-result|Result from bgdial API command|
|floor-change|Conference floor changed|
|record|Conference recording started or stopped|
|||

<br><br>


# Wrapping up

<br>
Conferences are one of the powerful features provided by FreeSWITCH. FreeSWITCH is also used exclusively for conference servers.
As you can see from the above article, there are numerous options for creating and conducting a conference. Conference methods can also be created in various ways, such as inbound, outbound, inbound + outbound, and a security function can be provided that allows COnference participants to participate by entering a PIN value. I will go over how to create an actual conference in the next article.







