// #include <iostream>
// #include <string>
// #include <vector>

// using json = nlohmann::json; 

// int main()
// {
//     MYSQL *conn = mysql_init(NULL);
//     if (mysql_real_connect(conn, "localhost", "db_user", "password", "test_db", 3306, NULL, 0) == NULL) 
//     {
//         std::cerr << "연결 실패: " << mysql_error(conn) << std::endl;
//         return 1;
//     }

//     // 2. SQL 쿼리 실행
//     if (mysql_query(conn, "SELECT id, name, level FROM users")) 
//     {
//         std::cerr << "쿼리 실패: " << mysql_error(conn) << std::endl;
//         return 1;
//     }

//     MYSQL_RES *res = mysql_store_result(conn);
//     MYSQL_ROW row;

//     // 3. JSON 활용 문법: 결과를 JSON 배열(리스트)에 담기
//     json user_list = json::array(); // 파이썬의 [ ] 생성과 동일

//     while ((row = mysql_fetch_row(res))) 
//     {
//         json user; // 파이썬의 { } 생성과 동일
//         user["id"] = std::stoi(row[0]);    // 문자열을 숫자로 변환하여 할당
//         user["name"] = row[1];             // 이름 할당
//         user["level"] = std::stoi(row[2]); // 레벨 할당
        
//         user_list.push_back(user); // 배열에 딕셔너리 추가
//     }

//     // 4. 최종 JSON 출력 (문자열로 변환)
//     // dump(4)는 파이썬의 json.dumps(indent=4)와 같습니다.
//     std::cout << user_list.dump(4) << std::endl;

//     // 자원 해제
//     mysql_free_result(res);
//     mysql_close(conn);
    
//     return 0;
// }