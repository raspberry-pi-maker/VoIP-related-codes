# Under the hood of mod_callcenter

The three most important elements in mod_callcenter are agent, tier, and queue.
These three elements have variables that greatly affect the operation of mod_callcenter. Let's see how these variables affect the operation of mod_callcenter.
<br><br>

# Agent

## __type__
* __callback__ :  try to reach the agent via the contact fields value. contact is phone information used by the counselor.
* __uuid-standby__ : try to bridge the call directly using the agent uuid. __Do not use unless you have special intention__

<br>

## __contact__
Extension information to be used by the agent. Since the agent uses an extension phone connected to FreeSWITCH, you can use the "user/extension number" format.You can add the call_timeout property value as follows.

```xml
[call_timeout=60]user/1001
```
* __call_timeout__ : Maximum time for the phone to ring. After this time, FreeSWITCH terminates the phone connection attempt. Therefore, if the agent does not answer the phone during this time, it is counted as an not answered call.


<br>

## __status__
One of the following values: __“Logged Out, Available, Available (On Demand), On Break”__. Only "Available", "Available (On Demand)" status can receive calls.

<br>

## __state__
One of the following values: __“Idle, Waiting, Receiving, In a queue call”__. Only "Waiting" state can receive calls.
When a call comes in through the queue and the agent's phone rings, the agent's state automatically changes to "Receiving." When you receive a call, it returns to the "In a queue call" state, and if you hang up or do not answer the phone, it returns to the "Waiting" state.


<br>

## __max-no-answer__
If agent fails to answer in this number of times, then the agent's Status will changed to 'On Break'. 
Generally, __this value is used as 1 to 3__. It is inappropriate for a call center agent to not answer a call in “Available/Waiting” status/state. If the agent is unable to answer the phone, the status should be changed to “On Break.”

<br>

## __wrap-up-time__
Waiting time to return to Available/Waiting after the inbound call ends. Generally, it is used to allow time needed for post-processing work like documenting phone call contents.
__During this time after the agent's call ends, the agent's status/state is "Available/Waiting." However, mod_callcenter does not attempt to connect inbound calls to the agent within this time__.

<br>

## __reject_delay_time__
If the agent presses the reject button on her phone, wait this defined time amount. In general, reject on most phones is the same as DND, which will be explained later. Therefore, this value can be set the same as the busy_delay_time value.

<br>

## __busy_delay_time__
If the agent is on Do Not Disturb, wait this defined time before trying him again. The DND (Do Not Disturb) button may not be present depending on the phone vendor. When you press the DND button, the phone sends a 486 "Busy Here" signal to FreeSWITCH.

<br>

## __no-answer-delay-time__
If the agent does not answer the call, wait this defined time before trying him again. The maximum time that the agent's phone can ring can be set in call_timeout of contact.

<br><br>


# Queue

## __Call Distribution Strategy__
The strategy defines how calls are distributed in a queue. 

|value|Descriptiondescription|
|------|---|
|__round-robin__|Rings the agent in position but remember last tried agent. It is generally the most used and recommended.|
|__longest-idle-agent__|Rings the agent who has been idle the longest taking into account tier level. It is generally the most used and recommended.|
|top-down|Rings the agent in order position starting from 1 for every member.|
|agent-with-least-talk-time|LRings the agent with least talk time. There is a disadvantage in that agents who start work late are focused on the call. This can only be used if all agents always start and end work at the same time.|
|agent-with-fewest-calls|Rings the agent with fewest calls. There is a disadvantage in that agents who start work late are focused on the call. This can only be used if all agents always start and end work at the same time.|
|~~ring-all~~|Rings all agents simultaneously. __Do not use unless you have special intention__ |
|~~random~~|Rings agents in random order. __Do not use unless you have special intention__ |
|~~ring-progressively~~|Rings agents in the same way as top-down, but keeping the previous members ringing. __Do not use unless you have special intention__ |
<br>

## __moh__
moh is an abbreviation for Music on Hold. This is a sound source that is played to the customer until the call that enters the queue is connected to an agent. Since mod_callcenter first starts playing moh and then looks up the appropriate agent for the strategy, moh will be played for a ring time until the available agent answers. It is not appropriate to insert a voice guidance in moh. This is because moh stops the moment the agent receives it. If you need guidance, use the playback application before entering the queue in dialplan. moh generally uses music files of appropriate length. If the file is short, it is played repeatedly.

