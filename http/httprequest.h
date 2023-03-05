#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <iostream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "buffer/buffer.h"
const std::unordered_set<std::string> DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};
enum PARSE_STATE { REQUEST_LINE = 0, HEADERS, BODY, GET_FINISH };
enum LINE_STATE { LINE_OK, LINE_OPEN, LINE_BAD };
enum HTTP_CODE {
  NO_REQUEST = 0,
  GET_REQUEST,
  POST_REQUEST,
  BAD_REQUEST,
  NO_RESOURCE,
  FORBIDDEN_REQUEST,
  FILE_REQUEST,
  INTERNAL_ERROR,
  CLOSED_CONNECTION
};
class HttpRequest {
 public:
  HttpRequest() : state(REQUEST_LINE) {}
  HTTP_CODE Parse(Buffer& buff);

 private:
  bool parseRequestLine(const std::string& line);  // 返回是否正确
  bool parseHeader(const std::string& line);
  bool parseBody(const std::string& line);  // 返回是否结束
  LINE_STATE readLine(Buffer& buff, char*& lineend);
  void parsePath();
  void parsePost();
  void parseFromUrlencoded();
  int converHex(char ch);

 private:
  std::string method, path, version, body;
  std::unordered_map<std::string, std::string> header;
  std::unordered_map<std::string, std::string> post;
  PARSE_STATE state;
};
#endif