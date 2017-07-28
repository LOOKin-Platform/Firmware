#ifndef JSON_H
#define JSON_H
/*
  Впомогательный класс работы с JSON для уменьшения размера прошивки
*/
#include "stringbuffer.h"
#include "writer.h"

#include <map>
#include <vector>
#include <string>

using namespace std;
using namespace rapidjson;

class JSON_t {
  public:
    static void   CreateArrayFromVector(vector<string> JSONVector, Writer<StringBuffer> &Writer);

    static void   CreateObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer);
    static void   AddToObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer);

    static string JSONString(map<string,string> JSONMap);
    static map<string,string> MapFromJsonString(string JSON);
};

#endif
