# SIPp Overview

<br>

* SIPp is an open‑source test tool and traffic generator for the SIP (Session Initiation Protocol) used in VoIP systems.
* It can simulate thousands of concurrent SIP calls to test performance, scalability, and reliability of SIP servers, proxies, and gateways.

* Features include:

* * Predefined call scenarios (e.g., UAC, UAS, registration, etc.)

* * Custom XML scenario scripting for flexible test cases

* * Support for TCP, UDP, TLS, and IPv6

* * Ability to measure response times, call setup rates, and stress test SIP infrastructure

* Widely used by developers, QA engineers, and telecom operators to validate VoIP solutions under real‑world traffic conditions.

<br>

SIPp offers a wide range of features through a vast array of available options. The following is a summary of the help information for the `sipp` command.

<br>

| Category | Option / Command / Signal | Description |
| :--- | :--- | :--- |
| **Usage** | `sipp remote_host[:remote_port] [options]` | |
| **Usage** | `sipp` | |
| **Example** | `./sipp` | Launch the interactive startup wizard from an interactive terminal |
| **Example** | `./sipp -sn uas` | Run SIPp with embedded server (uas) scenario |
| **Example** | `./sipp -sn uac 127.0.0.1` | On the same host, run SIPp with embedded client (uac) scenario |
| **Scenario file options** | `-sd` | Dumps a default scenario (embedded in the SIPp executable) |
| **Scenario file options** | `-sf` | Loads an alternate XML scenario file. To learn more about XML scenario syntax, use the -sd option to dump embedded scenarios. They contain all the necessary help. |
| **Scenario file options** | `-rxsf` | Loads an alternate receive xml scenario file as the second scenario - enabling a mixture of originating and terminating calls to be executed. If this is included then the second scenario MUST be a server mode scenario, and the first scenario (specified in -sf / -sn) MUST be a client-mode scenario. If both -snrx and -sfrx are omitted then only a single scenario is executed. |
| **Scenario file options** | `-oocsf` | Load out-of-call scenario. |
| **Scenario file options** | `-oocsn` | Load out-of-call scenario. |
| **Scenario file options** | `-sn` | Use a default scenario (embedded in the SIPp executable). If this option is omitted, the Standard SipStone UAC scenario is loaded.<br><br>Available values in this version:<br>- 'uac' : Standard SipStone UAC (default).<br>- 'uas' : Simple UAS responder.<br>- 'regexp' : Standard SipStone UAC - with regexp and variables.<br>- 'branchc' : Branching and conditional branching in scenarios - client.<br>- 'branchs' : Branching and conditional branching in scenarios - server.<br><br>Default 3pcc scenarios (see -3pcc option):<br>- '3pcc-C-A' : Controller A side (must be started after all other 3pcc scenarios)<br>- '3pcc-C-B' : Controller B side.<br>- '3pcc-A' : A side.<br>- '3pcc-B' : B side. |
| **Scenario file options** | `-rxrn` | Use a default scenario (embedded in the sipp executable) for the second scenario - enabling a mixture of originating and terminating calls to be executed. If this is included then the second scenario MUST be a server mode scenario, and the first scenario (specified in -sf / -sn) MUST be a client-mode scenario. If both -snrx and -sfrx are omitted then only a single scenario is executed. |
| **IP, port and protocol options** | `-t` | Set the transport mode:<br>- u1: UDP with one socket (default),<br>- un: UDP with one socket per call,<br>- ui: UDP with one socket per IP address. The IP addresses must be defined in the injection file.<br>- t1: TCP with one socket,<br>- tn: TCP with one socket per call,<br>- l1: TLS with one socket,<br>- ln: TLS with one socket per call,<br>- s1: SCTP with one socket,<br>- sn: SCTP with one socket per call, |
| **IP, port and protocol options** | `-i` | Set the local IP address for 'Contact:', 'Via:', and 'From:' headers. Default is primary host IP address. |
| **IP, port and protocol options** | `-p` | Set the local port number. Default is a random free port chosen by the system. |
| **IP, port and protocol options** | `-bind_local` | Bind socket to local IP address, i.e. the local IP address is used as the source IP address. If SIPp runs in server mode it will only listen on the local IP address instead of all IP addresses. |
| **IP, port and protocol options** | `-bind_to_device` | Bind socket to the specified network device. Requires superuser permissions. |
| **IP, port and protocol options** | `-ci` | Set the local control IP address |
| **IP, port and protocol options** | `-cp` | Set the local control port number. Default is 8888. |
| **IP, port and protocol options** | `-max_socket` | Set the max number of sockets to open simultaneously. This option is significant if you use one socket per call. Once this limit is reached, traffic is distributed over the sockets already opened. Default value is 50000 |
| **IP, port and protocol options** | `-max_reconnect` | Set the the maximum number of reconnection. |
| **IP, port and protocol options** | `-reconnect_close` | Should calls be closed on reconnect? |
| **IP, port and protocol options** | `-reconnect_sleep` | How long (in milliseconds) to sleep between the close and reconnect? |
| **IP, port and protocol options** | `-rsa` | Set the remote sending address to host:port for sending the messages. |
| **IP, port and protocol options** | `-tls_cert` | Set the name for TLS Certificate file. Default is 'cacert.pem' |
| **IP, port and protocol options** | `-tls_key` | Set the name for TLS Private Key file. Default is 'cakey.pem' |
| **IP, port and protocol options** | `-tls_ca` | Set the name for TLS CA file. If not specified, X509 verification is not activated. |
| **IP, port and protocol options** | `-tls_crl` | Set the name for Certificate Revocation List file. If not specified, X509 CRL is not activated. |
| **IP, port and protocol options** | `-tls_version` | Set the TLS protocol version to use (1.0, 1.1, 1.2, 1.3) -- default is autonegotiate |
| **IP, port and protocol options** | `-multihome` | Set multihome address for SCTP |
| **IP, port and protocol options** | `-heartbeat` | Set heartbeat interval in ms for SCTP |
| **IP, port and protocol options** | `-assocmaxret` | Set association max retransmit counter for SCTP |
| **IP, port and protocol options** | `-pathmaxret` | Set path max retransmit counter for SCTP |
| **IP, port and protocol options** | `-pmtu` | Set path MTU for SCTP |
| **IP, port and protocol options** | `-gracefulclose` | If true, SCTP association will be closed with SHUTDOWN (default). If false, SCTP association will be closed by ABORT. |
| **SIPp overall behavior options** | `-v` | Display version and copyright information. |
| **SIPp overall behavior options** | `-bg` | Launch SIPp in background mode. |
| **SIPp overall behavior options** | `-nostdin` | Disable stdin. |
| **SIPp overall behavior options** | `-plugin` | Load a plugin. |
| **SIPp overall behavior options** | `-sleep` | How long to sleep for at startup. Default unit is seconds. |
| **SIPp overall behavior options** | `-skip_rlimit` | Do not perform rlimit tuning of file descriptor limits. Default: false. |
| **SIPp overall behavior options** | `-buff_size` | Set the send and receive buffer size. |
| **SIPp overall behavior options** | `-sendbuffer_warn` | Produce warnings instead of errors on SendBuffer failures. |
| **SIPp overall behavior options** | `-lost` | Set the number of packets to lose by default (scenario specifications override this value). |
| **SIPp overall behavior options** | `-key` | `keyword value`<br>Set the generic parameter named "keyword" to "value". |
| **SIPp overall behavior options** | `-set` | `variable value`<br>Set the global variable parameter named "variable" to "value". |
| **SIPp overall behavior options** | `-tdmmap` | Generate and handle a table of TDM circuits.<br>A circuit must be available for the call to be placed.<br>Format: `-tdmmap {0-3}{99}{5-8}{1-31}` |
| **SIPp overall behavior options** | `-dynamicStart` | `variable value`<br>Set the start offset of dynamic_id variable |
| **SIPp overall behavior options** | `-dynamicMax` | `variable value`<br>Set the maximum of dynamic_id variable |
| **SIPp overall behavior options** | `-dynamicStep` | `variable value`<br>Set the increment of dynamic_id variable |
| **Call behavior options** | `-aa` | Enable automatic 200 OK answer for INFO, NOTIFY, OPTIONS and UPDATE. |
| **Call behavior options** | `-base_cseq` | Start value of [cseq] for each call. |
| **Call behavior options** | `-cid_str` | Call ID string (default `%u-%p@%s`). `%u`=call_number, `%s`=ip_address, `%p`=process_number, `%r`=random_integer, `%%`=% (in any order). |
| **Call behavior options** | `-cid_type` | Call ID generation mode. Values: default (aliases: format, legacy), uuid, uuid-compact (aliases: uuidcompact, uuid32), random (alias: random-hex), timestamp (alias: time). Modes other than default ignore -cid_str. |
| **Call behavior options** | `-d` | Controls the length of calls. More precisely, this controls the duration of 'pause' instructions in the scenario, if they do not have a 'milliseconds' section. Default value is 0 and default unit is milliseconds. |
| **Call behavior options** | `-deadcall_wait` | How long the Call-ID and final status of calls should be kept to improve message and error logs (default unit is ms). |
| **Call behavior options** | `-auth_uri` | Force the value of the URI for authentication.<br>By default, the URI is composed of remote_ip:remote_port. |
| **Call behavior options** | `-au` | Set authorization username for authentication challenges. Default is taken from -s argument |
| **Call behavior options** | `-ap` | Set the password for authentication challenges. Default is 'password' |
| **Call behavior options** | `-s` | Set the username part of the request URI. Default is 'service'. |
| **Call behavior options** | `-default_behaviors` | Set the default behaviors that SIPp will use. Possible values are:<br>- all : Use all default behaviors<br>- none : Use no default behaviors<br>- bye : Send byes for aborted calls<br>- abortunexp : Abort calls on unexpected messages<br>- pingreply : Reply to ping requests<br>- cseq : Check CSeq of ACKs<br>If a behavior is prefaced with a -, then it is turned off. Example: all,-bye |
| **Call behavior options** | `-nd` | No Default. Disable all default behavior of SIPp which are the following:<br>- On UDP retransmission timeout, abort the call by sending a BYE or a CANCEL<br>- On receive timeout with no ontimeout attribute, abort the call by sending a BYE or a CANCEL<br>- On unexpected BYE send a 200 OK and close the call<br>- On unexpected CANCEL send a 200 OK and close the call<br>- On unexpected PING send a 200 OK and continue the call<br>- On unexpected ACK CSeq do nothing<br>- On any other unexpected message, abort the call by sending a BYE or a CANCEL |
| **Call behavior options** | `-pause_msg_ign` | Ignore the messages received during a pause defined in the scenario |
| **Call behavior options** | `-callid_slash_ign` | Don't treat a triple-slash in Call-IDs as indicating an extra SIPp prefix. |
| **Injection file options** | `-rxinf` | Inject values from an external CSV file during calls into the scenarios.<br>First line of this file say whether the data is to be read in sequence (SEQUENTIAL), random (RANDOM), or user (USER) order.<br>Each line corresponds to one call and has one or more ';' delimited data fields. Those fields can be referred as [field0], [field1], ... in the xml scenario file. Several CSV files can be used simultaneously (syntax: -inf f1.csv -inf f2.csv ...) |
| **Injection file options** | `-inf` | Inject values from an external CSV file during calls into the scenarios.<br>First line of this file say whether the data is to be read in sequence (SEQUENTIAL), random (RANDOM), or user (USER) order.<br>Each line corresponds to one call and has one or more ';' delimited data fields. Those fields can be referred as [field0], [field1], ... in the xml scenario file. Several CSV files can be used simultaneously (syntax: -inf f1.csv -inf f2.csv ...) |
| **Injection file options** | `-infindex` | `file field`<br>Create an index of file using field. For example -inf ../path/to/users.csv -infindex users.csv 0 creates an index on the first key. |
| **Injection file options** | `-ip_field` | Set which field from the injection file contains the IP address from which the client will send its messages.<br>If this option is omitted and the '-t ui' option is present, then field 0 is assumed.<br>Use this option together with '-t ui' |
| **RTP behaviour options** | `-mi` | Set the local media IP address (default: local primary host IP address) |
| **RTP behaviour options** | `-rtp_echo` | Enable RTP echo. RTP/UDP packets received on media port are echoed to their sender.<br>RTP/UDP packets coming on this port + 2 are also echoed to their sender (used for sound and video echo). |
| **RTP behaviour options** | `-mb` | Set the RTP echo buffer size (default: 2048). |
| **RTP behaviour options** | `-min_rtp_port` | Minimum port number for RTP socket range. |
| **RTP behaviour options** | `-max_rtp_port` | Maximum port number for RTP socket range. |
| **RTP behaviour options** | `-mp` | Sets -min_rtp_port for backwards compatibility. |
| **RTP behaviour options** | `-rtp_payload` | RTP default payload type. |
| **RTP behaviour options** | `-rtp_threadtasks` | RTP number of playback tasks per thread. |
| **RTP behaviour options** | `-rtp_buffsize` | Set the rtp socket send/receive buffer size. |
| **RTP behaviour options** | `-rtpcheck_debug` | Write RTP check debug information to file |
| **RTP behaviour options** | `-srtpcheck_debug` | Write SRTP check debug information to file |
| **RTP behaviour options** | `-audiotolerance` | Audio error tolerance for RTP checks (0.0-1.0) -- default: 1.0 |
| **RTP behaviour options** | `-videotolerance` | Video error tolerance for RTP checks (0.0-1.0) -- default: 1.0 |
| **RTP behaviour options** | `-random_base_ssrc` | Use a random base SSRC for RTP streams instead of default value 0xCA110000 |
| **Call rate options** | `-r` | Set the call rate (in calls per seconds). This value can bechanged during test by pressing '+', '_', '*' or '/'. Default is 10.<br>pressing '+' key to increase call rate by 1 * rate_scale,<br>pressing '-' key to decrease call rate by 1 * rate_scale,<br>pressing '*' key to increase call rate by 10 * rate_scale,<br>pressing '/' key to decrease call rate by 10 * rate_scale. |
| **Call rate options** | `-rp` | Specify the rate period for the call rate. Default is 1 second and default unit is milliseconds. This allows you to have n calls every m milliseconds (by using -r n -rp m).<br>Example: -r 7 -rp 2000 ==> 7 calls every 2 seconds.<br>-r 10 -rp 5s => 10 calls every 5 seconds. |
| **Call rate options** | `-rate_scale` | Control the units for the '+', '-', '*', and '/' keys. |
| **Call rate options** | `-rate_increase` | Specify the rate increase every -rate_interval units (default is seconds).<br>This allows you to increase the load for each independent logging period.<br>Example: -rate_increase 10 -rate_interval 10s ==> increase calls by 10 every 10 seconds. |
| **Call rate options** | `-rate_max` | If -rate_increase is set, then quit after the rate reaches this value.<br>Example: -rate_increase 10 -rate_max 100 ==> increase calls by 10 until 100 cps is hit. |
| **Call rate options** | `-rate_interval` | Set the interval by which the call rate is increased. Defaults to the value of -fd. |
| **Call rate options** | `-no_rate_quit` | If -rate_increase is set, do not quit after the rate reaches -rate_max. |
| **Call rate options** | `-l` | Set the maximum number of simultaneous calls. Once this limit is reached, traffic is decreased until the number of open calls goes down. Default: (3 * call_duration (s) * rate). |
| **Call rate options** | `-m` | Stop the test and exit when 'calls' calls are processed |
| **Call rate options** | `-users` | Instead of starting calls at a fixed rate, begin 'users' calls at startup, and keep the number of calls constant. |
| **Retransmission and timeout options** | `-recv_timeout` | Global receive timeout. Default unit is milliseconds. If the expected message is not received, the call times out and is aborted. |
| **Retransmission and timeout options** | `-send_timeout` | Global send timeout. Default unit is milliseconds. If a message is not sent (due to congestion), the call times out and is aborted. |
| **Retransmission and timeout options** | `-timeout` | Global timeout. Default unit is seconds. If this option is set, SIPp quits after nb units (-timeout 20s quits after 20 seconds). |
| **Retransmission and timeout options** | `-timeout_error` | SIPp fails if the global timeout is reached is set (-timeout option required). |
| **Retransmission and timeout options** | `-max_retrans` | Maximum number of UDP retransmissions before call ends on timeout. Default is 5 for INVITE transactions and 7 for others. |
| **Retransmission and timeout options** | `-max_invite_retrans` | Maximum number of UDP retransmissions for invite transactions before call ends on timeout. |
| **Retransmission and timeout options** | `-max_non_invite_retrans` | Maximum number of UDP retransmissions for non-invite transactions before call ends on timeout. |
| **Retransmission and timeout options** | `-nr` | Disable retransmission in UDP mode. |
| **Retransmission and timeout options** | `-rtcheck` | Select the retransmission detection method: full (default) or loose. |
| **Retransmission and timeout options** | `-T2` | Global T2-timer in milli seconds |
| **Third-party call control options** | `-3pcc` | Launch the tool in 3pcc mode ("Third Party call control"). The passed IP address depends on the 3PCC role.<br>- When the first twin command is 'sendCmd' then this is the address of the remote twin socket. SIPp will try to connect to this address:port to send the twin command (This instance must be started after all other 3PCC scenarios). Example: 3PCC-C-A scenario.<br>- When the first twin command is 'recvCmd' then this is the address of the local twin socket. SIPp will open this address:port to listen for twin command. Example: 3PCC-C-B scenario. |
| **Third-party call control options** | `-master` | 3pcc extended mode: indicates the master number |
| **Third-party call control options** | `-slave` | 3pcc extended mode: indicates the slave number |
| **Third-party call control options** | `-slave_cfg` | 3pcc extended mode: indicates the file where the master and slave addresses are stored |
| **Performance and watchdog options** | `-timer_resol` | Set the timer resolution. Default unit is milliseconds. This option has an impact on timers precision.Small values allow more precise scheduling but impacts CPU usage.The default value is 10ms. |
| **Performance and watchdog options** | `-max_recv_loops` | Set the maximum number of messages received read per cycle. Increase this value for high traffic level. The default value is 1000. |
| **Performance and watchdog options** | `-max_sched_loops` | Set the maximum number of calls run per event loop. Increase this value for high traffic level. The default value is 1000. |
| **Performance and watchdog options** | `-watchdog_interval` | Set gap between watchdog timer firings. Default is 400. |
| **Performance and watchdog options** | `-watchdog_reset` | If the watchdog timer has not fired in more than this time period, then reset the max triggers counters. Default is 10 minutes. |
| **Performance and watchdog options** | `-watchdog_minor_threshold` | If it has been longer than this period between watchdog executions count a minor trip. Default is 500. |
| **Performance and watchdog options** | `-watchdog_major_threshold` | If it has been longer than this period between watchdog executions count a major trip. Default is 3000. |
| **Performance and watchdog options** | `-watchdog_major_maxtriggers` | How many times the major watchdog timer can be tripped before the test is terminated. Default is 10. |
| **Performance and watchdog options** | `-watchdog_minor_maxtriggers` | How many times the minor watchdog timer can be tripped before the test is terminated. Default is 120. |
| **Tracing, logging and statistics options** | `-f` | Set the statistics report frequency on screen. Default is 1 and default unit is seconds. |
| **Tracing, logging and statistics options** | `-trace_stat` | Dumps all statistics in `<scenario_name>_<pid>.csv` file. Use the '-h stat' option for a detailed description of the statistics file content. |
| **Tracing, logging and statistics options** | `-stat_delimiter` | Set the delimiter for the statistics file |
| **Tracing, logging and statistics options** | `-stf` | Set the file name to use to dump statistics |
| **Tracing, logging and statistics options** | `-fd` | Set the statistics dump log report frequency. Default is 60 and default unit is seconds. |
| **Tracing, logging and statistics options** | `-rfc3339` | Use timestamps in RFC3339 format. |
| **Tracing, logging and statistics options** | `-periodic_rtd` | Reset response time partition counters each logging interval. |
| **Tracing, logging and statistics options** | `-trace_msg` | Displays sent and received SIP messages in `<scenario file name>_<pid>_messages.log` |
| **Tracing, logging and statistics options** | `-message_file` | Set the name of the message log file. |
| **Tracing, logging and statistics options** | `-message_overwrite` | Overwrite the message log file (default true). |
| **Tracing, logging and statistics options** | `-trace_shortmsg` | Displays sent and received SIP messages as CSV in `<scenario file name>_<pid>_shortmessages.log` |
| **Tracing, logging and statistics options** | `-shortmessage_file` | Set the name of the short message log file. |
| **Tracing, logging and statistics options** | `-shortmessage_overwrite` | Overwrite the short message log file (default true). |
| **Tracing, logging and statistics options** | `-trace_counts` | Dumps individual message counts in a CSV file. |
| **Tracing, logging and statistics options** | `-trace_err` | Trace all unexpected messages in `<scenario file name>_<pid>_errors.log`. |
| **Tracing, logging and statistics options** | `-error_file` | Set the name of the error log file. |
| **Tracing, logging and statistics options** | `-error_overwrite` | Overwrite the error log file (default true). |
| **Tracing, logging and statistics options** | `-trace_error_codes` | Dumps the SIP response codes of unexpected messages to `<scenario file name>_<pid>_error_codes.log`. |
| **Tracing, logging and statistics options** | `-trace_calldebug` | Dumps debugging information about aborted calls to `<scenario_name>_<pid>_calldebug.log` file. |
| **Tracing, logging and statistics options** | `-calldebug_file` | Set the name of the call debug file. |
| **Tracing, logging and statistics options** | `-calldebug_overwrite` | Overwrite the call debug file (default true). |
| **Tracing, logging and statistics options** | `-trace_screen` | Dump statistic screens in the `<scenario_name>_<pid>_screens.log` file when quitting SIPp. Useful to get a final status report in background mode (-bg option). |
| **Tracing, logging and statistics options** | `-screen_file` | Set the name of the screen file. |
| **Tracing, logging and statistics options** | `-screen_overwrite` | Overwrite the screen file (default true). |
| **Tracing, logging and statistics options** | `-trace_rtt` | Allow tracing of all response times in `<scenario file name>_<pid>_rtt.csv`. |
| **Tracing, logging and statistics options** | `-rtt_freq` | freq is mandatory. Dump response times every freq calls in the log file defined by -trace_rtt. Default value is 200. |
| **Tracing, logging and statistics options** | `-trace_logs` | Allow tracing of `<log>` actions in `<scenario file name>_<pid>_logs.log`. |
| **Tracing, logging and statistics options** | `-log_file` | Set the name of the log actions log file. |
| **Tracing, logging and statistics options** | `-log_overwrite` | Overwrite the log actions log file (default true). |
| **Tracing, logging and statistics options** | `-ringbuffer_files` | How many error, message, shortmessage and calldebug files should be kept after rotation? |
| **Tracing, logging and statistics options** | `-ringbuffer_size` | How large should error, message, shortmessage and calldebug files be before they get rotated? |
| **Tracing, logging and statistics options** | `-max_log_size` | What is the limit for error, message, shortmessage and calldebug file sizes. |
| **Signal handling** | `USR1` | Similar to pressing the 'q' key. It triggers a soft exit of SIPp. No more new calls are placed and all ongoing calls are finished before SIPp exits.<br>Example: `kill -SIGUSR1 732` |
| **Signal handling** | `USR2` | Triggers a dump of all statistics screens in `<scenario_name>_<pid>_screens.log` file. Especially useful in background mode to know what the current status is.<br>Example: `kill -SIGUSR2 732` |
| **Exit codes** | `0` | All calls were successful |
| **Exit codes** | `1` | At least one call failed |
| **Exit codes** | `97` | Exit on internal command. Calls may have been processed |
| **Exit codes** | `99` | Normal exit without calls processed |
| **Exit codes** | `253` | RTP validation failure |
| **Exit codes** | `-1` | Fatal error |
| **Exit codes** | `-2` | Fatal error binding a socket |

