# Valet Parking

FreeSWITCH's Valet Parking function stores the person on the phone in a specific “parking slot” and automatically connects when another user accesses that slot number. In other words, it allows you to store and retrieve calls through a “parking lot number” more intuitively than a simple park.

<br>

## Valet parking concept

<br>

### basic principles

<br>

* The first person to enter the parking slot will be put on hold and listen to music (MOH).
* The second person to enter the same slot is automatically connected to the first person.
* Once the connection is complete, that slot becomes free again to accept new calls.
* You can specify the parking waiting time using the session variable valet_parking_timeout. If there is no pickup during this time, it will exit the parking slot.
* The picked up call enters the bridge state. And when the bridge is terminated, the valet_park application is terminated.
* If you press # in bridge mode, the bridge is disconnected and the valet_park application is terminated.

<br>

### Differences from parks

<br>

* park: Simply put on hold in the system, requires separate bridge/forwarding when retrieved.

* valet_park: Enables number-based recall, allowing users to immediately retrieve a call by pressing a specific number.

<br>


### Advantages and Precautions

<br>

#### Advantages

* Intuitive number-based recall → Improved user experience.
* Simplifying conference call connections.
* Stored safely without interruption of waiting parties.

<br>

#### Precautions

* Only one person can wait in the same slot.
* Attended transfer with some services (e.g. voicemail, conference) may cause compatibility issues. Blind transfer operates normally.

<br>

In a general telephone system, incoming calls are connected to a specific IVR, extension, or trunk line according to established rules.
However, in valet parking, incoming calls wait in a parked state and it is possible to actively decide whether to connect the waiting call.

To use mod_valetpark properly, you need to identify cars parked in parking slots in real time, so it is useful to use not only FreeSWITCH's functions, but also databases and electronic sign systems.

<br><br>

# valet_park API

<br>

## application

<br>

| Application |  Purpose  | Arguments  |
| --- |  --- | --- |
| valet_park |  Park or retrieve a call in a named lot. |<lotname> <extension> or <lotname> ask [<min>] [<max>] [<timeout_ms>] [<prompt>] or <lotname> auto in|out <min> <max> |

<br>

## Extension Argument Modes

<br>

| Mode |  Syntax  | Behavior  |
| --- |  --- | --- |
|Explicit |  <lotname> <extension> | Park at the specified extension; if a parked call already exists at that extension and is unbridged, retrieve and bridge to it. |
|Ask | <lotname> ask [<min>] [<max>] [<timeout_ms>] [<prompt>] | Play a prompt and collect DTMF from the caller to determine the extension number. |
| Auto-in |  <lotname> auto in <min> <max> | Automatically assign the lowest available extension in the range <min> to <max> and park the call. Announces the assigned slot to the caller. |
| Auto-out |  <lotname> auto out <min> <max> | Retrieve the longest-waiting parked call in the range <min> to <max>. |

<br>

## ask Mode Fallback Channel Variables

<br>

When arguments to ask mode are omitted, valet_park reads the following channel variables as defaults. Positional arguments take precedence over channel variables.

| Variable |  Purpose  | Default if absent  |
| --- |  --- | --- |
| valet_ext_min |  Minimum DTMF digit count for extension entry | 1 |
| valet_ext_max |  Maximum DTMF digit count for extension entry | 11 |
| valet_ext_to |  DTMF collection timeout in milliseconds | 10000 |
| valet_ext_prompt |  Sound file to play as the extension entry prompt | ivr/ivr-enter_ext_pound.wav |


<br>

## Channel Variables Honored by valet_park

<br>

| Variable |  Purpose  | 
| --- |  --- |
| valet_hold_music |  Hold music to play while parked. Falls back to the channel's configured hold music. |
| valet_announce_slot |  Set to false to suppress the slot announcement in auto in mode. Default: true. |
| valet_parking_timeout |  Seconds before the parked call is transferred to the orbit extension. |
| valet_parking_orbit_exten |  Extension to transfer to on parking timeout. |
| valet_parking_orbit_dialplan |  Dialplan to use for the orbit transfer. |
| valet_parking_orbit_context |  Context to use for the orbit transfer. |
| valet_parking_orbit_exit_key |  DTMF digit that immediately triggers the orbit transfer. |

<br>

## Channel Variables Set by valet_park

<br>

| Variable |  Value  | 
| --- |  --- |
| valet_lot_extension |  The extension number at which this channel is parked. |

<br><br>

# Dialplan

<br>

Parking slots must not overlap, so they must be managed well in the dial plan.
If you use the destination number as the parking slot name, you should not receive duplicate calls with the same number.

You can handle this directly in the dial plan, but using a lua script allows for a much more flexible design.

<br>

## Trunk dial plan

<br>

This is a dial plan that applies valet parking when a call is made to the trunk number 07040005000.

```xml
    <extension name="VALET_PARKING_TEST">
      <condition field="destination_number" expression="^(07040005000)$">
        <action application="set" data="continue_on_fail=true"/>
        <action application="set" data="hangup_after_bridge=true"/>		    
        <action application="log" data="ALERT ====  VALET_PARKING TEST START ==== "/>
        <action application="lua" data="sample/valet_park.lua"/>
      </condition>
    </extension>
```

