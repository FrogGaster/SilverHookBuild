#include "includes.h"
#include "sdk.h"
#include "kiero/minhook/include/MinHook.h"
#include <vector>
#include <Windows.h>
#include <iostream>
#include <string>
#include <Psapi.h>
#include <random>
#include "imgui/imgui_internal.h"

#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

//HOI4 Version 1.17

typedef bool(__fastcall* CSessionPost)(void* pThis, CCommand* pCommand, bool ForceSend);
CSessionPost CSessionPostHook;
CSessionPost CSessionPostTramp;

typedef CAddPlayerCommand* (__fastcall* GetCAddPlayerCommand)(void* pThis, CString* User, CString* Name, DWORD* unknown, int nMachineId, bool bHotjoin, __int64 a7);
GetCAddPlayerCommand CAddPlayerCommandHook;
GetCAddPlayerCommand CAddPlayerCommandTramp;

typedef CStartGameCommand* (__fastcall* GetCStartGameCommand)(void* pThis);
GetCStartGameCommand CStartGameCommandFunc;
GetCStartGameCommand CStartGameCommandTramp;

typedef CRemovePlayerCommand* (__fastcall* GetCRemovePlayerCommand)(void* pThis, int nMachineId, int eReason, long long a4);
GetCRemovePlayerCommand CRemovePlayerCommandHook;
GetCRemovePlayerCommand CRemovePlayerCommandTramp;

typedef CCrash* (__fastcall* GetCCrash)(void* pThis, unsigned int a1);
GetCCrash CCrashFunc;
GetCCrash CCrashTramp;

typedef CPauseGameCommand* (__fastcall* GetCPauseGameCommand)(void* pthis, __int64 a2, char a3);
GetCPauseGameCommand CPauseGameFunc;
GetCPauseGameCommand CPauseGameTramp;

typedef CAiEnableCommand* (__fastcall* GetEnableAI)(void* pThis, int* tag, int toggled);
GetEnableAI AIEnableFunc;


typedef LPVOID(__fastcall* GetCCommand)(__int64 a1);
GetCCommand GetCCommandFunc;

typedef CGameSpeed* (__fastcall* GetCGameSpeed)(void* pThis);
GetCGameSpeed IncreaseSpeedFunc;

//Boost
typedef EmptyTest* (__fastcall* GetCustomDiffM)(void* pThis, __int64 a2, int a3);
GetCustomDiffM BoostFunc;
GetCustomDiffM BoostTramp;

typedef EmptyTest* (__fastcall* GetAutoSave)(void* pThis, bool Send);
GetAutoSave AutoSaveFunc;


//Tagswitch
typedef __int64(__fastcall* CGameStateSetPlayer)(void* pThis, int* Tag);
CGameStateSetPlayer CGameStateSetPlayerHook;
CGameStateSetPlayer CGameStateSetPlayerTramp;


//Antiban
typedef bool(__fastcall* GetNetworkDisconnect)(void* pThis);
GetNetworkDisconnect NetDisconnectHook;
GetNetworkDisconnect NetDisconnectTramp;

typedef void(__fastcall* GetSessionSetState)(__int64 pThis, int a2);
GetSessionSetState SetStateHook;
GetSessionSetState SetStateTramp;

typedef void(__fastcall* GetMatchmakingLeaveLobby)(void* pThis);
GetMatchmakingLeaveLobby MatchmakingLeaveHook;
GetMatchmakingLeaveLobby MatchmakingLeaveTramp;

//Server
typedef void(__fastcall* GetAddHumanItem)(void* pThis, CHuman* Human);
GetAddHumanItem HumanHook;
GetAddHumanItem HumanTramp;

typedef bool(__fastcall* GetProxyServer)(void* pThis, bool bReconnect, int nMinTimeout, __int64 a4, __int64 a5);
GetProxyServer CProxyServerHook;
GetProxyServer CProxyServerTramp;


//Log
typedef void*(__fastcall* GetLogStream)(void* pThis, const char* Log);
GetLogStream CLogStreamHook;
GetLogStream CLogStreamTramp;

typedef void* (__fastcall* GetOperator)(void* pThis, __int64* Log);
GetOperator COperatorHook;
GetOperator COperatorTramp;

typedef void* (__fastcall* GetCLog)(void* pThis, void* a2, const char* pFile, int LineNum, int Category);
GetCLog CLogHook;
GetCLog CLogTramp;

typedef void* (__fastcall* GetCToken)(void* pThis, int Type, const char* pString);
GetCToken CTokenHook;
GetCToken CTokenTramp;

typedef int (*GetTokenInitialiser)();
GetTokenInitialiser CTokenInitHook;


//Savegame (too scary, removed)
char NameBuffer[64] = "filename";
char ExtensionBuffer[16] = ".hoi4";
CSaveGameMeta* LoadSaveMeta;
CSaveFile* AddExtMeta;
CCountryTag* gTag;
int* iLoad;
CString* gOutStr;
CString* g_SelectedCountry = new CString;

//DEBUG
bool bShowPlayers = false;
bool dSessionPost = false;
bool bDebug = true;
bool bLog = false;
bool bAddress = false;
bool bFindChost = false;
bool bConsoleAlloc = false;
bool bMapTokens = false;

//SessionPost
void* pCSession = nullptr;


//AddPlayerCommand
void* pCAddPlayer = nullptr;
bool bSpoofSteam = false;
bool bLongName = false;
bool bGhost2 = false;
bool bLobbyHostRemoval = false;
unsigned int gMachineID;

//RemovePlayerCommand
void* pCRemovePlayer = nullptr;
ERemovalReason dReason;
int RMID;
__int64 dRUnknown;


//GameSetState
void* pCGameState = nullptr;

bool Log = true;
bool MenuLogged = false;
bool DX11Success = false;

//Vector
struct CountryBoost
{
	CString* pGameCString;
	const char* pName;
	float currentValue;     
	float defaultValue;     
};

std::vector<CountryBoost> gCountryBoosts;
std::vector<std::string> IPVector;


//Debug
uintptr_t pInstance;
uintptr_t globalAddr;
//const char* items[] = { "Apple", "Banana", "Cherry" };
static int current_item = 0; // index of selected item


//ImGui Start
bool bMenuOpen = true;

//Buffer
char TagBuffer[16];
char PlayerName[1024];
char BoostBuffer[64];

//Cheat
bool bMPLobby = false;
bool bCrasher = false;
bool bFreeTemp = false;
bool bFreeUpgrade = false;
bool bGameSpeed = false;
bool bSeeCombat = false;
bool bSeeArmy = false;
bool bSeeDebug = false;

//Imgui End


//menus
Menu eMenu;
bool bLobbyMenu = false;
bool bGameMenu = false;
bool bSettingsMenu = false;
bool bEnabletdebug = false;


//Server - Lobby
unsigned int iLargestPacketSize = 1;
_ENetHost** LastENetHost;
CBlob* SwapData;
bool bShouldFreeze = false;
bool bRefuseConnect = false;
void* pCIdleLobby = nullptr;
int iHost = 0;
std::string CurrentHost = "";
void* pCSessionConfigToken = nullptr;
void* pCGameLobby = nullptr;

bool bHotJoin = false;
bool bChecksum = false;

//Strengthen
int boost = 0;
__int64 CountryTag;