<br><br>

# SIPp install

<br>

## Rocky9 build

<br>

```bash
sudo dnf update -y
sudo dnf install -y epel-release
sudo dnf config-manager --set-enabled crb

# Install essential build tools and libraries.
sudo dnf install -y git cmake make gcc-c++ \
    ncurses-devel \
    openssl-devel \
    libpcap-devel \
    lksctp-tools-devel \
    gsl-devel

# Change working directory and clone source
cd /usr/local/src
sudo git clone https://github.com/SIPp/sipp.git
cd sipp

# Initialize submodules (synchronize necessary files such as GTest)
sudo git submodule update --init

# TGenerate a Makefile including TLS, PCAP, SCTP, and GSL functionality.
sudo cmake . -DUSE_PCAP=1 -DUSE_SCTP=1 -DUSE_GSL=1
# compile
sudo make

# Copy to the /usr/local/bin directory so that it can be used regardless of the path.
cp sipp /usr/local/bin
```

<br>

## Rocky8 build

<br>

```bash
sudo dnf update -y
sudo dnf install -y epel-release
udo dnf config-manager --set-enabled powertools

# Install essential build tools and libraries.
sudo dnf install -y git cmake make gcc gcc-c++ \
    ncurses-devel \
    openssl-devel \
    libpcap-devel \
    lksctp-tools-devel \
    gsl-devel

cat <<EOF | sudo tee /usr/lib64/pkgconfig/sctp.pc
prefix=/usr
exec_prefix=\${prefix}
libdir=/usr/lib64
includedir=/usr/include

Name: SCTP
Description: SCTP library
Version: 1.0
Libs: -L\${libdir} -lsctp
Cflags: -I\${includedir}
EOF

export PKG_CONFIG_PATH=/usr/lib64/pkgconfig:$PKG_CONFIG_PATH
pkg-config --libs sctp


# Change working directory and clone source
cd /usr/local/src
sudo git clone https://github.com/SIPp/sipp.git
cd sipp

# Initialize submodules (synchronize necessary files such as GTest)
sudo git submodule update --init

# TGenerate a Makefile including TLS, PCAP, SCTP, and GSL functionality.
sudo cmake . -DUSE_PCAP=1 -DUSE_SCTP=1 -DUSE_GSL=1  -DCMAKE_EXE_LINKER_FLAGS="-lsctp -lgsl -lgslcblas"
# compile
sudo make

# Copy to the /usr/local/bin directory so that it can be used regardless of the path.
cp sipp /usr/local/bin
```

