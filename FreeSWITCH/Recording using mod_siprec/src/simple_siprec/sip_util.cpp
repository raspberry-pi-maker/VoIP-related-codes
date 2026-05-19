#include "global.h"
#include "epoll.h"
#include "util.h"

using namespace tinyxml2;


void *to_pcm_thread(int codec, const char *data, const char *data2);
SipRecCall_Factory g_call_factory;
/*
"audio 21560 RTP/AVP 0";
*/
int SDP::get_port_codec_from_media(const std::string &medialine, int *port, int *codec)
{
    std::istringstream iss(medialine);
    std::string av;
    std::string protocol;
    iss >> av >> *port >> protocol >> *codec;
    //std::cout << "Port: " << *port << std::endl;
    //std::cout << "Payload: " <<  *codec << std::endl;  
    return 0;
}

MediaInfo::MediaInfo(int p, int c, int sock)
{
  port = p;
  codec = c;
  rtp_sock = sock;
  src_port[0] = src_port[1] = 0;
  fh[0] = fh[1] = nullptr;

}

int MediaInfo::start_recording()
{
  for(int x = 0; x < 2; x++)
  {
    filename[x] += ("_" + std::to_string(x) + ".raw");
    fh[x] = fopen(filename[x].c_str(), "w+"); //Open the recording file.
  }
  return 0;
}
int MediaInfo::stop_recording()
{
  for(int x = 0; x < 2; x++){
    if (fh[x]){
      fclose(fh[x]);
      fh[x] = nullptr;
      std::cout << "End of recording" << std::endl;
    }

  }
  return 0;
}

std::string SIPMessage::getHeaderValue(const std::string& key) const {
    auto it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return ""; // return empty string if not found
}

SIPMessageType SIPMessage::detectSIPMessageType() {
    if (startLine.rfind("SIP/2.0", 0) == 0) {
        return SIPMessageType::Response;
    }

    // Common SIP methods
    const std::vector<std::string> methods = {
        "INVITE", "ACK", "BYE", "CANCEL", "REGISTER",
        "OPTIONS", "PRACK", "SUBSCRIBE", "NOTIFY",
        "PUBLISH", "INFO", "REFER", "MESSAGE", "UPDATE"
    };

    for (const auto& m : methods) {
        if (startLine.rfind(m, 0) == 0) {
            return SIPMessageType::Request;
        }
    }

    return SIPMessageType::Unknown;
}


long long SIPMessage::get_CSeq_number()
{
  std::string cseq = getHeaderValue("CSeq");  // Example: "113687289 INVITE"
  std::stringstream ss(cseq);
  std::string seq_num;
    
  // Read only the first token (before space)
  ss >> seq_num;
    
  return std::stoll(seq_num);  
}

std::string SIPMessage::getSIPMethod(std::string& code, std::string& description) {
    code = "";
    description = "";

    // 1. Response message parsing (SIP/2.0 200 OK)
    if (startLine.rfind("SIP/2.0", 0) == 0) {
        std::istringstream iss(startLine);
        std::string sip_version;
        
        iss >> sip_version >> code; // code like "200"
        std::getline(iss >> std::ws, description); // description like "OK"

        // Response messages must get original Method name from CSeq header
        std::string cseq = getHeaderValue("CSeq");
        if (!cseq.empty()) {
            std::istringstream iss_cseq(cseq);
            std::string seq, method;
            iss_cseq >> seq >> method;
            return method; // "INVITE", "OPTIONS", etc.
        }
        return "";
    }

    // 2. Request message parsing (INVITE sip:...)
    const std::vector<std::string> methods = {
        "INVITE", "ACK", "BYE", "CANCEL", "REGISTER",
        "OPTIONS", "PRACK", "SUBSCRIBE", "NOTIFY",
        "PUBLISH", "INFO", "REFER", "MESSAGE", "UPDATE"
    };

    for (const auto& m : methods) {
        if (startLine.rfind(m, 0) == 0) {
            return m; 
        }
    }

    return ""; 
}  

