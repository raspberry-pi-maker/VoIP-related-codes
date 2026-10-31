'''
간단하게 2000/Hello를 테스트하는 코드.
uv run uvicorn tests.simple_regi_test:app --host 0.0.0.0 --port 8097 --reload
'''

from fastapi import FastAPI, Request, Response

app = FastAPI()

# 1. 단말 등록 허용 XML 생성 (FreeSWITCH가 전달받은 비밀번호와 대조해 인증 처리)
def build_directory_xml(user: str, domain: str, password: str) -> str:
    return f"""<?xml version="1.0" encoding="utf-8"?>
<document type="freeswitch/xml">
  <section name="directory">
    <domain name="{domain}">
      <params>
        <param name="dial-string" value="{{^^:sip_invite_domain=${{dialed_domain}}}}user/{user}@${{dialed_domain}}"/>
      </params>
      <groups>
        <group name="default">
          <users>
            <user id="{user}">
              <params>
                <param name="password" value="{password}"/>
              </params>
              <variables>
                <variable name="user_context" value="default"/>
              </variables>
            </user>
          </users>
        </group>
      </groups>
    </domain>
  </section>
</document>"""

# 2. 등록 실패/인증 거절 시 반환할 XML (FreeSWITCH에게 사용자 없음/Not Found를 알림)
def build_not_found_xml() -> str:
    return """<?xml version="1.0" encoding="utf-8"?>
<document type="freeswitch/xml">
  <section name="directory">
    <result status="not found"/>
  </section>
</document>"""


@app.post("/fs/v1/directory")
async def directory(request: Request) -> Response:
    # mod_xml_curl은 application/x-www-form-urlencoded 형식으로 데이터를 보냅니다.
    form = await request.form()
    
    # section 확인 (directory 요청인지 검증)
    section = form.get("section")
    if section != "directory":
        return Response(content=build_not_found_xml(), media_type="application/xml")

    # 전화번호(단말 번호) 추출: FreeSWITCH 파라미터 중 user 또는 sip_auth_username
    phone = form.get("user") or form.get("sip_auth_username")
    domain = form.get("domain", "127.0.0.1")

    # 단말 번호가 2000인 경우 등록 허용 XML 반환
    if phone == "2000":
        # FreeSWITCH에 등록된 사용자 비밀번호(Hello)를 포함한 XML 응답
        xml_content = build_directory_xml(user="2000", domain=domain, password="Hello")
        return Response(content=xml_content, media_type="application/xml")

    # 번호가 일치하지 않으면 실패(Not Found) XML 반환
    return Response(content=build_not_found_xml(), media_type="application/xml")