<br><br>

# Testing SIPp in a NAT environment

<br>

Most VoIP devices today operate in NAT environments. Therefore, we will create a scenario in which a client (PhonerLite) located in a NAT environment places a call to SIPp, which is also situated in a NAT environment.

Unlike softswitches such as FreeSWITCH and Asterisk, SIPp does not fully support NAT environments. Therefore, it is advisable to configure the endpoint (PhonerLite) located behind NAT to use a STUN server to identify and utilize its public IP address.

<br>

![config](./image/1.png)

*Figure 1: Test Call Flow*


<br>

## PhonerLite in a NAT environment

<br>

Configure the Google STUN server in the PhonerLite configuration settings under "Server," as shown in the image below.
When a STUN server is configured, the softphone no longer uses a local private IP address but instead uses the public IP address obtained from the STUN server.

<br>

![config](./image/2.png)

*Figure 2: Configuring a STUN server*

<br>

### make a PhonerLite phonebook item

<br>

To facilitate SIPp connection testing, add an entry to the PhoneBook as follows.
SIPp will listen on port 5080, and the server address (X.X.139.21) is the public IP of the host where SIPp is running.

<br>

![config](./image/3.png)


<br>

## SIPp in a NAT environment

<br>

<br>

### Preparing Audio

<br>

