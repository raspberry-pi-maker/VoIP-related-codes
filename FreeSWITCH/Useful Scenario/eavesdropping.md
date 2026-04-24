# eavesdropping

Eavesdropping is the act of stealthily listening to a private conversation without the consent of the parties involved. The term originates from literally standing under the "eaves" (the edges of a roof) of a house to hear what is being said inside.

In the world of VoIP, Eavesdropping is a feature that allows a third party (typically a supervisor or administrator) to join an active call between two other participants.

* Silent Monitoring: Usually, the eavesdropper can hear both sides of the conversation, but their own microphone is muted so the original parties don't know they are being listened to.
* Barge-in/Whisper: Advanced versions of eavesdropping allow the supervisor to "barge in" (talk to both parties) or "whisper" (talk only to the internal agent without the external customer hearing).

The core application implementing the eavesdropping function in FreeSWITCH is eavesdrop.

Let's find out how to implement eavesdropping in Lua scripts using eavesdrop application.

<br>

## Dialplan

<br>

## Implementation of eavesdropping

<br>

To test a eavesdropping, you must first create a call.
Let's create the following scenario first.

1. Call FreeSWITCH line 07047378800 from external trunk 01090224600.
2. Extension 5007 answers a call from external line 01090224600.
3. Extension 5006 presses 885007 to eavesdrop on extension 5007's call.
4. FreeSWITCH executes Lua scripts registered in the dial plan.
5. Inside the Lua script scenario,  retrieves the UUID of 5007, which is currently on a call.
6. Then, call the eavesdrop application to request eavesdropping.

To do this, first create a dial plan as follows.


```xml
<!--trunk xml conf/dialplan/***.xml-->
    <extension name="TRUNK_CUSTOMER_BLUE2">
      <condition field="${sip_to_user}" expression="^(07047378800)$">
            <action application="log" data="ALERT ====  In-call IVR service Test  ======"/>
            <action application="spandsp_start_dtmf" data=""/>
            <action application="set" data="continue_on_fail=true"/>
            <action application="export" data="hold_music=$${base_dir}/sounds/common/elise.wav" />            
            <action application="bridge" data="USER/5007@$${domain}"/>

      </condition>
    </extension>
```
<br>

Then, add a scenario to the extension dial plan that handles 88 + extension number as follows. I will use 88 as the prefix for eavesdropping.

```xml
<!--conf/dialplan/default.xml-->
    <extension name="Local_Extension PBX_EAVESDROP">
      <condition field="destination_number" expression="^88(\d{4})$">
        <action application="log" data="ALERT ==== From Station to eavesdrop ==== "/>
        <action application="set" data="continue_on_fail=true"/>
        <action application="set" data="hangup_after_bridge=true"/>
        <action application="lua" data="eavesdrop.lua  $1" />
      </condition>
    </extension>
```
<br>

Finally, here is the Lua script to execute the eavesdropping.

<br>

```lua
-- eavesdrop.lua
local target_exten = argv[1] -- Target extension passed from the dialplan ($1)
local domain = session:getVariable("domain_name")

if session:ready() then
    local target_full = target_exten .. "@" .. domain
    
    -- 1. Retrieve all current channel information
    local channels_data = fs_api:execute("show", "channels")
    local target_uuid = ""

    -- 2. Find the line containing target_exten (e.g., 5007) and extract the UUID
    -- The output format of 'show channels' is typically 'uuid,direction,created,....'
    for line in channels_data:gmatch("[^\r\n]+") do
        if line:find(target_exten) then
            -- The first item separated by a comma (,) is the UUID
            target_uuid = line:match("([^,]+)")
            if target_uuid and #target_uuid > 10 then -- Verify valid UUID length
                break
            end
        end
    end

    freeswitch.log("WARNING", "Lookup Result - Target: " .. target_full .. " (Extracted UUID: " .. tostring(target_uuid) .. ")\n")

    -- 3. Validate and execute eavesdrop
    if (target_uuid and target_uuid ~= "" and not target_uuid:find("INVALID")) then
        session:streamFile(common_path .."dingdong.wav")
        freeswitch.log("INFO", "Eavesdrop execution successful: " .. target_exten .. " (UUID: " .. target_uuid .. ")")
        session:execute("eavesdrop", target_uuid)
    else
        freeswitch.log("ERR", "Eavesdrop failed: Active session for user " .. target_exten .. " not found.")
        session:streamFile(common_path .. "no_user_found.wav")
    end
end
```
<br>

"show channels" CLI command used to check all currently active channels (call sessions)
Naturally, this output value must also contain extension 5007 information.
If extension information does not exist, it is not a currently ongoing call. Therefore, it cannot be a target for eavesdropping.

From this output value, the uuid value of the internal line (5007) channel that you want to eavesdropping is taken.

The eavesdrop application requires the UUID of the channel to be eavesdroped as a parameter, and you simply need to pass the previously obtained UUID value.
One thing to note is that you must connect any call attempting to eavesdrop using the 'answer' command. This is because voice transmission is only possible in such cases.


### eavesdrop

<br>

The eavesdrop application provides not only simple eavesdropping but also the ability to intervene in calls when necessary.

For example:

* If you wish to speak with extension 5007 (B-Leg) during eavesdropping, press 2. In this case, the conversation will not be transmitted to the customer (A-Leg).

* If you press 1, conversely, you can speak with the customer (A-Leg), but extension 5007 (B-Leg) will not be able to hear the conversation.

* If you wish to speak with both A and B-Leg (three-way call), press 3. In this case, conversation between all participants in the call becomes possible.

* To return to the initial eavesdropping mode (where the eavesdropper's voice is not transmitted), press 0.

To use the above function, set the DTMF and whisper option values ​​to true as follows.

<br>

```xml
<action application="set" data="eavesdrop_enable_dtmf=true"/> <!-- false means no commands during eavesdrop -->
<action application="set" data="eavesdrop_whisper_aleg=true"/> <!-- enables whisper mode in aleg -->
<action application="set" data="eavesdrop_whisper_bleg=true"/> <!-- enables whisper mode in bleg -->
```

<br>

Alternatively, you can set the variable before calling eavesdrop in a Lua script as follows.

```lua
session:setVariable("eavesdrop_enable_dtmf", true)
session:setVariable("eavesdrop_whisper_aleg", true)
session:setVariable("eavesdrop_whisper_bleg", true)

-- necessary work

session:execute("eavesdrop", target_uuid)
```

<br>

# Wrapping up

<br>

eavesdrop is an application that provides eavesdropping capabilities. By utilizing various option values, you can use the following powerful features.

🌟 **Key Advantages of eavesdrop**

* Real-time Monitoring : Enables supervisors to listen to ongoing calls instantly, ensuring quality control and compliance.

* Training and Coaching : Supports whisper mode, allowing managers to give live feedback to agents without the customer hearing.

* Crisis Intervention : Provides the option to barge into a call (three-way mode) when immediate managerial involvement is required.

* Flexible Control : DTMF commands allow supervisors to switch between listening, whispering, and barging seamlessly during monitoring.

* Security and Permissions : Variables like eavesdrop_require_group ensure only authorized users can access monitoring functions.

* Operational Efficiency : Helps identify issues quickly, improve agent performance, and maintain customer satisfaction.


