# Tier

<br/>

A "Tier" serves as the link mapping agents to queues and is a core concept that determines the priority (level) and sequence (position) of call distribution.

<br/>

## Key Tier Attributes

<br/>

**level (priority group; default: 1):**

* A lower number indicates higher priority.

* When a call enters the queue, the system generally exhausts the Level 1 agent pool first before passing the call to Level 2 and then Level 3 (escalation).

**position (order within the group; default: 1):**

* Determines the sequence in which calls are offered to agents within the same level.

* This directly influences strategies such as "top-down" or "sequentially-by-agent-order."

<br/>

## Tier Assignment

Tiers link an agent to a queue and define the agent's priority within that queue. Agents with a lower level number are offered calls before those with a higher number. Within the same level, position determines order.

Tiers are defined under the <tiers> element of callcenter.conf.xml.

```xml
<tiers>
  <tier agent="1000@default" queue="support@default" level="1" position="1"/>
  <tier agent="1001@default" queue="support@default" level="2" position="1"/>
</tiers>
```
If level or position is omitted, it defaults to 1. When tier configuration is applied on restart, supplied values overwrite any runtime changes to level and position. Omitting both attributes preserves existing database values.

<br/>

### Tier Parameter Reference

| Parameter | Purpose | Accepted Values | Default |
| :--- | :--- | :--- | :--- |
| `agent` | Agent name, matching an entry in `<agents>`. | String | Required |
| `queue` | Queue name, matching an entry in `<queues>`. | String | Required |
| `level` | Priority group. Lower numbers are exhausted before higher numbers are tried (subject to `tier-rules-apply`). | Integer >= 1 | 1 |
| `position` | Order within a level when strategies that use position (for example, `top-down`, `sequentially-by-agent-order`) are active. | Integer >= 1 | 1 |
------

<br/>

📌 **Tier rules and escalation: When tier-rules-apply is false (the default), callers are offered to all tiers simultaneously without delay. When tier-rules-apply is true, callers start at level 1 and advance to the next level only after waiting tier-rule-wait-second seconds (multiplied by the level number if tier-rule-wait-multiply-level is true). If no agents exist in the current tier and tier-rule-no-agent-no-wait is true, the caller advances immediately.**

📌 **The `tier-rules-apply` value is specified in the `<queue>` configuration block within the `callcenter.conf.xml` file.**

<br/>

## Useful Scenarios

<br/>

###  Configuration example: A single agent handling calls from two queues

<br/>

Agent 1001 can be configured to primarily handle the "Support" queue while providing backup for the "Sales" queue during high call volumes, or to handle both queues with equal priority.

<br/>

### Scenario A: Primary task (Support) priority + Secondary task (Sales) backup

<br/>

* Support queue: Queued at priority level 1.

* Sales queue: Queued at priority level 2 (calls are routed here when no dedicated Sales agents are available or when they are busy on other calls).

```xml
<!-- callcenter.conf.xml -->
<configuration name="callcenter.conf" description="CallCenter">
  <queues>
    <!-- support queue -->
    <queue name="support@default">
      <param name="strategy" value="longest-idle-agent"/>
      <param name="tier-rules-apply" value="true"/>
      <param name="tier-rule-wait-second" value="15"/>
      <param name="tier-rule-no-agent-no-wait" value="true"/>
    </queue>

    <!-- Sales Queue (Escalates to Level 2 if a Level 1 agent does not answer within 15 seconds) -->
    <queue name="sales@default">
      <param name="strategy" value="longest-idle-agent"/>
      <!-- Tier rule activation: The call moves to the next level only after the specified wait time has elapsed. -->
      <param name="tier-rules-apply" value="true"/>
      <!-- If a Level 1 agent is unavailable or the call is not connected within 15 seconds, the call is transferred to a Level 2 agent. -->
      <param name="tier-rule-wait-second" value="15"/>
      <!-- If all Level 1 agents are logged out or on a call, the call is immediately passed to the next level without a wait time. -->
      <param name="tier-rule-no-agent-no-wait" value="true"/>
    </queue>
  </queues>

  <agents>
    <agent name="1001@default" type="callback" contact="[leg_timeout=15]user/1001@default" status="Available" />
  </agents>

  <tiers>
    <!-- 1001 agents are given top priority (level 1) in the support queue. -->
    <tier queue="support@default" agent="1001@default" level="1" position="1"/>

    <!-- 1001 agents stand by as backups (level 2) in the sales queue. -->
    <tier queue="sales@default" agent="1001@default" level="2" position="1"/>
  </tiers>
</configuration>
```

<br/>

### Scenario B: Receiving from two queues with equal priority (same level)

<br/>

If both queues are assigned `level="1"`, the call is immediately distributed to an agent from whichever queue generated the waiting call first (or based on the queue's scoring method).


```xml
<tiers>
  <tier queue="support@default" agent="1001@default" level="1" position="1"/>
  <tier queue="sales@default"   agent="1001@default" level="1" position="1"/>
</tiers>
```

<br/>

## Essential Queue Parameters for Multi-Queue Operation (tier-rule-*)

When operating a tiered structure by grouping multiple queues, the tier rule parameters within the <queue> element are crucial.


**tier-rules-apply (true / false):**

* false (default): Calls can be distributed simultaneously to agents across all tiers, regardless of their level.

* true: The system searches sequentially starting from Level 1, moving to the next level only after the specified wait time has elapsed.


**tier-rule-wait-second (in seconds):**

* The duration to wait at the current level before escalating the call to the next level.


**tier-rule-no-agent-no-wait (true / false):**

* true (recommended): If there are no agents at the current level, or if all agents are busy or offline, the call is immediately routed to agents at the next level without waiting for the timer to expire.

**tier-rule-wait-multiply-level (true / false):**

* true: The total wait time to reach Level N increases to $N \times \text{tier-rule-wait-second}$.

<br/>

## Runtime Control (CLI / ESL)

<br/>

You can dynamically create, delete, and query tier relationships via the CLI (fs_cli) or ESL without restarting FreeSWITCH or re-parsing the XML.

```bash
# Register agent 1001 at level 2, position 1 of the sales@default queue.
callcenter_config tier add sales@default 1001@default 2 1

# Changing a specific agent's tier level (e.g., promotion to Level 1)
callcenter_config tier set level sales@default 1001@default 1

# Disconnect agent tier from a specific queue
callcenter_config tier del sales@default 1001@default

# View the full list of registered tiers
callcenter_config tier list
```

