#pragma once
#include "cfg/option.h"
#include "emulator.h"
#include "Vanguard/VanguardJsonParser.h"
#include <string>
#include <variant>
#include <format>

#define VAR_NAME(Variable) (#Variable)

using SettingsTypes = std::variant<bool, int, float>;
using SettingsArray = std::vector< std::pair<std::string, SettingsTypes>>;

class VanguardSettings
{
public:
  SettingsArray array;
  std::pair<std::string, bool> Vsync;
  std::pair<std::string, bool> FloatVMUs;
  std::pair<std::string, int> Sh4Clock;

  // save the current value of required settings
  void SaveSettings()
  {
    save_setting<bool>(VAR_NAME(Vsync), Vsync, &config::VSync);
    save_setting<bool>(VAR_NAME(FloatVMUs), FloatVMUs, &config::FloatVMUs);
    save_setting<int>(VAR_NAME(Sh4Clock), Sh4Clock, &config::Sh4Clock);

  }

  // load the settings values sent from the dll hook into the emulator's settings
  void LoadSettings(JsonParser::JsonValue settings)
  {
    load_setting<bool>(&config::VSync, (*settings.json)[VAR_NAME(Vsync)]);
    load_setting<bool>(&config::FloatVMUs, (*settings.json)[VAR_NAME(FloatVMUs)]);
    load_setting<int>(&config::Sh4Clock, (*settings.json)[VAR_NAME(Sh4Clock)]);

    ::SaveSettings();
  }

  // Visits the variant value in a pair, determines the correct data type and returns it as a string
  std::string to_string(SettingsTypes var)
  {
    return std::visit([](auto arg) {return std::format("{}", arg); }, var);
  }

private:
  // saves the name and value of the setting to a pair, then push it onto the array
  template<typename T>
  void save_setting(std::string name, std::pair<std::string, SettingsTypes> variable, config::Option<T>* setting)
  {
    variable.first = name;
    variable.second = *setting;
    array.push_back(variable);
  }

  // loads the value of the parsed setting based on the requested data type
  template<typename T>
  void load_setting(config::Option<T>* setting, JsonParser::JsonValue value)
  {
      if (typeid(T) == typeid(bool))
          *setting = value.b;

      else if (typeid(T) == typeid(int))
          *setting = value.i;
  }
};
