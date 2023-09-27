# Dynamic usage of mod_callcenter


In the article [How to use mod_callcenter.md](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/Freeswitch%20mod_calltcenter/How%20to%20use%20mod_callcenter.md), I explained how to use mod_callcenter using the xml file in the conf directory.
In this article, I will minimize the use of xml and learn how to register, delete, and change the status of agents in real time. <br/><br/>


# Modifying XML files to minimize XML usage

The mod_callcenter configuration file is conf/autoload_configs/callcenter.conf.xml. This file consists of three major elements.


* queues : records queue information to be used in mod_callcenter. __Queue information cannot be dynamically managed, such as creation or deletion. Therefore, you must create information in the xml file.__
* agents : records agent information to be used in mod_callcenter. Agent information can be dynamically managed. Therefore, delete all information from the xml file.
* tiers  : records tier information to be used in mod_callcenter. Tier information can be dynamically managed. Therefore, delete all information from the xml file.
<br/><br/>

``` xml
<configuration name="callcenter.conf" description="CallCenter">
  <settings>
  </settings>

    <queues>
        <queue name="3000@default">
            <param name="strategy" value="round-robin"/>
            <param name="moh-sound" value="$${sounds_dir}/ivr/common/music-on-hold.wav"/>
            <param name="time-base-score" value="system"/>
            <param name="max-wait-time" value="0"/>
            <param name="max-wait-time-with-no-agent" value="0"/>
            <param name="max-wait-time-with-no-agent-time-reached" value="20"/>
            <param name="tier-rules-apply" value="false"/>
            <param name="tier-rule-wait-second" value="300"/>
            <param name="tier-rule-wait-multiply-level" value="true"/>
            <param name="tier-rule-no-agent-no-wait" value="false"/>
            <param name="discard-abandoned-after" value="60"/>
            <param name="abandoned-resume-allowed" value="false"/>
        </queue>
    </queues>
    
    <agents>
    </agents>    
    
    <tiers>
    </tiers>
</configuration> 

```

Now restart the Freeswitch process. <br/><br/>

# API Commands

The following commands will be used frequently in the future. Check the usage instructions in advance. <br/><br/>

## Important Agent APIs

* __callcenter_conifg agent list__ : Displays a list of agents currently registered with mod_callcenter.

* __callcenter_conifg agent del [agent name]__ : delete agent from mod_callcenter. Agent information is also deleted from the agents table in the callcenter DB.

* __callcenter_conifg agent add [agent name] [callaback or uuid-standby]__ : Register an agent with the current mod_callcenter. The agent type has one of the following values: callback or uuid-standby. 

* __callcenter_conifg agent get status [agent name]__ : Gets the status value of the agent. If the agent type is uuid-standby, you can additionally query the uuid value. It can be used like this: _"callcenter_conifg agent get uuid 1001"_

* __callcenter_conifg agent set [key(contact|status|state|type|max_no_answer|wrap_up_time|ready_time|reject_delay_time|busy_delay_time)][agent name] [value]__ : Agents registered with the agent add command do not have any separately specified properties except type. The agent has the following attribute values, and these values must be specified as needed.
Enter the property value corresponding to the key as the value value.

<br>

|key|values|description|
|------|---|---|
|contact|user/1000@default|Extension information to be used by the agent|
|status|Logged Out, Available, Available (On Demand), On Break|When you first log in, you start at status (Available) and state (Waiting).|
|state|Idle, Waiting, Receiving, In a queue call|Unless there are special cases, these values do not need to be changed. mod_callcenter maintains the appropriate state value.|
|type|callaback, uuid-standby|callback will try to reach the agent via the contact fields value. uuid-standby will try to bridge the call directly using the agent uuid. It is okay to use the callback type.|
|max_no_answer|seconds|agent fails to answer in thistime, then the agent's Status will changed to 'On Break'.|
|wrap_up_time|seconds|Waiting time to return to Available/Waiting after the consultation call ends. Generally, it is used to allow time needed for post-processing work.|
|ready_time|테스트2|테스트3|
|reject_delay_time|seconds|If the agent presses the reject button on her phone, wait this defined time amount.|
|busy_delay_time|seconds|If the agent is on Do Not Disturb, wait this defined time before trying him again.|


<br/><br/>


The agent __status__ value is one of the following values.
* __Logged Out__ 	Cannot receive queue calls.

* __Available__	Ready to receive queue calls.

* __Available (On Demand)__	State will be set to 'Idle' once the call ends (not automatically set to 'Waiting').

* __On Break__	Still Logged in, but will not receive queue calls.

<br/><br/>


## Important Tier APIs

Tier is information that connects the agent to the queue.

