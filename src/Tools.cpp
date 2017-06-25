#include "include/Tools.h"

#include <algorithm>
#include <iterator>

string Tools::ToLower(string Str) {
  string Result = Str;
  transform(Result.begin(), Result.end(), Result.begin(), ::tolower);

  return Result;
}