//Hook
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
uintptr_t GameBase = (uintptr_t)GetModuleHandleA("hoi4.exe");


void WriteLog(const std::string& text)
{
	char appDataPath[MAX_PATH];
	if (!GetEnvironmentVariableA("APPDATA", appDataPath, MAX_PATH))
		return;

	// Get current date/time
	std::time_t now = std::time(nullptr);
	std::tm localTime{};
	localtime_s(&localTime, &now);

	// Format date for filename: DD-MM-YYYY
	std::ostringstream dateStream;
	dateStream << std::put_time(&localTime, "%d-%m-%Y");

	// Format time for log entry: HH:MM:SS
	std::ostringstream timeStream;
	timeStream << std::put_time(&localTime, "%H:%M:%S");

	std::string folderPath = std::string(appDataPath) + "\\Silverhook";
	std::string filePath = folderPath + "\\log" + dateStream.str() + ".txt";

	// Create directory if it doesn't exist
	CreateDirectoryA(folderPath.c_str(), NULL);

	// Open or create file for appending
	HANDLE hFile = CreateFileA(
		filePath.c_str(),
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE)
		return;

	std::string finalText = "[" + timeStream.str() + "] " + text + "\r\n";

	DWORD bytesWritten;
	WriteFile(hFile, finalText.c_str(), (DWORD)finalText.size(), &bytesWritten, NULL);

	CloseHandle(hFile);
}

uintptr_t FindPattern(char* pattern, char* mask)
{
	uintptr_t base = GameBase;
	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)GameBase, &modInfo, sizeof(MODULEINFO));
	uintptr_t size = modInfo.SizeOfImage;

	uintptr_t patternLength = (uintptr_t)strlen(mask);

	for (uintptr_t i = 0; i < size - patternLength; i++)
{
	bool found = true;
	for (uintptr_t j = 0; j < patternLength; j++)
	{
		if (mask[j] != '?' && pattern[j] != *(char*)(base + i + j))
		{
			found = false;
			break; 
		}
	}
	if (found)
		return base + i;
}

	return 0xDEADBEEF;
}


//Console
void printm(const std::string& str) {
	try {
		FILE* fDummy;

		if (freopen_s(&fDummy, "CONOUT$", "w", stdout) != 0) {
			throw std::runtime_error("Failed to redirect stdout to CONOUT$.");
		}

		if (str.empty()) {
			std::cerr << "[SilverHook] Error: Empty string provided for printing." << std::endl;
			return;  
		}

		std::cout << "[SilverHook] " << str << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[SilverHook] Error: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[SilverHook] Unknown error occurred while printing." << std::endl;
	}
}

// made to seperate somthing, ignore
void printLog(const std::string& str) {
	try {
		
		FILE* fDummy;

		if (freopen_s(&fDummy, "CONOUT$", "w", stdout) != 0) {
			throw std::runtime_error("Failed to redirect stdout to CONOUT$.");
		}
		
		if (str.empty()) {
			std::cerr << "[SilverLog] Error: Empty string provided for printing." << std::endl;
			return;  
		}

		std::cout << "[SilverLog] " << str << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[SilverLog] Error: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[SilverLog] Unknown error occurred while printing." << std::endl;
	}
}


//Cheats
void PatchMemory(uintptr_t address, unsigned char* patch, DWORD size)
{
	DWORD oldProtect;
	VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((LPVOID)address, patch, size);
	VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
}


uintptr_t address = FindPattern(const_cast <char*>("\x74\x00\x83\xFA\x00\x75\x00\x48\x8B\xCB"), const_cast <char*>("x?xx?x?xxx"));
uintptr_t ReplaceAddress = FindPattern(const_cast <char*>("\x48\x8B\xCB\xE8\x00\x00\x00\x00\xEB\x00\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\x93"), const_cast <char*>("xxxx????x?xxxx????xxx"));

void MultiplayerLobbyHack()
{

	
	unsigned char patch[] = { 0x74, 0x0F, 0x83, 0xFA, 0x01, 0x00 }; 

	void* MLHmem = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (MLHmem != nullptr)
	{
		DWORD relativeOffset = ReplaceAddress - (address + sizeof(patch)); //1997D40
		memcpy(MLHmem, patch, sizeof(patch));
		*(BYTE*)((uintptr_t)MLHmem) = 0x0F;
		*(BYTE*)((uintptr_t)MLHmem + 1) = 0x83;
		*(DWORD*)((uintptr_t)MLHmem + 2) = relativeOffset;

		PatchMemory(address, (unsigned char*)MLHmem, sizeof(patch));
		VirtualFree(MLHmem, 0, MEM_RELEASE);
	}

}


uintptr_t BrigadeChangeCost = FindPattern(const_cast <char*>("\x48\x0F\xAF\x15\x00\x00\x00\x00\x4C\x8B\x01"), const_cast <char*>("xxxx????xxx"));
uintptr_t BrigadeGroupCost = FindPattern(const_cast <char*>("\x48\x0F\xAF\x0D\x00\x00\x00\x00\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x48\xF7\xE9\x48\xC1\xFA\x00\x48\x8B\xC2\x48\xC1\xE8\x00\x48\x03\xD0\x49\x01\x16"), const_cast <char*>("xxxx????xx????????xxxxxx?xxxxxx?xxxxxx"));
uintptr_t SupportCost = FindPattern(const_cast <char*>("\x48\x0F\xAF\x15\x00\x00\x00\x00\x48\xF7\xEA\x48\xC1\xFA\x00\x48\x8B\xC2\x48\xC1\xE8\x00\x48\x03\xD0\x48\x03\xFA"), const_cast <char*>("xxxx????xxxxxx?xxxxxx?xxxxxx"));

void FreeTemplate()
{
	unsigned char patch[] = {
		0x48, 0x31, 0xD2,  
		0x90, 0x90, 0x90, 0x90, 0x90  
	};

	PatchMemory(BrigadeChangeCost, patch, sizeof(patch));
	PatchMemory(BrigadeGroupCost, patch, sizeof(patch));
	PatchMemory(SupportCost, patch, sizeof(patch));
}

uintptr_t RampCost = FindPattern(const_cast <char*>("\x0F\xAF\x05\x00\x00\x00\x00\x03\x05\x00\x00\x00\x00\x48\x63\xC8"), const_cast <char*>("xxx????xx????xxx"));
uintptr_t BaseCost = FindPattern(const_cast <char*>("\x03\x05\x00\x00\x00\x00\x48\x63\xC8"), const_cast <char*>("xx????xxx"));

void FreeXP() {
	unsigned char RampPatch[]{
		0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x90
	};
	unsigned char BasePatch[]{
		0x83, 0xC0, 0x01, 0x90, 0x90, 0x90
	};

	PatchMemory(RampCost, RampPatch, sizeof(RampPatch));
	PatchMemory(BaseCost, BasePatch, sizeof(BasePatch));

}


uintptr_t iSeeCombat = FindPattern(const_cast <char*>("\x48\x8B\x41\x00\x80\xB8\x00\x00\x00\x00\x00\x75\x00\x48\x8B\x41"), const_cast <char*>("xxx?xx?????x?xxx"));

void SeeCombat() {
	unsigned char Patch[]{
		0xB0, 0x01, 0xC3  //See Combat
	};

	PatchMemory(iSeeCombat, Patch, sizeof(Patch));
}

