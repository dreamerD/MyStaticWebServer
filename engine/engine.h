#ifndef ENGINE_H
#define ENGINE_H
#include <functional>
#include <string>
#include <unordered_map>

#include "../http/httprequest.h"
#include "../http/httpresponse.h"
class Engine {
 public:
  // static void Run(int port = 1316, int trigMode = 3, int timeout = 6000,
  //                 bool optLinger = false, int threadNum = 6,
  //                 bool openLog = true, int logLevel = 1,
  //                 int logQueSize = 1024) {
  //   ListenAndServe(port, trigMode, timeout, optLinger, threadNum, openLog,
  //                  logLevel, logQueSize);
  // }
  static void GET(std::string pattern,
                  std::function<void(HttpRequest*, HttpResponse*)> handler) {
    addRoute("GET", pattern, handler);
  }
  static void POST(std::string pattern,
                   std::function<void(HttpRequest*, HttpResponse*)> handler) {
    addRoute("POST", pattern, handler);
  }
  static void ServeHTTP(HttpRequest* req, HttpResponse* res) {
    std::string key = req->GetMethod() + "-" + req->GetPath();
    if (router.count(key)) {
      router[key](req, res);
    } else {
      res->code = 404;
    }
  }

 private:
  static void addRoute(
      const std::string& method, std::string pattern,
      std::function<void(HttpRequest*, HttpResponse*)> handler) {
    std::string key = method + "-" + pattern;
    router[key] = handler;
  }
  static std::unordered_map<std::string,
                            std::function<void(HttpRequest*, HttpResponse*)>>
      router;
};
#endif