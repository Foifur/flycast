#include "..\core\Vanguard\VanguardHelpers.h"
#include "..\core\Vanguard\VanguardClientInitializer.h"

#include "..\core\hw\sh4\sh4_mem.h"
#include "..\core\hw\pvr\pvr_mem.h"
#include "..\core\hw\aica\aica_if.h"
#include "..\core\hw\pvr\elan.h"
#include "..\core\emulator.h"
#include "..\core\ui\gui_util.h"
#include "..\core\input\gamepad_device.h"


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
		case 3:
			elan::write_elanram(addr, val);
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


void Vanguard_loadsavestate(BSTR filename)
{
  // Convert the BSTR sent by Vanguard to std::string
  std::string filename_converted = BSTRToString(filename);

  emu.stop();
  dc_Vanguardloadstate(filename_converted);
  emu.start();
  EventManager::event(Event::Resume);
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
	const char* output_name = VanguardClient::system_core.c_str();
	size_t ulSize = strlen(output_name + sizeof(char));
	char* returnValue = NULL;

	returnValue = (char*)::CoTaskMemAlloc(ulSize);
	strcpy(returnValue, output_name);
	return returnValue;
}

//converts a BSTR received from the Vanguard client to std::string
std::string BSTRToString(BSTR string)
{
  std::wstring ws(string, SysStringLen(string));
  //std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::string converted_string = _bstr_t(string);
  return converted_string;
}

std::string getDirectory()
{
  char buffer[MAX_PATH] = {0};
  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  return std::string(buffer).substr(0, pos);
}
