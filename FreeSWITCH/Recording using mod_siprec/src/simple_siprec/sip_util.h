#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <regex>

enum class SIPMessageType {
    Request,
    Response,
    Unknown
};

/*
 미디어 정보를 담을 구조체 정의
 현재 mod_siprec에서는 하나의 rtp 소켓으로 rx,tx 모두 보냄(발신 포트가 다름).
*/
class MediaInfo {
public:  
    int port;
    int src_port[2];
    int codec;
    std::string media_type; // 필요시 어떤 미디어인지 구분하기 위함
    int rtp_sock;
    std::string filename[2];
    FILE *fh[2]; //recording file
    int start_recording();
    int stop_recording();
    MediaInfo(int port, int codec, int rtp_sock);
};

struct SDP {
    std::string version;
    std::string origin;
    std::string sessionName;
    std::string connection;
    std::string timing;
    std::vector<std::string> media;
    std::vector<std::string> attributes;
    int get_port_codec_from_media(const std::string &medialine, int *port, int *codec);
};

struct SIPMessage {
    std::string startLine;                  // Request-Line 또는 Status-Line
    std::map<std::string, std::string> headers;
    std::string body;
    
    std::string getHeaderValue(const std::string& key) const;
    SIPMessageType detectSIPMessageType();
    SDP parseSDP();
    int getPayloadType(const std::string& attrLine);

    std::string extract_body_line(const std::string& key);
    long long get_CSeq_number();
    std::string getSIPMethod(std::string& code, std::string& description);
    std::string extract_extension(const std::string& head_value);
    std::string extract_extension_from_sip();
    std::string extract_extension_to_sip();
    std::string extract_extension_from_startLine();
    std::string extract_domain_from_startLine();
    
    std::string extract_uri(const std::string& sip_header);
    std::string extract_uri_from_sip();
    std::string extract_uri_to_sip();
    std::string get_full_message();
};

class SipRecCall {
public:  
  SIPMessage *pMsg;
  std::vector<MediaInfo> media_list;
  int start_recording();
  int stop_recording();
  SipRecCall();
  SipRecCall(SIPMessage *msg);
  ~SipRecCall(); 
  int record(const unsigned char *msg_buf, int count);  
  
};

struct SipRecCall_Factory {
  std::map<std::string, SipRecCall *> calls;
  MediaInfo *find_mediainfo(int sock);
  SipRecCall *find_call(const std::string &cid);
  SipRecCall *find_call(int sock);
  bool remove_call(const std::string &cid);
  bool remove_call(SipRecCall *call);

};

extern SipRecCall_Factory g_call_factory;

SIPMessage *parseSIPMessage(const std::string& rawMessage);

int do_siprec_message(const char *msg_buf);
std::string make_siprec_invite_ok(SIPMessage *pMsg, const std::vector<MediaInfo> &info);
std::string make_siprec_invite_err(SIPMessage *pMsg);
std::string make_siprec_bye_ok(SIPMessage *pMsg);
