#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <assert.h>
#include <fcntl.h>  // open
#include <string.h>
#include <sys/mman.h>  // mmap, munmap
#include <sys/stat.h>  // stat
#include <unistd.h>    // close

#include <unordered_map>

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void Init(const std::string& srcDir, std::string& path,
            bool isKeepAlive = false, int code = -1);
  // void MakeResponse(Buffer& buff);
  void AddStatus(int code);
  void AddContent(const std::string& content);
  void UnmapFile();
  char* File();
  size_t FileLen() const;
  int Code() const { return code; }

 public:
  void addStateLine(Buffer& buff);
  void addHeader(Buffer& buff);
  void addContent(Buffer& buff);

  void errorHtml();
  std::string getFileType();
  void errorContent(Buffer& buff, std::string message);
  bool isKeepAlive;

  std::string path;
  std::string srcDir;
  std::string content;
  int code;
  char* mmFile;
  struct stat mmFileStat;

  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
  static const std::unordered_map<int, std::string> CODE_STATUS;
  static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif  // HTTP_RESPONSE_H