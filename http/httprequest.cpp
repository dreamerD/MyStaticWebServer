#include "httprequest.h"

HTTP_CODE HttpRequest::Parse(Buffer& buff) {
  while (1) {
    char* lineEnd = nullptr;
    LINE_STATE line_state = readLine(buff, lineEnd);  // /r/n 或 最后一个
    if (line_state == LINE_OPEN && state != BODY) {
      break;
    }
    if (line_state == LINE_BAD || lineEnd == nullptr) {
      return BAD_REQUEST;
    }
    std::string line(buff.ReadBeginPos(), lineEnd);
    if (state == REQUEST_LINE) {
      if (!parseRequestLine(line)) {
        return BAD_REQUEST;
      }
      // 调整读指针
      buff.MoveReadPos(lineEnd + 2);
      state = HEADERS;
    } else if (state == HEADERS) {
      if (!parseHeader(line)) {
        return BAD_REQUEST;
      }
      // 调整读指针
      buff.MoveReadPos(lineEnd + 2);
      if (state == GET_FINISH) {
        return GET_REQUEST;
      }
    } else if (state == BODY) {
      if (!parseBody(line)) {
        break;
      }
      // 调整读指针
      buff.MoveReadPos(lineEnd);
      return POST_REQUEST;
    }
  }
  return NO_REQUEST;
}
LINE_STATE HttpRequest::readLine(Buffer& buff, char*& lineend) {
  size_t tmp = buff.readPos;
  while (tmp < buff.writePos) {
    char ch = buff.GetChar(tmp);
    if (ch != '\r') {
      tmp++;
    } else {
      if (tmp + 1 < buff.writePos) {
        if (buff.GetChar(tmp + 1) == '\n') {
          lineend = buff.ReadBeginPos() + tmp - buff.readPos;
          return LINE_OK;
        }
        return LINE_BAD;
      } else {
        break;
      }
    }
  }
  return LINE_OPEN;
}
bool HttpRequest::parseRequestLine(const std::string& line) {
  // https://zhuanlan.zhihu.com/p/29993142
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch subMatch;
  if (std::regex_match(line, subMatch, patten)) {
    method = subMatch[1];
    path = subMatch[2];
    version = subMatch[3];
    state = HEADERS;
    parsePath();
    return true;
  }
  LOG_ERROR("RequestLine Error");
  return false;
}
bool HttpRequest::parseHeader(const std::string& line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch subMatch;
  if (std::regex_match(line, subMatch, patten)) {
    header[subMatch[1]] = subMatch[2];
  } else {
    if (method == "POST") {
      if (!header.count("Content-length")) {
        return false;
      }
      state = BODY;
    } else {
      state = GET_FINISH;
    }
  }
  return true;
}
bool HttpRequest::parseBody(const std::string& line) {
  long len = atol(header["Content-length"].c_str());
  if (line.size() < len) {
    return false;
  }
  body = line.substr(0, len);
  parsePost();
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
  return true;
}

void HttpRequest::parsePath() {
  if (path == "/") {
    path = "/index.html";
  } else {
    for (auto& item : DEFAULT_HTML) {
      if (item == path) {
        path += ".html";
        break;
      }
    }
  }
}

int HttpRequest::converHex(char ch) {
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return ch;
}

void HttpRequest::parsePost() {
  if (method == "POST" &&
      header["Content-Type"] == "application/x-www-form-urlencoded") {
    parseFromUrlencoded();
    if (DEFAULT_HTML_TAG.count(path)) {
      int tag = DEFAULT_HTML_TAG.find(path)->second;
      LOG_DEBUG("Tag:%d", tag);
      if (tag == 0 || tag == 1) {
        bool isLogin = (tag == 1);
        if (UserVerify(post["username"], post["password"], isLogin)) {
          path = "/welcome.html";
        } else {
          path = "/error.html";
        }
      }
    }
  }
}

void HttpRequest::parseFromUrlencoded() {
  if (body.size() == 0) {
    return;
  }

  std::string key, value;
  int num = 0;
  int n = body.size();
  int i = 0, j = 0;

  for (; i < n; i++) {
    char ch = body[i];
    switch (ch) {
      case '=':
        key = body.substr(j, i - j);
        j = i + 1;
        break;
      case '+':
        body[i] = ' ';
        break;
      case '%':
        num = converHex(body[i + 1]) * 16 + converHex(body[i + 2]);
        body[i + 2] = num % 10 + '0';
        body[i + 1] = num / 10 + '0';
        i += 2;
        break;
      case '&':
        value = body.substr(j, i - j);
        j = i + 1;
        post[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
        break;
      default:
        break;
    }
  }
  if (post.count(key) == 0 && j < i) {
    value = body.substr(j, i - j);
    post[key] = value;
  }
}

void HttpRequest::Init() {
  method = path = version = body = "";
  state = REQUEST_LINE;
  header.clear();
  post.clear();
}

std::string& HttpRequest::Path() { return path; }

bool HttpRequest::IsKeepAlive() const {
  if (header.count("Connection") == 1) {
    return header.find("Connection")->second == "keep-alive" &&
           version == "1.1";
  }
  return false;
}

bool HttpRequest::UserVerify(const std::string& name, const std::string& pwd,
                             bool isLogin) {
  if (name == "" || pwd == "") {
    return false;
  }
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  MYSQL* sql;
  SqlConnRAII(&sql, SqlConnPool::Instance());
  assert(sql);

  bool flag = false;
  unsigned int j = 0;
  char order[256] = {0};
  MYSQL_FIELD* fields = nullptr;
  MYSQL_RES* res = nullptr;

  if (!isLogin) {
    flag = true;
  }
  /* 查询用户及密码 */
  snprintf(order, 256,
           "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
           name.c_str());
  LOG_DEBUG("%s", order);

  if (mysql_query(sql, order)) {
    mysql_free_result(res);
    return false;
  }
  res = mysql_store_result(sql);
  j = mysql_num_fields(res);
  fields = mysql_fetch_fields(res);

  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
    std::string password(row[1]);
    /* 注册行为 且 用户名未被使用*/
    if (isLogin) {
      if (pwd == password) {
        flag = true;
      } else {
        flag = false;
        LOG_DEBUG("pwd error!");
      }
    } else {
      flag = false;
      LOG_DEBUG("user used!");
    }
  }
  mysql_free_result(res);

  /* 注册行为 且 用户名未被使用*/
  if (!isLogin && flag == true) {
    LOG_DEBUG("regirster!");
    bzero(order, 256);
    snprintf(order, 256,
             "INSERT INTO user(username, password) VALUES('%s','%s')",
             name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order)) {
      LOG_DEBUG("Insert error!");
      flag = false;
    }
    flag = true;
  }
  // SqlConnPool::Instance()->FreeConn(sql);
  LOG_DEBUG("UserVerify Success!!");
  return flag;
}