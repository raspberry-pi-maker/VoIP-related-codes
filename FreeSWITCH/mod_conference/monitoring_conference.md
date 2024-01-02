# Monitoring conference

__This document is applicable to FreeSWITCH 1.6 or higher. We recommend using 1.10 if possible.__

<br>

## Prerequisites

<br>

* [mod_conference](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/mod_conference.md). 
* [Very Simple Voice Conference Examples](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/voice_conference_example.md). 
* [Voice Conference with Inviting Example](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/voice_conference_invite.md). 
* [Invite other attendees during a conference](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/voice_conference_complex_invite.md). 

<br>

Previously, I looked at how to create conference rooms for various cases.
Unlike the scenario of joining an already created conference room using an inbound phone call, when inviting conference participants outbound, it is necessary to check whether they have been properly invited.

Let's look at the following lua scenario. This is the lua script used in [Voice Conference with Inviting Example](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/FreeSWITCH/mod_conference/voice_conference_invite.md). This is a scenario where a conference room is created for calls coming in through the SIP Tunk, and outbound is attempted through an extension line to attend the conference.

```lua
-- FreeSWITCH api
fs_api = freeswitch.API()

ani = session:getVariable("ani")
dnis = session:getVariable("destination_number") 

freeswitch.consoleLog("NOTICE", string.format("Simple voice trunk conference => Conference Call from [%s] to [%s]\n", ani, dnis))
session:execute("ring_ready")
session:sleep(500) 
session:answer()

conf_name = "1005"   --station number who I'll invite to the conference room
conf_profile = "simpleconf2"
flags = "mute|hangup"

--Prepare auto outcall
session:execute("set", "conference_auto_outcall_timeout=60")
session:execute("set", "conference_auto_outcall_flags=mute")  
session:execute("set", "conference_auto_outcall_caller_id_name=" .. ani)
session:execute("set", "conference_auto_outcall_caller_id_number=" .. ani)

--    session:execute("set", "conference_auto_outcall_maxwait=10") 
prefix_string = "{sip_contact_user=" .. ani .. ", answer_delay=0"
prefix_string = prefix_string .. "}"

freeswitch.consoleLog("WARNING", "prifix_string:" .. prefix_string .. "\n")
session:execute("set", "conference_auto_outcall_prefix=" .. prefix_string)

-- Invite station to the conference room "user/1005@$${domain}"
session:execute("conference_set_auto_outcall", "user/" .. conf_name .."@$${domain}")

-- Put the (trunk) caller in the conference room
session:execute("conference", string.format("%s@%s++flags{%s}", conf_name, conf_profile, flags))
freeswitch.consoleLog("NOTICE", "Simple voice trunk conference => End Conference\n")

-- destroy conference
conf_cmd = conf_name .. " kick all"
freeswitch.consoleLog("WARNING", "conference " .. conf_cmd)
fs_api:execute("conference", conf_cmd)

```

<br>

In the script above, the code that requests outbound to extension 1005 to attend the meeting is the following line.

<br>

```lua
-- Invite station to the conference room "user/1005@$${domain}"
session:execute("conference_set_auto_outcall", "user/" .. conf_name .."@$${domain}")
```

<br>

The application "conference_set_auto_outcall" returns immediately after the call and executes another task in FreeSWITCH to carry out this work. And while the outbound operation is in progress, the lua script continues, and the following code immediately allows an external phone to enter the conference room.

```lua
-- Invite station to the conference room "user/1005@$${domain}"
-- Put the (trunk) caller in the conference room
session:execute("conference", string.format("%s@%s++flags{%s}", conf_name, conf_profile, flags))
freeswitch.consoleLog("NOTICE", "Simple voice trunk conference => End Conference\n")
```

<br>

In this scenario, there is a problem in which the result of `conference_set_auto_outcall` cannot be confirmed.
If you want to create a scenario where you want to connect to another extension if extension 1005 is not answering the phone, is busy, or is not registered, the above scenario is not enough.

<br>

## Hook conference events

<br>

mod_lua provides an event hooking functionality. Just add the event you want to hook to in the /autoload_configs/lua.conf.xml file as follows.

```xml
<settings>
    ......
    <hook event="CUSTOM" subclass="conference::maintenance" script="event_conference.lua"/>
</settings>
```
And then restart FreeSWITCH. mod_lua cannot be reloaded. Now, whenever FreeSWITCH generates a conference::maintenance custom event, event_conference.lua is called.

<br>


### conference events

<br>
Conference events include:

<br>

|event|description|
|--|--|
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
|play-file-member_done|Member–level play file ended|
|lock|Conference locked, no one else can enter|
|unlock|Conference unlocked|
|transfer|Member transferred to another conference|
|bgdial-result|Result from bgdial API command|
|floor-change|Conference floor changed|
|start-recording|Conference recording started|
|stop-recording|Conference recording stopped|


<br>

### conference events callback lua script

<br>
FreeSWITCH will now call this Lua script whenever a conference event occurs. This script simply prints event content, but you can add additional functions depending on your needs.
<br>

