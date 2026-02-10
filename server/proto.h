#pragma once

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