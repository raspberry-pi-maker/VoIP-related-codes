#include"global.h"
#include"epoll.h"
#include"sip_parser.h"
#include"channel.h"

vector<void *> g_chanels;

/*
find channel by socket file
*/
CHANNEL *find_channel_by_sock(int sock)
{
    CHANNEL *ch;
    for (void * i : g_chanels)
    {
        ch = (CHANNEL *)i;
        if(sock == ch->rx_sock || sock == ch->tx_sock) return ch;
    }    
    return NULL;
}

/*
find channel by extension number
*/
CHANNEL *find_channel_by_callid(const char *callid)
{
    CHANNEL *ch;
    for (void * i : g_chanels)
    {
        ch = (CHANNEL *)i;
        if(0 == strcmp(ch->call_id.c_str(), callid)) return ch;
    }    
    return NULL;
}


/*
There are always two BYE messages. 
*/
bool update_bye_channel(rtp_direction dir, const char *callid)
{
    CHANNEL *ch = find_channel_by_callid(callid);
    if(NULL == ch) {
        LogToFile(err, "update_bye_channel channel not found callid[%s] ", callid);
        return false;
    }
    if(dir_rx == dir){    //this BYE is rx 
        close(ch->rx_sock); //close rtp socket
        ch->rx_sock = 0;
        ch->rx_port = 0;
        fclose(ch->rx_file);
        LogToFile(info, "RX channel reset");
    }
    else{
        close(ch->tx_sock); //close rtp socket
        ch->tx_sock = 0;
        ch->tx_port = 0;
        fclose(ch->tx_file);
        LogToFile(info, "TX channel reset");
    }
    if(!ch->tx_sock && !ch->rx_sock){   //if 2 BYE message has received, remove channel
        CHANNEL *tch;
        int index;
        int size = g_chanels.size();    
        for (index = 0; index < size; index++) {
            tch = (CHANNEL *)g_chanels[index];
            if(tch == ch) break;    //compare memory pointer directly
            //if(0 == strcmp(tch->call_id.c_str(), callid)) break;
        }    
        g_chanels.erase(g_chanels.begin() + index);
        delete ch;
        LogToFile(info, "RX TX channel reset => remove channel");
    }
    return true;
}
/*
There are always two INVITE messages. 
Therefore, it first searches to see if there is a previously created channel.
*/
CHANNEL *update_invite_channel(rtp_direction dir, const char *callid, int port, int codec, const char *name)
{
    CHANNEL *ch = find_channel_by_callid(callid);
    
    if(NULL == ch){     //If no channel exists, first make it 
        // This might be the first INVITE message
        // Make and add CHANNEL
        ch = new CHANNEL();
        ch->call_id = (string)callid;
        g_chanels.push_back((void *)ch);
        LogToFile(info, "Create new channel");
    }
    
    char filename[MAX_PATH];
    if(dir_rx == dir){    //this INVITE is rx 
        ch->rx_port = port;
        ch->rx_sock = buildsocket(port);    //Make UDP socket dynamically and add to epoll
        ch->rx_payload = codec;
        ch->rx_name = (string)name;
        sprintf(filename, "./%s_rx.pcm", callid);
        ch->rx_file = fopen(filename, "wb");
        LogToFile(info, "RX[%s] callid[%s] start", name, callid);
    }
    else{      //this INVITE is tx 
        fprintf(stderr, "tx start\n");
        ch->tx_port = port;
        ch->tx_sock = buildsocket(port);    //Make UDP socket dynamically and add to epoll
        ch->tx_payload = codec;
        ch->tx_name = (string)name;
        sprintf(filename, "./%s_tx.pcm", callid);
        ch->tx_file = fopen(filename, "wb");
        LogToFile(info, "TX[%s] callid[%s] start", name, callid);
    }
    
    
    return ch;
}