SIPp lacks an audio streaming engine and codecs.
The reason for this is easily inferred: SIPp is designed for high-volume call processing tests. Consequently, it maximizes processing performance by focusing on high-volume signaling handling and avoiding the use of codecs or audio streaming engines.

To stream audio, SIPp requires a pcap file containing captured RTP network packets.
The RTP pcap file holds pre-encoded audio, and the stored RTP packets include timestamps indicating when they were sent. Streaming is performed by utilizing these time intervals. It is an intelligent architecture that processes RTP while minimizing CPU usage.
Therefore, it is advisable to pre-generate the audio source to be played—in the form of a pcap file—for use when the call connects.

If you have audio files such as MP3 or WAV, you can create a pcap file using ffmpeg and tcpdump as follows.


```bash
# First console. 
# It stores data arriving on the local interface via UDP port 10000.
sudo tcpdump -i lo -n -w music_pcmu.pcap udp port 10000

# Second console
# Play the `bg.wav` file using ffmpeg, but fix the sample size to 160 bytes to match the packet size.
# 160 bytes corresponds to the audio size for a `ptime` of 20 when using the G.711 codec.
ffmpeg -re -i bg.wav -vn -af asetnsamples=160 -acodec pcm_mulaw -ar 8000 -ac 1 -f rtp rtp://127.0.0.1:10000

```