SDP SIPMessage::parseSDP() {
    SDP sdp;
    std::istringstream stream(body);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.rfind("v=", 0) == 0) {
            sdp.version = line.substr(2);
        } else if (line.rfind("o=", 0) == 0) {
            sdp.origin = line.substr(2);
        } else if (line.rfind("s=", 0) == 0) {
            sdp.sessionName = line.substr(2);
        } else if (line.rfind("c=", 0) == 0) {
            sdp.connection = line.substr(2);
        } else if (line.rfind("t=", 0) == 0) {
            sdp.timing = line.substr(2);
        } else if (line.rfind("m=", 0) == 0) {
            sdp.media.push_back(line.substr(2));
        } else if (line.rfind("a=", 0) == 0) {
            sdp.attributes.push_back(line.substr(2));
        }
    }
    return sdp;
}
 

int SIPMessage::getPayloadType(const std::string& attrLine) {
    // Example: "rtpmap:8 PCMA/8000"
    std::istringstream iss(attrLine);
    std::string prefix;
    int payload;

    iss >> prefix;
    size_t pos = prefix.find(':');
    if (pos != std::string::npos) {
        payload = std::stoi(prefix.substr(pos + 1));
        return payload;
    }
    return -1; // return -1 if failed
}

std::string SIPMessage::extract_body_line(const std::string& key)
{
    std::istringstream iss(body);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find(key) != std::string::npos) {
            return line; // return first line containing keyword
        }
    }
    return "";
}

std::string SIPMessage::extract_extension( const std::string& header)
{
    std::regex extension_regex(R"(sip:([^@]+)@)");
    std::smatch match;
    if (std::regex_search(header, match, extension_regex)) {
        return match[1]; // Captured extension number
    }
    return "";  
}
std::string SIPMessage::extract_extension_from_sip()
{
    std::string header = getHeaderValue("From");
    return extract_extension(header);

}
std::string SIPMessage::extract_extension_to_sip()
{
    std::string header = getHeaderValue("To");
    return extract_extension(header);
}

std::string SIPMessage::extract_extension_from_startLine()
{
    std::regex extension_regex(R"(^BYE\s+(?:sip:)?([^@\s]+)@)", std::regex_constants::icase);
    std::smatch match;

    if (std::regex_search(startLine, match, extension_regex)) {
        return match[1].str();
    }
    return "";
}

/*
INVITE sip:10.9.172.5:5070 SIP/2.0
*/
std::string SIPMessage::extract_domain_from_startLine()
{
    std::regex re(R"(sip:(?:[^@]+@)?([^ ]+))");
    std::smatch match;

    if (std::regex_search(startLine, match, re)) {
      return match[1];
    } else {
    }
    return "";
}

std::string SIPMessage::extract_uri(const std::string& sip_header)
{
    std::regex re(R"(@([^>;\s]+))");
    std::smatch match;

    if (std::regex_search(sip_header, match, re)) {
        std::string ip_address = match[1];
        return ip_address;
    } else {
        return "";
    }

    return "";
}

std::string SIPMessage::extract_uri_from_sip()
{
    std::string val = getHeaderValue("From");
    return extract_uri(val);
}

std::string SIPMessage::extract_uri_to_sip()
{
    std::string val = getHeaderValue("To");
    return extract_uri(val);
}

std::string SIPMessage::get_full_message()
{
  std::string msg = startLine + "\r\n";
  for (const auto& pair : headers) {
    msg += (pair.first + ": " + pair.second + "\r\n");
  } 
  msg += (body + "\r\n");
  return msg;  
}

SipRecCall::SipRecCall() : pMsg(nullptr)
{
}
SipRecCall::SipRecCall(SIPMessage *msg):SipRecCall() 
{
  pMsg = msg;
}

