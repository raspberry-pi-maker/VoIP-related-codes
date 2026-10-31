# Realtime extension management using Database

This is a method of managing extensions using DB instead of conf/directory. freeswitch performs authentication by passing terminal information as a parameter to the lua script every time the terminal attempts to register. See https://freeswitch.org/confluence/display/FREESWITCH/Lua+FreeSWITCH+Dbh.


There are two ways to dynamically manage device registration: using Lua scripts or using mod_xml_curl. The following table compares the pros and cons of the two methods.

# First - Using lua script
=======
<br>

| Criteria | Embedded Lua Script (`xml-handler-script`) | `mod_xml_curl` |
| :--- | :--- | :--- |
| **Mechanism** | Handled internally by FreeSWITCH's embedded Lua engine, intercepting requests and returning generated XML in-process. | FreeSWITCH sends an HTTP POST request to an external web server/API and parses the XML response. |
| **<span style="color: yellow;">Latency</span>** | **<span style="color: yellow;">Extremely Low</span>**<br>Executes in-memory/locally with no network overhead. | **<span style="color: yellow;">Higher</span>**<br>Incurs network overhead, including TCP handshakes and HTTP round-trip time (RTT). |
| **<span style="color: yellow;">Scalability & Clustering</span>** | **<span style="color: yellow;">Complex</span>**<br>Requires syncing script files and local data stores across every FreeSWITCH node. | **<span style="color: yellow;">Excellent</span>**<br>Multiple FreeSWITCH instances can easily point to a centralized backend behind a load balancer (Nginx, ALB). |
| **<span style="color: yellow;">Core Process Impact</span>** | **<span style="color: yellow;">High Risk</span>**<br>Blocking I/O (e.g., slow DB queries) in Lua directly consumes FreeSWITCH worker threads. | **<span style="color: yellow;">Low Risk</span>**<br>Business logic runs in an isolated process; FreeSWITCH core threads are not directly blocked. |
| **Failure Modes & Control** | Script exceptions fail immediately; caching and resilience must be handled within Lua/local storage. | Potential single point of failure if backend goes down; mitigable via FreeSWITCH timeouts and caching configs. |
| **Development Stack** | **Lua Only**<br>Limited to the Lua ecosystem and FreeSWITCH-specific Lua APIs. | **Any Stack**<br>Freedom to use any backend language or framework (Go, Node.js, Python, Java, etc.). |
| **Infrastructure Overhead** | **Minimal**<br>Runs entirely inside the FreeSWITCH server without external services. | **Moderate to High**<br>Requires provisioning, deploying, and maintaining web/API servers and load balancers. |
| **Observability & Tooling** | Relies on FreeSWITCH logs and custom file-based logging. | Out-of-the-box support for standard APMs (Datadog, New Relic), HTTP access logs, WAFs, and rate limiting. |
| **Recommended Use Cases** | • Single-node or small-scale PBX setups<br>• Embedded systems requiring millisecond-level registration<br>• Local Redis/SQLite lookups | • Multi-node large-scale VoIP / Enterprise platforms<br>• Integration with existing web-based auth, billing, or user databases<br>• Environments requiring rapid business logic iteration |

<br><br>

# Using lua script

<br>

>>>>>>> 9e19aa3071215b547ff4e24bc06b9b969a64bd4f
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

# Using mod_xml_curl

<br>

Since mod_xml_curl utilizes an external WAS (Web Application Server), it requires an independent process—such as Apache, Nginx, or Python FastAPI—to run.

Here, I will implement a simple web server using Python's FastAPI. If you have a large number of endpoints and require a stable service, you could also deploy the web server on a separate machine, independent of the FreeSWITCH server.

However, the installation of uv and detailed instructions on how to use it are not covered here.

<br>

## Modifying xml_curl.conf.xml
Open the file conf/autoload_configs/xml_curl.conf.xml and add 'directory' to the 'bindings' section of the web server endpoint configuration.

```xml
<configuration name="xml_curl.conf" description="cURL XML Gateway">
  <bindings>
    <binding name="directory_binding">
      <!-- Web server to query for authentication information URL -->
      <!-- The `/fs/directory` part at the end can be changed to anything you like. -->
      <param name="gateway-url" value="http://your-web-server.com/fs/directory" bindings="directory"/>
      
      <!-- HTTP request method (default: post) -->
      <param name="method" value="POST"/>
      
      <!-- Timeout and retry settings (optional) -->
      <param name="timeout" value="10"/>
      <param name="disable-100-continue" value="true"/>
    </binding>
  </bindings>
</configuration>
```

<br>

## Web Server (Backend) Implementation
When an endpoint sends a REGISTER request, FreeSWITCH sends an HTTP POST request to the URL mentioned above.

**Key POST parameters sent by FreeSWITCH**

* section: directory
* tag_name: domain or user
* key_name: name or id
* key_value: Domain name (e.g., 192.168.1.100 or sip.example.com)
* user: Endpoint registration ID (e.g., 1001)
* domain: SIP domain

<br>