* __callcenter_conifg tier list__ : Displays a list of tierss currently registered with mod_callcenter.

* __callcenter_config tier add [queue name][agent name] [[level]] [[position]]__ : Add a new tier mapping an agent to a queue. If no level or position is provided, they will default to 1. I recommend using the default value of 1 for these values. Therefore, the level and position values can be omitted in this command.

* __callcenter_config tier set [key(state|level|position)][queue name] [agent name][value]__ : Update tier value.

* __callcenter_config tier del [queue name][agent name]__ : Delete a tier

<br/><br/>

## Important Queue APIs
Queues cannot be created or deleted in real time. Only queue information registered in the callcenter.conf.xml file can be used. However, load, unload, and reload are possible for queue information registered in the xml file. Therefore, you can use commands such as load and reload after modifying the xml file.

* __callcenter_conifg queue list__ : Displays a list of queues currently registered with mod_callcenter.

* __callcenter_config queue load [queue_name]__ : Load a queue settings. Use after unloading. 

* __callcenter_config queue unload [queue_name]__ : Unload a queue settings 

* __callcenter_config queue reload [queue_name] [queue_name]__ : Reload a queue settings. It is useful to use after modifying the xml file. 

* __callcenter_config queue list agents [queue_name][status] [state]__ : List agents with a tier associated to the specified queue. If [status] is specified, only list agents with a given status.

* __callcenter_config queue list members [queue_name]__ : List callers present in the queue. Members refer to callers who entered the queue.
* __callcenter_config queue list tiers [queue_name]__ : List tiers associated to a specific queue.

* __callcenter_config queue count agents [queue_name][status]__ : Return the number agents with a tier associated to the specified queue. If [status] is specified, only count agents with a given status.
* __callcenter_config queue count members [queue_name]__ : Return the number of callers present in the queue (no of callers waiting in a queue + number of callers bridged with an agent). 

<br/><br/>





# Check the mod_callcenter configuration

The Freeswitch process was restarted with all agent lists and tier lists except for queue (3000) removed. Now let’s check this information. 


## Previous information may remain.
If the agents(1001, 1002) that already exist in the agents element of the xml file are deleted, information on the previously existing agents may appear as follows. The reason for this is that mod_callcenter stores queue, agent, and tier information in the DB. I don't have ODBC enabled so the information will be stored in sqlite DB.
The db/callcenter.db file will exist. Here, the previous agent information is not erased and exists.

```bash
freeswitch@blueivr> callcenter_config agent list
name|instance_id|uuid|type|contact|status|state|max_no_answer|wrap_up_time|reject_delay_time|busy_delay_time|no_answer_delay_time|last_bridge_start|last_bridge_end|last_offered_call|last_status_change|no_answer_count|calls_answered|talk_time|ready_time|external_calls_count
1001|single_box||callback|[call_timeout=60]user/1001|Logged Out|Waiting|3|10|30|60|10|0|0|0|1695639576|0|0|0|0|0
1002|single_box||callback|[call_timeout=60]user/1002|Logged Out|Waiting|3|10|30|60|10|0|0|0|1695639576|0|0|0|0|0
+OK
```

If previous information remains, it can be deleted with the callcenter_config agent del command.

```bash
freeswitch@blueivr> callcenter_config agent del 1001
+OK

2023-09-25 20:03:59.784449 [DEBUG] mod_callcenter.c:936 Deleted Agent 1001
freeswitch@blueivr> callcenter_config agent del 1002
+OK

2023-09-25 20:04:02.665037 [DEBUG] mod_callcenter.c:936 Deleted Agent 1002

freeswitch@blueivr> callcenter_config agent list
+OK
```
<br>
However, if you use the following settings, the agent information is deleted every time the mod_callcenter module starts. 

```xml
<configuration name="callcenter.conf" description="CallCenter">
  <settings>
  <param name="truncate-agents-on-load" value="true"/>
  </settings>
</configuration> 
```
<br>
Similarly, to reset tier information every time the mod_callcenter module starts, add the following.

```xml
<configuration name="callcenter.conf" description="CallCenter">
  <settings>
  <param name="truncate-tiers-on-load" value="true"/>
  </settings>
</configuration> 
```
<br>
As for the queue information, you can see that the "3000" queue created in the xml file was loaded by mod_callcenter.

