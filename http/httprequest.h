#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <iostream>
#include <regex>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "../buffer/buffer.h"
#include "../endecode/endecode.h"
#include "../log/log.h"
struct MultiFormPart {
  bool isFile = false;  // 是否为文件，否的话fileName和fileType是无效的
  // 注意：fileName可能会包含./
  // ../等路径信息，此时可能会有一些危险的操作，建议截取最后一个/之后的文件名或自行命名
  std::string fileName;
  std::string fileNameCharEncoding;
  // 有时可能文件名会乱码，因此请求中会有指定编码的文件名，对应字段 fileName*
  std::string fileName_;
  std::string fileType;
  std::string value;
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
  void Init();
  std::string& Path();
  bool IsKeepAlive() const;
  std::string GetMethod() const { return method; }
  std::string GetPath() const { return path; }
  std::string GetVersion() const { return version; }
  std::string GetBody() const { return body; }

 private:
  bool parseRequestLine(const std::string& line);  // 返回是否正确
  bool parseHeader(const std::string& line);
  bool parseBody(const std::string& line);  // 返回是否结束
  LINE_STATE readLine(Buffer& buff, char*& lineend);
  void parsePath();
  void parsePost();
  void parseFromUrlencoded();
  void parseFromMultiPartForm(const std::string& boundray);
  int converHex(char ch);

 private:
  std::string method, path, version, body;
  std::unordered_map<std::string, std::string> header;
  std::unordered_multimap<std::string, std::string> params;
  std::unordered_multimap<std::string, MultiFormPart> remap;
  std::unordered_multimap<std::string, std::string> post;
  PARSE_STATE state;
};
#endif