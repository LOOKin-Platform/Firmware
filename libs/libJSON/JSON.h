/*
*    JSON.h
*    Class designed to make abstraction between JSON library and user
*
*/

#ifndef LIBS_JSON_H
#define LIBS_JSON_H

//#include <map>
#include <vector>
#include <string>
#include <utility>
#include <map>

#include <esp_log.h>

#include <cJSON.h>
#include "Converter.h"

class JSON;
typedef void (*ArrayItemCallback)	(JSON);

using namespace std;

class JSON {
  public:
    JSON();
    JSON(const char *);
    JSON(string);
    JSON(cJSON *cJSONRoot);

    ~JSON();

    enum RootType { Object, Array, Undefined };
    RootType							GetType();
    cJSON*								GetRoot();
    void								SetDestructable(bool);

    bool								IsItemExists			(string Key);

    string								GetItem					(string Key);
    void								SetItem					(string Key, string Value);

    JSON								Detach					(string Key);

    void								SetItems				(map<string,string>, cJSON *Item = NULL);
    void								SetItems				(vector<pair<string,string>>, cJSON *Item = NULL);
    map<string,string>					GetItemsForKey			(string Key, bool CaseSensitive = false);
    map<string,string>					GetItems				(cJSON* = NULL, bool CaseSensitive = false);
    vector<pair<string,string>>			GetItemsUnordered		(cJSON* = NULL, bool CaseSensitive = false);

    void								SetObject				(string Key, vector<pair<string,string>> Value);

    void								SetStringArray			(string Key, vector<string> Array);
    vector<string>						GetStringArray			(string Key = "");

	template <typename T> void			SetUintArray			(string Key, vector<T>, uint8_t HexCount = 0);

    void								SetObjectsArray			(string Key, vector<map<string,string>> Value);
    void								SetObjectsArray			(string Key, vector<vector<pair<string,string>>> Value);
    vector<map<string,string>>			GetObjectsArray			(string Key = "");
    vector<vector<pair<string,string>>> GetObjectsArrayUnordered(string Key = "");

    string								ToString();

    static string						CreateStringFromVector(vector<string>);
    template <typename T> static string	CreateStringFromIntVector(vector<T>, uint8_t HexCount = 0);

  private:
	void								AddToMapOrTupple(map<string,string> &, string first, string second, bool CaseSensitive);
	void								AddToMapOrTupple(vector<pair<string,string>> &, string first, string second, bool CaseSensitive);

    template <typename T> void 			SetItemsTemplated		(T Values, cJSON *Item = NULL);
    template <typename T> T				GetItemsTemplated		(cJSON* = NULL, bool CaseSensitive = false);
    template <typename T> void			SetObjectsArrayTemplated(string Key, vector<T> Value);
    template <typename T> vector<T>		GetObjectsArrayTemplated(string Key = "");

    cJSON 	*Root;
    bool	IsDestructable = true;
};

#endif //DRIVERS_JSON_H