By stopping tcpdump after the ffmpeg playback finishes, you can create a pcap file containing the audio data. While `pcm_mulaw` is specified as the codec for ffmpeg playback, specifying `pcm_alaw` instead allows you to create a pcap file encoded in A-law.


<br>

### Preparing Scenarion Xml 

<br>

SIPp uses XML for its default scenarios. Create the XML as shown below.
This XML includes information on the audio file to be played upon connection (the file created earlier) and codec details.
It also contains the public IP address (X.X.139.21) of the host where SIPp is running. When SIPp operates in a NAT environment, private IP information is not required.
If SIPp interacts with a device located on the same host or within the internal network, use a private IP address instead of a public IP address.


<br>

```xml
<?xml version="1.0" encoding="UTF-8"?>
<scenario name="UAS Loop Media">

  <!-- 1. Receive initial INVITE -->
  <recv request="INVITE" />

  <!-- 2. Send initial 200 OK -->
  <send retrans="500">
    <![CDATA[
      SIP/2.0 200 OK
      [last_Via:]
      [last_From:]
      [last_To:];tag=[pid]SIPpTag[call_number]
      [last_Call-ID:]
      [last_CSeq:]
      Contact: <sip:X.X.139.21:[local_port]>
      Content-Type: application/sdp
      Content-Length: [len]

      v=0
      o=user1 53655765 2353687637 IN IP4 X.X.139.21
      s=SIPp
      c=IN IP4 X.X.139.21
      t=0 0
      m=audio [auto_media_port] RTP/AVP 0
      a=rtpmap:0 PCMU/8000
    ]]>
  </send>

  <!-- 3. Receive initial ACK -->
  <recv request="ACK" />

  <!-- Start media playback loop -->
  <label id="play_media" />
  <nop>
    <action>
      <exec play_pcap_audio="/usr/local/src/sipp/pcap/music_pcmu.pcap"/>
    </action>
  </nop>

  <!-- 4. Wait for BYE. If Re-INVITE arrives, branch to 'handle_reinvite' -->
  <recv request="INVITE" optional="true" next="handle_reinvite" />
  <recv request="BYE" timeout="10000" ontimeout="play_media" next="call_end" />

  <!-- 5. Normal call termination -->
  <label id="call_end"/>
  <send next="end_scenario">
</scenario>

```