SipRecCall::~SipRecCall()
{
  for (auto& m : media_list) {
    if (m.fh[0] && m.fh[1]){
      m.stop_recording();
      int codec = m.codec;
      std::string fname[2];
      fname[0] = m.filename[0];
      fname[1] = m.filename[1];
      std::thread p_thread([codec, fname](){
          to_pcm_thread(codec, fname[0].c_str(), fname[1].c_str());   // epoll_thread function
      });
      p_thread.detach();      
    }
  }
  delete pMsg;
}

int SipRecCall::start_recording()
{
  int codec = 0;
  int index = 0;

  for (auto& m : media_list) {
    m.filename[0] = m.filename[1] = pMsg->getHeaderValue("Call-ID") + "_" + std::to_string(index); 
    index ++;
    m.start_recording();
    
  }
  return 0;
}
int SipRecCall::stop_recording()
{
  for (auto& m : media_list) {
    m.stop_recording();
  }

  return 0;
}

int SipRecCall::record(const unsigned char *msg_buf, int count)
{
  return 0;
}


SipRecCall *SipRecCall_Factory::find_call(const std::string &cid)
{
    auto it = calls.find(cid);
    if (it != calls.end()) {
        return it->second;
    }
    return nullptr;   
}

SipRecCall *SipRecCall_Factory::find_call(int sock)
{
  for (auto& kv : calls) {
    std::string c = kv.first;
    SipRecCall *call = kv.second;

    for (const auto& m : call->media_list) {
      if (m.rtp_sock == sock) return call;
    }
  }
  return nullptr;   
}

MediaInfo *SipRecCall_Factory::find_mediainfo(int sock)
{
  SipRecCall *call = find_call(sock);
  if(!call) return nullptr;
  
  for (auto& m : call->media_list) {
    m.rtp_sock = sock;
    return &m;
  }
  return nullptr;   
}


bool SipRecCall_Factory::remove_call(const std::string &cid)
{
  for (auto& kv : calls) {
    std::string c = kv.first;
    SipRecCall *call = kv.second;
    
    if(c == cid){
      delete call;
      calls.erase(kv.first);
      std::cout << "CID:" << cid << " Call termination completed" << std::endl;
      return true;  
    }    
  }
   std::cout << " CID:" << cid << " Not found in call list" << std::endl;
  return false;
}

bool SipRecCall_Factory::remove_call(SipRecCall *call)
{

  for (auto& kv : calls) {
    SipRecCall *c = kv.second;
    
    if(c == call){
      delete c;
      calls.erase(kv.first);
      return true;  
    }    
  }
  return false;
}

std::string make_siprec_bye_ok(SIPMessage *pMsg)
{
  std::string ok("SIP/2.0 200 OK\r\n");
  ok += ("Via: " + pMsg->getHeaderValue("Via") + "\r\n");  
  ok += ("From: " + pMsg->getHeaderValue("From") + "\r\n");  
  ok += ("To: " + pMsg->getHeaderValue("To") + "\r\n");  
  ok += ("Call-ID: " + pMsg->getHeaderValue("Call-ID") + "\r\n");  
  ok += ("CSeq: " + pMsg->getHeaderValue("CSeq") + "\r\n");  
  ok += "Content-Length: 0\r\n\r\n";
  return ok;
}

std::string make_siprec_invite_err(SIPMessage *pMsg)
{
  std::string err("SIP/2.0 488 Not Acceptable Here\r\n");
  err += ("Via: " + pMsg->getHeaderValue("Via") + "\r\n");  
  err += ("From: " + pMsg->getHeaderValue("From") + "\r\n");  
  err += ("To: " + pMsg->getHeaderValue("To") + "\r\n");  
  err += ("Call-ID: " + pMsg->getHeaderValue("Call-ID") + "\r\n");  
  err += ("CSeq: " + pMsg->getHeaderValue("CSeq") + "\r\n");  
  err += "Content-Length: 0\r\n\r\n";
  return err;  
}