void UnSeeCombat() {
	unsigned char Patch[]{
		0x48, 0x8B, 0x41  //Original Combat
	};
	PatchMemory(iSeeCombat, Patch, sizeof(Patch));
}


uintptr_t iDebug = FindPattern(const_cast <char*>("\x80\x3D\x00\x00\x00\x00\x00\x74\x00\x49\x8B\x8F"), const_cast <char*>("xx?????x?xxx"));

void EnableDebug() {
	unsigned char Patch[]{
	0x80, 0x3D, 0xC8, 0x9F, 0x48, 0x02, 0x01
	};

	PatchMemory(iDebug, Patch, sizeof(Patch));
}

uintptr_t RandombLog = FindPattern(const_cast <char*>("\x80\x3D\x00\x00\x00\x00\x00\x74\x00\x41\xB9\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x44\x8B\xCF"), const_cast <char*>("xx?????x?xx????xx????xxx????xxxx?x????xxx"));

void fRandombLog() {
	unsigned char Patch[]{
	0x80, 0x3D, 0x4A, 0xD4, 0x21, 0x02, 0x01
	};

	PatchMemory(RandombLog, Patch, sizeof(Patch));
}

uintptr_t pInstanceAddr = FindPattern(const_cast <char*>("\x4C\x8B\x3D\x00\x00\x00\x00\x49\x63\xB7\x00\x00\x00\x00\x48\x85\xF6\x7E\x00\x33\xDB"), const_cast <char*>("xxx????xxx????xxxx?xx"));

int pInstRel = *(int*)(pInstanceAddr + 3);
uintptr_t pInstanceInstruct = pInstanceAddr + 7;
uintptr_t GameState = pInstanceInstruct + pInstRel;

void PrintDifficultyCountryTags() {

	uintptr_t gameState = *(uintptr_t*)GameState;
	if (!gameState) return;

	int count = *(int*)(gameState + 1076);           
	uintptr_t arrayAddr = *(uintptr_t*)(gameState + 1064); 

	for (int i = 0; i < count; ++i) {
		uintptr_t itemPtr = *(uintptr_t*)(arrayAddr + i * 8);
		if (!itemPtr) continue;

		uintptr_t pSetting = *(uintptr_t*)(itemPtr + 8);
		if (!pSetting) continue;

		uintptr_t keyStructAddr = pSetting + 56;

		//char buf[256]{};
		const char* tag = *(const char**)keyStructAddr;

		int multiplier = *(int*)(itemPtr + 208); 

		printf("[%d] Tag: %s, Value: %d\n", i, tag, multiplier);
	}
}



void InitCountryBoosts() {
	gCountryBoosts.clear();

	uintptr_t gameState = *(uintptr_t*)GameState;
	if (!gameState) return;

	int count = *(int*)(gameState + 1076);            
	uintptr_t arrayAddr = *(uintptr_t*)(gameState + 1064);

	for (int i = 0; i < count; ++i) {
		uintptr_t itemPtr = *(uintptr_t*)(arrayAddr + i * 8);
		if (!itemPtr) continue;

		uintptr_t pSetting = *(uintptr_t*)(itemPtr + 8);
		if (!pSetting) continue;

		uintptr_t keyStructAddr = pSetting + 56;
		CString* pGameCString = (CString*)keyStructAddr;
		const char* pName = *(const char**)keyStructAddr;
		int multiplier = *(int*)(itemPtr + 208);

		CountryBoost cb;
		cb.pGameCString = pGameCString;
		cb.pName = pName;
		cb.currentValue = multiplier;
		cb.defaultValue = multiplier;

		gCountryBoosts.push_back(cb);
	}
}

// super long patterns but i couldnt figure out how to shorten them haha
uintptr_t FOWByte = FindPattern(const_cast <char*>("\x38\x05\x00\x00\x00\x00\x0F\x94\xC0\x44\x0F\xB6\xC8\x88\x05\x00\x00\x00\x00\x45\x8B\xC1\x49\x83\xF0\x00\x49\x83\xC0\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x45\x84\xC9\x48\x0F\x44\xD0\xE8\x00\x00\x00\x00\x48\x8B\x0D"), const_cast <char*>("xx????xxxxxxxxx????xxxxxx?xxx?xxx????xxx????xxxxxxxx????xxx"));

uintptr_t FOWInstruct = FOWByte;
uintptr_t FOWrip = FOWInstruct + 6;
int FOWrel32 = *(int*)(FOWInstruct + 2);
uintptr_t FOWByteAddr = FOWrip + FOWrel32;


uintptr_t ATINSTRUCT = FindPattern(const_cast <char*>("\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x8E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x8E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x41\x8B\x8E"), const_cast <char*>("xxx????xxx????x????xxx????xxx????x????xxx"));

uintptr_t AT_INSTR = ATINSTRUCT;
uintptr_t instr_rip = AT_INSTR + 7;
int rel33 = *(int*)(AT_INSTR + 3);
uintptr_t traitsByteAddr = instr_rip + rel33;



void ToggleByte(uintptr_t byte) {
	unsigned char* pByte = reinterpret_cast<unsigned char*>(byte);


	*pByte = (*pByte == 0) ? 1 : 0;
}

template <typename T>
T ReadMemory(uintptr_t address)
{
	return *reinterpret_cast<T*>(address);
}

uintptr_t OffsetCalculator(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets)
{
	uintptr_t address = baseAddress;
	for (uintptr_t offset : offsets)
	{
		address = ReadMemory<uintptr_t>(address);
		address += offset;
	}
	return address;
}


void ChangeIntAddressValue(uintptr_t bAddr, uintptr_t bOff, int Tag)
{

	uintptr_t baseAddress = bAddr; 
	
	std::vector<uintptr_t> offsets = { bOff };
	
	uintptr_t finalAddress = OffsetCalculator(GameBase + baseAddress, offsets);

	DWORD* pValue = reinterpret_cast<DWORD*>(finalAddress);
	DWORD oldProtect;

	VirtualProtect(pValue, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &oldProtect);

	*pValue = Tag;

	VirtualProtect(pValue, sizeof(DWORD), oldProtect, &oldProtect);

}

void ChangeByteAddressValue(uintptr_t addr)
{
	uintptr_t address = GameBase + addr; 
	BYTE* pValue = (BYTE*)address;
	DWORD oldProtect;

	VirtualProtect(pValue, sizeof(BYTE), PAGE_EXECUTE_READWRITE, &oldProtect);
	
	if (*pValue == 1)
	{
		*pValue = 0;
	}
	else
	{
		*pValue = 1;
	}
	
	VirtualProtect(pValue, sizeof(BYTE), oldProtect, &oldProtect);
}


//IngameFunctions
void Crasher(int a1) {
	CCrash* x = (CCrash*)GetCCommandFunc(48);
	x = CCrashFunc(x, a1);
	CSessionPostTramp(pCSession, x, true);
}


void StartGameFunc() {
	CStartGameCommand* StartGame = (CStartGameCommand*)GetCCommandFunc(41);
	StartGame = CStartGameCommandFunc((StartGame));
	CSessionPostTramp(pCSession, StartGame, true);
}

void RemovalReason(ERemovalReason e) {
	
	CRemovePlayerCommand* RemovePlayer = (CRemovePlayerCommand*)GetCCommandFunc(41);
	RemovePlayer = CRemovePlayerCommandTramp(RemovePlayer, RMID, e, dRUnknown);
	CSessionPostTramp(pCSession, RemovePlayer, true);
}



