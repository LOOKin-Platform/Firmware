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

#include <esp_log.h>

using namespace std;
using namespace rapidjson;

class JSON_t {
  public:
    JSON_t(string = "");
    void    Parse(string);
    string  ToString(bool isArray = false);

    map<string,string>          GetParam();
    string                      GetParam(string);
    map<string,string>          GetObjectParam(string);
    vector<string>              GetVector(string = "");
    vector<map<string,string>>  GetMapsArray(string);

    void                        SetParam(string, string);
    void                        SetParam(map<string,string>);
    void                        SetObjectParam(string,map<string,string>);
    void                        SetVector(vector<string>);
    void                        SetVector(string, vector<string>);
    void                        SetMapsArray(string, vector<map<string,string>>);

    void                        AddToVector(string);
    void                        AddToVector(string, string);

  private:
    map<string,string>              Params;
    vector<string>                  Vector;
    map<string,map<string,string>>  ObjectsParams;
    map<string,vector<string>>      Arrays;
    map<string,vector<map<string,string>>> ArraysOfObjects;

  void  AddToObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer);
  void  CreateArrayFromVector(vector<string> JSONVector, Writer<StringBuffer> &Writer, string Key = "");
};

#endif
