# Realtime extension management using Database

This is a method of managing extensions using DB instead of conf/directory. freeswitch performs authentication by passing terminal information as a parameter to the lua script every time the terminal attempts to register. See https://freeswitch.org/confluence/display/FREESWITCH/Lua+FreeSWITCH+Dbh.

<br>

## Using lua script
To perform real-time authentication using Lua script, add the following to the lua.conf.xml file.

```xml
<param name="xml-handler-script" value="gen_dir_user_xml.lua"/>
<param name="xml-handler-bindings" value="directory"/>
<hook event="CUSTOM" subclass="sofia::register_failure" script="sofia_register_failure.lua"/> <!-- Handling register failure events -->
<hook event="CUSTOM" subclass="sofia::unregister" script="sofia_unregister.lua"/> <!-- Handling unregister events -->
<hook event="CUSTOM" subclass="sofia::expire" script="sofia_expire.lua"/>  <!-- Handling register expire events -->
```
<br>

## sample lua script (gen_dir_user_xml.lua)

<br>

The following is a Lua file for testing that does not use SQL. Because this example returns the authentication request number as xml, register will succeed for all requests.

<br>

```lua
-- gen_dir_user_xml.lua

freeswitch.consoleLog("notice", "Debug from gen_dir_user_xml.lua, provided params:\n" .. params:serialize() .. "\n")
 
local req_domain = params:getHeader("domain")
local req_key    = params:getHeader("key")  -- key:id
local req_user   = params:getHeader("user") -- station number

test_domain = '$${domain}'
test_id = req_user
test_pw = '1234004321'
test_user_context = 'default'
test_dial_string = '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})},${verto_contact(${dialed_user}@${dialed_domain})}'

XML_STRING =
[[<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="]] ..test_domain .. [[">
      <user id="]] ..test_id  .. [[">
        <params>
          <param name="password" value="]] ..test_pw .. [["/>
          <param name="dial-string" value="]] ..test_dial_string .. [["/>
        </params>
        <variables>
          <variable name="user_context" value="]] ..test_user_context .. [["/>
        </variables>
      </user>
    </domain>
  </section>
</document>]]

freeswitch.consoleLog("notice", "Debug from gen_dir_user_xml.lua, generated XML:\n" .. XML_STRING .. "\n")
```

<br>

## sofia events related to registration

<br>

You can receive events related to extension registration and add the necessary code. It is not strictly necessary to implement handling for these events.

<br>

| Event Subclass | Trigger Condition & Timing | Root Cause & Internal Behavior |
| :--- | :--- | :--- |
| `sofia::register_failure` | Triggered when a client's SIP registration attempt (`REGISTER`) is rejected. | • Authentication failure (invalid credentials, unknown user account)<br>• Blocked by ACL (Access Control List) or SIP profile restrictions<br>• Mismatched authentication headers or no response following a `401 Unauthorized` / `407 Proxy Authentication Required` challenge |
| `sofia::unregister` | Triggered when a client explicitly requests de-registration. | • Client shuts down or logs out and sends a `REGISTER` request with an `Expires: 0` header<br>• Explicit session termination using a `0` expiration parameter in the `Contact` header |
| `sofia::expire` | Triggered when a registration lease times out without being renewed. | • Client experiences network disconnection, power loss, or abrupt termination, failing to send a periodic `REGISTER` refresh<br>• FreeSWITCH internal timer detects registration timeout and purges the contact record from memory and the database |


<br>

```lua
-- sofia_register_failure.lua
--[[
to-user: 5000
to-host: X.X.X.X
network-ip: X.X.X.X
user-agent: Fanvil X3S 2.10.2.7008 0c383e1c1934
]]

to_user = event:getHeader("to-user")
network_ip = event:getHeader("network-ip")
user_agent = event:getHeader("user-agent")

freeswitch.consoleLog("ERR", "[sofia_register_failure.lua] " .. to_user .. ":" .. network_ip .. ":" .. user_agent .. " => delete memcache" .. "\n")
```
<br>


<br>

```lua
--sofia_unregister.lua
--[[
from-user: 5000
from-host: X.X.X.X
network-ip: X.X.X.X
user-agent: Fanvil X3S 2.10.2.7008 0c383e1c1934
]]

from_user = event:getHeader("from-user")
network_ip = event:getHeader("network-ip")
user_agent = event:getHeader("user-agent")

freeswitch.consoleLog("ERR", "[sofia_unregister.lua] " .. from_user .. ":" .. network_ip .. ":" .. user_agent .. "\n")
```
<br>


<br>

```lua
-- sofia_expire.lua
--[[
user: 51003
username: 51003
host: X.X.X.X
user-agent: Fanvil X3S 2.10.2.7008 0c383e1c1934
network-ip: X.X.X.X
]]
user = event:getHeader("user")
network_ip = event:getHeader("network-ip")
user_agent = event:getHeader("user-agent")

freeswitch.consoleLog("ERR", "[sofia_expire.lua] " .. user .. ":" .. network_ip .. ":" .. user_agent ..  "\n")
```
<br>


<br><br>

## Create sqlite3 database and table

The table must include the following columns:

* id, password (all varchars).

Test using sqlite3. Create a table in sqlite3 and enter test values as follows.


```sql
CREATE TABLE directory (
  id text  PRIMARY KEY   NOT NULL,
  password text NOT NULL
);

insert into directory values('5001', '1234004321');
insert into directory values('5002', '1234004321');
insert into directory values('5003', '1234004321');
insert into directory values('5004', '1234004321');
insert into directory values('5005', '1234004321');
```
<br><br>

## Extension management Lua file using Database
The following is a Lua script that manages extensions using the sqlite3 table created earlier.

```lua
freeswitch.consoleLog("notice", "Debug from gen_dir_user_xml.lua, provided params:\n" .. params:serialize() .. "\n")
 
local req_domain = params:getHeader("domain")
local req_key    = params:getHeader("key")  -- key:id로 넘어옴
local req_user   = params:getHeader("user") -- 내선 번호 

test_domain = '$${domain}'
test_user_context = 'default'
test_dial_string = '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})},${verto_contact(${dialed_user}@${dialed_domain})}'

local dsn = 'sqlite:///usr/local/freeswitch/db/blueswitch.db'
local dbh = freeswitch.Dbh(dsn)
if dbh:connected() == false then
  freeswitch.consoleLog("notice", "gen_dir_user_xml.lua cannot connect to database" .. dsn .. "\n")
  return
end
local my_query = string.format("select * from directory where `%s`='%s' limit 1",  req_key, req_user)
assert (dbh:query(my_query, function(u)
XML_STRING =
[[<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="]] ..req_domain .. [[">
      <user id="]] ..u.id  .. [[">
        <params>
          <param name="password" value="]] ..u.password .. [["/>
          <param name="dial-string" value="]] ..test_dial_string .. [["/>
        </params>
        <variables>
          <variable name="user_context" value="]] ..test_user_context .. [["/>
        </variables>
      </user>
    </domain>
  </section>
</document>]]

freeswitch.consoleLog("notice", "Debug from gen_dir_user_xml.lua, generated XML:\n" .. XML_STRING .. "\n")
end))
```
<br><br>

# wrapping up
Now, when a SIP register message comes from an extension phone, Freeswitch no longer searches for the xml file in conf/directory. Instead, extension phones are verified by searching in real time in the blueswitch.db database file.




