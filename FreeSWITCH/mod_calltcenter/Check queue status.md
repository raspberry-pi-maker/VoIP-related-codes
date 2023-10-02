# Check Queue Status

## __Prerequisite__
* [__How to use mod_callcenter__](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/Freeswitch%20mod_calltcenter/How%20to%20use%20mod_callcenter.md)
* [__Dynamic usage of mod_callcenter__](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/Freeswitch%20mod_calltcenter/Dynamic%20usage%20of%20mod_callcenter.md)
* [__Under the hood of mod_callcenter__](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/Freeswitch%20mod_calltcenter/Dynamic%20usage%20of%20mod_callcenter.md)

<br><br>
To check the status of the queue, I used the following ESL command.
```bash
freeswitch@blueivr> callcenter_config queue list
name|strategy|moh_sound|time_base_score|tier_rules_apply|tier_rule_wait_second|tier_rule_wait_multiply_level|tier_rule_no_agent_no_wait|discard_abandoned_after|abandoned_resume_allowed|max_wait_time|max_wait_time_with_no_agent|max_wait_time_with_no_agent_time_reached|record_template|calls_answered|calls_abandoned|ring_progressively_delay|skip_agents_with_external_calls|agent_no_answer_status
3000@default|round-robin|/usr/local/freeswitch/sounds/ivr/common/music-on-hold.wav|system|false|300|true|false|60|false|0|0|20||0|0|0|true|On Break
+OK
```
However, with this API, you can know the various property values of the queue, but you cannot know information such as the number of calls currently in the queue, the number connected to the agent, and the number of calls waiting. In a typical call center, the ability to monitor this queue information in real time is essential.

There is “__callcenter_config member list__” in the API list of the [mod_callcenter](https://developer.signalwire.com/freeswitch/FreeSWITCH-Explained/Modules/mod_callcenter_1049389/#listing-members) document, but it does not work when actually tested. And even if you check in the mod_callcenter.c file, this API does not exist, until FreeSWITCH version 10.6.
To accurately determine the status of calls that have entered the queue, the only way is to check the database directly. It is also possible to add an API by modifying the mod_callcenter.c file.

<br><br>

# Database query to catch queue information

<br>
Since I have not enabled a relational database in the conf/callcenter.conf.xml file, FreeSWITCH will use the sqlite3 database. Therefore, the callcenter.db file in the db directory will be used as the database.

<br>

```bash
[root@tmoney_pbx scripts]# sqlite3 /usr/local/freeswitch/db/callcenter.db
sqlite> .headers ON
sqlite> .tables
agents   members  tiers 
sqlite> select * from members;
sqlite> 
```
<br>

You can see that there are three tables: “agents”, “members”, and “tiers”. Among these, there is member information queued in the members table. However, since there are no calls currently in the queue, there is no query result.
<br><br>


## Database query with queue call

Therefore, to properly test, it must be done while a call has entered the queue. This time, we will try to add a call to the queue without a Logon agent present. Since this call probably has no agent to connect to, it will wait for the time specified in the queue settings and then exit the queue and be treated as an abandoned call.

```bash
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|056fe8bc-7b8e-4aac-a94d-5f9297c1fd44|634a22e2-84de-48f7-bdcf-78577a0097ce|01058400592|07070067000|1696211979|1696211979|0|0|0|0|0|||Waiting
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|056fe8bc-7b8e-4aac-a94d-5f9297c1fd44|634a22e2-84de-48f7-bdcf-78577a0097ce|01058400592|07070067000|1696211979|1696211979|0|0|0|0|0|||Waiting
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|056fe8bc-7b8e-4aac-a94d-5f9297c1fd44||01058400592|07070067000|1696211979|1696211979|0|0|1696211990|0|0|||Abandoned
```
<br>

The first two queries are the query results while moh is heard. While waiting because there is no agent to connect, the member state has the value "Waiting".
And the last query is a query with the call terminated. Because the call has ended, there are no calls in the queue. However, in order to inform you of the abandoned call, the abandoned call information is maintained and displayed for a few seconds. The state of this state is "Abandoned". This query information disappears after a while as follows:

<br>

```bash
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|056fe8bc-7b8e-4aac-a94d-5f9297c1fd44||01058400592|07070067000|1696211979|1696211979|0|0|1696211990|0|0|||Abandoned
sqlite> select * from members;
sqlite> 
```
<br>

## member's state 

The state values that a member can have are as follows.

|state|description|
|------|---|
|Unknown|Unknown state
|Waiting|Waiting in queue because no agent is available
|Trying|The call is connecting to an agent. The agent's phone is ringing.
|Answered|Successfully connected to an agent and the number currently being consulted
|Abandoned|Calls that fail to connect to an agent and are disconnected. This value disappears after a while. Both calls that timeout according to the queue setting value and calls that a member terminates on their own while waiting in the queue are included.

<br>
The following is the result of entering a call into the queue while the agent is logged in. You can see that the state is "Trying" while the agent's phone is ringing, and changes to "Answered" after the agent answers the call.
And when the call ends, it immediately disappears from the members list.

<br>

```bash
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|b74fd35f-e67a-46da-adb3-e422cad01821|296f0f7f-5f37-4a11-ad5d-51caa5793d56|01058400592|07070067000|1696215989|1696215989|0|0|0|0|0|1001|single_box|Trying
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|b74fd35f-e67a-46da-adb3-e422cad01821|296f0f7f-5f37-4a11-ad5d-51caa5793d56|01058400592|07070067000|1696215989|1696215989|0|1696215997|0|0|0|1001|single_box|Answered
sqlite> select * from members;
queue|instance_id|uuid|session_uuid|cid_number|cid_name|system_epoch|joined_epoch|rejoined_epoch|bridge_epoch|abandoned_epoch|base_score|skill_score|serving_agent|serving_system|state
07070067000@default|single_box|b74fd35f-e67a-46da-adb3-e422cad01821|296f0f7f-5f37-4a11-ad5d-51caa5793d56|01058400592|07070067000|1696215989|1696215989|0|1696215997|0|0|0|1001|single_box|Answered
sqlite> select * from members;
```

<br>

Many column values in this table are values managed by mod_callcenter. The values we are interested in are:

|column|description|
|------|---|
|queue|Currently entered queue name
|cid_number|Member's phone number. Corresponds to ANI value
|system_epoch|Timestamp when the call was connected to FreeSWITCH
|joined_epoch|Timestamp when the call was connected to current queue
|bridge_epoch|Timestamp when the agent was connected
|abandoned_epoch|Abandoned timestamp
|serving_agent|Agent serving customers

<br>
Using the above values, we can know all the queue information we need.


<br><br>


## Some SQL queries


<br>

|SQL|description|
|------|---|
|select count(*) from members where queue = '07070067000@default' and state = 'Waiting'; |Check the number of waiting calls in the 07070067000@default queue.
|select count(*) from members where queue = '07070067000@default' and state = 'Answered';|Check the number of connected calls in the 07070067000@default queue.