```lua
--[[
 File Name : event_conference.lua
 Description : hook event(Conference::maintenance>add-member/del-member/execute_app)
]]

-- FreeSWITCH api
fs_api = freeswitch.API()


function print_add_member()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
	conf_member_id = event:getHeader("Member-ID")
    profile = event:getHeader("Conference-Profile-Name")
    size = event:getHeader("Conference-Size")
    ani = event:getHeader("Caller-Caller-ID-Number")
    dnis = event:getHeader("Caller-Destination-Number")
    direction = event:getHeader("Caller-Direction")
    conference = conf_name .."@" ..profile
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .."\nMember-ID:" .. conf_member_id .. "\nDirection:" .. direction .. "\nANI:" .. ani.. "\nDNIS:" .. dnis  .. "\nSize:" .. size.. "\n")   
end

function print_del_member()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
	conf_member_id = event:getHeader("Member-ID")
    profile = event:getHeader("Conference-Profile-Name")
    size = event:getHeader("Conference-Size")
    ani = event:getHeader("Caller-Caller-ID-Number")
    dnis = event:getHeader("Caller-Destination-Number")
    direction = event:getHeader("Caller-Direction")
    conference = conf_name .."@" ..profile
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .."\nMember-ID:" .. conf_member_id .. "\nDirection:" .. direction .. "\nANI:" .. ani.. "\nDNIS:" .. dnis  .. "\nSize:" .. size.. "\n")
end

function print_bgdial_result()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    result = event:getHeader("Result")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .."\nResult:" .. result.. "\n")
end

function print_floor_change()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    floor = event:getHeader("Floor")
    if floor then
        freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .. "\nFloor:" .. floor   .. "\n")
    else
        freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .. "\n")
    end    
end

function print_start_recording()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    path = event:getHeader("Path")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action  .. "\nPath:" .. path   .. "\n")
end
function print_stop_recording()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    path = event:getHeader("Path")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action  .. "\nPath:" .. path   .. "\n")
end

function print_play_file_member_done()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile

    conf_member_id = event:getHeader("Member-ID")
    ani = event:getHeader("Caller-Caller-ID-Number")
    dnis = event:getHeader("Caller-Destination-Number")
    direction = event:getHeader("Caller-Direction")
    file = event:getHeader("File")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .."\nMember-ID:" .. conf_member_id .. "\nDirection:" .. direction .. "\nANI:" .. ani .. "\nDNIS:" .. dnis .. "\nFile:" .. file   .. "\n")
end

function print_conference_create()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action  .. "\n")
end

function print_conference_destroy()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action  .. "\n")
end

function print_execute_app()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    conf_member_id = event:getHeader("Member-ID")
    direction = event:getHeader("Caller-Direction")
    application = event:getHeader("Application")
    ani = event:getHeader("Caller-ANI")
    dnis = event:getHeader("Caller-Destination-Number")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .. "\nMember-ID:" .. conf_member_id .. "\nCaller-Direction:" .. direction  .. "\nANI:" .. ani .. "\nDNIS:" .. dnis.. "\nApplication:" .. application  .. "\n")
end
function print_dtmf()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    profile = event:getHeader("Conference-Profile-Name")
    conference = conf_name .."@" ..profile
    conf_member_id = event:getHeader("Member-ID")
    direction = event:getHeader("Caller-Direction")
    dtmf = event:getHeader("DTMF-Key")
    ani = event:getHeader("Caller-ANI")
    dnis = event:getHeader("Caller-Destination-Number")
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .. "\nMember-ID:" .. conf_member_id .. "\nCaller-Direction:" .. direction  .. "\nANI:" .. ani .. "\nDNIS:" .. dnis.. "\nDTMF:" .. dtmf  .. "\n")
end


action = event:getHeader("action")

if(action == "add-member") then
    print_add_member()
elseif(action == "del-member") then
    print_del_member()
elseif(action == "bgdial-result") then
    print_bgdial_result()
elseif(action == "floor-change") then
    print_floor_change()
elseif(action == "start-recording") then
    print_start_recording()
elseif(action == "stop-recording") then
    print_stop_recording()
elseif(action == "play-file-member-done") then
    print_play_file_member_done()
elseif(action == "conference-create") then
    print_conference_create()
elseif(action == "conference-destroy") then
    print_conference_destroy()
elseif(action == "execute-app") then
    print_execute_app()
elseif(action == "dtmf") then
    print_dtmf()

end


```

<br>


## Get the number of conference participants

<br>
In the case of programs that receive events and operate asynchronously, such as the Lua script above, there are times when it is necessary to check the status of the conference room. The most frequently needed function is to check the number of participants in the current conference room. You can simply check this in the Lua script as follows.

<br>

The print_add_member function, which handles the "add-member" event that is called when a member is added to the conference room, can be improved as follows.

<br>

```lua
function print_add_member()
    action = event:getHeader("action")
    conf_name = event:getHeader("Conference-Name")
    conf_member_id = event:getHeader("Member-ID")
    profile = event:getHeader("Conference-Profile-Name")
    size = event:getHeader("Conference-Size")
    ani = event:getHeader("Caller-Caller-ID-Number")
    dnis = event:getHeader("Caller-Destination-Number")
    direction = event:getHeader("Caller-Direction")
    conference = conf_name .."@" ..profile
    freeswitch.consoleLog("WARNING",  "\nConference:" ..conference .."\naction:" .. action .."\nMember-ID:" .. conf_member_id .. "\nDirection:" .. direction .. "\nANI:" .. ani.. "\nDNIS:" .. dnis  .. "\nSize:" .. size.. "\n")

    conf_cmd = conf_name .. " list count"
    count = fs_api:execute("conference", conf_cmd)
    freeswitch.consoleLog("WARNING",  "Current conference attendee count:" .. count .. "\n")
    
end
```

<br>

If you use a conference room name that does not exist, the count value will not be a number and an error message will appear as follows.

```bash
-ERR Conference 1005@simpleconf2 not found
```







