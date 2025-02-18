#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
// pulled and modified from https://github.com/UponTheSky/ray-tracing-in-series/blob/json-parser-devto/ray_tracer/utils/json_parser.h   //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <comdef.h>
#include <string>
#include <map>
#include <string>
#include <iostream>

namespace JsonParser
{
  using text_it = std::string::iterator;

  union JsonValue
  {
    int i;
    double d;
    bool b;
    std::map<std::string, JsonValue>* json;
  };

  JsonValue ParseJson(const BSTR& data);

  void ReadData(const BSTR& data, std::string& output);

  JsonValue ParsePrimitive(const std::string& text, text_it start, text_it end);

  std::pair<std::string, JsonValue> RetriveKeyValuePair(const std::string& text, text_it& it);

  JsonValue ParseJsonHelper(const std::string& text, text_it& it);

  }
