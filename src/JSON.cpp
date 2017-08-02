#include "JSON.h"
#include "Converter.h"

#include "document.h"
#include "error/en.h"

static char tag[] = "JSON_t";

JSON_t::JSON_t(string JSONString) {

  Params          = map<string,string>();
  ObjectsParams   = map<string,map<string,string>>();
  Vector          = vector<string>();
  Arrays          = map<string,vector<string>>();
  ArraysOfObjects = map<string,vector<map<string,string>>>();

  if (!JSONString.empty())
    Parse(JSONString);
}

void JSON_t::Parse(string JSON) {
  Document document;
  ParseResult IsSuccess = document.Parse(JSON.c_str());

  if (!IsSuccess) {
    ESP_LOGE(tag, "Failed to parse JSON string: %s (%u)", rapidjson::GetParseError_En(IsSuccess.Code()), IsSuccess.Offset());
    return;
  }

  if (document.IsObject()) {
    for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
      if (document[itr->name].IsArray()) {
        string Key      = Converter::ToLower(itr->name.GetString());
        bool   IsVector = false;

        vector<string>              itVector      = vector<string>();
        vector<map<string,string>>  itMapsVector  = vector<map<string,string>>();

        for (auto& ArrayItem : document[itr->name].GetArray()) {
          if (ArrayItem.IsObject()) {
            IsVector = false;

            map<string,string> tmpMap = map<string,string>();
            for (Value::ConstMemberIterator MapVectorIt = ArrayItem.MemberBegin(); MapVectorIt != ArrayItem.MemberEnd(); ++MapVectorIt)
                tmpMap[Converter::ToLower(MapVectorIt->name.GetString())] = MapVectorIt->value.GetString();

            itMapsVector.push_back(tmpMap);
          }
          else {
            IsVector = true;
            itVector.push_back(ArrayItem.GetString());
          }
        }

        if (IsVector)
          Arrays[Key]           = itVector;
        else
          ArraysOfObjects[Key]  = itMapsVector;
      }
      else if (document[itr->name].IsObject()) {
        map<string,string> ObjectItemMap = map<string,string>();
        for (Value::ConstMemberIterator ObjectItemIt = document[itr->name].MemberBegin(); ObjectItemIt != document[itr->name].MemberEnd(); ++ObjectItemIt)
            ObjectItemMap[Converter::ToLower(ObjectItemIt->name.GetString())] = ObjectItemIt->value.GetString();

        ObjectsParams[Converter::ToLower(itr->name.GetString())] = ObjectItemMap;
      }
      else
        Params[Converter::ToLower(itr->name.GetString())] = itr->value.GetString();
    }
  }
  else {
    for (auto& ArrayItem : document.GetArray()) {
      if (ArrayItem.IsString())
        Vector.push_back(ArrayItem.GetString());
    }
  }
}

string JSON_t::ToString(bool isArray) {
  StringBuffer sb;
  Writer<StringBuffer> Writer(sb);

  if (isArray) {
    CreateArrayFromVector(Vector, Writer);
  }
  else {
    Writer.StartObject();
    AddToObjectFromMap(Params, Writer);

    if (ObjectsParams.size() > 0) {
      for(auto const &Item : Arrays) {
        Writer.Key(Item.first.c_str());
        Writer.StartObject();
        AddToObjectFromMap(ObjectsParams[Item.first], Writer);
        Writer.EndObject();
      }
    }

    if (Arrays.size() > 0) {
      for(auto const &Item : Arrays) {
        Writer.Key(Item.first.c_str());
        Writer.StartArray();
        for (int i=0; i < Item.second.size(); i++) {
          Writer.String(Item.second[i].c_str());
        }
        Writer.EndArray();
      }
    }

    if (ArraysOfObjects.size() > 0) {
      for(auto const &Item : ArraysOfObjects) {
        Writer.Key(Item.first.c_str());
        Writer.StartArray();

        for (int i=0; i < Item.second.size(); i++) {
          Writer.StartObject();
          for(auto &MapItem : Item.second[i]) {
            Writer.Key(MapItem.first.c_str());
            Writer.String(MapItem.second.c_str());
          }
          Writer.EndObject();
        }

        Writer.EndArray();
      }
    }

    Writer.EndObject();
  }

  return string(sb.GetString());

}

map<string,string> JSON_t::GetParam() {
    return Params;
}

string JSON_t::GetParam(string Key) {
  string KeyL = Converter::ToLower(Key);
  return (Params.count(KeyL) > 0) ? Params[KeyL] : "";
}

map<string,string> JSON_t::GetObjectParam(string Param) {
  return (ObjectsParams.count(Param) > 0) ? ObjectsParams[Param] : map<string,string>();
}

vector<string> JSON_t::GetVector(string Param) {
  if (!Param.empty() && Arrays.count(Param) > 0)
    return Arrays[Param];

  if (Param.empty())
    return Vector;

  return vector<string>();
}

vector<map<string,string>> JSON_t::GetMapsArray(string Param) {
  if (!Param.empty() && ArraysOfObjects.count(Param) > 0)
    return ArraysOfObjects[Param];

  return vector<map<string,string>>();
}

void JSON_t::SetParam(string Key, string Value) {
  Params[Key] = Value;
}

void JSON_t::SetParam(map<string,string> Value) {
  for(auto const &Item : Value)
    Params[Item.first] = Item.second;
}

void JSON_t::SetObjectParam(string Key, map<string,string> Value) {
  ObjectsParams[Key] = Value;
}

void JSON_t::SetVector(vector<string> Value) {
  Vector = Value;
}

void JSON_t::SetVector(string Key, vector<string> Value) {
  Arrays[Key] = Value;
}

void JSON_t::SetMapsArray(string Key, vector<map<string,string>> Value) {
  ArraysOfObjects[Key] = Value;
}

void JSON_t::AddToVector(string Value) {
  Vector.push_back(Value);
}

void JSON_t::AddToVector(string Key, string Value) {
  Arrays[Key].push_back(Value);
}


void JSON_t::CreateArrayFromVector(vector<string> JSONVector, Writer<StringBuffer> &Writer, string Key) {

  if (!Key.empty())
    Writer.Key(Key.c_str());

  Writer.StartArray();

  for (auto& Item : JSONVector)
    Writer.String(Item.c_str());

  Writer.EndArray();
}

void JSON_t::AddToObjectFromMap(map<string,string> JSONMap, Writer<StringBuffer> &Writer) {
  for(auto const &JSONItem : JSONMap) {
    Writer.Key(JSONItem.first.c_str());
    Writer.String(JSONItem.second.c_str());
  }
}
