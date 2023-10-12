In FreeSWITCH, CDRs can be saved in various formats.
<br> <br> 

# CDR(Call Detail Record)

CDR is a record of telephone transactions that occurred on FreeSWITCH. This record stores information such as caller, recipient, call start time, and call duration. The items that can be recorded in the CDR can be customized.
Commonly used CDR formats are as follows.

* csv : Use the mod_cdr_csv module.
* xml : Use the mod_cdr_xml module.
* sqlite3 : Use the mod_cdr_sqlite module.
* rdb(relational Database) : Use the mod_odbc_cdr module.
* MongoDB : Use the mod_cdr_mongodb module.
* PGSql(PostgreSQL) : Use the mod_cdr_pg_csv module.

<br> 
You can save cdrs in a variety of formats, but their usage is pretty much the same. The most important usage is deciding which column values to store.

<br> 

## mod_cdr_csv
Save the cdr to a text file in csv format.

For a detailed description of mod_cdr_csv, refer to [FreeSWITCH Document about mod_cdr_csv](https://developer.signalwire.com/freeswitch/FreeSWITCH-Explained/Modules/mod_cdr_csv_6587322/) .

First enable the mod_cdr_csv module at conf/autoload_configs/modules.conf.xml:

```xml
<!-- Event Handlers -->
<load module="mod_cdr_csv"/>
<load module="mod_event_socket"/>
<!--<load module="mod_cdr_sqlite"/>-->
<!--<load module="mod_odbc_cdr"/>-->
```
<br> 

Then modify the conf/autoload_configs/csr_csv.conf.xml.
This is the default csr_csv.conf.xml. The contents of the xml file are self-explanatory.

* rotate-on-hup: When enabled, after sending a HUP signal to FreeSWITCH (i.e., when the FreeSWITCH server is stopped), the logfile will be rotated to a format like this:
__Master.csv.YYYY-mm-dd-HH-MM-SS__  
For example : If you use the kill command to send the sighub signal to freeswitch, the process will not be terminated, but you will see that the csv file is backed up in Master.csv.YYYY-mm-dd-HH-MM-SS format.
```bash
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# ls -al
total 20
drwxr-x--- 2 root root 4096 Oct 10 23:26 .
drwxr-xr-x 4 root root 4096 Oct 10 23:41 ..
-rw-r--r-- 1 root root  635 Oct 10 23:44 1001.csv
-rw-r--r-- 1 root root  355 Oct 10 23:44 1002.csv
-rw-r--r-- 1 root root 1439 Oct 10 23:44 Master.csv

root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# ps -ef|grep freeswitch
root      430645       1  0 Oct10 ?        00:00:32 freeswitch -nonat -nc
root      437955  430586  0 00:35 pts/1    00:00:00 grep --color=auto freeswitch
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# kill -1 430645
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# 
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# ps -ef|grep freeswitch
root      430645       1  0 Oct10 ?        00:00:33 freeswitch -nonat -nc
root      437980  430586  0 00:36 pts/1    00:00:00 grep --color=auto freeswitch
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# ls -al
total 20
drwxr-x--- 2 root root 4096 Oct 11 00:36 .
drwxr-xr-x 4 root root 4096 Oct 11 00:36 ..
-rw-r--r-- 1 root root    0 Oct 11 00:36 1001.csv
-rw-r--r-- 1 root root  635 Oct 10 23:44 1001.csv.2023-10-11-00-36-14
-rw-r--r-- 1 root root    0 Oct 11 00:36 1002.csv
-rw-r--r-- 1 root root  355 Oct 10 23:44 1002.csv.2023-10-11-00-36-14
-rw-r--r-- 1 root root    0 Oct 11 00:36 Master.csv
-rw-r--r-- 1 root root 1439 Oct 10 23:44 Master.csv.2023-10-11-00-36-14

```

* default-template : Template to use for cdr format
* legs : * default-template : Template to use for cdr format. The default value is a, but ab is often required to leave an accurate cdr.
Possible values : __a, b, ab__


<br> 

```xml
<configuration name="cdr_csv.conf" description="CDR CSV Format">
  <settings>
    <!-- 'cdr-csv' will always be appended to log-base -->
    <!--<param name="log-base" value="/var/log"/>-->
    <param name="default-template" value="example"/>
    <!-- This is like the info app but after the call is hung up -->
    <!--<param name="debug" value="true"/>-->
    <param name="rotate-on-hup" value="true"/>
    <!-- may be a b or ab -->
    <param name="legs" value="a"/>
	<!-- Only log in Master.csv -->
	<!-- <param name="master-file-only" value="true"/> -->
  </settings>
  <templates>
    <template name="sql">INSERT INTO cdr VALUES ("${caller_id_name}","${caller_id_number}","${destination_number}","${context}","${start_stamp}","${answer_stamp}","${end_stamp}","${duration}","${billsec}","${hangup_cause}","${uuid}","${bleg_uuid}", "${accountcode}");</template>
    <template name="example">"${caller_id_name}","${caller_id_number}","${destination_number}","${context}","${start_stamp}","${answer_stamp}","${end_stamp}","${duration}","${billsec}","${hangup_cause}","${uuid}","${bleg_uuid}","${accountcode}","${read_codec}","${write_codec}"</template>
    <template name="snom">"${caller_id_name}","${caller_id_number}","${destination_number}","${context}","${start_stamp}","${answer_stamp}","${end_stamp}","${duration}","${billsec}","${hangup_cause}","${uuid}","${bleg_uuid}", "${accountcode}","${read_codec}","${write_codec}","${sip_user_agent}","${call_clientcode}","${sip_rtp_rxstat}","${sip_rtp_txstat}","${sofia_record_file}"</template>
    <template name="linksys">"${caller_id_name}","${caller_id_number}","${destination_number}","${context}","${start_stamp}","${answer_stamp}","${end_stamp}","${duration}","${billsec}","${hangup_cause}","${uuid}","${bleg_uuid}","${accountcode}","${read_codec}","${write_codec}","${sip_user_agent}","${sip_p_rtp_stat}"</template>
    <template name="asterisk">"${accountcode}","${caller_id_number}","${destination_number}","${context}","${caller_id}","${channel_name}","${bridge_channel}","${last_app}","${last_arg}","${start_stamp}","${answer_stamp}","${end_stamp}","${duration}","${billsec}","${hangup_cause}","${amaflags}","${uuid}","${userfield}"</template>
    <template name="opencdrrate">"${uuid}","${signal_bond}","${direction}","${ani}","${destination_number}","${answer_stamp}","${end_stamp}","${billsec}","${accountcode}","${userfield}","${network_addr}","${regex('${original_caller_id_name}'|^.)}","${sip_gateway_name}"</template>
  </templates>
</configuration>
```
<br> 

### template

The conf/autoload_configs/csr_csv.conf.xml file is designed to use the example template. Let's take a look at the example template.
Column values in the template are channel variables. For an explanation of channel variables, refer to the [basic_concept.md](https://github.com/raspberry-pi-maker/VoIP-related-codes/blob/main/FreeSWITCH/basic/basic_concept.md) .

<br> 

### csv example

I will leave a cdr using the example template. One call each for internal and external calls will be made.

* trunk call : I made a call from 91342657(name:spypi) to FreeSWITCH extension 1001
* internal call : I made a call from FreeSWITCH extension 1001 to FreeSWITCH extension 1002

```bash
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# cat Master.csv 
"spypi","91342657","*1001","blueivr","2023-10-10 23:24:01","2023-10-10 23:24:03","2023-10-10 23:24:11","10","8","NORMAL_CLEARING","eb1e406c-8871-4d69-9631-e7130f7ae390","1e764847-babe-44c7-8187-af0ce971b1ae","","PCMU","PCMU"
"1001","1001","1002","default","2023-10-10 23:26:08","2023-10-10 23:26:10","2023-10-10 23:26:15","7","5","NORMAL_CLEARING","7866825d-f38c-449e-a562-95250032e426","5a94f22d-8ab1-4e64-9a8c-5f37b9ed9bb9","1001","PCMA","PCMA"
```
<br> 

__This cdr doesn't seem to have no problems. But there is a caveat. ${destination_number} is not the number where the call was actually received. This is the phone number called by the party that created the A Leg. Therefore, it is impossible to know who answered the phone from this CDR.__

<br> 

### csv with both leg example

I will modify conf/autoload_configs/csr_csv.conf.xml to log both legs.

<br> 

``xml
<configuration name="cdr_csv.conf" description="CDR CSV Format">
  <settings>
    ......
    <!-- may be a b or ab -->
    <param name="legs" value="ab"/>
    ......
  </templates>
</configuration>
```
<br> 

Then reload the mod_cdr_csv module with fs_cli

<br> 

```bash
freeswitch@blueivr> reload mod_cdr_csv
+OK Reloading XML
+OK module unloaded
+OK module loaded

2023-10-10 23:41:25.667040 [NOTICE] switch_loadable_module.c:1318 Deleting API Function 'cdr_csv'
2023-10-10 23:41:25.667040 [DEBUG] switch_loadable_module.c:1320 Write lock interface 'cdr_csv' to wait for existing references.
2023-10-10 23:41:25.667040 [CONSOLE] switch_loadable_module.c:2396 Stopping: mod_cdr_csv
2023-10-10 23:41:25.667040 [INFO] switch_time.c:1430 Timezone reloaded 1750 definitions
2023-10-10 23:41:25.667040 [DEBUG] switch_event.c:2147 Event Binding deleted for mod_cdr_csv:TRAP
2023-10-10 23:41:25.667040 [CONSOLE] switch_loadable_module.c:2416 mod_cdr_csv unloaded.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:368 Adding default template.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template sql.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template example.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template snom.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template linksys.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template asterisk.
2023-10-10 23:41:25.667040 [DEBUG] mod_cdr_csv.c:415 Adding template opencdrrate.
2023-10-10 23:41:25.667040 [CONSOLE] switch_loadable_module.c:1803 Successfully Loaded [mod_cdr_csv]
2023-10-10 23:41:25.667040 [NOTICE] switch_loadable_module.c:412 Adding API Function 'cdr_csv'
```
<br> 

Then make calls again.
This picture is the first trunk phone call.

![leg](./image/csv_1.png)

<br> 


```bash
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# cat Master.csv
"spypi","91342657","1001","blueivr","2023-10-10 23:43:58","2023-10-10 23:43:59","2023-10-10 23:44:06","8","7","NORMAL_CLEARING","02132a5c-b9b1-4dbf-b79c-bdf408854867","","1001","PCMU","PCMU"
"spypi","91342657","*1003","blueivr","2023-10-10 23:43:58","2023-10-10 23:43:59","2023-10-10 23:44:06","8","7","NORMAL_CLEARING","99205d8f-5ec4-402e-b174-7a8bb8c23a61","02132a5c-b9b1-4dbf-b79c-bdf408854867","","PCMU","PCMU"
"Extension 1001","1001","1002","default","2023-10-10 23:44:11","2023-10-10 23:44:13","2023-10-10 23:44:19","8","6","NORMAL_CLEARING","515f34c7-35d4-4b65-99f2-edaad9411fa7","","1002","PCMA","PCMA"
"1001","1001","1002","default","2023-10-10 23:44:11","2023-10-10 23:44:13","2023-10-10 23:44:19","8","6","NORMAL_CLEARING","a03307a6-3658-4dde-a25b-bb6aecb96945","515f34c7-35d4-4b65-99f2-edaad9411fa7","1001","PCMA","PCMA"

```
<br> 

As follows, 4 CDRs occur for 2 calls. For one call, both A Leg and B Leg cdrs are created. 
However, while the B Leg CDR has A Leg uuid information, the A leg CDR does not have information about the B leg uuid.
And there is no DNIS information (*1003) in the B Leg CDR. This is because when FreeSWITCH connects the B Leg, the destination_number is 1001, not *1003.
But perhaps the format you want is to combine the A Leg and B Leg CDRs into one CDR. Two CDRs for one issue are not efficient because there are many columns that contain the same information overlapping.

<br> 

### csv to SQL with both leg CDR to one record

This time, let's move the cdr value to sql database while solving the above problem. To migrate to sql database, be sure to activate the rotate-on-hup value in the conf/autoload_configs/csr_csv.conf.xml file.

```xml
<param name="rotate-on-hup" value="true"/>
```
<br> 

What this does is to allow the Master.csv and other CDR files to be rotated via the same method as used by the mod_logfile method. The python script below uses one of these methods to rotate the log file and load the file into a database. (The naming format for the rotated files is from e.g. Master.csv to Master.csv.yyyy-mm-dd-hh-mm-ss)
The database uses sqlite3, but other databases can be applied without difficulty.

And this is the Master.csv file. This file will be changed to Master.csv.YYYY-mm-dd-HH-MM-SS when the python program sends sighub signal to freeswitch. And the python program reads this Master.csv.YYYY-mm-dd-HH-MM-SS and merge two legs cdr into one sql row.

<br> 

```bash
root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# ls -al
total 20
drwxr-x--- 2 root root 4096 Oct 11 01:20 .
drwxr-xr-x 4 root root 4096 Oct 11 01:20 ..
-rw-r--r-- 1 root root  833 Oct 11 18:15 Master.csv

root@ubuntusrv:/usr/local/freeswitch/log/cdr-csv# cat Master.csv
"spypi","91342657","1001","blueivr","2023-10-10 23:43:58","2023-10-10 23:43:59","2023-10-10 23:44:06","8","7","NORMAL_CLEARING","02132a5c-b9b1-4dbf-b79c-bdf408854867","","1001","PCMU","PCMU"
"spypi","91342657","*1003","blueivr","2023-10-10 23:43:58","2023-10-10 23:43:59","2023-10-10 23:44:06","8","7","NORMAL_CLEARING","99205d8f-5ec4-402e-b174-7a8bb8c23a61","02132a5c-b9b1-4dbf-b79c-bdf408854867","","PCMU","PCMU"
"Extension 1001","1001","1002","default","2023-10-10 23:44:11","2023-10-10 23:44:13","2023-10-10 23:44:19","8","6","NORMAL_CLEARING","515f34c7-35d4-4b65-99f2-edaad9411fa7","","1002","PCMA","PCMA"
"1001","1001","1002","default","2023-10-10 23:44:11","2023-10-10 23:44:13","2023-10-10 23:44:19","8","6","NORMAL_CLEARING","a03307a6-3658-4dde-a25b-bb6aecb96945","515f34c7-35d4-4b65-99f2-edaad9411fa7","1001","PCMA","PCMA"
```

```python
#!/usr/bin/python3

'''
This python script sends singhub to freeswitch to make rotated cdr.
Then read csv file records and insert to sqlite3 db.
Especially this program merges A and B legs and make only one record,
And the cdr_csv.conf.xml's template name is example.

'''

import subprocess
import csv
import sqlite3
import time, os, glob

def send_signal():
    # process call and get output
    process = subprocess.Popen(["killall", "-HUP", "freeswitch"], stdout=subprocess.PIPE)
    output, err = process.communicate()
    exit_code = process.wait()

# For convenience, I don't create indexes
def create_table(conn):
    table_sql = "CREATE TABLE IF NOT EXISTS cdr \
(caller_id_number text NOT NULL, \
destination_number text NOT NULL, \
answered_number text NOT NULL, \
start_stamp text NOT NULL, \
answer_stamp text NOT NULL, \
end_stamp text NOT NULL, \
duration int NOT NULL, \
billsec int NOT NULL, \
hangup_cause text NOT NULL, \
uuid text NOT NULL, \
bleg_uuid text NOT NULL \
); "

    try:
        c = conn.cursor()
        c.execute(table_sql)
    except Error as e:
        print(e)
'''
example template channel variables : 
caller_id_name,caller_id_number,destination_number,context,start_stamp,answer_stamp,end_stamp,
duration,billsec,hangup_cause,uuid,bleg_uuid,accountcode,read_codec,write_codec
'''
def insert_table(conn, cdr):
    uuid1 = cdr[0][10]
    bleg_uuid1 = cdr[0][11]
    uuid2 = cdr[1][10]
    bleg_uuid2 = cdr[1][11]
    print(cdr[0], "\n")
    print(cdr[1], "\n")
    if uuid1 == bleg_uuid2 or bleg_uuid1 == uuid2:
        print("OK, two cdrs are matching")
    else:
        print("ERR, two cdrs are not matching")
        return

    if bleg_uuid1 == "":
        aleg = 1
        bleg = 0
    elif bleg_uuid2 == "":
        aleg = 0
        bleg = 1
    else:
        print("**blegUUID1:", bleg_uuid1)
        print("**blegUUID2:", bleg_uuid2)
        return
    
    caller_id_number = cdr[aleg][1]    
    destination_number = cdr[aleg][2]    
    answered_number = cdr[bleg][2] 
    start_stamp = cdr[aleg][4]  
    answer_stamp = cdr[aleg][5]  
    end_stamp = cdr[aleg][6]
    duration   = int(cdr[aleg][7])
    billsec   = int(cdr[aleg][8])
    hangup_cause   = cdr[aleg][9]
    uuid   = cdr[aleg][10]
    bleg_uuid   = cdr[aleg][11]

    try:
        sql = "insert into cdr values(?,?,?,?,?,?,?,?,?,?,?);"
        c = conn.cursor()
        c.execute(sql, (caller_id_number, destination_number, answered_number,start_stamp, answer_stamp, end_stamp, duration, billsec, hangup_cause, uuid, bleg_uuid))
        conn.commit()
    except Error as e:
        print(e)


    
conn = sqlite3.connect("/usr/local/freeswitch/db/my_cdr.db")
create_table(conn)
send_signal() 
time.sleep(1)   # wait for the freeswitch to backup csv files
# Now backup csv files might be created
path_dir = '/usr/local/freeswitch/log/cdr-csv'
backup_dir = '/usr/local/freeswitch/log/cdr-csv-back'     
if not os.path.exists(backup_dir):
    os.mkdir(backup_dir) 

glob_dir = '/usr/local/freeswitch/log/cdr-csv/Master.csv.*'

file_list = list(glob.glob(glob_dir))
for file in file_list:
    filename = path_dir + '/' + file
    f = open(filename, 'r', encoding='utf-8')
    if not f:
        print("File Open Error[%s]"%filename)
    rdr = csv.reader(f)

    print("\n====File [%s] ==== "%filename)
    
    #length = len(rdr)   #length should be even number
    index = 0
    datas = []
    for line in rdr:
        datas.append(line)

    length = len(datas) / 2  
    #read  2 csv lines to merge
    for i in range(int(length)):
        cdr = []
        cdr.append(datas[i * 2])
        cdr.append(datas[i * 2 + 1])
        insert_table(conn, cdr)

    backupfile = backup_dir + '/' + file
    os.replace(filename, backupfile)    
        
```

<br> <br> 

Let's run the Python program and check the sqlite DB.

```bash
root@ubuntusrv:/usr/local/freeswitch/scripts# python3 cdr_csv.py 

====File [/usr/local/freeswitch/log/cdr-csv/Master.csv.2023-10-11-00-36-14] ==== 
['spypi', '91342657', '1001', 'blueivr', '2023-10-10 23:43:58', '2023-10-10 23:43:59', '2023-10-10 23:44:06', '8', '7', 'NORMAL_CLEARING', '02132a5c-b9b1-4dbf-b79c-bdf408854867', '', '1001', 'PCMU', 'PCMU'] 

['spypi', '91342657', '*1003', 'blueivr', '2023-10-10 23:43:58', '2023-10-10 23:43:59', '2023-10-10 23:44:06', '8', '7', 'NORMAL_CLEARING', '99205d8f-5ec4-402e-b174-7a8bb8c23a61', '02132a5c-b9b1-4dbf-b79c-bdf408854867', '', 'PCMU', 'PCMU'] 

OK, two cdrs are matching
['Extension 1001', '1001', '1002', 'default', '2023-10-10 23:44:11', '2023-10-10 23:44:13', '2023-10-10 23:44:19', '8', '6', 'NORMAL_CLEARING', '515f34c7-35d4-4b65-99f2-edaad9411fa7', '', '1002', 'PCMA', 'PCMA'] 

['1001', '1001', '1002', 'default', '2023-10-10 23:44:11', '2023-10-10 23:44:13', '2023-10-10 23:44:19', '8', '6', 'NORMAL_CLEARING', 'a03307a6-3658-4dde-a25b-bb6aecb96945', '515f34c7-35d4-4b65-99f2-edaad9411fa7', '1001', 'PCMA', 'PCMA'] 

OK, two cdrs are matching
```

<br> <br> 

Then check the sqlite3 DB. A-leg and B-leg cdrs are successfully merged into sqlite table. __In particular, in the first row, you can see that both DNIS (*1003) and the extension number 1001 that actually received the call are included.__

```sql
root@ubuntusrv:/usr/local/freeswitch/db# sqlite3 my_cdr.db 
SQLite version 3.31.1 2020-01-27 19:55:54
Enter ".help" for usage hints.
sqlite> select * from cdr;
91342657|*1003|1001|2023-10-10 23:43:58|2023-10-10 23:43:59|2023-10-10 23:44:06|8|7|NORMAL_CLEARING|99205d8f-5ec4-402e-b174-7a8bb8c23a61|02132a5c-b9b1-4dbf-b79c-bdf408854867
1001|1002|1002|2023-10-10 23:44:11|2023-10-10 23:44:13|2023-10-10 23:44:19|8|6|NORMAL_CLEARING|a03307a6-3658-4dde-a25b-bb6aecb96945|515f34c7-35d4-4b65-99f2-edaad9411fa7

```

<br> <br> 


# Wrapping up

<br> 
mod_cdr_csv is the simplest way to write a cdr. If you know how to use mod_cdr_csv, leaving a cdr using other methods is not much different.
We looked into using the cdr left over from mod_cdr_csv and putting it into the database. You can create an independent program, register it in the crontab, send a sighup signal at regular intervals, create a backup of the csv file, and then use these files to insert them into the database.








