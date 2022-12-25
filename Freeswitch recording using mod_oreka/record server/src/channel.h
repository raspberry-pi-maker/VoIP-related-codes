#pragma once
#include <sys/time.h> 
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "global.h"

using namespace std;

enum rtp_direction {
    dir_rx = 0,
    dir_tx
};

typedef struct
{
    string rx_name, tx_name;
    struct timeval phone_time;

    int tx_port, rx_port;
    int tx_sock, rx_sock;
    FILE *tx_file, *rx_file;    //FILE for audio file recording
    int rx_payload, tx_payload;    // 0 or 8(G.711)
    u_char my_ip[4], other_ip[4];
    string call_id;
    
} CHANNEL;

extern vector<void *> g_chanels;

CHANNEL *find_channel_by_sock(int sock);
CHANNEL *find_channel_by_callid(const char *callid);
CHANNEL *update_invite_channel(rtp_direction dir, const char *callid, int port, int codec, const char *name);
bool update_bye_channel(rtp_direction dir, const char *callid);

