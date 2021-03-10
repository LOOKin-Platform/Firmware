/*
*    JSON.cpp
*    Class designed to make abstraction between JSON library and user
*
*/

#include "JSON.h"

#include <cstring>

const char tag[] = "JSON";

JSON::JSON() {
	Root = cJSON_CreateObject();
}

JSON::JSON(const char* JSONString) {
	if (JSONString != NULL && strlen(JSONString) != 0) {
		Root = cJSON_ParseWithOpts(JSONString, 0, 1);

		if (Root == NULL)
			ESP_LOGE(tag, "Parse error");
	}
	else
		Root = cJSON_CreateObject();
}

JSON::JSON(string JSONString) {
	Converter::FindAndReplace(JSONString, "\\u", "\\\\u"); 	// UTF16 Hack

	if (!JSONString.empty()) {
		Root = cJSON_ParseWithOpts(JSONString.c_str(), 0, 0);

		if (Root == NULL)
			ESP_LOGE(tag, "Parse error");
	}
	else
		Root = cJSON_CreateObject();
}

JSON::JSON(cJSON *cJSONRoot) {
	Root = cJSONRoot;
}

cJSON* JSON::GetRoot() {
	return Root;
}

void JSON::SetDestructable(bool Value) {
	IsDestructable = Value;
}


JSON::~JSON() {
	if (IsDestructable)
		cJSON_Delete(Root);
}

JSON::RootType JSON::GetType() {
	if (Root != NULL) {
		if (Root->type == cJSON_Array) return Array;
		if (Root->type == cJSON_Object)return Object;
	}

	return Undefined;
}

bool JSON::IsItemExists(string Key) {
	if (Root == NULL)
		return false;

	cJSON *Value = cJSON_GetObjectItem(Root, Key.c_str());
	return (Value != NULL);
}

string JSON::GetItem(string Key) {
	string Result = "";

	if (Root != NULL) {
		cJSON *Value = cJSON_GetObjectItem(Root, Key.c_str());

		if (Value != NULL) {
			Result = Value->valuestring;
		}
	}

	return Result;
}

void JSON::SetItem(string Key, string Value) {
	cJSON_AddStringToObject(Root, Key.c_str(), Value.c_str());
}

vector<string> JSON::GetKeys() {
	vector<string> Result = vector<string>();

	if (Root != NULL)
	{
		cJSON *Child = Root->child;

		while(Child)
		{
			Result.push_back(string(Child->string));
			Child = Child->next;
		}
	}

	return Result;
}

JSON JSON::Detach(string Key) {
	if (Root != NULL) {

		cJSON *Value = cJSON_GetObjectItem(Root, Key.c_str());

		//cJSON *Value = cJSON_DetachItemFromObject(Root, Key.c_str());
		if (Value != NULL)
			return JSON(Value);
	}

	return JSON();
}

void JSON::Attache(string Key, JSON Item) {
	if (Item.Root == NULL)
		return;

	if (IsItemExists(Key))
		cJSON_ReplaceItemInObject(Root, Key.c_str(), Item.Root);
	else
		cJSON_AddItemToObject(Root, Key.c_str(), Item.Root);
}

void JSON::SetItems(map<string,string> Values, cJSON *Item) {
	SetItemsTemplated<map<string,string>>(Values, Item);
}

void JSON::SetItems(vector<pair<string,string>> Values, cJSON *Item) {
	SetItemsTemplated<vector<pair<string,string>>>(Values, Item);
}

template <typename T>
void JSON::SetItemsTemplated(T Values, cJSON *Item) {
	if (Root != NULL || Item != NULL)
		for(auto& Value : Values)
			cJSON_AddStringToObject((Item != NULL) ? Item : Root, Value.first.c_str(), Value.second.c_str());
}
template void JSON::SetItemsTemplated<map<string,string>> (map<string,string> Values, cJSON *Item);
template void JSON::SetItemsTemplated<vector<pair<string,string>>>(vector<pair<string,string>> Values, cJSON *Item);

