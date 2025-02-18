#include "Vanguard/VanguardHelpers.h"
#include "Vanguard/VanguardClientInitializer.h"
#include "Vanguard/VanguardJsonParser.h"
#include "Vanguard/VanguardEmuSettings.h"

#include "hw\sh4\sh4_mem.h"
#include "hw\pvr\pvr_mem.h"
#include "hw\aica\aica_if.h"
#include "hw\flashrom\nvmem.h"
#include "hw\pvr\elan.h"
#include "emulator.h"
#include "ui\gui_util.h"

#include <codecvt>
#include <sstream>

void FormatJsonData(VanguardSettings& settings, std::ostringstream& json_string);

unsigned char Vanguard_peekbyte(long long addr, int selection)
{
	u8 byte = 0;
	u16 data = 0;
	long long mod = 0;
	long long newAddr = 0;

	switch (selection)
	{
		// RAM read
		case 0:
			byte = ReadMem8(addr);
			break;
		// VRAM read
		case 1:
			byte = pvr_read32p<u8>(addr);
			break;
		// AICA RAM Read
		case 2:
			byte = aica::readAicaReg<u8>(addr);
			break;
		// ELAN RAM Read
		case 3:
			byte = elan::read_elanram<u8>(addr);
			break;
		default:
			break;
	}
	return byte;
}

void Vanguard_pokebyte(long long addr, unsigned char val, int selection)
{
	u16 value = 0;

	switch (selection)
	{
		// RAM write
		case 0:
			WriteMem8(addr, val);
			break;
		// VRAM write
		case 1:
			// Since we can't only write 8 bits at a time, we need to grab the next address and fake a 16 bit write instead
			value = val << 8 | (pvr_read32p<u8>(addr));
			pvr_write32p<u16>(addr, value);
			break;
		// AICA RAM Write
		case 2:
			aica::writeAicaReg(addr, val);
			break;
		// ELAN RAM Write
		case 3:
			elan::write_elanram(addr, val);
			break;
		default:
			break;

	}

}

bool VanguardClient::ok_to_corestep = false;
void Vanguard_pause(bool pauseUntilCorrupt)
{
	EventManager::event(Event::Pause);
	VanguardClient::ok_to_corestep = false;
}

void Vanguard_resume()
{
	EventManager::event(Event::Resume);
	VanguardClient::ok_to_corestep = true;
}


void Vanguard_savesavestate(BSTR filename, bool wait)
{
    if (emu.state == Emulator::State::Running)
    {
		//Convert the BSTR sent by Vanguard to std::string
		std::string filename_converted = BSTRToString(filename);
		dc_Vanguardsavestate(filename_converted);
		EventManager::event(Event::Resume);
    }
}

// Naomi games weren't happy when you tried to load a savestate right away, so we'll set a flag instead
bool VanguardClient::load_savestate = false;
std::string VanguardClient::state_to_load;
void Vanguard_loadsavestate(BSTR filename)
{
  // Convert the BSTR sent by Vanguard to std::string
  std::string filename_converted = BSTRToString(filename);

  VanguardClient::state_to_load = filename_converted;
  VanguardClient::load_savestate = true;
}


bool VanguardClient::loading = false;
void Vanguard_loadROM(BSTR filename)
{
  VanguardClient::loading = true;
  VanguardClient::ok_to_corestep = false;

  std::string converted_filename = BSTRToString(filename);

  gui_start_game(converted_filename);


  MSG msg;
  // We have to do it this way to prevent deadlock due to synced calls. It sucks but it's required
  // at the moment
  while (VanguardClient::loading)
  {
    Sleep(20);
    //these lines of code perform the equivalent of the Application.DoEvents method
    ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    ::GetMessage(&msg, NULL, 0, 0);
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  Sleep(100);  // Give the emu thread a chance to recover
  
}

void Vanguard_finishLoading()
{
  VanguardClient::loading = false;
}

void Vanguard_closeGame()
{
	gui_stop_game();
}

void Vanguard_prepShutdown()
{
	gui_stop_game();
}

bool VanguardClient::close_emulator = false;
void Vanguard_forceStop()
{
	// Trying to close the emulator from here can cause it to hang occasionally, so just set a flag to handle it normally during os_DoEvents()
	VanguardClient::close_emulator = true;
}

std::string VanguardClient::system_core = "EMPTY";
char* Vanguard_getSystemCore()
{
	// store the output as a string, then convert it to char*
	std::string tmp = VanguardClient::system_core;

	std::vector<char> _output(tmp.begin(), tmp.end());
	_output.push_back('\0');

	char* output = (char*)LocalAlloc(LMEM_FIXED, _output.size() + 1);
	if (!output)
		return NULL;

	memcpy(output, _output.data(), _output.size() + 1);

	return output;
}

// Saves all required emulator settings and returns it to the hook DLL to store with the savestate
char* Vanguard_saveEmuSettings()
{
	// create a new settings class and store all values
	VanguardSettings _settings;
	_settings.SaveSettings();

	// write the json data to a stringstream
	std::ostringstream out;
	FormatJsonData(_settings, out);

	// store the output as a string, then convert it to char*
	std::string tmp = out.str();

	std::vector<char> _output(tmp.begin(), tmp.end());
	_output.push_back('\0');

	char* output = (char*)LocalAlloc(LMEM_FIXED, _output.size() + 1);
	if (!output)
		return NULL;

	memcpy(output, _output.data(), _output.size() + 1);
	return output;
}

// Loads all required emulator settings sent by the hook DLL before loading the savestate
void Vanguard_loadEmuSettings(BSTR settings)
{
	JsonParser::JsonValue parsed_settings = JsonParser::ParseJson(settings);

	VanguardSettings _settings;
	_settings.LoadSettings(parsed_settings);
}

//converts a BSTR received from the Vanguard client to std::string
std::string BSTRToString(BSTR string)
{
	std::wstring ws(string, SysStringLen(string));
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string converted_string = converter.to_bytes(ws);
	return converted_string;
}

std::string getDirectory()
{
  char buffer[MAX_PATH] = {0};
  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  return std::string(buffer).substr(0, pos);
}

// formats the saved settings into a JSON format
void FormatJsonData(VanguardSettings& settings, std::ostringstream& json_string)
{
	// beginning of json string
	json_string << "{\n";

	// iterate through all settings
	for (int i = 0; i < settings.array.size(); i++)
	{
		std::string value(settings.to_string(settings.array[i].second));

		// flycast stores bools as "yes" and "no" so we need this hack
		if (std::holds_alternative<bool>(settings.array[i].second))
		{
			if (settings.to_string(settings.array[i].second).find("false"))
			{
				json_string << "  \"" << settings.array[i].first
					<< "\": " << "yes";
			}
			else
			{
				json_string << "  \"" << settings.array[i].first
					<< "\": " << "no";
			}
		}
		else
		{
			json_string << "  \"" << settings.array[i].first
				<< "\": " << settings.to_string(settings.array[i].second);
		}

		json_string << "  \"" << settings.array[i].first
			<< "\": " << value;

		// if the value is a whole number float, add ".0" so the parser understands
		if (std::holds_alternative<float>(settings.array[i].second) &&
			value.length() == 1)
		{
			json_string << ".0";
		}

		// only add a comma if there are more values to be parsed
		if (i + 1 < settings.array.size())
			json_string << ",\n";
		else
			json_string << "\n";
	}

	// end of json string
	json_string << "}";
}