<br>

### Preparing SIPp command

<br>

Prepare the SIPp command as follows.

<br>

``` bash
# sipp -sf uas_media.xml -i [local ip] -mi [local ip] -p [sip port] -mp [local media port]

sipp -sf uas_media.xml -i 10.10.10.10 -mi l10.10.10.10 -p 5080 -mp 10000

```


<br>

## Test

<br>

Everything is now ready. Run SIPp as follows. Then, using PhoneLite on your PC, place a call to the address in the phonebook you created earlier.

<br>

```bash
sipp -sf uas_media.xml -i 10.10.10.10 -mi 10.10.10.10 -p 5080 -mp 100001:10000

```

If there are no issues with settings such as the firewall, the audio file you created (bg.wav) will play on your PC. And the call information will appear on the SIPp screen as follows.

```bash
------------------------------ Scenario Screen -------- [1-9]: Change Screen --
  Port   Total-time  Total-calls  Transport
  5080      91.15 s            2  UDP

  0 new calls during 1.001 s period	  1 ms scheduler resolution
  1 calls                                 Peak was 1 calls, after 53 s
  0 Running, 3 Paused, 3 Woken up
  0 dead call msg (discarded)
  3 open sockets                          0/0/0 UDP errors (send/recv/cong)
  768 Total RTP pckts sent                0.000 last period RTP rate (kB/s)

                                 Messages  Retrans   Timeout   Unexpected-Msg
0 :  ----------> INVITE             2         0         0         0
1 :  <---------- 200                2         4         0
2 :  ----------> ACK                1         0         0         0
3 :              [ NOP ]
4 :  ----------> INVITE             0         0         0         0
5 :  ----------> BYE                1         0         0         0
6 :  <---------- 200                1         0
7 :  <---------- 200                0         0
8 :  ----------> ACK                0         0         0         0
------------------------------ SIPp Server Mode -------------------------------

```

