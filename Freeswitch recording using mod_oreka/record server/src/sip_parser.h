#pragma once

int do_sip_message(const char *msg_buf);
int get_audioinfo(const char *pSDP, int &port, int &codec);
