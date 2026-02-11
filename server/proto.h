#pragma once

#ifdef SERVER_BUILD
    #include <mysql.h>
    extern MYSQL* conn;
    extern std::map<int, std::shared_ptr<ChatClient>> clients;
    extern std::mutex clients_mutex;
#endif

#include <map>          // std::map을 쓰기 위해 필요
#include <mutex>        // std::mutex를 쓰기 위해 필요
#include <memory>       // std::shared_ptr을 쓰기 위해 필요
#include <string>       // std::string을 쓰기 위해 필요
#include <nlohmann/json.hpp>


// std:: shared_ptr<ChatClient> client :메모리 관리를 사람이 직접 하지 않고 시스템이 자동으로 하게 만들기 위해서 사용
// -> molloc 을 대체하는 기능 : std:: shcared_prt : 공유되는 포인터
// -> client 들이 전부 같은곳을 바라보게한다

// std::map<int, shared_ptr> : 
// c와 다르게 접속자를 찾을 때 for문을 돌며 빈자리를 찾을 필요 없음. 소켓 번호만 알면 바로 접근 가능

// json::parse(): sscanf 대신 이 한 줄로 모든 데이터를 읽음. 
// 데이터 형식이 바뀌어도 json을 사용한다면 변경 필요 x.

 // inline : 헤더 파일을 여러 .cpp 파일에서 인클루드하면, 링크 단계에서 "중복 정의(Multiple Definition)" 에러가 발생. 각 .cpp 파일이 자신만의 함수 실행 파일을 만들기 때문에 중복방지 기능
 
class ChatClient 
{
    public:
    int sock;
    std::string user_id;
    std::string nickname;
    int room_id = -1;
    int state = 0;

    // 구현부 앞에 inline 추가
    inline ChatClient(int s) : sock(s) {}

    inline void update_from_json(const nlohmann::json& payload) 
    {
        if (payload.contains("id")) user_id = payload["id"];
        if (payload.contains("nickname")) nickname = payload["nickname"];
    }

    inline ~ChatClient()
    {
        if (sock != -1)
            ::close(sock);
        std::cout << "클라이언트 객체 소멸 (ID: " << user_id << ")\n";
    }
};



// 패킷 타입 정의 (JSON의 "type"에 들어갈 숫자)
#define PKT_LOGIN_REQ      1    // 로그인 요청
#define PKT_LOGIN_RES      101  // 로그인 응답

#define PKT_SIGNUP_REQ     2    // 회원가입 요청
#define PKT_SIGNUP_RES     102  // 회원가입 응답

#define PKT_CHAT_MSG       3    // 채팅 메시지 전송
#define PKT_ROOM_ENTER     4    // 채팅방 입장 요청
#define PKT_ROOM_EXIT      5    // 채팅방 퇴장 요청

// [Client State Numbers] 상태 
#define STATE_NONE         0    // 접속 직후 (미인증)
#define STATE_LOGIN        1    // 로그인 완료 (로비 상태)
#define STATE_CHAT         2    // 채팅방 내부
#define STATE_SETTING      3    // 개인설정 메뉴

// [Data Limits] 제한
#define MAX_USER_ID        20   // ID 최대 길이
#define MAX_NICKNAME       100  // 닉네임 최대 길이
#define BUFFER_SIZE        1024 // 통신 버퍼 크기

// 응답 결과 코드
#define RES_SUCCESS       "success"
#define RES_FAIL          "fail"
#define RES_DUPLICATE_ID   "duplicate_id"  // 아이디 중복 시
#define RES_DB_ERROR       "db_error"      // DB 서버 문제 시