void DefaultImGui() {
	ImVec4 NormalColor = ImVec4(0.047f, 0.047f, 0.047f, 1.0f);
	ImVec4 HoverColor = ImVec4(0.066f, 0.066f, 0.066f, 1.0f);
	ImVec4 ActiveColor = ImVec4(0.076f, 0.076f, 0.076f, 1.0f);
	ImVec4 BackgroundColor = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
	ImVec4 PureBlackColor = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
	ImVec4 WhiteColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Button] = NormalColor;  // Normal Colors
	style.Colors[ImGuiCol_FrameBg] = NormalColor;
	style.Colors[ImGuiCol_ResizeGrip] = NormalColor;
	style.Colors[ImGuiCol_Separator] = NormalColor;

	style.Colors[ImGuiCol_ButtonHovered] = HoverColor; //Hovered Colors
	style.Colors[ImGuiCol_ResizeGripHovered] = HoverColor;
	style.Colors[ImGuiCol_FrameBgHovered] = HoverColor;
	style.Colors[ImGuiCol_SeparatorHovered] = HoverColor;

	style.Colors[ImGuiCol_ButtonActive] = ActiveColor; // Active Colors
	style.Colors[ImGuiCol_ResizeGripActive] = ActiveColor;
	style.Colors[ImGuiCol_SeparatorActive] = ActiveColor;
	style.Colors[ImGuiCol_FrameBgActive] = ActiveColor;

	style.Colors[ImGuiCol_WindowBg] = BackgroundColor; // Background

	style.Colors[ImGuiCol_TitleBg] = PureBlackColor; // Title
	style.Colors[ImGuiCol_TitleBgCollapsed] = PureBlackColor;
	style.Colors[ImGuiCol_TitleBgActive] = PureBlackColor;

	style.Colors[ImGuiCol_CheckMark] = WhiteColor; // Checkmark
}

void LightMenu2() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.75f, 0.75f, 0.75f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	
}