<br>

## __time-base-score__
Every phone call that enters the queue is assigned a score. A call will be connected to a counselor in this score order. There are two ways to award points. system and queue.

* __system__ : Scores are calculated from the moment the call is connected. Therefore, if a call has gone through a long IVR service before entering the queue, it may be given a higher score than other phone calls already in the queue.

* __queue__ : Scores are awarded based on the time the call enters the queue. Therefore, you will be connected to an agent in the order in which you entered the queue. 

Generally, the queue option is used more.

<br>

## __max_wait_time__
Set the maximum time to wait in the queue. If this value is 0, The caller waits in the queue until an agent answers the call. If this value is non-zero, calls that have elapsed this time are taken out of the queue. Therefore, the callcenter application is terminated in the dial plan and proceeds to the next dial plan. The following dial plan is an example of playing an announcement message to a call that leaves the queue after the waiting time has elapsed and ending the service. If you want a callback scenario, you can create and connect an IVR callback scenario. __If not defined, default value is 0__.

```xml
<extension name="callcenter-example">
    <condition field="destination_number" expression="^07070067000$">
        <!-- play a message before entering the queue. -->
        <action application="playback" data="ivr/ivr-welcome.wav"/>
        <action application="playback" data="ivr/ivr-one_moment_please.wav"/>
        <!-- hangup after successful bridge to agent -->
        <action application="set" data="hangup_after_bridge=true"/>
        <!-- queue caller -->
        <action application="set" data="result=${luarun(callcenter-announce-position.lua ${uuid} example@default 10000)}"/>
        <action application="callcenter" data="example@default"/>
        <!-- if no agent was reached and using max-wait-time - play wav file and hangup -->
        <action application="playback" data="ivr/ivr-please_call_again_later.wav"/>
        <action application="hangup"/>
    </condition>
</extension>

```

<br>

## __max-wait-time-with-no-agent__
Set the maximum time to wait in the queue when there are no Available/Waiting agents. If this value is 0, the caller  waits in the queue until an agent log in and answers the call. If this value is non-zero and there are no available agents, calls that have elapsed this time are taken out of the queue. Therefore, the callcenter application is terminated in the dial plan and proceeds to the next dial plan. The following dial plan is an example of playing an announcement message to a call that leaves the queue after the waiting time has elapsed and ending the service. If you want a callback scenario, you can create and connect an IVR callback scenario same as above.


<br>

## __tier-rules-apply__
Can be True or False. This defines if we should apply the following tier rules when a caller advances through a queue's tiers. If False, they will use all tiers with no wait.
*__Tier-related settings are used when using two or more queues. If agents log in to one queue, set this value to False.__*

* __tier-rule-wait-second__ : The time in seconds that a caller is required to wait before advancing to the next tier. This will be multiplied by the tier level if tier-rule-wait-multiply-level is set to True. If tier-rule-wait-multiply-level is set to false, then after tier-rule-wait-second's have passed, all tiers are open for calls in the tier-order and no advancement (in terms of waiting) to another tier is made.

* __tier-rule-wait-multiply-level__ : Can be True or False. If False, then once tier-rule-wait-second is passed, the caller is offered to all tiers in order (level/position). If True, the __tier-rule-wait-second__ will be multiplied by the tier level and the caller will have to wait on every tier __tier-rule-wait-second__'s before advancing to the next tier.

* __tier-rule-no-agent-no-wait__ : Can be True or False. If True, callers will skip tiers that don't have agents available. Otherwise, they are be required to wait before advancing. Agents must be logged off to be considered not available.


<br>

## __discard-abandoned-after__
The number of seconds before we completely remove an abandoned member(call) from the queue. When used in conjunction with abandoned-resume-allowed, callers can come back into a queue and resume their previous position.

<br>

## __abandoned-resume-allowed__
The number of seconds before we completely remove an abandoned member from the queue. When used in Can be True or False. If True, a caller who has abandoned the queue can re-enter and resume their previous position in that queue. In order to maintain their position in the queue, they must not abandoned it for longer than the number of seconds defined in 'discard-abandoned-after'.