And this is an extension dial plan that connects valet parked calls from an extension.

```xml
  <extension name="Valet Parking Test">
	 <condition field="destination_number" expression="^5000$">
        <action application="log" data="ALERT ==== Valet Parking Test For extension ======"/>
        <action application="set" data="continue_on_fail=true"/>
        <action application="set" data="hangup_after_bridge=false"/> 
        <action application="lua" data="sample/valet_park.lua" />
    </condition>
  </extension> 
```

And this is a Lua script that implements both parking and pickup.

```lua
--[[
Test number : 07040005000
This scenario assumes that the agent knows the slot name and retrieves the extension value to connect to the parked call.
]]

me = session:getVariable("sip_to_user")
you = session:getVariable("caller_id_number")


local session = session;
local api = freeswitch.API();

-- Function to extract values inside <extension> tags from XML
local function get_valet_json(xml_str, target_lot)
    local results = { lot = target_lot, extensions = {} }
    
    -- Pattern matching to extract uuid and extension number
    -- Match pattern <extension uuid="UUID">NUMBER</extension>
    for uuid, ext in xml_str:gmatch('<extension uuid="([^"]+)">([^<]+)</extension>') do
        table.insert(results.extensions, {
            uuid = uuid,
            extension = ext
        })
    end
    
    --return json.encode(results)
    return results
end

--[[
  @param target_lot_name: lot name to search (e.g., "my_lot")
  @param data_table: dictionary created by get_valet_table function
  @return: first extension number of the lot (e.g., "7001"), nil if not found
--]]
local function get_extension_by_lot_name(target_lot_name, data_table)
    -- Check if the dictionary lot name matches the target name
    if data_table.lot == target_lot_name then
        -- Check if the extensions array has values
        if data_table.extensions and #data_table.extensions > 0 then
            -- Return the first parked number
            return data_table.extensions[1].extension
        end
    end
    return nil
end

function pickup()
  if session:ready() then
      session:answer();

      local lot_name = "my_lot";
      local slot_number = "7001";

      -- 1. Check if there is a parked call in the current slot (valet_info lot_name)
      -- Verify if the result contains slot_number
      --[[
        <lots>
          <lot name="my_lot">
            <extension uuid="5ef3c304-e30a-49af-ae27-084e7592392c">7001</extension>
          </lot>
        </lots>      
      ]]
      local status = api:execute("valet_info", lot_name);
      local status_json = get_valet_json(status, lot_name)
      
      if status_json == nil then
        return
      end
      local ext_number = get_extension_by_lot_name(lot_name, status_json)
      if ext_number == nil then
        return
      end
      
      -- If slot_number exists in the status string (e.g., "<7001> ...")
      if status and string.find(status, slot_number) then
          freeswitch.consoleLog("warning", "--- [Parking] A exists. Attempting connection: " .. ext_number .. " ---\n");
          session:execute("valet_park", lot_name .. " " .. slot_number);
      else
          -- 2. Handle case where A has already hung up or slot is empty
          freeswitch.consoleLog("warning", "--- [Parking] Slot " .. slot_number .. " is empty. Ending call.---\n");
          session:hangup();
      end
  end
end


function park()
  music = "common/elise.wav"
  if session:ready() then
      session:answer();
      -- Use this to disable the # key
      --session:execute("bind_digit_action", "my_realm,#,exec:log,INFO #_pressed_but_ignored")
      
      session:execute("export", "valet_parking_timeout=10")
      session:execute("export", "valet_hold_music=" ..music)
      -- Announcement message (optional)
      -- session:execute("playback", "phrase:voicemail_play_greeting");
      
      -- Park into slot 7001 of lot named 'my_lot'
      -- valet_park [lot_name] [slot_number]
      session:execute("valet_park", "my_lot 7001");
      session:sleep(500)
  end
end


local firstChar = you:sub(1,1)
if string.len(you) == 4 and firstChar == '5' then
  pickup()
  freeswitch.consoleLog("warning", "[Pick]  End. Scenario finished\n");
else
  park()  
  if session:ready() then
    freeswitch.consoleLog("warning", "[Park] End code (#) pressed or timeout\n")
  else
    freeswitch.consoleLog("warning", "[Park] Bridge disconnected\n")
  end

end

```

Both parking and pickup were handled in one scenario.
In lua, the lot name and slot number of the parking slot are fixed as "my_lot 7001", but this information must be managed dynamically using a database, etc., so that unique values ​​can always be used.


* The valet_park application parks calls based on lot name(my_lot) and slot number(7001).
* When a parked call is dropped, a timeout occurs, or the user presses # to exit the valet_park application, that slot is no longer occupied.
* This means that slots are automatically emptied and deleted. In other words, the slot named "my_lot 7001" will no longer remain active once the parked session ends.

<br><br>


## Wrapping up

<br>

Valet parking is not a function that is used often, but it can be very useful in some cases.
Although it is not a call center, a place that must handle consultation calls can use an electronic signboard to provide information about parking calls and provide a function where available employees can pick up the call and provide consultation.