# Lua FreeSWITCH Dbh

<br>
I summarize how to handle databases in lua, FreeSWITCH's important scripting language.
Languages such as Python and JavaScript make it easy to learn how to handle databases, but the lua language is not as popular as these languages, so manuals are relatively lacking.

<br>
There are two main ways to handle datamase in Lua. One is to use luasql supported by the Lua language, and the other is to use Dbh (Database Handler) provided by FreeSWITCH.


<br>

## Dbh and SQLite3

<br>
SQLite3 is a local database used by FreeSWITCH Core. This database cannot be accessed via a network, so its scalability is low, but it is sufficient for data management in a single process. Additionally, because it directly manipulates files, it has the advantage of being able to process data at a faster rate than RDBMS (Relational Database).

<br>
The table and values used in the test are random tables that I use, so you can change them to suit your needs.
<br>

### Query Example
```lua
--[[
If an absolute path is not specified, it is assumed that the database file is in FreeSWITCH's db folder.  
If you want to open a database file in a different path, enter the absolute path name.
]]

local dbh = freeswitch.Dbh("sqlite://pds") -- sqlite database in subdirectory "/usr/local/freeswitch/db"

--[[
  Query
]]
local sql = string.format("select ID, GRP_ID, PHONE, STATUS from AGENT")

dbh:query(sql, 
  function(row)
    freeswitch.consoleLog("INFO", string.format("ID[%d] GRP_ID[%s] PHONE[%s]", row.ID, row.GRP_ID, row.PHONE))
  end
)

dbh:release()

```
<br>
You can get the result as follows. You can see that the function is called for all rows returned by the query statement.:

<br>

```bash
freeswitch@blueivr> luarun mytest.lua
+OK

2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[1] GRP_ID[3001] PHONE[50001]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[2] GRP_ID[3001] PHONE[50002]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[3] GRP_ID[3001] PHONE[50003]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[4] GRP_ID[3001] PHONE[50004]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[5] GRP_ID[3001] PHONE[50005]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[6] GRP_ID[3001] PHONE[50006]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[7] GRP_ID[3001] PHONE[50007]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[8] GRP_ID[3001] PHONE[50008]
2024-05-25 19:09:33.428407 [INFO] switch_cpp.cpp:1465 ID[9] GRP_ID[3001] PHONE[50009]
```

<br>

### update Query Example

<br>

If it is a SQL statement such as update, insert, or drop rather than a query that retrieves row information, the function parameter is not needed in the query function. The function parameter can be omitted and used as follows.

<br>

```lua
--[[
If an absolute path is not specified, it is assumed that the database file is in FreeSWITCH's db folder.  
If you want to open a database file in a different path, enter the absolute path name.
]]

local dbh = freeswitch.Dbh("sqlite://pds") -- sqlite database in subdirectory "/usr/local/freeswitch/db"

--[[
  Query
]]
local sql = string.format("update AGENT set GRP_ID = '3002' where PHONE = '50002'")
dbh:query(sql)

sql = string.format("select ID, GRP_ID, STATUS from AGENT where PHONE = '50002' ")

dbh:query(sql, 
  function(row)
    freeswitch.consoleLog("INFO", string.format("ID[%d] GRP_ID[%s] PHONE[%s]", row.ID, row.GRP_ID, row.PHONE))
  end
)

dbh:release()

```
<br>
You can get the result as follows:


<br>

```bash
freeswitch@blueivr> luarun mytest.lua
+OK

2024-05-25 19:19:08.988371 [INFO] switch_cpp.cpp:1465 ID[2] GRP_ID[3002] 
```

<br>

## Dbh and UnixODBC

<br>
Using Dbh and ODBC is the easiest way to access a relational database like SQL Server, MySQL, MariaDB, Oracle.
To use ODBC, unixODBC package and the odbc driver for Linux must be installed. If the odbc driver is properly installed, driver information must exist in the /etc/odbcinst.ini file.  For reference, I downloaded MySQL ODBC driver 8.4 for Linux from the MySQL website and installed it.

<br>

```ini
[root@spypiggy mypds]# cat /etc/odbcinst.ini
[PostgreSQL]
Description=ODBC for PostgreSQL
Driver=/usr/lib/psqlodbcw.so
Setup=/usr/lib/libodbcpsqlS.so
Driver64=/usr/lib64/psqlodbcw.so
Setup64=/usr/lib64/libodbcpsqlS.so
FileUsage=1


[MariaDB]
Description=ODBC for MariaDB
Driver=/usr/lib/libmaodbc.so
Driver64=/usr/lib64/libmaodbc.so
FileUsage=1

[MySQL ODBC 8.4 Unicode Driver]
DRIVER=/usr/lib64/libmyodbc8w.so
UsageCount=1

[MySQL ODBC 8.4 ANSI Driver]
DRIVER=/usr/lib64/libmyodbc8a.so
UsageCount=1
```