<br>

If you use sngrep to inspect SIP packets, the call should be established as follows for it to be considered normal. The INVITE message will display information as shown below. The boxed area indicates the part where the address was changed to a public IP due to the use of a STUN server.

![config](./image/4.png)

And if you terminate the call, you can also verify the handling of the BYE message.

<br>

![config](./image/5.png)

<br><br>

# Tips

<br>

## Handling Public IP Variables

<br>

In the previous XML file, the public IP address was entered directly in the format X.X.139.21.
However, this approach lacks scalability and is inconvenient, as the XML file must be modified every time the host being tested changes.

You can use the keyword `public_ip` in the XML file and assign a value to it in the `sipp` command.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<scenario name="UAS Loop Media">

  <!-- 1. Receive initial INVITE -->
  <recv request="INVITE" />

  <!-- 2. Send initial 200 OK -->
  <send retrans="500">
    <![CDATA[
      SIP/2.0 200 OK
      [last_Via:]
      [last_From:]
      [last_To:];tag=[pid]SIPpTag[call_number]
      [last_Call-ID:]
      [last_CSeq:]
      Contact: <sip:[public_ip]:[local_port]>
      Content-Type: application/sdp
      Content-Length: [len]

      v=0
      o=user1 53655765 2353687637 IN IP4 [public_ip]
      s=SIPp
      c=IN IP4 [public_ip]
      t=0 0
      m=audio [auto_media_port] RTP/AVP 0
      a=rtpmap:0 PCMU/8000
    ]]>
  </send>

  <!-- 3. Receive initial ACK -->
  <recv request="ACK" />

  <!-- Start media playback loop -->
  <label id="play_media" />
  <nop>
    <action>
      <exec play_pcap_audio="/usr/local/src/sipp/pcap/music_pcmu.pcap"/>
    </action>
  </nop>

  <!-- 4. Wait for BYE. If Re-INVITE arrives, branch to 'handle_reinvite' -->
  <recv request="INVITE" optional="true" next="handle_reinvite" />
  <recv request="BYE" timeout="10000" ontimeout="play_media" next="call_end" />

  <!-- 5. Normal call termination -->
  <label id="call_end"/>
  <send next="end_scenario">
