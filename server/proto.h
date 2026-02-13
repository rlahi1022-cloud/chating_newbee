#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <unistd.h>
#include <nlohmann/json.hpp>

/* ============================================================
   서버 전용 설정 (SERVER_BUILD 정의 시 활성화)
   ============================================================ */
#ifdef SERVER_BUILD
    #include <mariadb/conncpp.hpp>
    class ChatClient;
    extern std::shared_ptr<sql::Connection> conn;
    extern std::map<int, std::shared_ptr<ChatClient>> clients;
    extern std::mutex clients_mutex;
#endif

/* ============================================================
    시스템 설정 및 제한
   ============================================================ */
#define SERVER_PORT     5010     // 현재 사용 중인 포트
#define MAX_CLIENT      100     // 접속제한
#define BUFFER_SIZE     1024    // 버퍼제한
#define MAX_USER_ID     20      // 아이디제한
#define MAX_NICKNAME    100     //닉네임길이제한

// 사용자 관리 (300번대)
#define PKT_SIGNUP_REQ     300   // 회원가입 요청
#define PKT_SIGNUP_RES     301   // 회원가입 응답
#define PKT_LOGIN_REQ      302   // 로그인 요청
#define PKT_LOGIN_RES      303   // 로그인 응답
#define PKT_ID_CHECK_REQ   304   // 아이디 중복 확인 요청
#define PKT_ID_CHECK_RES   305   // 아이디 중복 확인 응답
#define PKT_NICK_CHECK_REQ 306   // 닉네임 중복 확인 요청
#define PKT_NICK_CHECK_RES 307   // 닉네임 중복 확인 응답

// 개인 설정 및 상태 (310번대)
#define PKT_NICK_CHANGE_REQ 310  // 닉네임 변경 요청
#define PKT_NICK_CHANGE_RES 311  // 닉네임 변경 응답
#define PKT_SETTING_UPDATE_REQ 320 // 통합 설정 변경 요청
#define PKT_SETTING_UPDATE_RES 322 // 설정 변경 응답

// 채팅방  (100번대)
#define PKT_ROOM_LIST_REQ   102  // 채팅방 목록 요청
#define PKT_ROOM_LIST_RES   103  // 채팅방 목록 응답
#define PKT_ROOM_ENTER_REQ  104  // 채팅방 입장 요청
#define PKT_ROOM_ENTER_RES  105  // 채팅방 입장 응답
#define PKT_CHAT_MESSAGE    110  // 채팅 메시지 전송
#define PKT_ROOM_EXIT_REQ   112  // 채팅방 퇴장 요청
#define PKT_ROOM_EXIT_RES   113  // 채팅방 퇴장 응답
#define PKT_ROOM_CREATE_REQ   120   // 방 생성 요청
#define PKT_ROOM_CREATE_RES   121   // 방 생성 응답

// =========================
// 개인 메시지 (200번대)
// =========================
#define PKT_MSG_SEND_REQ     200 // 메시지 보내기 요청
#define PKT_MSG_SEND_RES     201 // 메시지 보내기 응답
#define PKT_MSG_LIST_REQ     202 // 내 메시지 목록 요청
#define PKT_MSG_LIST_RES     203 // 내 메시지 목록 응답
#define PKT_MSG_READ_REQ     204 // 메시지 읽기 요청
#define PKT_MSG_READ_RES     205 // 메시지 읽기 응답


/* ============================================================
   상태 및 결과 코드
   ============================================================ */
// [Client State]
#define STATE_NONE         0    // 접속 직후 (미인증)
#define STATE_LOBBY        1    // 로그인 완료 (로비 상태)
#define STATE_CHAT         2    // 채팅방 내부
#define STATE_SETTING      3    // 개인설정 메뉴

// [Result Strings]
#define RES_SUCCESS        "success"
#define RES_FAIL           "fail"
#define RES_DUPLICATE      "duplicate"
#define RES_AVAILABLE      "available"
#define RES_DB_ERROR       "db_error"

/* ============================================================
   클라이언트 세션 관리 클래스
   ============================================================ */
class ChatClient 
{
public:
    int sock;
    std::string user_id;
    std::string nickname;
    int room_id = -1;
    int state = STATE_NONE;

    inline ChatClient(int s) : sock(s) {}

    inline void update_from_json(const nlohmann::json& payload) 
    {
        if (payload.contains("id"))
            user_id = payload["id"].get<std::string>();

        if (payload.contains("nickname"))
            nickname = payload["nickname"].get<std::string>();
    }

    inline ~ChatClient()
    {
        if (sock != -1) 
        {
            ::close(sock);
        }
        // 서버 로그용 (사용자 ID가 있을 때만 출력)
        if (!user_id.empty()) 
        {
            std::cout << "클라이언트 객체 소멸 (ID: " << user_id << ")\n";
        }
    }
};