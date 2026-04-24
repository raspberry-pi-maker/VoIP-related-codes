# In-call IVR service

If an in-call IVR service is required, there are several ways to implement the scenario.

1. Transferring the call from an external or internal line to an IVR connected to the PBX to receive the service, and then transferring the call back to the original internal line upon service termination.

2. Creating a three-way call or conference call to involve the IVR line.

3. Utilizing features provided by the PBX, such as FreeSWITCH or Asterisk.

Methods 1 and 2 are primarily used in traditional PBX systems where external lines, internal lines, and IVRs (internal or external lines) are connected around the PBX.
Services can also be implemented using methods 1 and 2 in FreeSWITCH.
However, FreeSWITCH allows for the implementation of such services using a simpler method.

You can use the bind_meta_app application that was used in attended transfer.

The following link contains an implementation of attended transfer using FreeSWITCH's bind_meta_app application.
If you are interested in attended transfer, please read this article. A detailed explanation of bind_meta_app is also available in this article.

* [attended transfer](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/FreeSWITCH/Useful%20Scenario/attended_transfer.md)

<br>


# Dialplan

<br>

## Implementation of in-call IVR service

<br>

To do this, first create a dial plan as follows.
The explanation of how to create a sip profile for a domestic dial plan is omitted.

To test a call transfer, you must first create a call.
Let's create the following scenario first.

1. Call FreeSWITCH line 07047378800 from external trunk 01090224600.
2. Extension 5007 answers a call from external line 01090224600.
3. 5007 press *5 to perform an IVR service.
4. FreeSWITCH executes Lua scripts registered in the dial plan.
5. Inside the Lua script scenario,  The dingdong sound is played only to b leg.
6. Inside the Lua script scenario, the guide message plays on a Legs, and requests DTMF input. Leg b only has a hold sound.
7. Just before the Lua script ends, play the dingdong sound once again only to b Leg to announce the termination of the IVR service.
8. The IVR service ends and returns to call mode.

To do this, first create a dial plan as follows.


```xml
<!--trunk xml conf/dialplan/***.xml-->
    <extension name="TRUNK_CUSTOMER_BLUE2">
      <condition field="${sip_to_user}" expression="^(07047378302)$">
            <action application="log" data="ALERT ====  In-call IVR service Test  ======"/>
            <action application="spandsp_start_dtmf" data=""/>
            <action application="set" data="continue_on_fail=true"/>
            <action application="export" data="hold_music=$${base_dir}/sounds/common/elise.wav" />            
            <!-- Press *5 during a call to run extension 90 -->
            <action application="bind_meta_app" data="5 b o execute_extension::90 XML features"/>
            <action application="bridge" data="USER/5007@$${domain}"/>

      </condition>
    </extension>
```
<br>

You can modify above dialplan to suit your environment.You just need to create a dial plan that allows a connection from the external line to the internal line.

📌 **Note:IVR services provided in a bridged state may have limitations. In particular, session functions must be verified through testing to ensure they are functioning correctly.**

<br>

```xml
<include>
  <context name="features">
    <extension name="incall_ivr">
    <condition field="destination_number" expression="^90$">
        <action application="log" data="ALERT feature incall service"/>
        <!-- executes bind_meta_app_get_digits.lua -->
        <action application="lua" data="sample/bind_meta_app_get_digits.lua"/>
      </condition>
    </extension>
  </context>
</include>
```

<br>

And the following is the last Lua script.

```lua
-- bind_meta_app_get_digits.lua 
if session:ready() then
    local b_uuid = session:getVariable("bridge_uuid")
    local api = freeswitch.API()

    if b_uuid then
        -- Plug directly into only the B-Leg output (Write).
        api:execute("uuid_displace", b_uuid .. " start common/dingdong.wav 0 w") 
        freeswitch.consoleLog("ALERT", ">>>> Agent-only Beep transmission\n")
    end
    
    
    session:flushDigits()
    local digits = session:playAndGetDigits(1, 4, 1, 5000, "#", "common/xfer_get4digits.wav", "", "\\d+")
    
    if (digits and digits ~= "") then
        freeswitch.consoleLog("ALERT", ">>>> input DTMF success" .. digits .. "\n")
        
        -- Store the collected numbers in variables or log them so that agents can know them.
        session:setVariable("customer_entered_digits", digits)

        -- [Successful completion]
        -- Add your own scenario here
        if b_uuid then
          -- Plug directly into only the B-Leg output (Write).
          api:execute("uuid_displace", b_uuid .. " start common/dingdong.wav 0 w") 
        end
    else
        freeswitch.consoleLog("ALERT", ">>>> No customer input or timeout\n")
        -- [Failure termination]
        -- Add your own scenario here
        if b_uuid then
          api:execute("uuid_displace", b_uuid .. " start common/dingdong.wav 0 w") 
        end
    end
    freeswitch.consoleLog("ALERT", ">>>>  In-call IVR service END\n")
end
```

<br>

### bind_meta_app

<br>

Please pay attention to the parameters in the XML that calls the bind_meta_app application.

```xml
<action application="bind_meta_app" data="5 ab o execute_extension::90 XML features"/>
```
<br>

| Parameter | Value (Example) | Description |
| --- | --- | --- |
| **Meta_Key** | 5 | Trigger Number: The digit the user presses. It typically follows the asterisk prefix. In this case, the action triggers when *5 is pressed. |
| **Binding_Flags** | b | Detection Direction: Determines which leg's input to monitor. a: Caller, b: Callee, ab: Both sides. |
| **Listen_Leg_Flags** | o | Execution Target: Specifies which session will actually execute the application. s: Self, o: Other, b: Both. |
| **Application** | execute_extension | The name of the FreeSWITCH application to be called when the trigger occurs. |
| **Data** | 90 XML features | Arguments: The parameters passed to the application. It instructs to find and execute extension 90 within the features context of the XML dialplan. |

I used 'b' for Binding_Flags because I will be calling the IVR service from my extension. And since the actual IVR service will be served to Leg a, I used 'o' for Listen_Leg_Flags, which signifies the opposite side.

<br>

### save IVR results

<br>

In the Lua example above, the DTMF value was simply stored in the customer_entered_digits session variable.
Depending on your scenario, you can save this value to a database or send it to a FreeSWITCH custom event.