</scenario>

```

<br>

When executing the command, use the -key option to inject the public IP value into the public_ip variable. You can assign a value to `public_ip` in the `sipp` command as follows.

```bash
sipp -sf uas_media.xml -key public_ip X.X.139.21 -i 10.10.10.10 -mi 10.10.10.10 -p 5080 -mp 100001:10000

```

<br>

## Add IP Authentication

<br>

If you run SIPp as a daemon program, IP authentication for incoming trunk calls may be required.
In this case, you can add the following ACL information to the XML file using a regular expression.
The XML below uses a regular expression to validate the IP address; if the condition is met, it sends a ring signal, and otherwise, it terminates the call.

```xml
<!-- Receive INVITE and check IP -->
<recv request="INVITE">
  <action>
    <!-- Check if the allowed IP (e.g., 192.168.1.50) exists in the Via header and save the result to the 'ip_matched' variable -->
    <ereg regexp="192\.168\.1\.50" search_in="hdr" header="Via:" assign_to="ip_matched"/>
  </action>
</recv>
<!-- If the regular expression matches (allowed IP), jump to the 'accept_invite' label -->

<nop next="accept_invite" test="ip_matched"/>
<!-- If the regular expression does not match (disallowed IP), respond with 403 Forbidden -->
<send>
  <![CDATA[
    SIP/2.0 403 Forbidden
    [last_Via:]
    [last_From:]
    [last_To:];tag=[pid]SIPpTag01[call_number]
    [last_Call-ID:]
    [last_CSeq:]
    Content-Length: 0
  ]]>
</send>
<!-- Jump to the call termination label -->
<nop next="end_call"/>

 <!-- === Allowed IP processing area === -->

<label id="accept_invite"/>
<send>
  <![CDATA[
    SIP/2.0 180 Ringing
    [last_Via:]
    [last_From:]
    [last_To:];tag=[pid]SIPpTag01[call_number]
    [last_Call-ID:]
    [last_CSeq:]
    Content-Length: 0
  ]]>
</send>
```