## XML format to be returned by the web server
The web server must query a database (or other source) and provide a response in XML format—containing the user's SIP password—with the `Content-Type: text/xml` header.

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="example.com">
      <params>
        <param name="dial-string" value="{presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(${dialed_user}@${dialed_domain})}"/>
      </params>
      <groups>
        <group name="default">
          <users>
            <user id="1001">
              <params>
                <!-- Password used for SIP REGISTER authentication -->
                <param name="password" value="secret1234"/>
                <!-- Or, when using a precomputed MD5 hash:
                <param name="a1-hash" value="[md5(user:realm:password)]"/>
                -->
                <param name="reverse-auth-user" value="1001"/>
                <param name="reverse-auth-pass" value="secret1234"/>
              </params>
              <variables>
                <variable name="user_context" value="default"/>
                <variable name="accountcode" value="1001"/>
              </variables>
            </user>
          </users>
        </group>
      </groups>
    </domain>
  </section>
</document>
```

<br>

**If no user is found or the entity is not subject to authentication:**

When there is no matching user, an empty (NotFound) XML response must be returned so that FreeSWITCH can proceed to check the local directory cache or evaluate subsequent rules.

```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="result">
    <result status="not found"/>
  </section>
</document>
```

<br>

## FastAPI 

<br>

The following is FastAPI code that grants registration if the terminal number is 2000 and the password is "Hello".

<br>

```python
from fastapi import FastAPI, Request, Response

app = FastAPI()

# 1. Generate XML for allowing endpoint registration (FreeSWITCH performs authentication by comparing it against the received password)
def build_directory_xml(user: str, domain: str, password: str) -> str:
    return f"""<?xml version="1.0" encoding="utf-8"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="{domain}">
      <params>
        <param name="dial-string" value="{{^^:sip_invite_domain=${{dialed_domain}}}}user/{user}@${{dialed_domain}}"/>
      </params>
      <groups>
        <group name="default">
          <users>
            <user id="{user}">
              <params>
                <param name="password" value="{password}"/>
              </params>
              <variables>
                <variable name="user_context" value="default"/>
              </variables>
            </user>
          </users>
        </group>
      </groups>
    </domain>
  </section>
</document>"""

# 2. XML to return upon registration failure/authentication rejection (notifies FreeSWITCH that the user does not exist/is "Not Found")
def build_not_found_xml() -> str:
    return """<?xml version="1.0" encoding="utf-8"?>
<document type="freeswitch/xml">
  <section name="directory">
    <result status="not found"/>
  </section>
</document>"""


@app.post("/fs/v1/directory")
async def directory(request: Request) -> Response:
    # mod_xml_curl sends data in the application/x-www-form-urlencoded format.
    form = await request.form()
    
    # Verify the section (validate whether it is a directory request)
    section = form.get("section")
    if section != "directory":
        return Response(content=build_not_found_xml(), media_type="application/xml")

    # Phone number (terminal number) extraction: The `user` or `sip_auth_username` parameter in FreeSWITCH
    phone = form.get("user") or form.get("sip_auth_username")
    domain = form.get("domain", "127.0.0.1")

    # Return an XML response allowing registration if the terminal number is 2000.
    if phone == "2000":
        # FreeSWITCH에 등록된 사용자 비밀번호(Hello)를 포함한 XML 응답
        xml_content = build_directory_xml(user="2000", domain=domain, password="Hello")
        return Response(content=xml_content, media_type="application/xml")

    # If the number does not match, return a "Not Found" XML response.
    return Response(content=build_not_found_xml(), media_type="application/xml")
```

<br>

### Operating Principle

<br>

**Digest Authentication:**

* Instead of the SIP endpoint transmitting its password in plain text, FreeSWITCH issues a challenge (nonce) to the endpoint.

* FreeSWITCH calls this API (`/fs/v1/directory`) to verify the password for a given endpoint (e.g., extension 2000).

* When the API returns an XML response containing `<param name="password" value="Hello"/>`, FreeSWITCH compares the hash value provided by the endpoint with a value calculated internally. If they match, it approves the registration (sending a `REGISTER 200 OK` response).

<br>

**Handling Failures:**

* If the extension number does not match or the endpoint lacks the necessary permissions, the API returns `<result status="not found"/>`. In such cases, FreeSWITCH rejects the registration by sending a `403 Forbidden` or `404 Not Found` response to the endpoint.


<br>

## run FastAPI 

<br>

After saving the above Python file as simple_regi_test.py,

```bash

uv run uvicorn simple_regi_test:app --host 0.0.0.0 --port 8099 --reload 

