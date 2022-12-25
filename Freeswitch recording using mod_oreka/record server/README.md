# record server

This program analyzes the SIP messages sent by Freeswitch's mod_oreka module, then receives the RTP packets and creates a recording file.
<br><br>

**prerequisite**

* osip : Used for parsing SIP messages.  Reference :  [SIP Protocol](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/SIP%20Protocol/SIP%20Protocol%20Alalyzing%20using%20osip, "osip link")

* audio conversion : Convert rtp packets of G.711 codec to pcm (wav)   Reference :  [Audio Conversion](https://github.com/raspberry-pi-maker/VoIP-related-codes/tree/main/audio%20conversion, "audio conversion link")
