/*
using namespace std;

#include "include/Query.h"
#include "include/API.h"
#include <stdio.h>
#include <iostream>

int main() {

  char *TestString;
  TestString = "GET /device/status/?param1=hello&param2=128 HTTP/1.1";

  Query_t Query(TestString);

  cout << "Исходная строка: " << TestString << "\n";
  cout << "QueryType: "       << Query.Type << "\n";
  cout << "RequestHeader: "   << Query.RequestHeader << "\n";
  cout << "RequestedUrl: "    <<  Query.RequestedUrl << "\n";

  cout << "Параметры: " << "\n";
  for (const auto &p : Query.Params) {
      cout << "m[" << p.first << "] = " << p.second << '\n';
  }

  cout << "Части пути к API: " << "\n";
  for (const auto &p : Query.RequestedUrlParts) {
      cout << p << '\n';
  }

  cout << API::Handle(Query);

}
*/