```bash
freeswitch@blueivr> callcenter_config queue list
name|strategy|moh_sound|time_base_score|tier_rules_apply|tier_rule_wait_second|tier_rule_wait_multiply_level|tier_rule_no_agent_no_wait|discard_abandoned_after|abandoned_resume_allowed|max_wait_time|max_wait_time_with_no_agent|max_wait_time_with_no_agent_time_reached|record_template|calls_answered|calls_abandoned|ring_progressively_delay|skip_agents_with_external_calls|agent_no_answer_status
3000@default|round-robin|/usr/local/freeswitch/sounds/ivr/common/music-on-hold.wav|system|false|300|true|false|60|false|0|0|20||0|0|0|true|On Break
+OK
```
<br><br>

# Agent dynamic management
Now, I will use the esl command to dynamically register agents, create tiers, and connect them to the queue.

## Add Agent
Register agents with the “callcenter_config agent add” command and check with the “callcenter_config agent list” command.

```bash
freeswitch@blueivr> callcenter_config agent add 1001 callback
+OK

2023-09-26 00:25:51.285197 [DEBUG] mod_callcenter.c:908 Adding Agent 1001 with type callback with default status Logged Out
freeswitch@blueivr> callcenter_config agent list
name|instance_id|uuid|type|contact|status|state|max_no_answer|wrap_up_time|reject_delay_time|busy_delay_time|no_answer_delay_time|last_bridge_start|last_bridge_end|last_offered_call|last_status_change|no_answer_count|calls_answered|talk_time|ready_time|external_calls_count
1001|single_box||callback||Logged Out|Waiting|0|0|0|0|0|0|0|0|0|0|0|0|0|0
+OK
```
<br>

However, the agent does not yet have the most important contact information. Contact information must be updated. You can also set information such as busy_delay_time, no_answer_delay_time, ready_time, and reject_delay_time.



```bash
freeswitch@blueivr> callcenter_config agent set reject_delay_time 1001 30
+OK

2023-09-26 00:43:42.004461 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set reject_delay_time = 30
freeswitch@blueivr> callcenter_config agent set busy

[     busy_delay_time]	


freeswitch@blueivr> callcenter_config agent set busy_delay_time 1001 60
+OK

2023-09-26 00:44:00.924977 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set busy_delay_time = 60
freeswitch@blueivr> callcenter_config agent set no

[no_answer_delay_time]	


freeswitch@blueivr> callcenter_config agent set no_answer_delay_time 1001 10
+OK

2023-09-26 00:44:10.645067 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set no_answer_delay_time = 10

freeswitch@blueivr> callcenter_config agent set contact 1001  "[call_timeout=60]user/1001"
+OK

2023-09-26 00:49:05.784430 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set contact = "[call_timeout=60]user/1001"

freeswitch@blueivr> callcenter_config agent set wrap_up_time 1001 10
+OK

2023-09-26 00:55:39.605134 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set wrap_up_time = 10
freeswitch@blueivr> callcenter_config agent list
name|instance_id|uuid|type|contact|status|state|max_no_answer|wrap_up_time|reject_delay_time|busy_delay_time|no_answer_delay_time|last_bridge_start|last_bridge_end|last_offered_call|last_status_change|no_answer_count|calls_answered|talk_time|ready_time|external_calls_count
1001|single_box||callback|"[call_timeout=60]user/1001"|Logged Out|Waiting|0|10|30|60|10|0|0|0|0|0|0|0|0|0
+OK

```
<br><br>

## Add Tier
Since we have registered the agent, now is the time to create a tier to connect the agent and the queue. 

```bash
freeswitch@blueivr> callcenter_config tier add 3000@default 1001
+OK

2023-09-26 00:57:58.704916 [DEBUG] mod_callcenter.c:1221 Adding Tier on Queue 3000@default for Agent 1001, level 1, position 1

freeswitch@blueivr> callcenter_config tier list
queue|agent|state|level|position
3000@default|1001|Ready|1|1
+OK
```

Now, real-time agent registration has been completed. To test, change the agent's status to Available/Waiting.

<br><br>

# Test

After changing the agent's status, the call is placed in the 3000@default queue.

```bash
freeswitch@blueivr> callcenter_config agent set status 1001 Available
+OK

2023-09-26 01:03:30.665176 [DEBUG] mod_callcenter.c:1180 Updated Agent 1001 set status = Available
freeswitch@blueivr> callcenter_config agent list 1001
name|instance_id|uuid|type|contact|status|state|max_no_answer|wrap_up_time|reject_delay_time|busy_delay_time|no_answer_delay_time|last_bridge_start|last_bridge_end|last_offered_call|last_status_change|no_answer_count|calls_answered|talk_time|ready_time|external_calls_count
1001|single_box||callback|"[call_timeout=60]user/1001"|Available|Waiting|0|10|30|60|10|0|0|0|1695657810|0|0|0|0|0
+OK

```

The call will be routed to the 1001 extension that the 1001 agent is using.
