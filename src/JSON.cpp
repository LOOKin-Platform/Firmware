#include "JSON.h"

#include "document.h"

void JSON_t::CreateArrayFromVector(vector<string> JSONVector, Writer<StringBuffer> &Writer) {
  Writer.StartArray();

  for (auto& Item : JSONVector)
    Writer.String(Item.c_str());

  Writer.EndArray();
}

void JSON_t::CreateObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer) {
  Writer.StartObject();
  AddToObjectFromMap(JSONMap,Writer);
  Writer.EndObject();
}

void JSON_t::AddToObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer) {
  for(auto const &JSONItem : JSONMap) {
    Writer.Key(JSONItem.first.c_str());
    Writer.String(JSONItem.second.c_str());
  }
}

string JSON_t::JSONString(map<string,string> JSONMap) {
  StringBuffer sb;
  Writer<StringBuffer> Writer(sb);

  CreateObjectFromMap(JSONMap, Writer);

  return string(sb.GetString());
}

map<string,string> JSON_t::MapFromJsonString(string JSON) {
  map<string,string> Result = map<string,string>();

  Document document;
  document.Parse(JSON.c_str());

  for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    Result[itr->name.GetString()] = itr->value.GetString();

  return Result;
}