std::string make_siprec_invite_ok(SIPMessage *pMsg, const std::vector<MediaInfo> &info)
{
  uuid_t uuid;
  char uuid_str[37]; // UUID
  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_str);
  
  SDP sdp = pMsg->parseSDP();
  std::string domain = pMsg->extract_domain_from_startLine();  
  std::string ok("SIP/2.0 200 OK\r\n");
  ok += ("Via: " + pMsg->getHeaderValue("Via") + "\r\n");  
  ok += ("From: " + pMsg->getHeaderValue("From") + "\r\n");  
  ok += ("To: " + pMsg->getHeaderValue("To") + ";tag=" + static_cast<std::string>(uuid_str) +"\r\n");  
  ok += ("Call-ID: " + pMsg->getHeaderValue("Call-ID") + "\r\n");  
  ok += ("CSeq: " + pMsg->getHeaderValue("CSeq") + "\r\n");  
  ok += ("Contact: <sip:srs@" + domain + ">\r\n");  
  ok += ("Content-Type: application/sdp\r\n");  

  //  IPv4 extract
  std::regex re(R"((\d{1,3}(?:\.\d{1,3}){3}))");
  std::smatch match;
  std::string ip(domain);
  if (std::regex_search(domain, match, re)) {
      ip = match[1];  
  }

  std::string content("");
  content += ("v=" + sdp.version + "\r\n");  
  content += ("o=srs 1234 5678 IN IP4 " + ip + "\r\n");  
  content += ("s=Siprec\r\n");

  content += ("c=IN IP4 " + ip + "\r\n");
  
  content += ("t=" + sdp.timing + "\r\n");  
  /*
  content += ("m=audio 0 RTP/AVP 0\r\n");  
  content += ("m=audio 0 RTP/AVP 0\r\n\r\n");  
  */
  std::string buf;
  for (size_t i = 0; i < info.size(); ++i) {
    buf = "m=audio " + std::to_string(info[i].port) + " RTP/AVP " +  std::to_string(info[i].codec) + "\r\n";
    content += buf;
    if(info[i].codec == 0){
      content += ("a=rtpmap:0 PCMU/8000\r\n");  
    }
    else{
      content += ("a=rtpmap:8 PCMA/8000\r\n");  
    }
  }
  
  ok += ("Content-Length: " + std::to_string(content.length()) + "\r\n\r\n");  
  ok += content;
  return ok;
}

// ... (rest of the code unchanged except LogToFile → std::cout)

