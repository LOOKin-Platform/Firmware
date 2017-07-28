#include "JSON.h"

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
