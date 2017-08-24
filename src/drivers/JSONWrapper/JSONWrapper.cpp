/*
*    JSON.cpp
*    Class designed to make abstraction between JSON library and user
*
*/

#include "JSONWrapper.h"

static char tag[] = "JSON";

JSON::JSON(string JSONString) {
  if (!JSONString.empty()) {
    Root = cJSON_ParseWithOpts(JSONString.c_str(), 0, 0);
    if (Root == NULL)
      ESP_LOGE(tag, "Parse error");
  }
  else
    Root = cJSON_CreateObject();
}

JSON::~JSON() {
  cJSON_Delete(Root);
}

string JSON::GetItem(string Key) {
  if (Root != NULL) {
    cJSON *Value = cJSON_GetObjectItem(Root, Key.c_str());
    if (Value != NULL)
      return Value->valuestring;
  }

    return "";
}

void JSON::SetItem(string Key, string Value) {
  cJSON_AddStringToObject(Root, Key.c_str(), Value.c_str());
}

map<string,string> JSON::GetItems(cJSON *Parent) {
  map<string,string> Result = map<string,string>();

  if (Root != NULL || Parent != NULL) {
    cJSON *Child = (Parent != NULL) ? Parent->child : Root->child;

    while( Child ) {
      if (Child->type != cJSON_Array)
        Result[Converter::ToLower(Child->string)] = Child->valuestring;

      Child = Child->next;
    }
  }

  return Result;
}

void JSON::SetItems(map<string,string> Values, cJSON *Item) {
  if (Root != NULL || Item != NULL)
    for(auto& Value : Values)
      cJSON_AddStringToObject((Item != NULL) ? Item : Root, Value.first.c_str(), Value.second.c_str());
}

void JSON::SetObject(string Key, map<string,string> Values) {
  if (Root != NULL) {
    cJSON *Item = cJSON_CreateObject();

    for(auto &Value : Values)
      cJSON_AddStringToObject(Item, Value.first.c_str(), Value.second.c_str());

    cJSON_AddItemToObject(Root, Key.c_str(), Item);
  }
}

void JSON::SetStringArray(string Key, vector<string> Values) {
  cJSON *Array = cJSON_CreateArray();

  for(auto& Value : Values)
    cJSON_AddItemToArray(Array, cJSON_CreateString(Value.c_str()));

  cJSON_AddItemToObject(Root, Key.c_str(), Array);
}

vector<map<string,string>> JSON::GetObjectsArray(string Key) {
  vector<map<string,string>> Result = vector<map<string,string>>();

  if (Root != NULL) {
    cJSON *Array = (!Key.empty()) ? cJSON_GetObjectItem(Root, Key.c_str())
                                  : Root;

    if (Array->type == cJSON_Array) {
      cJSON *Child = Array->child;
      while(Child) {
        if (Child->type != cJSON_Array)
          Result.push_back(GetItems(Child));

        Child = Child->next;
      }
    }
  }

  return Result;
}

void JSON::SetObjectsArray(string Key, vector<map<string,string>> Values) {
  if (Root != NULL) {

    cJSON *Array = cJSON_CreateArray();

    for (auto& Value : Values) {
      cJSON *Item = cJSON_CreateObject();
      SetItems(Value, Item);

      cJSON_AddItemToArray(Array, Item);
    }

    if (!Key.empty())
      cJSON_AddItemToObject(Root, Key.c_str(), Array);
    else
      Root = Array;
  }
}

string JSON::ToString() {
  return cJSON_PrintUnformatted(Root);
}

string JSON::CreateStringFromVector(vector<string> Vector) {
  cJSON *Array = cJSON_CreateArray();

  for(auto &Item : Vector)
    cJSON_AddItemToArray(Array, cJSON_CreateString(Item.c_str()));

  string Result = cJSON_PrintUnformatted(Array);
  cJSON_Delete(Array);

  return Result;
}
