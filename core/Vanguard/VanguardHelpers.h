#pragma once

#include <wtypes.h>
#include <comdef.h>
#include <string>

#define EXPORT extern "C" __declspec(dllexport)

EXPORT unsigned char Vanguard_peekbyte(long long addr, int selection);

EXPORT void Vanguard_pokebyte(long long addr, unsigned char val, int selection);

EXPORT void Vanguard_pause(bool pauseUntilCorrupt = false);

EXPORT void Vanguard_resume();

EXPORT void Vanguard_savesavestate(BSTR filename, bool wait);

EXPORT void Vanguard_loadsavestate(BSTR filename);

EXPORT void Vanguard_loadROM(BSTR filename);

EXPORT void Vanguard_finishLoading();

EXPORT void Vanguard_closeGame();

EXPORT void Vanguard_prepShutdown();

EXPORT void Vanguard_forceStop();

EXPORT char* Vanguard_getSystemCore();

class VanguardClient
{
public:
  static bool loading;
  static bool ok_to_corestep;
  inline static bool pauseUntilCorrupt;
  static bool close_emulator;
  static std::string system_core;
};

inline HINSTANCE vanguard = LoadLibraryA("../RTCV/VanguardHook.dll");
inline DWORD error;
/* CallImportedFunction -- calls a function that has been exported from the Vanguard */
/*                         client to be accessed by the emulator                     */
/*                                                                                   */
/* arguments:                                                                        */
/* const char* function name -- the name of the `DllExport`ed Vanguard function      */
/* std::string string        -- the function's input string (if it has one)          */
template <typename T>
T CallImportedFunction(char* function_name, std::string string = "")
{
  // since we don't have access to a C++/CLI wrapper, we need to convert the game name string
  // into a BSTR so that the C# function can read it. We also need to make sure we allocate
  // the BSTR into memory before conversion, and then free the memory after we've sent it off
  BSTR converted_string = SysAllocString(L"");

  if (!string.empty())
  {
    // change the string into a char array and convert it to a BSTR
    const char* char_array = string.c_str();
    converted_string = _com_util::ConvertStringToBSTR(char_array);
  }

  // check to see if we converted a string and need to pass an input argument
  // this is kind of hacky...
  if (SysStringLen(converted_string))
  {
    // find the function in the dll and call it
    typedef T (*FUNC)(BSTR);
    FUNC function = (FUNC)GetProcAddress(vanguard, function_name);
    if (!function)
      error = GetLastError();

    // make sure to free the BSTR from memory once we're done with it
    SysFreeString(converted_string);
    return function(converted_string);
  }
  else
  {
    // find the function in the dll and call it
    typedef T (*FUNC)();
    FUNC function = (FUNC)GetProcAddress(vanguard, function_name);
    if (!function)
      error = GetLastError();

    // make sure to free the BSTR from memory once we're done with it
    SysFreeString(converted_string);
    return function();
  }
}

std::string BSTRToString(BSTR string);

std::string getDirectory();
