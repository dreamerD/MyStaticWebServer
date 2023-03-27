#include "engine/engine.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "server/webserver.h"
void f(HttpRequest* req, HttpResponse* res) {
  res->AddContent("Hello World!");
};
void h(HttpRequest* req, HttpResponse* res) {
  res->AddContent(UTF8Url::Decode(req->GetBody()));
};
int main() {
  Engine::GET("/hello", f);
  Engine::POST("/hello", h);
  ListenAndServe();
}