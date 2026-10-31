# Gateway Overview

<br>

A gateway is a configuration used to connect to external SIP servers (carriers, ITSPs, SIP trunks, other PBXs, etc.).

* sip_profiles/internal.xml → For endpoint registration (IP Phone, Softphone)
* sip_profiles/external.xml → For connecting to external SIP providers
* gateway → An object used to register with a specific SIP server or handle call routing

<br>

Sample

<br>

```xml
<gateway name="my_provider">
    <param name="username" value="1001"/>
    <param name="password" value="pass123"/>
    <param name="realm" value="sip.provider.com"/>
    <param name="proxy" value="sip.provider.com"/>
    <param name="register" value="true"/>
</gateway>
```

<br>

## Key Parameters

<br>

#### name

<br>

The gateway name, used in the Dialplan as follows:

```xml
<gateway name="my_provider">
```

<br>

```xml
sofia/gateway/my_provider/01012345678
```

<br>

#### username, password

<br>

SIP authentication credentials used during SIP REGISTER and INVITE authentication. If the gateway does not require authentication, these values are ignored (it does not matter if they exist).

```xml
<param name="username" value="1001"/>
<param name="password" value="secret"/>
```


<br>

#### realm

<br>

The SIP authentication domain, usually specified by the carrier/provider.

```xml
<param name="realm" value="sip.provider.com"/>
```

<br>

#### proxy

<br>

The SIP server address to send requests to, serving as the destination for INVITE and REGISTER. Usually specified by the carrier/provider. In most cases, it uses the same value as realm.

```xml
<param name="proxy" value="sip.provider.com"/>
```

<br>

#### register

<br>

Specifies whether to perform SIP registration. true → Send REGISTER. false → Use peer mode without registration. This value is commonly set to false in IP authentication setups.

```xml
<param name="register" value="true"/>
```

<br>

#### register-proxy

<br>

Specifies a dedicated proxy for REGISTER. Used when the REGISTER server and the call processing server are separate.

```xml
<param name="register-proxy" value="sip.provider.com"/>
```

<br>

#### from-user

<br>

The From User in the SIP header

```xml
<param name="from-user" value="1001"/>
```

<br>

#### from-domain

<br>

The domain part of the From header in the SIP header

```xml
<param name="from-domain" value="provider.com"/>
```

<br>

#### caller-id-in-from

<br>

Reflects the caller ID in the From header. Required by some carriers.

```xml
<param name="caller-id-in-from" value="true"/>
```

<br>

#### expire-seconds

<br>

Registration renewal interval. To renew REGISTER every hour, configure it as follows:

```xml
<param name="expire-seconds" value="3600"/>
```

<br>

#### retry-seconds

<br>

Retry interval upon registration failure. To retry after 30 seconds:

```xml
<param name="retry-seconds" value="30"/>
```

<br>

#### ping

<br>

OPTIONS Ping interval. Used for alive checks on the remote PBX. To check status via OPTIONS requests every 30 seconds:

```xml
<param name="ping" value="30"/>
```

<br>

#### register-transport

<br>

Specifies the transport protocol

```xml
<param name="register-transport" value="udp"/>
```

<br><br>


## Checking Gateway Status

<br>

```shell
# In fs_cli

# Gateway list:
sofia status gateway

# Check specific Gateway:
sofia status gateway my_provider

```

<br><br>


# Applying Dynamic Gateways

<br>

FreeSWITCH's mod_sofia itself does not support a built-in feature to query Gateway configurations (XML) within sip_profiles directly from a database (direct SQL queries), but this can be implemented using mod_xml_curl.

<br>

## How It Works

<br>

1. When FreeSWITCH starts or commands like `sofia profile <name> rescan` are executed, mod_xml_curl sends an HTTP POST request to the backend web server (Node.js, Python FastAPI, PHP, etc.).
2. The parameters `section=directory` or `section=configuration&key_value=sofia.conf` in the request body are identified.
3. The backend web server queries Gateway information from the database (PostgreSQL, MySQL, etc.).
4. Based on the retrieved data, FreeSWITCH-compliant XML (`<gateways><gateway name="...">...</gateway></gateways>`) is dynamically generated and returned as the HTTP response.

**Advantages:**

* Since it follows the standard FreeSWITCH architecture, it is reliable and stable. Even if the Gateway list changes in the database, updates can be applied without restarting FreeSWITCH by simply rescanning the profile.
* Fine-grained control over caching, authentication, logging, etc., is possible at the web backend layer.


