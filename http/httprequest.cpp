#include "httprequest.h"

HTTP_CODE HttpRequest::Parse(Buffer& buff) {
  while (1) {
    if (state != BODY) {
      char* lineEnd = nullptr;
      LINE_STATE line_state = readLine(buff, lineEnd);  // /r/n 或 最后一个
      if (line_state == LINE_OPEN) {
        printf("Line is OPEN\n");
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
      }
    } else {
      std::string body(buff.ReadBeginPos(), buff.WriteBeginPos());
      if (parseBody(body)) {
        buff.MoveReadPos(buff.WriteBeginPos());
        return POST_REQUEST;
      } else {
        break;
      }
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
          lineend = buff.ReadBeginPos() + tmp - buff.readPos;  // 此时指向 \r
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
      printf("POST REQUEST\n");
      if (!header.count("Content-Length")) {
        printf("POST REQUEST ERROR\n");
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
  printf("body = %s", body.c_str());
  size_t len = atol(header["Content-Length"].c_str());
  if (line.size() < len) {
    return false;
  }
  body = line.substr(0, len);
  parsePost();
  LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
  return true;
}

void HttpRequest::parsePath() {
  size_t left = this->path.find('?');
  if (left == std::string::npos) {
    return;
  }
  left += 1;
  size_t right = left;
  while (right <= this->path.size()) {
    if (right == this->path.size() || this->path[right] == '&') {
      size_t old_left = left;
      while (left < right && this->path[left] != '=') {
        left++;
      }
      this->params.insert({this->path.substr(old_left, left - old_left),
                           right > left + 1
                               ? this->path.substr(left + 1, right - left - 1)
                               : ""});
      right++;
      left = right;
    } else {
      right++;
    }
  }
  this->path.erase(left);
}

int HttpRequest::converHex(char ch) {
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return ch;
}

void HttpRequest::parsePost() {
  if (method == "POST") {
    if (header["Content-Type"] == "application/x-www-form-urlencoded") {
      printf("Content Type=application/x-www-form-urlencoded");
      parseFromUrlencoded();
    } else {
      size_t pos =
          header["Content-Type"].find("application/x-www-form-urlencoded");
      // --{boundary}(\r\n)
      // Content-Disposition: form-data; name="{key}"(\r\n)
      // (\r\n)
      // {value}(\r\n)
      // --{boundary}

      // --{boundary}(\r\n)
      // Content-Disposition: form-data; name="{key}";
      // filename="{filename}"(\r\n)[;
      // filename*={编码方式}''{对应编码的filename}] Content-Type:
      // {文件格式}(\r\n)
      // (\r\n)
      // (\r\n)
      // {二进制数据}(\r\n)
      // --{boundary}
      if (pos != std::string::npos) {
        parseFromMultiPartForm(header["Content-Type"].substr(
            header["Content-Type"].find("boundary=", pos) +
            strlen("boundary=")));
      } else {
        return;
      }
    }
  }
}

void HttpRequest::parseFromMultiPartForm(const std::string& boundary) {
  std::string_view view{body};
  std::regex reg_name{
      "\\r\\nContent-Disposition:\\s*form-data;\\s*name=\"(.*)\"(?:\\s*;\\s*"
      "filename=\"(.*)\"(?:\\s*;\\s*filename\\*=(.*)''(.*))?\\r\\nContent-"
      "Type:\\s*(.*))?\\r\\n\\r\\n"};
  std::string find_bound = "--" + boundary;
  MultiFormPart data;
  size_t pos = 0, pos_2 = 0;
  if ((pos = view.find(find_bound)) != std::string::npos) {
    pos += find_bound.size();
    for (; (pos_2 = view.find(find_bound, pos)) != std::string::npos;) {
      std::string_view block =
          view.substr(pos, pos_2 - pos);  // 截取两个bound之间的内容
      std::match_results<std::string_view::const_iterator> block_match_view;
      if (std::regex_search(block.begin(), block.end(), block_match_view,
                            reg_name)) {  // 匹配 name 和可能存在的 filename
        data.fileName = block_match_view.str(2);
        data.fileNameCharEncoding = block_match_view.str(3);
        data.fileName_ = block_match_view.str(4);
        data.fileType = block_match_view.str(5);
        data.isFile = data.fileName.size() > 0;
        const char* str_p = &(*(block_match_view[0].second));
        size_t len = block.end() - 2 - block_match_view[0].second;
        data.value = std::string(str_p, len);
        remap.insert({block_match_view.str(1), data});
      }
      pos = pos_2 + find_bound.size();
    }
  }
}

void HttpRequest::parseFromUrlencoded() {
  if (body.size() == 0) {
    return;
  }
  std::string key, value;
  std::string str = UTF8Url::Decode(body);
  int i = 0, j = 0;
  int n = str.size();
  for (; i < n; i++) {
    char ch = str[i];
    switch (ch) {
      case '=':
        key = str.substr(j, i - j);
        j = i + 1;
        break;
      case '&':
        value = str.substr(j, i - j);
        j = i + 1;
        post.insert({key, value});
        printf("%s = %s\n", key.c_str(), value.c_str());
        break;
      default:
        break;
    }
  }
  if (j < i) {
    value = str.substr(j, i - j);
    post.insert({key, value});
    printf("%s = %s\n", key.c_str(), value.c_str());
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