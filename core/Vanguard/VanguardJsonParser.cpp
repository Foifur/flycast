//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
// pulled and modified from https://github.com/UponTheSky/ray-tracing-in-series/blob/json-parser-devto/ray_tracer/utils/json_parser.cpp //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "Vanguard/VanguardJsonParser.h"
#include "Vanguard/VanguardHelpers.h"
#include <sstream>
#include <algorithm>

using namespace JsonParser;

JsonValue JsonParser::ParseJson(const BSTR& data)
{
  // 1. read the text data
  std::string text;
  ReadData(data, text);

  // 2. parse the text with the helper function and return
  text_it start = text.begin();
  return ParseJsonHelper(text, start);
}

void JsonParser::ReadData(const BSTR& data, std::string& output)
{
  std::string line;

  std::string converted_data = BSTRToString(data);

  std::stringstream json_data;
  json_data << converted_data;

  while (std::getline(json_data, line))
  {
    output.append(line);
  }
}


JsonValue JsonParser::ParsePrimitive(const std::string& text, text_it start, text_it end)
{
  std::string substr = text.substr(start - text.begin(), end - start);

  // flycast stores "yes" and "no" instead of "true" and "false" for...some reason
  if (substr.find("yes") != std::string::npos)
    {
      return {.b = true};
    }
  else if (substr.find("no") != std::string::npos)
    {
      return {.b = false};
    }
  else if (substr.find(".") != std::string::npos)
    {
      return {.d = std::stod(substr)};
    }
  else
    {
      return {.i = std::stoi(substr)};
    }
}

std::pair<std::string, JsonValue> JsonParser::RetriveKeyValuePair(const std::string& text,
                                                                  text_it& it)
{
  if (it == text.end())
    throw std::invalid_argument("string is empty");

  // ignore white spaces & line breaks
  while (*it == ' ' || *it == '\n')
  {
    it++;
  }

  text_it curr_it;
  std::string key;
  JsonValue value;
  // if hit a double quote for the first time, it is a key
  if (*it == '\"')
  {
    curr_it = ++it;
    while (*it != '\"')
    {
      it++;
    }

    key = text.substr(curr_it - text.begin(), it - curr_it);
    if (*(++it) == ':')
      it++;
    else
      throw std::invalid_argument("did not find ':' designating a value to the key");
    it++;
  }

  // now we need to have its corresponding value
  while (*it == ' ' || *it == '\n')
  {
    it++;
  }

  if (*it == '{')
  {
    // another json format
    value = ParseJsonHelper(text, it);
  }
  else
  {
    // primitive value(double or int)
    curr_it = it;
    while (*it != ',' && *it != '\n' && *it != '}')
    {
      it++;
    }
    value = ParsePrimitive(text, curr_it, it);
  }

  // after parsing the value, check whether the current iterator points to a comma
  if (*it == ',')
  {
    it++;
  }

  return std::make_pair(key, value);
}

JsonValue JsonParser::ParseJsonHelper(const std::string& text, text_it& it)
{
  if (*it != '{')  // must start with the left curly bracket
    throw std::invalid_argument("string does not start with '{'");
  it++;

  std::map<std::string, JsonValue>* json_map = new std::map<std::string, JsonValue>;

  do
  {
    const auto [key, value] = RetriveKeyValuePair(text, it);
    (*json_map)[key] = value;

    while (*it == ' ' || *it == '\n')
    {
      it++;
    }
  } while (*it != '}');

  it++;  // after '}'

  return {.json = json_map};
}
