#pragma once

// 패킷 타입 정의 (JSON의 "type"에 들어갈 숫자)
#define PKT_LOGIN_REQ     1
#define PKT_LOGIN_RES     101
#define PKT_SIGNUP_REQ    2
#define PKT_SIGNUP_RES    102

// 응답 결과 코드
#define RES_SUCCESS       "success"
#define RES_FAIL          "fail"