<br>
Then, enter the database information you want to access in the odbc.ini file.
<br>

```bash
[root@spypiggy mypds]# cat /etc/odbc.ini
[bluebay_mysql]
Description     = connect to pbx
Driver          = MariaDB
SERVER          = localhost
PORT            = 3306
USER=root
PASSWORD=dongu
DATABASE=blueswitch2

[bluebay_mysql_pbx]
Description     = connect to pbx_bluebay
Driver          = MySQL ODBC 8.4 Unicode Driver
SERVER          = "Your Server IP Address"
PORT            = 3306
DATABASE        = PDS

```
<br>

### Test ODBC Connection

<br>
If you have installed unixODBC and set the odbcinst.ini and odbc.ini files properly, you can test the connection as follows using isql command.


``` bash
[root@spypiggy mypds]# isql -v bluebay_mysql_pbx "your ID" "Your Password"
+---------------------------------------+
| Connected!                            |
|                                       |
| sql-statement                         |
| help [tablename]                      |
| quit                                  |
|                                       |
+---------------------------------------+
SQL> 
```
<br>

### Query Example

The advantage of FreeSWITCH's dbh is that it provides the same interface whether SQLite3 or ODBC is used. 
Therefore, the query and update example is the same as SQLite3 above except for the part where the DB handle is retrieved.


```lua
--[[
Use information from odbc.ini. 
]]

local dbh = freeswitch.Dbh("bluebay_mysql_pbx", "your ID","Your Password!")

--[[
  Query
]]
local sql = string.format("select ID, GRP_ID, PHONE, STATUS from AGENT")

dbh:query(sql, 
  function(row)
    freeswitch.consoleLog("INFO", string.format("ID[%d] GRP_ID[%s] PHONE[%s]", row.ID, row.GRP_ID, row.PHONE))
  end
)

dbh:release()
```
<br>

### Stored Procedure Call

It is very difficult to find documentation on how to use Stored Procedures in FreeSWITCH Dbh. Let’s briefly look at how to call a MySQL procedure.

The following is a simple MySQL procedure created for testing. The procedure name is SAMPLE_PROCEDURE2 and it receives a phone number as a parameter. Then, using this phone number as a conditional clause, the row is searched and returned.

```sql
CREATE DEFINER=`blueswitch`@`%` PROCEDURE `SAMPLE_PROCEDURE2`(    
IN PHONE_VAL VARCHAR(32)
)
BEGIN
START TRANSACTION;
SELECT *  from AGENT where PHONE = PHONE_VAL;
COMMIT;
END
```

<br>

Now let’s test this procedure using FreeSWITCH Dbh. Pass 50001 as a parameter to the procedure.

```lua
local dbh = freeswitch.Dbh("bluebay_mysql_pbx", "your ID","Your Password!")
dbh:query("CALL PDS.SAMPLE_PROCEDURE2(50001)",
    function(qrow)
        for key, val in pairs(qrow) do
            freeswitch.consoleLog("INFO", "Result:" ..tostring(key) .." " ..tostring(val))
        end
    end
)
```
<br>
Below are the test results.

<br>

```bash
freeswitch@blueivr> luarun mytest.lua
+OK

2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATUS Available
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATUS_TM 1716611305
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:DIALER_Lock_TM 1716615648
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:ID 1
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATE_Lock_TM 1716611546
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:REQ_LO 0
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATE_TM 1716611546
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:GRP_ID 3001
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATE_Lock Lock
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:PHONE 50001
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:DIALER_Lock Lock
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:UUID_KEY 383421ca-d11c-49f3-bb51-500536a2195e
2024-05-25 22:02:26.268386 [INFO] switch_cpp.cpp:1465 Result:STATE Bridged
```

If the procedure returns multiple rows, the function will be called repeatedly.
If you want to return a specific value instead of row, do the following:


```lua
CREATE DEFINER=`blueswitch`@`%` PROCEDURE `SAMPLE_PROCEDURE2`(    
IN PHONE_VAL VARCHAR(32)
)
BEGIN
DECLARE VAL VARCHAR(64) DEFAULT "Hello World";
START TRANSACTION;
SELECT VAL AS debug_value;
COMMIT;
END
```
<br>

```bash
freeswitch@blueivr> luarun mytest.lua
+OK

2024-05-25 22:10:02.068381 [INFO] switch_cpp.cpp:1465 Result:debug_value Hello World

```
You can see that the debug_value key contains "Hello World".