int do_siprec_message(const char *msg_buf)
{
  std::string code(""), description("");
  SIPMessage *pMsg = parseSIPMessage(msg_buf);
  std::string method = pMsg->getSIPMethod(code, description);
  std::string to = pMsg->extract_extension_to_sip();
  std::string from = pMsg->extract_extension_from_sip();
  std::string cid = pMsg->getHeaderValue("Call-ID");

  if (method == "INVITE" && code == ""){  // Received INVITE
    //std::cout << "SIPREC INVITE message\n" << msg_buf << std::endl;
    SDP sdp = pMsg->parseSDP();
    std::string xml("");
    // Find XML start and end positions
    size_t xml_start = pMsg->body.find("<recording");
    size_t xml_end = pMsg->body.find("</recording>");
    if (xml_start != std::string::npos && xml_end != std::string::npos) {
      xml = pMsg->body.substr(xml_start, xml_end - xml_start + 12);
    } else {
      std::cout << "XML not found." << std::endl;
      delete pMsg;
      return 0;
    }
    XMLDocument doc;
    doc.Parse(xml.c_str());
    XMLElement* root = doc.FirstChildElement("recording");
    if (!root) {
        std::cerr << "XML parsing failed." << std::endl;
        return 1;
    }
    XMLElement* session = root->FirstChildElement("session");
    for (XMLElement* participant = session->FirstChildElement("participant");
        participant != nullptr;
        participant = participant->NextSiblingElement("participant")) {

        const char* id = participant->Attribute("participant_id");
        const char* send = participant->FirstChildElement("send")->GetText();
        std::cout << "Participant: " << id << ", Stream: " << send << std::endl;
    }

    SipRecCall *call = new SipRecCall(new SIPMessage(*pMsg));
    for (const auto& item : sdp.media) {
      int current_port, current_codec;
      sdp.get_port_codec_from_media(item, &current_port, &current_codec);
      std::cout << "INVITE media:" << item << "  port:" << current_port << "  codec:" <<current_codec << std::endl;

      int rtp_sock = buildsocket(current_port + 1000);    //Make UDP socket dynamically and add to epoll
      if(rtp_sock <= 0){ //err
        std::cout << "RTP Socket bind error" << item << "  port:" << current_port + 1000  << std::endl;
      }
      MediaInfo minfo(current_port + 1000, current_codec, rtp_sock);
      
      call->media_list.push_back(minfo);
    }
    
    std::string response;
    response = make_siprec_invite_ok(pMsg, call->media_list);
    int ret = sendto(g_siprec_sock, response.c_str(), response.length(), 0, (struct sockaddr*)&(g_siprec_addr), g_siprec_addr_len);
    if(ret <= 0){
      std::cout << "Error sending SIPREC INVITE OK response [" << ret << "]" << std::endl;
    }
    call->start_recording();
    g_call_factory.calls[cid] = call;
    
    
  }
  else if (method == "BYE" && code == ""){  // Received BYE
    std::string response = make_siprec_bye_ok(pMsg);
    int ret = sendto(g_siprec_sock, response.c_str(), response.length(), 0, (struct sockaddr*)&(g_siprec_addr), g_siprec_addr_len);
    if(ret <= 0){
      std::cout << "Error sending SIPREC BYE OK response [" << ret << "]" << std::endl;
    }
    g_call_factory.remove_call(cid);
  }
  else if (method == "ACK" && code == ""){  // Received ACK
    // Nothing to do
  }
  
  delete pMsg;
  return 0;
}


SIPMessage *parseSIPMessage(const std::string& rawMessage) {
    SIPMessage *msg = new SIPMessage;
    std::istringstream stream(rawMessage);
    std::string line;
    bool firstLine = true;
    bool inBody = false;

    while (std::getline(stream, line)) {
        // Remove CRLF
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (firstLine) {
            msg->startLine = line;
            firstLine = false;
            continue;
        }

        if (line.empty()) {
            // After the blank line is the Body
            inBody = true;
            continue;
        }

        if (!inBody) {
            // 헤더 파싱
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                // Remove leading and trailing spaces
                while (!value.empty() && value.front() == ' ') value.erase(value.begin());
                msg->headers[key] = value;
            }
        } else {
            msg->body += line + "\n";
        }
    }

    //In the case of a Delayed Offer, an SDP may be included in the ACK.
    std::string code(""), description("");
    std::string method = msg->getSIPMethod(code, description);
    if (method == "INVITE" || method == "ACK" ){
      //debug_SDP(msg);
    }

    return msg;
}

void *to_pcm_thread(int codec, const char *data, const char *data2)
{
  //exec_command
  //sox -t al -r 8000 -c 1 f89eca5c-ce27-123f-8abb-d20d227d4ce8_0_0.raw -r 16000 -e signed-integer -b 16 output.wav
  std::string f[2];
  f[0] = data;
  f[1] = data2;
  
  std::string cmd("sox -t ");
  if(codec == 0 ) cmd += "au ";
  else cmd += "al ";
  cmd += "-r 8000 -c 1 ";
  
  std::string postfix(" -r 16000 -e signed-integer -b 16 ");
  
  for(int x = 0; x < 2; x++){
    std::string command = cmd + f[x] + postfix + f[x] + ".wav" ;
    exec_command(command.c_str());
    std::remove(f[x].c_str());
  }


  return nullptr;
}