map<string,string> JSON::GetItemsForKey(string Key, bool CaseSensitive) {
	if (Root == NULL) return map<string,string>();

	return GetItems(cJSON_GetObjectItem(Root, Key.c_str()), CaseSensitive);
}


map<string,string> JSON::GetItems(cJSON* Parent, bool CaseSensitive) {
	if (Parent == NULL && Root == NULL)
		return  map<string,string>();

	if (Parent == NULL) {
		if (Root->type == cJSON_Array)
			return map<string,string>();
	}
	else
		if (Parent->type == cJSON_Array)
			return map<string,string>();

	return GetItemsTemplated<map<string,string>>(Parent, CaseSensitive);
}

vector<pair<string,string>> JSON::GetItemsUnordered (cJSON* Parent, bool CaseSensitive) {
	return GetItemsTemplated<vector<pair<string,string>>>(Parent, CaseSensitive);
}

template <typename T>
T JSON::GetItemsTemplated(cJSON *Parent, bool CaseSensitive) {
	T Result = T();

	if (Root != NULL || Parent != NULL) {
		cJSON *Child = (Parent != NULL) ? Parent->child : Root->child;

		while( Child ) {
			if (Child->type != cJSON_Array && Child->type == cJSON_String)
				AddToMapOrTupple(Result,  Child->string, Child->valuestring, CaseSensitive);

			Child = Child->next;
		}
	}

	return Result;
}
template map<string,string> 			JSON::GetItemsTemplated(cJSON *Parent, bool CaseSensitive);
template vector<pair<string,string>> 	JSON::GetItemsTemplated(cJSON *Parent, bool CaseSensitive);

void JSON::SetObject(string Key, vector<pair<string,string>> Values) {
	if (Root != NULL) {
		cJSON *Item = cJSON_CreateObject();

		for(auto &Value : Values)
			cJSON_AddStringToObject(Item, Value.first.c_str(), Value.second.c_str());

		cJSON_AddItemToObject(Root, Key.c_str(), Item);
	}
}

vector<string> JSON::GetStringArray(string Key) {

	vector<string> Result = vector<string>();

	if (Root != NULL) {
		cJSON *Array = (!Key.empty()) ? cJSON_GetObjectItem(Root, Key.c_str()) : Root;

	    if (Array->type == cJSON_Array)
	    {
	    	cJSON *Child = Array->child;

	    	while(Child) {
	    		if (Child->type == cJSON_String)
	    			Result.push_back(Child->valuestring);

	    		Child = Child->next;
	    	}
	    }
	}

	return Result;
}

void JSON::SetStringArray(string Key, vector<string> Values) {
	cJSON *Array = cJSON_CreateArray();

	for(auto& Value : Values)
		cJSON_AddItemToArray(Array, cJSON_CreateString(Value.c_str()));

	cJSON_AddItemToObject(Root, Key.c_str(), Array);
}

template <typename T>
void JSON::SetUintArray(string Key, vector<T> Values, uint8_t HexCount) {
	cJSON *Array = cJSON_CreateArray();

	for(auto& Value : Values)
		if (HexCount == 0)
			cJSON_AddItemToArray(Array, cJSON_CreateString(Converter::ToString(Value).c_str()));
		else
			cJSON_AddItemToArray(Array, cJSON_CreateString(Converter::ToHexString((T)Value, HexCount).c_str()));

	cJSON_AddItemToObject(Root, Key.c_str(), Array);
}
template void JSON::SetUintArray<uint8_t> (string Key, vector<uint8_t>, uint8_t);
template void JSON::SetUintArray<uint16_t>(string Key, vector<uint16_t>, uint8_t);

void JSON::SetObjectsArray(string Key, vector<map<string,string>> Values) {
	SetObjectsArrayTemplated<map<string,string>>(Key, Values);
}

void JSON::SetObjectsArray(string Key, vector<vector<pair<string,string>>> Values) {
	SetObjectsArrayTemplated<vector<pair<string,string>>>(Key, Values);
}