void InitImGui()
{
	ImGui::CreateContext();
	ImGui::StyleColorsLight();
	
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 18.0f, NULL);
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	
	//DefaultImGui();
	LightMenu2();
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			InitImGui();
			ImGui::SetNextWindowSize(ImVec2(350, 1050));
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	//Controls
	if (GetAsyncKeyState(VK_INSERT) & 1)
		bMenuOpen = !bMenuOpen;


	if (bMenuOpen)
	{
		if (Log && !MenuLogged) {
			WriteLog("Menu Displayed");
			MenuLogged = true;
		}
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureMouse = true;
		io.WantCaptureKeyboard = true;

		if (GetAsyncKeyState(VK_LBUTTON))
		{
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
		}
		else
		{
			io.MouseReleased[0] = true;
			io.MouseDown[0] = false;
			io.MouseClicked[0] = false;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


		ImGui::Begin("SilverHook 3.1 Final", &bMenuOpen);

		ImGui::Text("Menus");
		ImGui::Columns(2);
		if (ImGui::Button(u8"Lobby Menu", ImVec2(140, 28)))
		{
			eMenu = eLobbyMenu;
		}; 
		ImGui::NextColumn();
		if (ImGui::Button(u8"Ingame Menu", ImVec2(140, 28)))
		{
			eMenu = eIngameMenu;
		};
		ImGui::NextColumn();
		if (ImGui::Button(u8"Boost Menu", ImVec2(140, 28)))
		{
			eMenu = eBoostMenu;
		}; 
		
		ImGui::NextColumn();
		if (ImGui::Button(u8"Debug Menu", ImVec2(140, 28)))
		{
			eMenu = eDebugMenu;
		};

		
		ImGui::Columns(1);
		if (eMenu == eLobbyMenu)
		{
			ImGui::Text("Host IP:");
			ImGui::SameLine();
			ImGui::Text(CurrentHost.c_str());
			ImGui::Text("Cheats");
			ImGui::Checkbox("KILL Lobby INSTANTLY", &bLobbyHostRemoval);
			ImGui::Checkbox("Spoof Steam Name", &bSpoofSteam);
			ImGui::Checkbox("AntiBan", &bRefuseConnect);
			ImGui::Checkbox("Multiplayer Lobby Hack", &bMPLobby);	
			ImGui::Checkbox("Join as Ghost", &bGhost2);
			ImGui::Checkbox("Hotjoin bypass", &bHotJoin);
			ImGui::Checkbox("Checksum bypass", &bChecksum);
			ImGui::Text("Function Calls");
			ImGui::Text("");
			ImGui::Columns(2);
			if (ImGui::Button("Start Game", ImVec2(140, 28)) && pCSession != nullptr)
			{

				StartGameFunc();

			}
			ImGui::NextColumn();
			if (ImGui::Button("E D", ImVec2(140, 28)) && pCSession != nullptr)
			{

				Crasher(60927); // ?

			}
			ImGui::NextColumn();
			if (ImGui::Button("D D", ImVec2(140, 28)) && pCSession != nullptr)
			{

				Crasher(0);

			}
			ImGui::NextColumn();
			ImGui::Columns(1);
			ImGui::Text("Fake Name");
			ImGui::InputText("", PlayerName, IM_ARRAYSIZE(PlayerName));
			ImGui::SameLine();


			ImGui::Text("");
			ImGui::Text("Credits: Silver");
		}


		if (eMenu == eIngameMenu) {
			ImGui::Text("Cheats");
			
			ImGui::Checkbox("Free Templates", &bFreeTemp);
			ImGui::Checkbox("Free Upgrades", &bFreeUpgrade);
			ImGui::Text("Free Upgrades for Mech!");

			ImGui::Checkbox("See Combat", &bSeeCombat);

			ImGui::Checkbox("Crasher", &bCrasher);
			ImGui::Text("Only use crasher when\ngame has finished loading.");
			ImGui::Text("Function Calls");
			ImGui::Columns(2);
			if (ImGui::Button("Enable AI on all", ImVec2(140, 28)) && pCSession != nullptr)
			{
				int i = 0;
				do {
					int* TagPtr = &i;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 2);
					CSessionPostTramp(pCSession, EnableAI, 1);
					i++;
				} while (i <= 100);
			}
			ImGui::NextColumn();
			if (ImGui::Button("Disable AI on all", ImVec2(140, 28)) && pCSession != nullptr)
			{
				int i = 0;
				do {
					int* TagPtr = &i;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 0);
					CSessionPostTramp(pCSession, EnableAI, 1);
					i++;
				} while (i <= 100);
			}
			ImGui::NextColumn();
			if (ImGui::Button("Infinite Pause", ImVec2(140, 28)) && pCSession != nullptr) {
				CString* empty = new CString;
				CPauseGameCommand* UnpauseGame = (CPauseGameCommand*)GetCCommandFunc(88);
				UnpauseGame = CPauseGameFunc(UnpauseGame, (__int64)empty, 1);
				CSessionPostTramp(pCSession, UnpauseGame, true);
			}
			ImGui::NextColumn();
			if (ImGui::Button("Ghost Pause", ImVec2(140, 28)) && pCSession != nullptr) {
				CString* empty = new CString;
				CPauseGameCommand* UnpauseGame = (CPauseGameCommand*)GetCCommandFunc(88);
				UnpauseGame = CPauseGameFunc(UnpauseGame, (__int64)empty, 0);
				CSessionPostTramp(pCSession, UnpauseGame, true);
			}
			ImGui::NextColumn();
			if (ImGui::Button("Increase Speed", ImVec2(140, 28)) && pCSession != nullptr) {
				CGameSpeed* IncreaseSpeed = (CGameSpeed*)GetCCommandFunc(48);
				IncreaseSpeed = IncreaseSpeedFunc(IncreaseSpeed);
				CSessionPostTramp(pCSession, IncreaseSpeed, true);
			}

			ImGui::NextColumn();
			if (ImGui::Button("Enable Debug", ImVec2(140, 28)) && pCSession != nullptr)
			{
				EnableDebug();
			}
			ImGui::NextColumn();
			if (ImGui::Button("FOW", ImVec2(140, 28)) && pCSession != nullptr)
			{
				ToggleByte(FOWByteAddr);
			}
			ImGui::NextColumn();
			if (ImGui::Button("AllowTraits", ImVec2(140, 28)) && pCSession != nullptr)
			{
				ToggleByte(traitsByteAddr);
			}
			ImGui::Columns(1);
			ImGui::Text("Hold CTRL+ALT after enabling\ndebug to see tags");
			ImGui::Text("");
			ImGui::Text("Country Tag:");
			ImGui::SetNextItemWidth(70.f);
			ImGui::InputText("", TagBuffer, IM_ARRAYSIZE(TagBuffer));
			ImGui::SameLine();

			if (ImGui::Button("Enable AI") && pCSession != nullptr)
			{
				std::string sTagBuffer = TagBuffer;
				char* endptr;
				int a1 = std::strtol(TagBuffer, &endptr, 10);

				if (sTagBuffer.length() > 0)
				{
					int* TagPtr = &a1;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 2);
					CSessionPostTramp(pCSession, EnableAI, 1);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Disable AI") && pCSession != nullptr)
			{
				std::string sTagBuffer = TagBuffer;
				char* endptr;
				int a1 = std::strtol(TagBuffer, &endptr, 10);
				if (sTagBuffer.length() > 0)
				{
					int* TagPtr = &a1;
					CAiEnableCommand* DisableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					DisableAI = AIEnableFunc(DisableAI, TagPtr, 0);
					CSessionPostTramp(pCSession, DisableAI, 1);
				}
			}
			if (ImGui::Button("Tagswitch") && pCSession != nullptr)
			{
				std::string sTagBuffer = TagBuffer;

				if (sTagBuffer.length() > 0)
				{
					int Tag = std::stoi(sTagBuffer);
					int* pTag = &Tag;
					if (pCGameState != nullptr)
						CGameStateSetPlayerTramp(pCGameState, pTag);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				memset(TagBuffer, 0, sizeof(TagBuffer));
			}
			ImGui::Text("");
			ImGui::Text("Credits: Silver");
		}



		if (eMenu == eBoostMenu) {
			ImGui::Text("");
			ImGui::Text("(THX polux) Boost Amount:");
			ImGui::SetNextItemWidth(70.f);
			ImGui::InputText("", BoostBuffer, IM_ARRAYSIZE(BoostBuffer));
			ImGui::SameLine();

			if (ImGui::Button("Boost") && pCSession != nullptr)
			{
				std::string sBoostBuffer = BoostBuffer;
				
				if (sBoostBuffer.length() > 0)
				{
					boost = std::stoi(BoostBuffer);
					EmptyTest* Strength = (EmptyTest*)GetCCommandFunc(88);
					boost = boost * 2500;
					Strength = BoostFunc(Strength, (__int64)g_SelectedCountry, boost);
					CSessionPostTramp(pCSession, Strength, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("1m"))
			{
				boost = 1000000;
				
				EmptyTest* Strength = (EmptyTest*)GetCCommandFunc(88);
				
				Strength = BoostFunc(Strength, (__int64)g_SelectedCountry, boost);
				
				CSessionPostTramp(pCSession, Strength, true);

				memset(BoostBuffer, 0, sizeof(BoostBuffer));
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				boost = 0;
				EmptyTest* Strength = (EmptyTest*)GetCCommandFunc(88);
				Strength = BoostFunc(Strength, (__int64)g_SelectedCountry, boost);
				CSessionPostTramp(pCSession, Strength, true);
				memset(BoostBuffer, 0, sizeof(BoostBuffer));
			}
			if (!gCountryBoosts.empty()) {
				// preview name for the combo button
				const char* previewRaw = gCountryBoosts[current_item].pName;

				// strip prefix if present and capitalize
				std::string previewName;
				if (previewRaw) {
					const char* tagStart = strstr(previewRaw, "custom_diff_");
					if (tagStart)
						tagStart += strlen("custom_diff_"); // move past prefix (apparently there is custom_diff_weak etc for mods, so just remove custom diff for looks.
					else
						tagStart = previewRaw;

					previewName = tagStart;

					// capitalize
					for (char& c : previewName)
						c = toupper(c);
				}
				else {
					previewName = "???";
				}

				if (ImGui::BeginCombo("Countries", previewName.c_str())) {
					for (int n = 0; n < gCountryBoosts.size(); n++) {
						const char* rawName = gCountryBoosts[n].pName;

						// strip prefix and capitalize
						const char* tagStart = strstr(rawName, "custom_diff_");
						if (tagStart)
							tagStart += strlen("custom_diff_");
						else
							tagStart = rawName;

						std::string itemName = tagStart;
						for (char& c : itemName)
							c = toupper(c);

						bool is_selected = (current_item == n);

						if (ImGui::Selectable(itemName.c_str(), is_selected)) {
							current_item = n;
							g_SelectedCountry = gCountryBoosts[n].pGameCString;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
			}
			if (ImGui::Button("Re-Initialise"))
			{
				InitCountryBoosts();
			}
			ImGui::Text("CountryTag: ");
			ImGui::SameLine();
			std::string tag = std::to_string(CountryTag);
			ImGui::Text(tag.c_str());
			ImGui::SameLine();
			ImGui::Text("");
			ImGui::Text("Credits: Silver");
		}



		if (eMenu == eDebugMenu) 
		{
			//ImGui::Text("Press Numpad 9 to display all IPs"); forgot if this exists, ppl wont use anyways
			ImGui::Text("Just for hax0r, thanks to polux and armer for da help finding things");
			ImGui::Checkbox("Debug", &bDebug);
			ImGui::Checkbox("SessionPost", &dSessionPost);
			ImGui::Checkbox("Players", &bShowPlayers);
			ImGui::Checkbox("Hoi4 Log", &bLog);
			ImGui::Checkbox("Show Host IP", &bFindChost);
			if (ImGui::Button("print stuff"))
			{
				printm("Hello"); //For other testing, removed.
			}
			
			
		}
		

		if (bCrasher) {
			Crasher(1);
			//add autosave here 2 break, forgot to add and IDC
		}
		if (bMPLobby) {
			MultiplayerLobbyHack();
			
		}
		if (bDebug && !bConsoleAlloc) {
				AllocConsole();
				bConsoleAlloc = true;
		}
		if (bSeeCombat) {
			SeeCombat();
		}
		else {
			UnSeeCombat();
		}
		if (bFreeTemp) {
			FreeTemplate();
		}
		if (bFreeUpgrade) {
			FreeXP();
		}


		ImGui::End();
		ImGui::Render();
		pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	return oPresent(pSwapChain, SyncInterval, Flags);
}





bool __fastcall hkCSessionPost(void* pThis, CCommand* pCommand, bool ForceSend)
{
	
	pCSession = pThis;
	if (bDebug && dSessionPost) {
		printm("CSP Called");
	}
	return CSessionPostTramp(pThis, pCommand, ForceSend);
}

CAddPlayerCommand* __fastcall hkCAddPlayerCommand(CAddPlayerCommand* pThis, CString* User, CString* Name, DWORD* unknown, int nMachineId, bool bHotjoin, __int64 a7)
{
	InitCountryBoosts();
	if (bLobbyHostRemoval)
		nMachineId = 1;
	gMachineID = nMachineId;
	if (bSpoofSteam)
	{
		const char* Name = PlayerName;
		User->assign(Name, strlen(Name));
	}

	if (bLongName)
	{
		const char* Long = "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW";
		User->assign(Long, strlen(Long));
		Name->assign(Long, strlen(Long));

	}
	
	int i = 0;
	int* TagPtr = &i;
	CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
	 
	if (bDebug) {
		printm("APC Called");
		printm("Machine ID:");
		std::string x = std::to_string(nMachineId);
		printm(x);
	}

	if(!bGhost2)
		return CAddPlayerCommandTramp(pThis, (CString*)User, (CString*)Name, unknown, nMachineId, bHotjoin, a7);

	if(bGhost2)
		return (CAddPlayerCommand * )AIEnableFunc(EnableAI, TagPtr, 0);

	return CAddPlayerCommandTramp(pThis, (CString*)User, (CString*)Name, unknown, nMachineId, bHotjoin, a7);
}

CRemovePlayerCommand* __fastcall hkCRemovePlayerCommand(void* pThis, int _machineID, ERemovalReason eReason, __int64 a4)
{

	pCRemovePlayer = pThis; RMID = _machineID; dReason = eReason; dRUnknown = a4;
	bRefuseConnect = false;
	if (bDebug) {
		printm("RPC Called");
	}
	
	return CRemovePlayerCommandTramp(pThis, _machineID, eReason, a4);

}

__int64 __fastcall hkCGameStateSetPlayer(void* pThis, int* Tag)
{

	pCGameState = pThis;
	if (bDebug) {
		printm("CGSSP Called");
	}
	return CGameStateSetPlayerTramp(pThis, Tag);
}

EmptyTest* __fastcall hkCustomDiffM(void* pThis, __int64 Tag, int Boost) {
	if (bDebug) {
		printm("DiffMultipley");
		printm(std::to_string(Boost));
	}
	CountryTag = Tag;

	return BoostTramp(pThis, Tag, Boost);
}

CCrash* __fastcall hkCrash(void* pThis, unsigned int a1) {

	if (bDebug) {
		printm("Crash Called");
		std::string x = std::to_string(a1);
		printm(x);
	}


	return CCrashTramp(pThis, a1);
}

CPauseGameCommand* __fastcall hkPauseGame(void* pThis, __int64 a2, char a3, char a4) {
	if (bDebug) {
		printm("hkPauseGame Called");
	}
	return CPauseGameTramp(pThis, a2, a3);
}

bool __fastcall hkNetDisconnect(void* pThis) {

	if (bDebug)
		printm("hkNetDisconnect Called");

	if (!bRefuseConnect)
		return NetDisconnectTramp(pThis);
	
	return 0;
	

}
bool Tester1 = false;
bool Tester2 = false;
void __fastcall hkSetState(__int64 pThis, int a2) {
	if (bDebug)
		printm("hkSetState Called");

	if (a2 == 15 && bHotJoin) {
		a2 = 13;
		
	}
	if (a2 == 16 && bChecksum) {
		a2 = 13;
	}
	//if (a2 == 9) {
		//a2 = 13;
	//}

	


	//if (a2 == 17 ) {
	//	a2 = 13;
	//}

	/*if (Tester1 && bHotJoin) {
		Tester1 = false;
		a2 = 6;
		*(DWORD*)(pThis + 72) = 5;
		Tester2 = true;
	}
	if (a2 == 17 && bHotJoin) {

		a2 = 13;
		*(DWORD*)(pThis + 72) = 13;
		printf("value a2 = %u\n", a2);
		printf("value = %u\n", *(DWORD*)(pThis + 72));
		//Tester1 = true;
	}
	
	if (a2 == 4 && Tester2) {
		a2 = 6;
		*(DWORD*)(pThis + 72) = 5;
		Tester2 = false;
	}*/
	if (bDebug) {
		printf("ORGvalue a2 = %u\n", a2);
		printf("ORGvalue = %u\n", *(DWORD*)(pThis + 72));
	}

	if (!bRefuseConnect)
		return SetStateTramp(pThis, a2);
	return (void)0;
}

void __fastcall hkMatchMakingLeave(void* pThis) {

	if (bDebug)
		printm("hkMatchMaking Called");
	if (!bRefuseConnect)
		return MatchmakingLeaveTramp(pThis);
}

void __fastcall hkHuman(void* pThis, CHuman* Human) {
	//40 steam
	//72 User
	//176 MachineID
	if (bDebug) {
		printm("AddPlayerItem Called");
	}
	unsigned int nMachine = 0;

	if (bShowPlayers) {
		if (!bConsoleAlloc) {
			AllocConsole();
			bConsoleAlloc = true;
		}
	
		CString* nSteamName = (CString*)((uintptr_t)Human + 32);
		std::string sName = (char*)nSteamName;

		CString* nHOI4Name = (CString*)((uintptr_t)Human + 64);
		std::string hName = (char*)nHOI4Name;

		nMachine = *(unsigned int*)((uintptr_t)Human + 152);


		//std::string sName = (char*)Human->_Name;
		//std::string hName = (char*)Human->_User;
		//std::string nMachineX = (char*)Human->_nMachineId;

		std::string sFName = "Steam Name: "  + sName;
		std::string hFName = "HOI4 Name: " + hName;
		std::string fMachineID = "Machine ID: " + std::to_string(nMachine);

		printm(sFName);
		printm(hFName);
		printm(fMachineID);
		printm(" ");

	}

	int i = 0;
	int* TagPtr = &i;
	CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
	if (nMachine == gMachineID && bGhost2) {
		//return (void)AIEnableFunc(EnableAI, TagPtr, 0);
		*(unsigned int*)((uintptr_t)Human + 152) = 1;
	}
	else {
		return HumanTramp(pThis, Human);
	}
}

bool __fastcall hkCProxyServer(void* pThis, bool bReconnect, int nMinTimeout, __int64 a4, __int64 a5 ) {

	if (bDebug) {
		//printm("ProxyServer Called");
	}


	return CProxyServerTramp(pThis, bReconnect, nMinTimeout, a4, a5);
}

void* __fastcall hkLogStream(void* pThis, const char* Log) {
	std::string m = Log;
	if (bLog) {
		if (!bConsoleAlloc) {
			AllocConsole();
			bConsoleAlloc = true;
		}
		std::string s = "LogStream: " + m;
		printLog(s);
	}
	if (m.find("CProxyServer::PackageCallback") != std::string::npos) {
		bAddress = true;
	}
	return CLogStreamTramp(pThis, Log);
}

void* __fastcall hkLog(void* pThis, void* a2, const char* pFile, int lineNo, int Category) {


	if (bLog) {
		if (!bConsoleAlloc) {
			AllocConsole();
			bConsoleAlloc = true;
		}
		std::string m = pFile;
		std::string s = "File: " + m + " " + std::to_string(lineNo) + " " + std::to_string(Category);
		printLog(s);
	}

	return CLogTramp(pThis, a2, pFile, lineNo, Category);
}

void* __fastcall hkOperator(void* pThis, __int64* Log) {

		if (bAddress) {
			__int64* rawPtr = Log;
			if (Log[3] >= 16) {
				rawPtr = (__int64*)*Log;
			}

			std::string m = (char*)rawPtr;
			CurrentHost = m;
			std::string s = "IP Address: " + m;
			if (bFindChost) {
				if (!bConsoleAlloc) {
					AllocConsole();
					bConsoleAlloc = true;
				}
				if (!(std::find(IPVector.begin(), IPVector.end(), m) != IPVector.end())) {
					IPVector.push_back(m);
					printLog(s);
				}
				if (GetAsyncKeyState(VK_NUMPAD9) & 1) {
					for (const std::string& str : IPVector) {
						printLog(str);
					}
					Sleep(500);
				}
			}
		}
	
	if (bLog) {
		__int64* rawPtr = Log;
		if (Log[3] >= 16) {
			rawPtr = (__int64*)*Log;

		}

		std::string m = (char*)rawPtr;
		std::string s = "Operator Value: " + m;
		printLog(s);
		
	}
	bAddress = false;
	return COperatorTramp(pThis, Log);
}
std::string TokenString = "";
void printString(const std::string& str)
{
	std::ofstream file("tokenMap.txt", std::ios::app);
	file << str << '\n';
}

void* __fastcall hkToken(void* pThis, int Type, const char* pString) {

	//printm("Token Called");
	TokenString = "{" + std::to_string(Type) + ", \"" + pString + "\"},";
	if(bMapTokens)
		printString(TokenString);
	//printm(TokenString);
	return CTokenTramp(pThis, Type, pString);
}

void HookFunctions() {
	if(Log)
		WriteLog("HookFunctions Initiated");
	CString* Test = new CString;
	CountryBoost cb;
	cb.pGameCString = Test;
	cb.pName = "Uninitalised";
	cb.currentValue = 0;
	cb.defaultValue = 0;

	gCountryBoosts.push_back(cb);


	IPVector.push_back("0.0.0.0:0");

	//DONE
	CSessionPostHook = CSessionPost(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xD9\x45\x84\xC0"), const_cast <char*>("xxxx?xxxx????xxxxxxxxx")));
	MH_CreateHook(CSessionPostHook, &hkCSessionPost, (LPVOID*)&CSessionPostTramp);
	MH_EnableHook(CSessionPostHook);
	//Done
	CAddPlayerCommandHook = GetCAddPlayerCommand(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x49\x8B\xF9\x49\x8B\xD8\x48\x8B\xF1\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4E\x00\xC7\x46\x00\x00\x00\x00\x00\x66\x89\x4E\x00\x48\x89\x4E\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x06\x48\x8D\x4E\x00\xE8\x00\x00\x00\x00\x90\x48\x8D\x4E"), const_cast <char*>("xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xxxx")));
	MH_CreateHook(CAddPlayerCommandHook, &hkCAddPlayerCommand, (LPVOID*)&CAddPlayerCommandTramp);
	MH_EnableHook(CAddPlayerCommandHook);
	//Done
	CGameStateSetPlayerHook = CGameStateSetPlayer(FindPattern(const_cast <char*>("\x40\x56\x57\x48\x83\xEC\x00\x8B\x02\x48\x89\x6C\x24"), const_cast <char*>("xxxxxx?xxxxxx")));

	MH_CreateHook(CGameStateSetPlayerHook, &hkCGameStateSetPlayer, (LPVOID*)&CGameStateSetPlayerTramp);
	MH_EnableHook(CGameStateSetPlayerHook);

	//Done
	CRemovePlayerCommandHook = GetCRemovePlayerCommand(FindPattern(const_cast <char*>("\x48\x89\x4C\x24\x00\x53\x48\x83\xEC\x00\x48\x8B\xD9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4B\x00\xC7\x43\x00\x00\x00\x00\x00\x66\x89\x4B\x00\x48\x89\x4B\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x03\x44\x89\x43"), const_cast <char*>("xxxx?xxxx?xxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx")));


	



	MH_CreateHook(CRemovePlayerCommandHook, &hkCRemovePlayerCommand, (LPVOID*)&CRemovePlayerCommandTramp);
	MH_EnableHook(CRemovePlayerCommandHook);

	//DONE
	CCrashFunc = GetCCrash(FindPattern(const_cast <char*>("\x45\x33\xC0\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x44\x89\x41\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x44\x89\x41\x00\x4C\x89\x41\x00\x89\x51\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x48\x8D\x05"), const_cast <char*>("xxxxx??xxx????xx?????xxxxxxxxx?xx?????xxxx?xxx?xx?xxxxxxxxxxxxxxxxxx?xxxx?xxx")));
	MH_CreateHook(CCrashFunc, &hkCrash, (LPVOID*)&CCrashTramp);
	MH_EnableHook(CCrashFunc);


	//DONE
	CStartGameCommandFunc = GetCStartGameCommand(FindPattern(const_cast <char*>("\x33\xD2\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x89\x51\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x89\x51\x00\x48\x89\x51\x00\xC3\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57"), const_cast <char*>("xxxx??xxx????xx?????xxxxxxxx?xx?????xxx?xxx?xxxxxxxx?x")));
	//DONE
	GetCCommandFunc = GetCCommand(FindPattern(const_cast <char*>("\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x33\xC0"), const_cast <char*>("x????xxxxxxxxxxxx????xxxxxxxxxxxxx")));
	//DONE
	AIEnableFunc = GetEnableAI(FindPattern(const_cast <char*>("\x45\x33\xC9\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x44\x89\x49\x00\x48\x89\x01\x66\x44\x89\x49\x00\x4C\x89\x49\x00\xC7\x41\x00\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x8B\x02\x89\x41\x00\x48\x8B\xC1\x44\x89\x41\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53"), const_cast <char*>("xxxxx??xxx????xxx?xxxxxxx?xxx?xx?????xx?????xxxx?xxxxxx?xxxxxxxxxx")));
	//Done
	CPauseGameFunc = GetCPauseGameCommand(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x41\x0F\xB6\xF1\x41\x0F\xB6\xD8\x48\x8B\xF9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\x66\x89\x4F\x00\x48\x89\x4F\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8D\x4F\x00\xE8\x00\x00\x00\x00\x88\x5F\x00\x40\x88\x77\x00\x48\x8B\xC7\x48\x8B\x5C\x24\x00\x48\x8B\x74\x24\x00\x48\x83\xC4\x00\x5F\xC3\xCC\xCC\xCC\xCC\xCC\x33\xD2"), const_cast <char*>("xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xx?xxx?xxxxxxx?xxxx?xxx?xxxxxxxxx")));
	MH_CreateHook(CPauseGameFunc, &hkPauseGame, (LPVOID*)&CPauseGameTramp);
	MH_EnableHook(CPauseGameFunc);

	//Done
	IncreaseSpeedFunc = GetCGameSpeed(FindPattern(const_cast <char*>("\x33\xD2\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x89\x51\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x89\x51\x00\x48\x89\x51\x00\xC3\xCC\xCC\xCC\x45\x33\xC0\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x44\x89\x41\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x44\x89\x41\x00\x4C\x89\x41\x00\x89\x51"), const_cast <char*>("xxxx??xxx????xx?????xxxxxxxx?xx?????xxx?xxx?xxxxxxxxx??xxx????xx?????xxxxxxxxx?xx?????xxxx?xxx?xx")));

	//Done
	BoostFunc = GetCustomDiffM(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x49\x8B\xD8\x48\x8B\xF9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\x66\x89\x4F\x00\x48\x89\x4F\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8D\x4F\x00\xE8\x00\x00\x00\x00\x48\x89\x5F\x00\x48\x8B\xC7\x48\x8B\x5C\x24\x00\x48\x83\xC4\x00\x5F\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x45\x33\xC0"), const_cast <char*>("xxxx?xxxx?xxxx?xxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xxx?xxxxxxx?xxx?xxxxxxxxxxxx")));
	MH_CreateHook(BoostFunc, &hkCustomDiffM, (LPVOID*)&BoostTramp);
	MH_EnableHook(BoostFunc);

	/*uintptr_t _pInstanceAddr = uintptr_t(FindPattern(const_cast <char*>("\x48\x83\x3D\x00\x00\x00\x00\x00\x75\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x89\x44\x24\x00\x33\xDB"), const_cast <char*>("xxx?????x?x????x????xxxx?xx")));
	int32_t displacement = *(int32_t*)(_pInstanceAddr + 3);
	globalAddr = displacement - (_pInstanceAddr + 7);//_pInstanceAddr + 7 + displacement;
	pInstance = GameBase + globalAddr;

	uintptr_t instr = uintptr_t(FindPattern(const_cast <char*>("\x4C\x8B\x3D\x00\x00\x00\x00\x49\x63\xB7\x00\x00\x00\x00\x48\x85\xF6\x7E\x00\x33\xDB"), const_cast <char*>("xxx????xxx????xxxx?xx")));
	int rel = (int)(instr + 3);
	uintptr_t nextInstr = instr + 7;
	uintptr_t variable = nextInstr + rel;*/


	//DONE
	NetDisconnectHook = GetNetworkDisconnect(FindPattern(const_cast <char*>("\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\x60\x00\x32\xC0\xC3\xCC\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\x32\xC0\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3"), const_cast <char*>("xxxx?xxxxxx?xxxxxxxx?xxxxxx????xxxxxxxxxxxxxxxxxxxxx?xxxxxx????x")));
	MH_CreateHook(NetDisconnectHook, &hkNetDisconnect, (LPVOID*)&NetDisconnectTramp);
	MH_EnableHook(NetDisconnectHook);
	//Done
	SetStateHook = GetSessionSetState(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x63\xDA"), const_cast <char*>("xxxx?xxxx?xxxx?xxxx????xxx")));
	MH_CreateHook(SetStateHook, &hkSetState, (LPVOID*)&SetStateTramp);
	MH_EnableHook(SetStateHook);
	//DONE
	MatchmakingLeaveHook = GetMatchmakingLeaveLobby(FindPattern(const_cast <char*>("\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3\x48\x83\xEC"), const_cast <char*>("xxxx?xxxxxx????xxxxx?xxxxxx????xxxx")));
	MH_CreateHook(MatchmakingLeaveHook, &hkMatchMakingLeave, (LPVOID*)&MatchmakingLeaveTramp);
	MH_EnableHook(MatchmakingLeaveHook);


	
	HumanHook = GetAddHumanItem(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x00\x4C\x8B\xEA\x48\x8B\xF1"), const_cast <char*>("xxxx?xxxxxxxxxxxxxxxxx?xxxxxx")));
	MH_CreateHook(HumanHook, &hkHuman, (LPVOID*)&HumanTramp);
	MH_EnableHook(HumanHook);


	//CProxyServerHook = GetProxyServer(FindPattern(const_cast <char*>("\x48\x8B\xC4\x48\x89\x58\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\x00\x0F\x29\x78\x00\x4D\x8B\xF1"), const_cast <char*>("xxxxxx?xxxxxxxxxxxxxx????xxx????xxx?xxx?xxx")));
	//MH_CreateHook(CProxyServerHook, &hkCProxyServer, (LPVOID*)&CProxyServerTramp);
	//MH_EnableHook(CProxyServerHook);

	//Done
	CLogStreamHook = GetLogStream(FindPattern(const_cast <char*>("\x40\x53\x48\x83\xEC\x00\x48\x8B\xD9\x49\xC7\xC0\x00\x00\x00\x00\x49\xFF\xC0\x42\x80\x3C\x02\x00\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\xC3\x48\x83\xC4\x00\x5B\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x54\x24"), const_cast <char*>("xxxxx?xxxxxx????xxxxxxx?x?x????xxxxxx?xxxxxxxxxxxxxx")));
	MH_CreateHook(CLogStreamHook, &hkLogStream, (LPVOID*)&CLogStreamTramp);
	MH_EnableHook(CLogStreamHook);

	//Done
	CLogHook = GetCLog(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x48\x89\x54\x24\x00\x41\x56\x48\x83\xEC\x00\x41\x8B\xF1"), const_cast <char*>("xxxx?xxxx?xxxx?xxxx?xxxxx?xxx")));
	MH_CreateHook(CLogHook, &hkLog, (LPVOID*)&CLogTramp);
	MH_EnableHook(CLogHook);
	//Done
	COperatorHook = GetOperator(FindPattern(const_cast <char*>("\x48\x83\x7A\x00\x00\x76\x00\x48\x8B\x12\xE9\x00\x00\x00\x00\xCC\x48\x83\x79"), const_cast <char*>("xxx??x?xxxx????xxxx")));
	MH_CreateHook(COperatorHook, &hkOperator, (LPVOID*)&COperatorTramp);
	MH_EnableHook(COperatorHook);



	// TOKENS
	CTokenInitHook = GetTokenInitialiser(FindPattern(const_cast <char*>("\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8D\x44\x24"), const_cast <char*>("x????x????xxxxxxx")));
	//DONE
	CTokenHook = GetCToken(FindPattern(const_cast <char*>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x00\xC7\x01\x00\x00\x00\x00\x4C\x8B\xF1\xC6\x41\x00\x00\x49\x8B\xF0"), const_cast <char*>("xxxx?xxxx?xxxx?xxxxx?xx????xxxxx??xxx")));
	MH_CreateHook(CTokenHook, &hkToken, (LPVOID*)&CTokenTramp);
	MH_EnableHook(CTokenHook);



	if(Log)
		WriteLog("HookFunctions Finished");


}






	

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	if(Log)
		WriteLog("MainThread Initiated");
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			if (Log) {
				WriteLog("RenderType: DX11 Success");
				DX11Success = true;
			}
			kiero::bind(8, (void**)& oPresent, hkPresent);
			init_hook = true;
		}
		if(Log && !DX11Success)
			WriteLog("RenderType: DX11 Possible Failure");
	} while (!init_hook);

	HookFunctions();
	return TRUE;
}



BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hMod);
		if(Log)
			WriteLog("Process Attached");
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);

		break;
	}
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}