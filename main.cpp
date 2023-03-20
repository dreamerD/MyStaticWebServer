#include "server/webserver.h"
int main() {
  WebServer server(1316, 3, 6000, false, 6306, "root", "root", "mydb", 12, 6,
                   true, 1, 1024);
  server.Start();
}