using namespace std;

#include "include/Query.h"
#include <stdio.h>
#include <iostream>
#include <sstream>

int main() {

  char *TestString;
  TestString = "POST /device/status/?param1=hello&param2=128 HTTP/1.1 \r\n \r\n param3=bugaga+34&param4=test%20qwerty";

  Query_t Query(TestString);

  cout << "Исходная строка: " << TestString << "\n";
  cout << "QueryType: "       << Query.Type << "\n";
  cout << "RequestHeader: "   << Query.RequestHeader << "\n";
  cout << "RequestedUrl: "    <<  Query.RequestedUrl << "\n";

  cout << "Параметры: " << "\n";
  for (const auto &p : Query.Params) {
      cout << "Param[" << p.first << "] = " << p.second << '\n';
  }

  cout << "Части пути к API: " << "\n";
  for (const auto &p : Query.RequestedUrlParts) {
      cout << p << '\n';
  }
}
