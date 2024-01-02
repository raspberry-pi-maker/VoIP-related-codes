# Regular Expression

<br>
Regular expressions are widely used in various computer systems, including Linux.
Regular expressions are also frequently used in FreeSWITCH, especially in dialplans.
In dial plan, it is mainly used to check the validity of extension or trunk line numbers. Therefore, in this article, we will mainly look at regular expressions that verify numbers.

<br>

## Regular expressions in the dialplan

<br>
For example, the dialplan below works when the called number is a digit in the 3000 range (3000 ~ 3999).
<br>

```xml
<extension name="Trunk  PBX call">
  <condition field="destination_number" expression="^(3\d{3})$">
      <action application="set" data="continue_on_fail=true"/>
      <action application="log" data="ALERT ==== [3XXX] NODE_RED TEST1 START ==== "/>
      <action application="lua" data="inbound.lua" />
  </condition>
</extension>
```

<br>

## Useful Regular expressions in FreeSWITCH

<br>

|Chacter|Description|Usage|Usage description|
|--|--|--|--|
|^|assert position at the start of a line|^(123)$|only "123" matches|
|$|assert position at the end of a line|^(123)$|only "123" matches|
|\d|matches one digit|\d\d|2 digit number like 12, 23, 45|
|\D|matches any chacter that's not a digit|\D\D|2 characters like Ab, cd, @#|
|.|matches any chacter|..|2 characters(includes number) like Ab, cd, @#, 12, 34|
|(a\|A)|a or A|^(0\|1)$ |0 or 1 |
|[abc]|a single character in the list abc|[1234]| 1,2,3,4 matches|
|[0-9]|one digit in the range  0 to 9|3[0-9]|30 to 39 matches|
|{x}|matches the previous token exactly x times|[12]{2}|11,12,21,22 matches|
|{x,y}|matches the previous token between x ~ y times|[12]{1,2}|1,2, 11,12,21,22 matches|
|*|matches the previous token between 0 ~ unlimited times|^(12*)$|1, 12,122,1222,... matches|
|+|matches the previous token between 1 ~ unlimited times|^(12+)$|12,122,1222,... matches|
|?|matches the previous token between 0 ~ 1 times|^(12?)$|1, 12 matches|
|||||


<br>

## Case Study

<br>

|Requirement|Regular Expression|
|-----|-----|
| 4000 ~ 4999 | ^(4\d{3})$ |
| 1001 ~ 1005 or 5001 ~ 5005 | ^([15]00[12345])$ |
| Any number | ^(\d+)$ |
| random numbers starting with 5 | ^5(\d*)$ |
| When ** is pressed | `^[*][*]$` |
| 123 or 456 | ^(123\|456)$ |
| 6 digits |  ^(\d{6})$ |
|4 to 6 digits|  ^(\d{4,6})$ |
|||


<br>