<br>

## How to Use mod_xml_curl

<br>

Add it to `autoload_configs/modules.conf.xml` so that FreeSWITCH loads the module upon startup:

```xml
<configuration name="modules.conf" description="Modules">
  <modules>
    <!-- Uncomment or add -->
    <load module="mod_xml_curl"/>
  </modules>
</configuration>
```

<br>

### Configuring mod_xml_curl.conf.xml

<br>

Specifies the web server URL to query and the request scope when FreeSWITCH requests Sofia configurations (or Gateway setups).

```xml
<!-- autoload_configs/mod_xml_curl.conf.xml: -->
<configuration name="xml_curl.conf" description="cURL XML Gateway">
  <bindings>
    <binding name="sofia_gateways">
      <!-- Backend web server endpoint URL -->
      <param name="gateway-url" value="[http://127.0.0.1:8000/fs/sofia_config](http://127.0.0.1:8000/fs/sofia_config)" bindings="configuration"/>
      
      <!-- HTTP communication settings (timeout and retry) -->
      <param name="timeout" value="10"/>
      <param name="method" value="POST"/>
    </binding>
  </bindings>
</configuration>
```

**Binding Behavior:**
* Whenever FreeSWITCH needs to load configurations related to `sofia.conf.xml`, it sends an HTTP POST request to the specified `gateway-url`.

<br>

### Backend Web Server Implementation Example (Python / FastAPI)

<br>

FreeSWITCH sends POST data formatted as `application/x-www-form-urlencoded`. The primary parameters are as follows:

* section: configuration
* key_name: name
* key_value: sofia.conf
* profile: Requested profile name (e.g., external)

<br>

```python
# Python server example

from fastapi import FastAPI, Form, Response
import asyncpg # Example asynchronous PostgreSQL driver

app = FastAPI()

# Example gateway table structure to fetch from DB:
# gateways (profile_name, gw_name, username, password, realm, proxy, register)

@app.post("/fs/sofia_config")
async def get_sofia_config(
    section: str = Form(...),
    key_value: str = Form(...)
):
    # If the request is not for sofia.conf configuration, return an empty 'NotFound' response (fallback to local XML)
    if section != "configuration" or key_value != "sofia.conf":
        return Response(
            content='<?xml version="1.0"?><document type="freeswitch/xml"><section name="result"><result status="not found" /></section></document>',
            media_type="application/xml"
        )

    # 1. Query active gateways from DB (mock query logic)
    # rows = await db.fetch("SELECT * FROM gateways WHERE enabled = true")
    gateways = [
        {
            "profile": "external",
            "name": "carrier_a",
            "username": "user123",
            "password": "secret_password",
            "realm": "sip.carrier-a.com",
            "register": "true"
        }
    ]

    # 2. Generate FreeSWITCH-compliant XML
    # Construct the gateways tag within the profile.
    xml_output = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="freeswitch/xml">
  <section name="configuration" description="Sofia Configuration">
    <configuration name="sofia.conf" description="sofia Endpoint">
      <profiles>
        <profile name="external">
          <gateways>
"""
    for gw in gateways:
        if gw["profile"] == "external":
            xml_output += f"""
            <gateway name="{gw['name']}">
              <param name="username" value="{gw['username']}"/>
              <param name="password" value="{gw['password']}"/>
              <param name="realm" value="{gw['realm']}"/>
              <param name="register" value="{gw['register']}"/>
            </gateway>"""

    xml_output += """
          </gateways>
        </profile>
      </profiles>
    </configuration>
  </section>
</document>"""

    return Response(content=xml_output, media_type="application/xml")
```

<br><br>

# Application and Testing Procedure

<br>

```bash
# Load module and reload XML: Load the module in the fs_cli console
freeswitch> load mod_xml_curl
freeswitch> reloadxml


# Reload configured Gateway information
freeswitch> sofia profile external rescan

# Check Gateway registration status: Verify that the Gateway inserted in DB is registered properly
freeswitch> sofia status
freeswitch> sofia profile external gwlist
freeswitch> sofia status gateway carrier_a
```
<br>

## Tips

<br>

* Not Found Fallback Handling: If a request not handled by the web server (dialplan, directory, etc.) arrives, you must return an XML containing `<result status="not found" />` so that default XML configurations on the local disk operate normally.
* Debugging: Executing the `xml_curl debug_on` command in fs_cli allows you to inspect real-time logs containing the HTTP request data sent to the web server by FreeSWITCH and the full received response XML payload.