```


<br><br>

<br>

# Second - Using mod_xml_curl
By using `mod_xml_curl` to delegate directory bindings to a web server, you can configure FreeSWITCH to dynamically call a backend API—querying a database—whenever an endpoint sends a `REGISTER` request (authentication), and then return the appropriate XML for user authentication.


Compared to the previous Lua-based approach, the architecture and division of responsibilities are modified as follows:

* Endpoint registration/authentication (REGISTER): Dynamic XML responses are provided by a web server (via database lookup) through the `directory` binding in `mod_xml_curl`.

* Event handling (e.g., `register_failure`, `unregister`, `expire`): Since `mod_xml_curl` functions solely as an XML fetcher rather than an asynchronous event handler, it is standard practice to continue using existing mechanisms—such as Lua hooks, the Event Socket Library (ESL), or an MQTT integration module—to detect and handle these events.


<br>

## loadinf mod_xml_curl
Uncomment or add `mod_xml_curl` in `autoload_configs/modules.conf.xml`.

```xml
<load module="mod_xml_curl"/>
```
<br>

## mod_xml_curl configuration (autoload_configs/xml_curl.conf.xml)

<br>

Configure the binding so that requests for the 'directory' section are forwarded to a web server (HTTP API) that performs database lookups.


```xml
<configuration name="xml_curl.conf" description="cURL XML Gateway">
<bindings>
<binding name="directory_binding">
<!-- Specify the section to be processed -->
<param name="bindings" value="directory"/>
<!-- Web API URL to receive POST requests from FreeSWITCH -->
<param name="gateway-url" value="http://127.0.0.1:8080/fs/directory" method="POST"/>
<!-- Timeout and retry settings (to prevent network latency issues) -->
<param name="timeout" value="2"/>
<param name="disable-100-continue" value="true"/>
</binding>
</bindings>
</configuration>
```

<br>

## Parameters Sent by FreeSWITCH to the Web Server

When an endpoint attempts a REGISTER request, FreeSWITCH sends an `application/x-www-form-urlencoded` POST request to the configured `gateway-url`. The primary parameters included in this request are as follows:

* section: directory
* tag_name: domain
* key_name: name
* key_value: SIP domain or IP address (e.g., 192.168.0.10)
* user: The SIP user number attempting to register (e.g., 1001)
* domain: SIP domain

<br>


## Web Backend Implementation (Database Query and XML Response)

Based on the provided `user` and `domain` values, the web server queries a database (such as MySQL or PostgreSQL) and returns an XML-formatted directory containing passwords and parameters.


```python
from fastapi import FastAPI, Form, Response
import pymysql

app = FastAPI()

def get_user_from_db(user_id: str, domain: str):
    # 실제 환경에서는 커넥션 풀을 사용하세요.
    conn = pymysql.connect(
        host="localhost", user="fs_user", password="password", db="freeswitch", charset="utf8mb4"
    )
    with conn.cursor() as cursor:
        cursor.execute(
            "SELECT password FROM sip_users WHERE username = %s AND domain = %s AND status = 'active'",
            (user_id, domain)
        )
        result = cursor.fetchone()
    conn.close()
    return result[0] if result else None

@app.post("/fs/directory")
async def handle_fs_directory(
    section: str = Form(...),
    tag_name: str = Form(None),
    key_name: str = Form(None),
    key_value: str = Form(None),
    user: str = Form(None),
    domain: str = Form(None)
):
    if section != "directory" or not user:
        return Response(content=empty_not_found_xml(), media_type="text/xml")

    # DB에서 비밀번호 조회
    password = get_user_from_db(user, domain or key_value)

    if not password:
        # If the user does not exist in the DB or is in an inactive state, return a "Not Found" XML.
        return Response(content=empty_not_found_xml(), media_type="text/xml")

    # Generate XML for successful authentication
    xml_content = f"""<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="{domain or key_value}">
      <params>
        <param name="dial-string" value="{{^^:sip_invite_domain=${{dialed_domain}}}}user/${{dialed_user}}@${{dialed_domain}}"/>
      </params>
      <users>
        <user id="{user}">
          <params>
            <param name="password" value="{password}"/>
            <param name="vm-password" value="{password}"/>
          </params>
          <variables>
            <variable name="accountcode" value="{user}"/>
            <variable name="user_context" value="default"/>
            <variable name="effective_caller_id_name" value="Extension {user}"/>
            <variable name="effective_caller_id_number" value="{user}"/>
          </variables>
        </user>
      </users>
    </domain>
  </section>
</document>"""

    return Response(content=xml_content, media_type="text/xml")

def empty_not_found_xml():
    return """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="result">
    <result status="not found"/>
  </section>
</document>"""
```

<br>

## Existing Hook Event Handling

Since `sofia::register_failure`, `sofia::unregister`, and `sofia::expire` are registered as events, they cannot be handled by `mod_xml_curl`, which functions as an XML generator.

* Retaining Lua Hooks: You can continue to use Lua hooks as defined in `lua.conf.xml` without any conflicts, while delegating only the provision of authentication XML to `mod_xml_curl`.

* Transitioning to ESL (Event Socket Library): If you intend to consolidate all business logic into a central server, a more effective approach is to connect a Python or Node.js backend daemon to the FreeSWITCH Event Socket (inbound/outbound). This allows you to subscribe to specific custom events—such as `sofia::register_failure`, `sofia::unregister`, and `sofia::expire`—and handle database operations asynchronously.

<br><br>

# wrapping up
Now, when a SIP register message comes from an extension phone, Freeswitch no longer searches for the xml file in conf/directory. Instead, extension phones are verified by searching in real time in the blueswitch.db database file.