template <typename T>
void JSON::SetObjectsArrayTemplated(string Key, vector<T> Values) {
	if (Root != NULL) {
		cJSON *Array = cJSON_CreateArray();

		for (auto& Value : Values) {
			cJSON *Item = cJSON_CreateObject();
			SetItemsTemplated<T>(Value, Item);

			cJSON_AddItemToArray(Array, Item);
		}

		if (!Key.empty())
			cJSON_AddItemToObject(Root, Key.c_str(), Array);
		else
			Root = Array;
	}
}
template void JSON::SetObjectsArrayTemplated<map<string,string>>(string Key, vector<map<string,string>> Values);
template void JSON::SetObjectsArrayTemplated<vector<pair<string,string>>>(string Key, vector<vector<pair<string,string>>> Values);

vector<map<string,string>> JSON::GetObjectsArray (string Key) {
	return GetObjectsArrayTemplated<map<string,string>>(Key);
}

vector<vector<pair<string,string>>> JSON::GetObjectsArrayUnordered (string Key) {
	return GetObjectsArrayTemplated<vector<pair<string,string>>>(Key);
}

template <typename T>
vector<T> JSON::GetObjectsArrayTemplated(string Key) {
	vector<T> Result = vector<T>();

	if (Root != NULL) {
		cJSON *Array = (!Key.empty()) ? cJSON_GetObjectItem(Root, Key.c_str()) : Root;

		if (Array->type == cJSON_Array) {
			cJSON *Child = Array->child;

			while(Child) {
				if (Child->type != cJSON_Array)
					Result.push_back(GetItemsTemplated<T>(Child));

				Child = Child->next;
			}
		}
	}

	return Result;
}
template vector<map<string,string>> JSON::GetObjectsArrayTemplated<map<string,string>> (string Key);
template vector<vector<pair<string,string>>> JSON::GetObjectsArrayTemplated<vector<pair<string,string>>> (string Key);

string JSON::ToString() {
    char *out = cJSON_PrintUnformatted(Root);

	string Result(out);
	free(out);

	Converter::FindAndReplace(Result, "\\\\u", "\\u"); 	// UTF16 Hack

	return Result;
}

string JSON::CreateStringFromVector(vector<string> Vector) {
	cJSON *Array = cJSON_CreateArray();

	for(auto &Item : Vector)
		cJSON_AddItemToArray(Array, cJSON_CreateString(Item.c_str()));

	char *data = cJSON_PrintUnformatted(Array);
	string Result(data);
	free(data);

	cJSON_Delete(Array);

	return Result;
}

template <typename T>
string JSON::CreateStringFromIntVector(vector<T> Vector, uint8_t HexCount) {
	cJSON *Array = cJSON_CreateArray();

	for(auto &Item : Vector) {
		if (HexCount == 0 || Item < 0)
			cJSON_AddItemToArray(Array, cJSON_CreateString(Converter::ToString<T>(Item).c_str()));
		else
			cJSON_AddItemToArray(Array, cJSON_CreateString(Converter::ToHexString((T)Item, HexCount).c_str()));
	}

	char *data = cJSON_PrintUnformatted(Array);
	string Result(data);
	free(data);

	cJSON_Delete(Array);

	return Result;
}
template string JSON::CreateStringFromIntVector<int32_t> (vector<int32_t>, uint8_t);
template string JSON::CreateStringFromIntVector<uint8_t> (vector<uint8_t>, uint8_t);
template string JSON::CreateStringFromIntVector<uint16_t>(vector<uint16_t>, uint8_t);

void JSON::AddToMapOrTupple(map<string,string> &Value, string first, string second, bool CaseSensitive) {
	Value[(CaseSensitive) ? first : Converter::ToLower(first)] = second;
}

void JSON::AddToMapOrTupple(vector<pair<string,string>> &Value, string first, string second, bool CaseSensitive) {
	Value.push_back(make_pair((CaseSensitive) ? first : Converter::ToLower(first), second));
}
