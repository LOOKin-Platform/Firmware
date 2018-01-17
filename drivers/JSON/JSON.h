/*
*    JSON.h
*    Class designed to make abstraction between JSON library and user
*
*/

#ifndef DRIVERS_JSON_H
#define DRIVERS_JSON_H

#include <map>
#include <vector>
#include <string>

#include "cJSON.h"
#include <esp_log.h>

#include "Convert.h"

using namespace std;

class JSON {
  public:
    JSON(string = "");

    ~JSON();
    string                      GetItem(string Key);
    void                        SetItem(string Key, string Value);

    map<string,string>          GetItems(cJSON* = NULL);
    void                        SetItems(map<string,string>, cJSON *Item = NULL);

    void                        SetObject(string Key, map<string,string> Value);

    void                        SetStringArray(string Key, vector<string> Array);

    vector<map<string,string>>  GetObjectsArray(string Key = "");
    void                        SetObjectsArray(string Key, vector<map<string,string>> Value);

    string                      ToString();

    static string               CreateStringFromVector(vector<string>);
  private:
    cJSON *Root;
};

#endif //DRIVERS_JSON_H
