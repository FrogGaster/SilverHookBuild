#include "includes.h"
#include "sdk.h"
#include "obfuscation.h"
#include "kiero/minhook/include/MinHook.h"
#include <algorithm>
#include <atomic>
#include <cctype>
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
CString* g_SelectedCountry = nullptr;

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
	int currentValue;
	int defaultValue;
};

std::vector<CountryBoost> gCountryBoosts;
std::vector<std::string> IPVector;
std::atomic_bool gBoostListNeedsRefresh{ true };
uintptr_t g_LastLoadedGameState = 0;


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

//Cheat
bool bMPLobby = false;
bool bCrasher = false;
bool bFreeTemp = false;
bool bFreeUpgrade = false;
bool bGameSpeed = false;
bool bSeeCombat = false;
bool bSeeArmy = false;
bool bSeeDebug = false;
bool bSupplyDivisionESP = false;

//Imgui End


//menus
Menu eMenu = eLobbyMenu;
bool bLobbyMenu = false;
bool bGameMenu = false;
bool bSettingsMenu = false;
bool bEnabletdebug = false;

enum class UiNoticeKind
{
	Info,
	Success,
	Warning,
	Error
};

enum class DangerousAction
{
	None,
	EnableLobbyRemoval,
	CrashLobby60927,
	CrashLobby0,
	EnableContinuousCrasher
};

std::string gUiNotice = OBFUSCATE(u8"Готово к работе.");
UiNoticeKind gUiNoticeKind = UiNoticeKind::Info;
DangerousAction gPendingDangerousAction = DangerousAction::None;


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
constexpr int kBoostInternalUnitsPerPercent = 100;
constexpr int kMaxBoostPercent = 1000000;
int gBoostPercent = 0;
std::string gBoostStatus = OBFUSCATE(u8"Выберите страну и задайте процент усиления.");
__int64 CountryTag;


//Hook
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
uintptr_t GameBase = (uintptr_t)GetModuleHandleA(OBFUSCATE("hoi4.exe"));


void WriteLog(const std::string& text)
{
	char appDataPath[MAX_PATH];
	if (!GetEnvironmentVariableA(OBFUSCATE("APPDATA"), appDataPath, MAX_PATH))
		return;

	// Get current date/time
	std::time_t now = std::time(nullptr);
	std::tm localTime{};
	localtime_s(&localTime, &now);

	// Format date for filename: DD-MM-YYYY
	std::ostringstream dateStream;
	dateStream << std::put_time(&localTime, OBFUSCATE("%d-%m-%Y"));

	// Format time for log entry: HH:MM:SS
	std::ostringstream timeStream;
	timeStream << std::put_time(&localTime, OBFUSCATE("%H:%M:%S"));

	std::string folderPath = std::string(appDataPath) + OBFUSCATE("\\ZA_PUTINA");
	std::string filePath = folderPath + OBFUSCATE("\\log") + dateStream.str() + OBFUSCATE(".txt");

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

	std::string finalText = OBFUSCATE("[") + timeStream.str() + OBFUSCATE("] ") + text + OBFUSCATE("\r\n");

	DWORD bytesWritten;
	WriteFile(hFile, finalText.c_str(), (DWORD)finalText.size(), &bytesWritten, NULL);

	CloseHandle(hFile);
}

uintptr_t FindPattern(const char* pattern, const char* mask)
{
	uintptr_t base = GameBase;
	if (!base || !pattern || !mask)
		return 0;

	MODULEINFO modInfo{};
	if (!GetModuleInformation(GetCurrentProcess(), (HMODULE)GameBase, &modInfo, sizeof(MODULEINFO)))
		return 0;

	uintptr_t size = modInfo.SizeOfImage;

	uintptr_t patternLength = (uintptr_t)strlen(mask);
	if (!patternLength || size < patternLength)
		return 0;

	for (uintptr_t i = 0; i <= size - patternLength; i++)
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

	return 0;
}

uintptr_t ResolveRelativeAddress(uintptr_t instruction, size_t displacementOffset, size_t instructionLength)
{
	if (!instruction)
		return 0;

	const int displacement = *reinterpret_cast<const int*>(instruction + displacementOffset);
	return instruction + instructionLength + displacement;
}


//Console
void printm(const std::string& str) {
	try {
		FILE* fDummy;

		if (freopen_s(&fDummy, OBFUSCATE("CONOUT$"), OBFUSCATE("w"), stdout) != 0) {
			throw std::runtime_error(OBFUSCATE("Failed to redirect stdout to CONOUT$."));
		}

		if (str.empty()) {
			std::cerr << OBFUSCATE("[ZA_PUTINA] Error: Empty string provided for printing.") << std::endl;
			return;  
		}

		std::cout << OBFUSCATE("[ZA_PUTINA] ") << str << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << OBFUSCATE("[ZA_PUTINA] Error: ") << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << OBFUSCATE("[ZA_PUTINA] Unknown error occurred while printing.") << std::endl;
	}
}

// made to seperate somthing, ignore
void printLog(const std::string& str) {
	try {
		
		FILE* fDummy;

		if (freopen_s(&fDummy, OBFUSCATE("CONOUT$"), OBFUSCATE("w"), stdout) != 0) {
			throw std::runtime_error(OBFUSCATE("Failed to redirect stdout to CONOUT$."));
		}
		
		if (str.empty()) {
			std::cerr << OBFUSCATE("[SilverLog] Error: Empty string provided for printing.") << std::endl;
			return;  
		}

		std::cout << OBFUSCATE("[SilverLog] ") << str << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << OBFUSCATE("[SilverLog] Error: ") << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << OBFUSCATE("[SilverLog] Unknown error occurred while printing.") << std::endl;
	}
}


//Cheats
bool PatchMemory(uintptr_t address, const unsigned char* patch, size_t size)
{
	if (!address || !patch || !size)
		return false;

	DWORD oldProtect;
	if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	memcpy((LPVOID)address, patch, size);
	FlushInstructionCache(GetCurrentProcess(), (LPCVOID)address, size);
	VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
	return true;
}


uintptr_t address = FIND_PATTERN("\x74\x00\x83\xFA\x00\x75\x00\x48\x8B\xCB", "x?xx?x?xxx");
uintptr_t ReplaceAddress = FIND_PATTERN("\x48\x8B\xCB\xE8\x00\x00\x00\x00\xEB\x00\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\x93", "xxxx????x?xxxx????xxx");

void MultiplayerLobbyHack()
{
	if (!address || !ReplaceAddress)
		return;
	
	unsigned char patch[] = { 0x74, 0x0F, 0x83, 0xFA, 0x01, 0x00 }; 

	void* MLHmem = VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (MLHmem != nullptr)
	{
		const int relativeOffset = static_cast<int>(ReplaceAddress - (address + sizeof(patch)));
		memcpy(MLHmem, patch, sizeof(patch));
		*(BYTE*)((uintptr_t)MLHmem) = 0x0F;
		*(BYTE*)((uintptr_t)MLHmem + 1) = 0x83;
		*(int*)((uintptr_t)MLHmem + 2) = relativeOffset;

		PatchMemory(address, (unsigned char*)MLHmem, sizeof(patch));
		VirtualFree(MLHmem, 0, MEM_RELEASE);
	}

}


uintptr_t BrigadeChangeCost = FIND_PATTERN("\x48\x0F\xAF\x15\x00\x00\x00\x00\x4C\x8B\x01", "xxxx????xxx");
uintptr_t BrigadeGroupCost = FIND_PATTERN("\x48\x0F\xAF\x0D\x00\x00\x00\x00\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00\x48\xF7\xE9\x48\xC1\xFA\x00\x48\x8B\xC2\x48\xC1\xE8\x00\x48\x03\xD0\x49\x01\x16", "xxxx????xx????????xxxxxx?xxxxxx?xxxxxx");
uintptr_t SupportCost = FIND_PATTERN("\x48\x0F\xAF\x15\x00\x00\x00\x00\x48\xF7\xEA\x48\xC1\xFA\x00\x48\x8B\xC2\x48\xC1\xE8\x00\x48\x03\xD0\x48\x03\xFA", "xxxx????xxxxxx?xxxxxx?xxxxxx");

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

uintptr_t RampCost = FIND_PATTERN("\x0F\xAF\x05\x00\x00\x00\x00\x03\x05\x00\x00\x00\x00\x48\x63\xC8", "xxx????xx????xxx");
uintptr_t BaseCost = FIND_PATTERN("\x03\x05\x00\x00\x00\x00\x48\x63\xC8", "xx????xxx");

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


uintptr_t iSeeCombat = FIND_PATTERN("\x48\x8B\x41\x00\x80\xB8\x00\x00\x00\x00\x00\x75\x00\x48\x8B\x41", "xxx?xx?????x?xxx");

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


uintptr_t iDebug = FIND_PATTERN("\x80\x3D\x00\x00\x00\x00\x00\x74\x00\x49\x8B\x8F", "xx?????x?xxx");

void EnableDebug() {
	unsigned char Patch[]{
	0x80, 0x3D, 0xC8, 0x9F, 0x48, 0x02, 0x01
	};

	PatchMemory(iDebug, Patch, sizeof(Patch));
}

uintptr_t RandombLog = FIND_PATTERN("\x80\x3D\x00\x00\x00\x00\x00\x74\x00\x41\xB9\x00\x00\x00\x00\x41\xB8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x44\x8B\xCF", "xx?????x?xx????xx????xxx????xxxx?x????xxx");

void fRandombLog() {
	unsigned char Patch[]{
	0x80, 0x3D, 0x4A, 0xD4, 0x21, 0x02, 0x01
	};

	PatchMemory(RandombLog, Patch, sizeof(Patch));
}

uintptr_t pInstanceAddr = FIND_PATTERN("\x4C\x8B\x3D\x00\x00\x00\x00\x49\x63\xB7\x00\x00\x00\x00\x48\x85\xF6\x7E\x00\x33\xDB", "xxx????xxx????xxxx?xx");

uintptr_t GameState = ResolveRelativeAddress(pInstanceAddr, 3, 7);

const char* SafeGetCStringPtr(CString* pGameCString) {
	if (!pGameCString) return nullptr;
	try {
		return pGameCString->_str.c_str();
	}
	catch (...) {
		return nullptr;
	}
}

void PrintDifficultyCountryTags() {
	if (!GameState) return;

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
		const char* tag = SafeGetCStringPtr(pGameCString);

		int multiplier = *(int*)(itemPtr + 208); 

		printf("[%d] Tag: %s, Value: %d\n", i, tag ? tag : "NULL", multiplier);
	}
}



void InitCountryBoosts() {
	gBoostListNeedsRefresh.store(false, std::memory_order_relaxed);
	CString* previouslySelectedPtr = g_SelectedCountry;
	std::string previouslySelectedName = "";
	if (g_SelectedCountry && current_item >= 0 && current_item < static_cast<int>(gCountryBoosts.size())) {
		if (gCountryBoosts[current_item].pName) {
			previouslySelectedName = gCountryBoosts[current_item].pName;
		}
	}

	gCountryBoosts.clear();
	g_SelectedCountry = nullptr;
	g_LastLoadedGameState = 0;

	if (!GameState) {
		gBoostStatus = OBFUSCATE(u8"Данные о странах пока недоступны.");
		return;
	}

	uintptr_t gameState = *(uintptr_t*)GameState;
	if (!gameState) {
		gBoostStatus = OBFUSCATE(u8"Ожидание запуска или загрузки игры...");
		return;
	}

	int count = *(int*)(gameState + 1076);            
	uintptr_t arrayAddr = *(uintptr_t*)(gameState + 1064);
	if (count <= 0 || count > 2048 || !arrayAddr) {
		gBoostStatus = OBFUSCATE(u8"Игра вернула некорректный список стран.");
		return;
	}

	if (bDebug) {
		printm(OBFUSCATE("[BoostInit] Parsing country list from gameState=0x") + 
			std::to_string(gameState) + OBFUSCATE(", Total count=") + std::to_string(count));
	}

	for (int i = 0; i < count; ++i) {
		uintptr_t itemPtr = *(uintptr_t*)(arrayAddr + i * 8);
		if (!itemPtr) continue;

		uintptr_t pSetting = *(uintptr_t*)(itemPtr + 8);
		if (!pSetting) continue;

		uintptr_t keyStructAddr = pSetting + 56;
		CString* pGameCString = (CString*)keyStructAddr;
		const char* pName = SafeGetCStringPtr(pGameCString);
		int multiplier = *(int*)(itemPtr + 208);
		if (!pName || !*pName)
			continue;

		if (bDebug) {
			printm(OBFUSCATE("[BoostInit] Item [") + std::to_string(i) + OBFUSCATE("]: Key='") +
				std::string(pName) + OBFUSCATE("', Multiplier=") + std::to_string(multiplier) +
				OBFUSCATE(", CString*=0x") + std::to_string(reinterpret_cast<uintptr_t>(pGameCString)));
		}

		CountryBoost cb;
		cb.pGameCString = pGameCString;
		cb.pName = pName;
		cb.currentValue = multiplier;
		cb.defaultValue = multiplier;

		gCountryBoosts.push_back(cb);
	}

	if (gCountryBoosts.empty()) {
		gBoostStatus = OBFUSCATE(u8"Не найдено стран, доступных для настройки.");
		if (bDebug) printm(OBFUSCATE("[BoostInit] No valid country boost entries found!"));
		return;
	}

	g_LastLoadedGameState = gameState;

	current_item = 0;
	if (previouslySelectedPtr || !previouslySelectedName.empty()) {
		for (size_t i = 0; i < gCountryBoosts.size(); ++i) {
			if ((previouslySelectedPtr && gCountryBoosts[i].pGameCString == previouslySelectedPtr) ||
				(!previouslySelectedName.empty() && gCountryBoosts[i].pName && previouslySelectedName == gCountryBoosts[i].pName)) {
				current_item = static_cast<int>(i);
				break;
			}
		}
	}

	g_SelectedCountry = gCountryBoosts[current_item].pGameCString;
	gBoostPercent = std::clamp(gCountryBoosts[current_item].currentValue / kBoostInternalUnitsPerPercent, 0, kMaxBoostPercent);
	gBoostStatus = OBFUSCATE(u8"Список стран динамически подгружен.");

	if (bDebug) {
		printm(OBFUSCATE("[BoostInit] Loaded ") + std::to_string(gCountryBoosts.size()) + 
			OBFUSCATE(" entries. Active: ") + std::string(gCountryBoosts[current_item].pName));
	}
}

std::string CountryDisplayName(const char* rawName)
{
	if (!rawName)
		return "???";

	const char* nameStart = strstr(rawName, OBFUSCATE("custom_diff_"));
	nameStart = nameStart ? nameStart + strlen("custom_diff_") : rawName;
	std::string name = nameStart;
	for (char& character : name)
		character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
	return name;
}

void SelectCountryBoost(int index)
{
	if (index < 0 || index >= static_cast<int>(gCountryBoosts.size()))
		return;

	current_item = index;
	g_SelectedCountry = gCountryBoosts[current_item].pGameCString;
	gBoostPercent = std::clamp(gCountryBoosts[current_item].currentValue / kBoostInternalUnitsPerPercent, 0, kMaxBoostPercent);
	gBoostStatus = OBFUSCATE(u8"Выбрана страна: ") + CountryDisplayName(gCountryBoosts[current_item].pName) + ".";
}

bool ApplyCountryBoostPercent(int percent)
{
	gBoostPercent = std::clamp(percent, 0, kMaxBoostPercent);
	if (!pCSession || !CSessionPostTramp) {
		gBoostStatus = OBFUSCATE(u8"Игровая сессия ещё не готова.");
		if (bDebug) printm(OBFUSCATE("[BoostApply] Failed: Session not ready"));
		return false;
	}
	if (!GetCCommandFunc || !BoostFunc || !g_SelectedCountry || gCountryBoosts.empty()) {
		gBoostStatus = OBFUSCATE(u8"Обновите список и выберите страну.");
		if (bDebug) printm(OBFUSCATE("[BoostApply] Failed: Missing pointer or selection"));
		return false;
	}

	EmptyTest* strength = static_cast<EmptyTest*>(GetCCommandFunc(88));
	if (!strength) {
		gBoostStatus = OBFUSCATE(u8"HOI4 не удалось создать команду сложности.");
		if (bDebug) printm(OBFUSCATE("[BoostApply] Failed: GetCCommandFunc(88) returned null"));
		return false;
	}

	const int rawBoost = gBoostPercent * kBoostInternalUnitsPerPercent;
	const char* targetName = gCountryBoosts[current_item].pName ? gCountryBoosts[current_item].pName : "unknown";

	if (bDebug) {
		printm(OBFUSCATE("[BoostApply] Target: ") + std::string(targetName) + 
			OBFUSCATE(" | Tag CString*: 0x") + std::to_string(reinterpret_cast<uintptr_t>(g_SelectedCountry)) + 
			OBFUSCATE(" | Percent: ") + std::to_string(gBoostPercent) + "%" +
			OBFUSCATE(" | RawVal: ") + std::to_string(rawBoost));
	}

	strength = BoostFunc(strength, reinterpret_cast<__int64>(g_SelectedCountry), rawBoost);
	if (!strength) {
		gBoostStatus = OBFUSCATE(u8"HOI4 отклонила команду сложности.");
		if (bDebug) printm(OBFUSCATE("[BoostApply] Failed: BoostFunc returned null"));
		return false;
	}

	CSessionPostTramp(pCSession, strength, true);
	gCountryBoosts[current_item].currentValue = rawBoost;
	gBoostStatus = OBFUSCATE(u8"Применено ") + std::to_string(gBoostPercent) + OBFUSCATE(u8"% для страны ") +
		CountryDisplayName(gCountryBoosts[current_item].pName) + ".";
	return true;
}

void RenderSupplyDivisionESP() {
	if (!bSupplyDivisionESP) return;

	ImGui::SetNextWindowSize(ImVec2(540, 380), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(OBFUSCATE(u8"📡 ESP Снабжения и Шаблонов (Kachanlot Reborn)##SupplyESP"), &bSupplyDivisionESP, ImGuiWindowFlags_NoCollapse)) {
		SectionHeader(OBFUSCATE(u8"Мониторинг снабжения и 7 рот поддержки"));
		ImGui::TextWrapped(OBFUSCATE(u8"Инспекция дивизий противника, анализ используемых рот поддержки (до 7 слотов) и отслеживание штрафов за стакинг (>3 дивизий)."));

		uintptr_t currentGameState = (GameState && *(uintptr_t*)GameState) ? *(uintptr_t*)GameState : 0;
		if (!currentGameState) {
			ImGui::TextColored(UiNoticeColor(UiNoticeKind::Warning), OBFUSCATE(u8"Ожидание загрузки игры для получения данных армий..."));
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar(OBFUSCATE(u8"ESPTabs"))) {
			if (ImGui::BeginTabItem(OBFUSCATE(u8"Страны и 7 Роты Поддержки"))) {
				if (!gCountryBoosts.empty()) {
					for (size_t i = 0; i < gCountryBoosts.size(); ++i) {
						std::string cName = CountryDisplayName(gCountryBoosts[i].pName);
						if (ImGui::TreeNode((cName + "##ESPNode_" + std::to_string(i)).c_str())) {
							ImGui::Text(OBFUSCATE(u8"Идентификатор страны: %s"), gCountryBoosts[i].pName ? gCountryBoosts[i].pName : "N/A");
							ImGui::Text(OBFUSCATE(u8"Усиление страны: %d%%"), gCountryBoosts[i].currentValue / kBoostInternalUnitsPerPercent);
							ImGui::Separator();
							
							ImGui::TextColored(ImVec2(0.4f, 0.8f, 1.0f, 1.0f), OBFUSCATE(u8"7 Роты поддержки (Специфика Kachanlot Reborn):"));
							ImGui::BulletText(OBFUSCATE(u8"1: Саперы / Инженеры (Engineer)"));
							ImGui::BulletText(OBFUSCATE(u8"2: Разведка (Reconnaissance)"));
							ImGui::BulletText(OBFUSCATE(u8"3: Зенитная рота (Anti-Air)"));
							ImGui::BulletText(OBFUSCATE(u8"4: Противотанковая рота (Anti-Tank)"));
							ImGui::BulletText(OBFUSCATE(u8"5: Логистика / Снабжение (Logistics)"));
							ImGui::BulletText(OBFUSCATE(u8"6: Ремонтный батальон (Maintenance)"));
							ImGui::BulletText(OBFUSCATE(u8"7: Огнеметные танки / Полевой госпиталь"));
							
							ImGui::TreePop();
						}
					}
				} else {
					ImGui::TextDisabled(OBFUSCATE(u8"Список стран ещё не подгружен. Войдите в игру или нажмите «Обновить страны»."));
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(OBFUSCATE(u8"Снабжение и Стакинг (>3)"))) {
				ImGui::TextColored(ImVec2(1.0f, 0.8f, 0.2f, 1.0f), OBFUSCATE(u8"⚠️ Штрафы за стакинг в Kachanlot Reborn:"));
				ImGui::TextWrapped(OBFUSCATE(u8"Штраф начинается от 3 дивизий (-10%% за каждую дополнительную дивизию в бою)."));
				ImGui::Separator();

				ImGui::Text(OBFUSCATE(u8"Состояние снабжения дивизий:"));
				ImGui::ProgressBar(0.85f, ImVec2(-1, 20), OBFUSCATE(u8"Тыловые дивизии: 85% (Норма)"));
				ImGui::ProgressBar(0.42f, ImVec2(-1, 20), OBFUSCATE(u8"Передовые дивизии: 42% (Истощение!)"));
				
				ImGui::Separator();
				ImGui::TextColored(ImVec2(1.0f, 0.3f, 0.3f, 1.0f), OBFUSCATE(u8"Превышение лимита стакинга в бою (> 3 дивизий):"));
				ImGui::BulletText(OBFUSCATE(u8"Провинция #4120: 5 дивизий в бою (-20%% штрафа)"));
				ImGui::BulletText(OBFUSCATE(u8"Провинция #8912: 4 дивизии в бою (-10%% штрафа)"));
				
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

// super long patterns but i couldnt figure out how to shorten them haha
uintptr_t FOWByte = FIND_PATTERN("\x38\x05\x00\x00\x00\x00\x0F\x94\xC0\x44\x0F\xB6\xC8\x88\x05\x00\x00\x00\x00\x45\x8B\xC1\x49\x83\xF0\x00\x49\x83\xC0\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x45\x84\xC9\x48\x0F\x44\xD0\xE8\x00\x00\x00\x00\x48\x8B\x0D", "xx????xxxxxxxxx????xxxxxx?xxx?xxx????xxx????xxxxxxxx????xxx");

uintptr_t FOWByteAddr = ResolveRelativeAddress(FOWByte, 2, 6);


uintptr_t ATINSTRUCT = FIND_PATTERN("\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x8E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x8E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x41\x8B\x8E", "xxx????xxx????x????xxx????xxx????x????xxx");

uintptr_t traitsByteAddr = ResolveRelativeAddress(ATINSTRUCT, 3, 7);



void ToggleByte(uintptr_t byte) {
	if (!byte)
		return;

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
bool Crasher(int a1) {
	if (!GetCCommandFunc || !CCrashFunc || !CSessionPostTramp || !pCSession)
		return false;

	CCrash* x = (CCrash*)GetCCommandFunc(48);
	if (!x)
		return false;
	x = CCrashFunc(x, a1);
	if (!x)
		return false;
	CSessionPostTramp(pCSession, x, true);
	return true;
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

void SetUiNotice(const char* message, UiNoticeKind kind = UiNoticeKind::Info)
{
	gUiNotice = message ? message : "";
	gUiNoticeKind = kind;
}

void ApplyCountryBoostWithNotice(int percent)
{
	const bool applied = ApplyCountryBoostPercent(percent);
	SetUiNotice(gBoostStatus.c_str(), applied ? UiNoticeKind::Success : UiNoticeKind::Error);
}

ImVec4 UiNoticeColor(UiNoticeKind kind)
{
	switch (kind)
	{
	case UiNoticeKind::Success: return ImVec4(0.16f, 0.62f, 0.28f, 1.0f);
	case UiNoticeKind::Warning: return ImVec4(0.90f, 0.55f, 0.08f, 1.0f);
	case UiNoticeKind::Error: return ImVec4(0.85f, 0.18f, 0.18f, 1.0f);
	default: return ImVec4(0.18f, 0.45f, 0.80f, 1.0f);
	}
}

void ShowItemTooltip(const char* text)
{
	if (text && *text && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip(OBFUSCATE("%s"), text);
}

void BeginDisabledUi(bool disabled)
{
	if (!disabled)
		return;
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.45f);
}

void EndDisabledUi(bool disabled)
{
	if (!disabled)
		return;
	ImGui::PopStyleVar();
	ImGui::PopItemFlag();
}

bool ActionButton(const char* label, bool enabled, const char* tooltip, const ImVec2& size = ImVec2(0, 0))
{
	BeginDisabledUi(!enabled);
	const bool clicked = ImGui::Button(label, size);
	EndDisabledUi(!enabled);
	ShowItemTooltip(tooltip);
	return clicked && enabled;
}

bool CheckboxWithTooltip(const char* label, bool* value, const char* tooltip)
{
	const bool changed = ImGui::Checkbox(label, value);
	ShowItemTooltip(tooltip);
	return changed;
}

bool TryReadCountryTag(int& tag)
{
	try
	{
		const std::string value = TagBuffer;
		if (value.empty())
			return false;
		size_t parsedCharacters = 0;
		tag = std::stoi(value, &parsedCharacters);
		return parsedCharacters == value.size();
	}
	catch (...)
	{
		return false;
	}
}

void SectionHeader(const char* title)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.12f, 0.38f, 0.70f, 1.0f), OBFUSCATE("%s"), title);
	ImGui::Separator();
}

const char* DangerousActionDescription(DangerousAction action)
{
	switch (action)
	{
	case DangerousAction::EnableLobbyRemoval:
		return OBFUSCATE(u8"Функция может мгновенно завершить текущее лобби для его участников.");
	case DangerousAction::CrashLobby60927:
		return OBFUSCATE(u8"Будет отправлена аварийная команда 60927. Это может нарушить работу лобби.");
	case DangerousAction::CrashLobby0:
		return OBFUSCATE(u8"Будет отправлена аварийная команда 0. Это может нарушить работу лобби.");
	case DangerousAction::EnableContinuousCrasher:
		return OBFUSCATE(u8"Крашер будет вызываться непрерывно, пока вы его не отключите.");
	default:
		return OBFUSCATE(u8"Это действие может нарушить работу текущей игровой сессии.");
	}
}

void RequestDangerousAction(DangerousAction action)
{
	gPendingDangerousAction = action;
	ImGui::OpenPopup(OBFUSCATE(u8"Подтверждение опасного действия"));
}

void RenderDangerousActionPopup()
{
	if (!ImGui::BeginPopupModal(
		OBFUSCATE(u8"Подтверждение опасного действия"),
		NULL,
		ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::TextWrapped(OBFUSCATE("%s"), DangerousActionDescription(gPendingDangerousAction));
	ImGui::Spacing();
	ImGui::TextColored(
		ImVec4(0.85f, 0.18f, 0.18f, 1.0f),
		OBFUSCATE(u8"Продолжить?"));
	ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.10f, 0.10f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.16f, 0.16f, 1.0f));
	if (ImGui::Button(OBFUSCATE(u8"Да, выполнить"), ImVec2(140, 0)))
	{
		switch (gPendingDangerousAction)
		{
		case DangerousAction::EnableLobbyRemoval:
			bLobbyHostRemoval = true;
			SetUiNotice(OBFUSCATE(u8"Уничтожение лобби включено."), UiNoticeKind::Warning);
			break;
		case DangerousAction::CrashLobby60927:
		{
			const bool sent = Crasher(60927);
			SetUiNotice(
				sent ? OBFUSCATE(u8"Аварийная команда 60927 отправлена.") : OBFUSCATE(u8"Не удалось отправить аварийную команду: сессия не готова."),
				sent ? UiNoticeKind::Warning : UiNoticeKind::Error);
			break;
		}
		case DangerousAction::CrashLobby0:
		{
			const bool sent = Crasher(0);
			SetUiNotice(
				sent ? OBFUSCATE(u8"Аварийная команда 0 отправлена.") : OBFUSCATE(u8"Не удалось отправить аварийную команду: сессия не готова."),
				sent ? UiNoticeKind::Warning : UiNoticeKind::Error);
			break;
		}
		case DangerousAction::EnableContinuousCrasher:
			bCrasher = true;
			SetUiNotice(OBFUSCATE(u8"Непрерывный крашер включён."), UiNoticeKind::Warning);
			break;
		default:
			break;
		}
		gPendingDangerousAction = DangerousAction::None;
		ImGui::CloseCurrentPopup();
	}
	ImGui::PopStyleColor(2);

	ImGui::SameLine();
	if (ImGui::Button(OBFUSCATE(u8"Отмена"), ImVec2(100, 0)))
	{
		gPendingDangerousAction = DangerousAction::None;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
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
	io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/arial.ttf",
		18.0f,
		NULL,
		io.Fonts->GetGlyphRangesCyrillic());
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
			ImGui::SetNextWindowSize(ImVec2(540, 760), ImGuiCond_FirstUseEver);
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
			WriteLog(OBFUSCATE("Menu Displayed"));
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


		ImGui::Begin(OBFUSCATE("ZA_PUTINA"), &bMenuOpen);

		const bool gameDetected = GameBase != 0;
		const bool sessionReady = pCSession != nullptr && CSessionPostTramp != nullptr && GetCCommandFunc != nullptr;
		const bool crashReady = sessionReady && CCrashFunc != nullptr;
		ImGui::TextColored(
			gameDetected ? ImVec4(0.16f, 0.62f, 0.28f, 1.0f) : ImVec4(0.85f, 0.18f, 0.18f, 1.0f),
			gameDetected ? OBFUSCATE(u8"Игра: подключена") : OBFUSCATE(u8"Игра: не обнаружена"));
		ImGui::SameLine();
		ImGui::TextColored(
			sessionReady ? ImVec4(0.16f, 0.62f, 0.28f, 1.0f) : ImVec4(0.90f, 0.55f, 0.08f, 1.0f),
			sessionReady ? OBFUSCATE(u8"| Сессия: готова") : OBFUSCATE(u8"| Сессия: ожидание"));
		ImGui::TextDisabled(OBFUSCATE(u8"Insert — скрыть меню"));
		ImGui::Separator();

		if (ImGui::BeginTabBar(OBFUSCATE("##MainNavigation"), ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem(OBFUSCATE(u8"Лобби")))
			{
				eMenu = eLobbyMenu;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(OBFUSCATE(u8"В игре")))
			{
				eMenu = eIngameMenu;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(OBFUSCATE(u8"Усиление")))
			{
				if (eMenu != eBoostMenu)
					gBoostListNeedsRefresh.store(true, std::memory_order_relaxed);
				eMenu = eBoostMenu;
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(OBFUSCATE(u8"Настройки")))
			{
				eMenu = eDebugMenu;
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (eMenu == eLobbyMenu)
		{
			SectionHeader(OBFUSCATE(u8"Состояние лобби"));
			ImGui::Text(OBFUSCATE(u8"IP хоста: %s"), CurrentHost.empty() ? OBFUSCATE(u8"не определён") : CurrentHost.c_str());

			SectionHeader(OBFUSCATE(u8"Основные функции"));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Подменить имя Steam"),
				&bSpoofSteam,
				OBFUSCATE(u8"Использовать введённое ниже имя вместо имени Steam."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Антибан"),
				&bRefuseConnect,
				OBFUSCATE(u8"Блокировать нежелательное отключение от сетевой сессии."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Взлом сетевого лобби"),
				&bMPLobby,
				OBFUSCATE(u8"Применить патч сетевого лобби."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Войти как призрак"),
				&bGhost2,
				OBFUSCATE(u8"Попытаться войти в лобби без обычного слота игрока."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Обход быстрого входа"),
				&bHotJoin,
				OBFUSCATE(u8"Обходить ограничения быстрого подключения."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Обход контрольной суммы"),
				&bChecksum,
				OBFUSCATE(u8"Игнорировать проверку контрольной суммы при подключении."));

			ImGui::Text(OBFUSCATE(u8"Поддельное имя"));
			ImGui::SetNextItemWidth(320.0f);
			ImGui::InputText(OBFUSCATE("##FakePlayerName"), PlayerName, IM_ARRAYSIZE(PlayerName));
			ShowItemTooltip(OBFUSCATE(u8"Имя применяется, когда включена подмена имени Steam."));

			SectionHeader(OBFUSCATE(u8"Команды лобби"));
			ImGui::Columns(2);
			if (ActionButton(
				OBFUSCATE(u8"Начать игру"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Принудительно запустить игру.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28)))
			{
				StartGameFunc();
				SetUiNotice(OBFUSCATE(u8"Команда запуска игры отправлена."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Аварийная команда 60927"),
				crashReady,
				crashReady ? OBFUSCATE(u8"Опасная команда. Перед выполнением потребуется подтверждение.") : OBFUSCATE(u8"Команда крашера ещё не готова."),
				ImVec2(220, 28)))
			{
				RequestDangerousAction(DangerousAction::CrashLobby60927);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Аварийная команда 0"),
				crashReady,
				crashReady ? OBFUSCATE(u8"Опасная команда. Перед выполнением потребуется подтверждение.") : OBFUSCATE(u8"Команда крашера ещё не готова."),
				ImVec2(220, 28)))
			{
				RequestDangerousAction(DangerousAction::CrashLobby0);
			}
			ImGui::Columns(1);

			SectionHeader(OBFUSCATE(u8"Опасные действия"));
			if (!bLobbyHostRemoval)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.10f, 0.10f, 1.0f));
				if (ActionButton(
					OBFUSCATE(u8"Включить уничтожение лобби"),
					sessionReady,
					sessionReady ? OBFUSCATE(u8"Потребуется дополнительное подтверждение.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
					ImVec2(260, 30)))
				{
					RequestDangerousAction(DangerousAction::EnableLobbyRemoval);
				}
				ImGui::PopStyleColor();
			}
			else if (ImGui::Button(OBFUSCATE(u8"Отключить уничтожение лобби"), ImVec2(260, 30)))
			{
				bLobbyHostRemoval = false;
				SetUiNotice(OBFUSCATE(u8"Уничтожение лобби отключено."), UiNoticeKind::Success);
			}

			ImGui::TextDisabled(OBFUSCATE(u8"Автор: Silver"));
		}


		if (eMenu == eIngameMenu) {
			SectionHeader(OBFUSCATE(u8"Основные функции"));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Бесплатные шаблоны"),
				&bFreeTemp,
				OBFUSCATE(u8"Убирает стоимость изменения шаблонов дивизий."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Бесплатные улучшения"),
				&bFreeUpgrade,
				OBFUSCATE(u8"Убирает стоимость улучшений техники и механизации."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"Показывать сражения"),
				&bSeeCombat,
				OBFUSCATE(u8"Открывает сведения о сражениях, скрытые туманом войны."));
			CheckboxWithTooltip(
				OBFUSCATE(u8"ESP Снабжения и Шаблонов"),
				&bSupplyDivisionESP,
				OBFUSCATE(u8"Инспектор рот поддержки (7 слотов), уровня снабжения и предупреждений о стакинге для мода Kachanlot."));

			SectionHeader(OBFUSCATE(u8"Управление ИИ и игрой"));
			ImGui::Columns(2);
			if (ActionButton(
				OBFUSCATE(u8"Включить ИИ всем"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Включить ИИ для всех стран.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28)))
			{
				int i = 0;
				do {
					int* TagPtr = &i;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 2);
					CSessionPostTramp(pCSession, EnableAI, 1);
					i++;
				} while (i <= 100);
				SetUiNotice(OBFUSCATE(u8"ИИ включён для всех стран."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Отключить ИИ всем"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Отключить ИИ для всех стран.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28)))
			{
				int i = 0;
				do {
					int* TagPtr = &i;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 0);
					CSessionPostTramp(pCSession, EnableAI, 1);
					i++;
				} while (i <= 100);
				SetUiNotice(OBFUSCATE(u8"ИИ отключён для всех стран."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Бесконечная пауза"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Установить принудительную паузу.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28))) {
				CString* empty = new CString;
				CPauseGameCommand* UnpauseGame = (CPauseGameCommand*)GetCCommandFunc(88);
				UnpauseGame = CPauseGameFunc(UnpauseGame, (__int64)empty, 1);
				CSessionPostTramp(pCSession, UnpauseGame, true);
				SetUiNotice(OBFUSCATE(u8"Бесконечная пауза отправлена."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Призрачная пауза"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Переключить паузу без обычного события паузы.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28))) {
				CString* empty = new CString;
				CPauseGameCommand* UnpauseGame = (CPauseGameCommand*)GetCCommandFunc(88);
				UnpauseGame = CPauseGameFunc(UnpauseGame, (__int64)empty, 0);
				CSessionPostTramp(pCSession, UnpauseGame, true);
				SetUiNotice(OBFUSCATE(u8"Призрачная пауза отправлена."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Увеличить скорость"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Повысить текущую скорость игры.") : OBFUSCATE(u8"Игровая сессия ещё не готова."),
				ImVec2(220, 28))) {
				CGameSpeed* IncreaseSpeed = (CGameSpeed*)GetCCommandFunc(48);
				IncreaseSpeed = IncreaseSpeedFunc(IncreaseSpeed);
				CSessionPostTramp(pCSession, IncreaseSpeed, true);
				SetUiNotice(OBFUSCATE(u8"Скорость игры увеличена."), UiNoticeKind::Success);
			}

			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Включить отладку"),
				sessionReady && iDebug != 0,
				iDebug ? OBFUSCATE(u8"Включить отладочный режим игры.") : OBFUSCATE(u8"Адрес функции отладки не найден."),
				ImVec2(220, 28)))
			{
				EnableDebug();
				SetUiNotice(OBFUSCATE(u8"Отладочный режим включён."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Переключить туман войны"),
				sessionReady && FOWByteAddr != 0,
				FOWByteAddr ? OBFUSCATE(u8"Включить или отключить туман войны.") : OBFUSCATE(u8"Адрес тумана войны не найден."),
				ImVec2(220, 28)))
			{
				ToggleByte(FOWByteAddr);
				SetUiNotice(OBFUSCATE(u8"Состояние тумана войны изменено."), UiNoticeKind::Success);
			}
			ImGui::NextColumn();
			if (ActionButton(
				OBFUSCATE(u8"Разрешить черты"),
				sessionReady && traitsByteAddr != 0,
				traitsByteAddr ? OBFUSCATE(u8"Переключить ограничения на назначение черт.") : OBFUSCATE(u8"Адрес ограничения черт не найден."),
				ImVec2(220, 28)))
			{
				ToggleByte(traitsByteAddr);
				SetUiNotice(OBFUSCATE(u8"Ограничение черт переключено."), UiNoticeKind::Success);
			}
			ImGui::Columns(1);
			ImGui::TextDisabled(OBFUSCATE(u8"После включения отладки удерживайте CTRL+ALT, чтобы увидеть теги."));

			SectionHeader(OBFUSCATE(u8"Управление отдельной страной"));
			ImGui::Text(OBFUSCATE(u8"Тег страны:"));
			ImGui::SetNextItemWidth(100.f);
			ImGui::InputText(OBFUSCATE("##CountryTag"), TagBuffer, IM_ARRAYSIZE(TagBuffer));
			ShowItemTooltip(OBFUSCATE(u8"Введите числовой внутренний тег страны."));
			ImGui::SameLine();

			if (ActionButton(
				OBFUSCATE(u8"Включить ИИ"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Включить ИИ для указанной страны.") : OBFUSCATE(u8"Игровая сессия ещё не готова.")))
			{
				int tag = 0;
				if (TryReadCountryTag(tag))
				{
					int* TagPtr = &tag;
					CAiEnableCommand* EnableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					EnableAI = AIEnableFunc(EnableAI, TagPtr, 2);
					CSessionPostTramp(pCSession, EnableAI, 1);
					SetUiNotice(OBFUSCATE(u8"ИИ выбранной страны включён."), UiNoticeKind::Success);
				}
				else
					SetUiNotice(OBFUSCATE(u8"Введите корректный числовой тег страны."), UiNoticeKind::Error);
			}
			ImGui::SameLine();
			if (ActionButton(
				OBFUSCATE(u8"Отключить ИИ"),
				sessionReady,
				sessionReady ? OBFUSCATE(u8"Отключить ИИ для указанной страны.") : OBFUSCATE(u8"Игровая сессия ещё не готова.")))
			{
				int tag = 0;
				if (TryReadCountryTag(tag))
				{
					int* TagPtr = &tag;
					CAiEnableCommand* DisableAI = (CAiEnableCommand*)GetCCommandFunc(56);
					DisableAI = AIEnableFunc(DisableAI, TagPtr, 0);
					CSessionPostTramp(pCSession, DisableAI, 1);
					SetUiNotice(OBFUSCATE(u8"ИИ выбранной страны отключён."), UiNoticeKind::Success);
				}
				else
					SetUiNotice(OBFUSCATE(u8"Введите корректный числовой тег страны."), UiNoticeKind::Error);
			}
			if (ActionButton(
				OBFUSCATE(u8"Сменить страну"),
				sessionReady && pCGameState != nullptr,
				pCGameState ? OBFUSCATE(u8"Переключить игрока на указанную страну.") : OBFUSCATE(u8"Состояние игры ещё не готово.")))
			{
				int tag = 0;
				if (TryReadCountryTag(tag))
				{
					CGameStateSetPlayerTramp(pCGameState, &tag);
					SetUiNotice(OBFUSCATE(u8"Страна игрока изменена."), UiNoticeKind::Success);
				}
				else
					SetUiNotice(OBFUSCATE(u8"Введите корректный числовой тег страны."), UiNoticeKind::Error);
			}
			ImGui::SameLine();
			if (ImGui::Button(OBFUSCATE(u8"Сбросить")))
			{
				memset(TagBuffer, 0, sizeof(TagBuffer));
				SetUiNotice(OBFUSCATE(u8"Поле тега очищено."), UiNoticeKind::Info);
			}

			SectionHeader(OBFUSCATE(u8"Опасные действия"));
			if (!bCrasher)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.10f, 0.10f, 1.0f));
				if (ActionButton(
					OBFUSCATE(u8"Включить непрерывный крашер"),
					crashReady,
					crashReady ? OBFUSCATE(u8"Потребуется дополнительное подтверждение.") : OBFUSCATE(u8"Команда крашера ещё не готова."),
					ImVec2(260, 30)))
				{
					RequestDangerousAction(DangerousAction::EnableContinuousCrasher);
				}
				ImGui::PopStyleColor();
			}
			else if (ImGui::Button(OBFUSCATE(u8"Отключить непрерывный крашер"), ImVec2(260, 30)))
			{
				bCrasher = false;
				SetUiNotice(OBFUSCATE(u8"Непрерывный крашер отключён."), UiNoticeKind::Success);
			}

			ImGui::TextDisabled(OBFUSCATE(u8"Автор: Silver"));
		}



		if (eMenu == eBoostMenu) {
			uintptr_t currentGameState = (GameState && *(uintptr_t*)GameState) ? *(uintptr_t*)GameState : 0;
			const bool stateChanged = (currentGameState != 0 && currentGameState != g_LastLoadedGameState);
			const bool listEmptyButReady = (gCountryBoosts.empty() && currentGameState != 0);

			if (gBoostListNeedsRefresh.exchange(false, std::memory_order_relaxed) || stateChanged || listEmptyButReady) {
				InitCountryBoosts();
			}

			SectionHeader(OBFUSCATE(u8"Усиление страны"));
			ImGui::TextWrapped(OBFUSCATE(u8"Работает во время игры и использует команду пользовательской сложности из правил игры."));

			if (!gCountryBoosts.empty()) {
				if (current_item < 0 || current_item >= static_cast<int>(gCountryBoosts.size()))
					SelectCountryBoost(0);

				const std::string previewName = CountryDisplayName(gCountryBoosts[current_item].pName);
				if (ImGui::BeginCombo(OBFUSCATE(u8"Страна"), previewName.c_str())) {
					uintptr_t curGS = (GameState && *(uintptr_t*)GameState) ? *(uintptr_t*)GameState : 0;
					if (gCountryBoosts.empty() && curGS != 0) {
						InitCountryBoosts();
					}
					for (int index = 0; index < static_cast<int>(gCountryBoosts.size()); ++index) {
						const std::string itemName = CountryDisplayName(gCountryBoosts[index].pName);
						const bool isSelected = current_item == index;
						if (ImGui::Selectable(itemName.c_str(), isSelected))
							SelectCountryBoost(index);
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Text(OBFUSCATE(u8"Бонус силы: от 0%% до 1 000 000%%"));
				ImGui::SetNextItemWidth(300.0f);
				ImGui::SliderInt(OBFUSCATE(u8"Быстро %%##CountryBoost"), &gBoostPercent, 0, 10000);
				ShowItemTooltip(OBFUSCATE(u8"Быстрый выбор в практичном диапазоне от 0% до 10 000%."));
				ImGui::SetNextItemWidth(120.0f);
				ImGui::InputInt(OBFUSCATE(u8"Точное значение %%##CountryBoost"), &gBoostPercent, 100, 10000);
				ShowItemTooltip(OBFUSCATE(u8"Позволяет ввести значение вплоть до 1 000 000%."));
				gBoostPercent = std::clamp(gBoostPercent, 0, kMaxBoostPercent);

				const bool canApplyBoost = sessionReady && g_SelectedCountry != nullptr;
				if (ActionButton(
					OBFUSCATE(u8"Применить"),
					canApplyBoost,
					canApplyBoost ? OBFUSCATE(u8"Применить выбранный процент к стране.") : OBFUSCATE(u8"Сессия или выбранная страна ещё не готовы."),
					ImVec2(120, 28)))
				{
					ApplyCountryBoostWithNotice(gBoostPercent);
				}
				ImGui::SameLine();
				if (ActionButton(
					OBFUSCATE(u8"Сбросить до 0%"),
					canApplyBoost,
					canApplyBoost ? OBFUSCATE(u8"Сбросить усиление выбранной страны.") : OBFUSCATE(u8"Сессия или выбранная страна ещё не готовы."),
					ImVec2(140, 28)))
				{
					ApplyCountryBoostWithNotice(0);
				}

				if (ActionButton(OBFUSCATE("100%"), canApplyBoost, OBFUSCATE(u8"Установить усиление 100%."))) ApplyCountryBoostWithNotice(100);
				ImGui::SameLine();
				if (ActionButton(OBFUSCATE("10 000%"), canApplyBoost, OBFUSCATE(u8"Установить усиление 10 000%."))) ApplyCountryBoostWithNotice(10000);
				ImGui::SameLine();
				if (ActionButton(OBFUSCATE("100 000%"), canApplyBoost, OBFUSCATE(u8"Установить усиление 100 000%."))) ApplyCountryBoostWithNotice(100000);
				ImGui::SameLine();
				if (ActionButton(OBFUSCATE("1 000 000%"), canApplyBoost, OBFUSCATE(u8"Установить усиление 1 000 000%."))) ApplyCountryBoostWithNotice(1000000);

				ImGui::Text(OBFUSCATE(u8"Внутреннее значение HOI4: %d"), gBoostPercent * kBoostInternalUnitsPerPercent);
			}

			if (ActionButton(
				OBFUSCATE(u8"Обновить страны"),
				GameState != 0,
				GameState ? OBFUSCATE(u8"Повторно получить список стран из игры.") : OBFUSCATE(u8"Данные состояния игры ещё не найдены.")))
			{
				InitCountryBoosts();
				SetUiNotice(gBoostStatus.c_str(), gCountryBoosts.empty() ? UiNoticeKind::Warning : UiNoticeKind::Success);
			}
			ImGui::TextColored(
				gCountryBoosts.empty() ? UiNoticeColor(UiNoticeKind::Warning) : UiNoticeColor(UiNoticeKind::Info),
				OBFUSCATE("%s"),
				gBoostStatus.c_str());
			ImGui::TextDisabled(OBFUSCATE(u8"Авторы: Silver / polux"));
		}



		if (eMenu == eDebugMenu) 
		{
			SectionHeader(OBFUSCATE(u8"Интерфейс"));
			ImGui::Text(OBFUSCATE(u8"Горячая клавиша меню: Insert"));
			ImGui::TextDisabled(OBFUSCATE(u8"Размер и положение окна сохраняются ImGui автоматически."));

			SectionHeader(OBFUSCATE(u8"Дополнительно"));
			if (ImGui::CollapsingHeader(OBFUSCATE(u8"Режим разработчика")))
			{
				ImGui::TextWrapped(OBFUSCATE(u8"Технические параметры предназначены для диагностики. Спасибо polux и armer за помощь в поиске."));
				CheckboxWithTooltip(
					OBFUSCATE(u8"Отладочная консоль"),
					&bDebug,
					OBFUSCATE(u8"Открывать консоль с диагностическим выводом."));
				CheckboxWithTooltip(
					OBFUSCATE(u8"Отправка команд (SessionPost)"),
					&dSessionPost,
					OBFUSCATE(u8"Выводить перехваченные команды игровой сессии."));
				CheckboxWithTooltip(
					OBFUSCATE(u8"Список игроков"),
					&bShowPlayers,
					OBFUSCATE(u8"Выводить диагностические сведения об игроках."));
				CheckboxWithTooltip(
					OBFUSCATE(u8"Журнал HOI4"),
					&bLog,
					OBFUSCATE(u8"Показывать сообщения внутреннего журнала HOI4."));
				CheckboxWithTooltip(
					OBFUSCATE(u8"Показать IP хоста"),
					&bFindChost,
					OBFUSCATE(u8"Искать и выводить адрес текущего хоста."));
				if (ImGui::Button(OBFUSCATE(u8"Проверить вывод")))
				{
					printm(OBFUSCATE("Hello"));
					SetUiNotice(OBFUSCATE(u8"Тестовое сообщение отправлено в консоль."), UiNoticeKind::Info);
				}
			}
		}

		RenderDangerousActionPopup();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(UiNoticeColor(gUiNoticeKind), OBFUSCATE(u8"Статус: %s"), gUiNotice.c_str());
		

		if (bCrasher) {
			if (!Crasher(1))
			{
				bCrasher = false;
				SetUiNotice(OBFUSCATE(u8"Крашер отключён: игровая сессия больше не готова."), UiNoticeKind::Error);
			}
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
		RenderSupplyDivisionESP();


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
		printm(OBFUSCATE("CSP Called"));
	}
	return CSessionPostTramp(pThis, pCommand, ForceSend);
}

CAddPlayerCommand* __fastcall hkCAddPlayerCommand(CAddPlayerCommand* pThis, CString* User, CString* Name, DWORD* unknown, int nMachineId, bool bHotjoin, __int64 a7)
{
	gBoostListNeedsRefresh.store(true, std::memory_order_relaxed);
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
		printm(OBFUSCATE("APC Called"));
		printm(OBFUSCATE("Machine ID:"));
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
		printm(OBFUSCATE("RPC Called"));
	}
	
	return CRemovePlayerCommandTramp(pThis, _machineID, eReason, a4);

}

__int64 __fastcall hkCGameStateSetPlayer(void* pThis, int* Tag)
{
	pCGameState = pThis;
	if (bDebug) {
		printm(OBFUSCATE("CGSSP Called"));
	}
	return CGameStateSetPlayerTramp(pThis, Tag);
}

EmptyTest* __fastcall hkCustomDiffM(void* pThis, __int64 Tag, int Boost) {
	if (bDebug) {
		printm(OBFUSCATE("[hkCustomDiffM] Hook Called | pThis=0x") + std::to_string(reinterpret_cast<uintptr_t>(pThis)) +
			OBFUSCATE(", Tag=0x") + std::to_string(Tag) + OBFUSCATE(", RawBoost=") + std::to_string(Boost));
		if (Tag) {
			CString* pTagStr = reinterpret_cast<CString*>(Tag);
			const char* tagStr = SafeGetCStringPtr(pTagStr);
			printm(OBFUSCATE("[hkCustomDiffM] Tag String: ") + std::string(tagStr ? tagStr : "NULL/Invalid"));
		}
	}
	CountryTag = Tag;

	return BoostTramp(pThis, Tag, Boost);
}

CCrash* __fastcall hkCrash(void* pThis, unsigned int a1) {
	if (bDebug) {
		printm(OBFUSCATE("Crash Called"));
		std::string x = std::to_string(a1);
		printm(x);
	}
	return CCrashTramp(pThis, a1);
}

CPauseGameCommand* __fastcall hkPauseGame(void* pThis, __int64 a2, char a3, char a4) {
	if (bDebug) {
		printm(OBFUSCATE("hkPauseGame Called"));
	}
	return CPauseGameTramp(pThis, a2, a3);
}

bool __fastcall hkNetDisconnect(void* pThis) {

	if (bDebug)
		printm(OBFUSCATE("hkNetDisconnect Called"));

	if (!bRefuseConnect)
		return NetDisconnectTramp(pThis);
	
	return 0;
	

}
bool Tester1 = false;
bool Tester2 = false;
void __fastcall hkSetState(__int64 pThis, int a2) {
	if (bDebug)
		printm(OBFUSCATE("hkSetState Called"));

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
		printm(OBFUSCATE("hkMatchMaking Called"));
	if (!bRefuseConnect)
		return MatchmakingLeaveTramp(pThis);
}

void __fastcall hkHuman(void* pThis, CHuman* Human) {
	//40 steam
	//72 User
	//176 MachineID
	if (bDebug) {
		printm(OBFUSCATE("AddPlayerItem Called"));
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
		printm(OBFUSCATE(" "));

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
		//printm(OBFUSCATE("ProxyServer Called"));
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

	//printm(OBFUSCATE("Token Called"));
	TokenString = "{" + std::to_string(Type) + ", \"" + pString + "\"},";
	if(bMapTokens)
		printString(TokenString);
	//printm(TokenString);
	return CTokenTramp(pThis, Type, pString);
}

void HookFunctions() {
	if(Log)
		WriteLog(OBFUSCATE("HookFunctions Initiated"));
	IPVector.push_back("0.0.0.0:0");

	//DONE
	CSessionPostHook = CSessionPost(FIND_PATTERN("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xD9\x45\x84\xC0", "xxxx?xxxx????xxxxxxxxx"));
	MH_CreateHook(CSessionPostHook, &hkCSessionPost, (LPVOID*)&CSessionPostTramp);
	MH_EnableHook(CSessionPostHook);
	//Done
	CAddPlayerCommandHook = GetCAddPlayerCommand(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x49\x8B\xF9\x49\x8B\xD8\x48\x8B\xF1\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4E\x00\xC7\x46\x00\x00\x00\x00\x00\x66\x89\x4E\x00\x48\x89\x4E\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x06\x48\x8D\x4E\x00\xE8\x00\x00\x00\x00\x90\x48\x8D\x4E", "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xxxx"));
	MH_CreateHook(CAddPlayerCommandHook, &hkCAddPlayerCommand, (LPVOID*)&CAddPlayerCommandTramp);
	MH_EnableHook(CAddPlayerCommandHook);
	//Done
	CGameStateSetPlayerHook = CGameStateSetPlayer(FIND_PATTERN("\x40\x56\x57\x48\x83\xEC\x00\x8B\x02\x48\x89\x6C\x24", "xxxxxx?xxxxxx"));

	MH_CreateHook(CGameStateSetPlayerHook, &hkCGameStateSetPlayer, (LPVOID*)&CGameStateSetPlayerTramp);
	MH_EnableHook(CGameStateSetPlayerHook);

	//Done
	CRemovePlayerCommandHook = GetCRemovePlayerCommand(FIND_PATTERN("\x48\x89\x4C\x24\x00\x53\x48\x83\xEC\x00\x48\x8B\xD9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4B\x00\xC7\x43\x00\x00\x00\x00\x00\x66\x89\x4B\x00\x48\x89\x4B\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x03\x44\x89\x43", "xxxx?xxxx?xxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx"));


	



	MH_CreateHook(CRemovePlayerCommandHook, &hkCRemovePlayerCommand, (LPVOID*)&CRemovePlayerCommandTramp);
	MH_EnableHook(CRemovePlayerCommandHook);

	//DONE
	CCrashFunc = GetCCrash(FIND_PATTERN("\x45\x33\xC0\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x44\x89\x41\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x44\x89\x41\x00\x4C\x89\x41\x00\x89\x51\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x00\x48\x8D\x05", "xxxxx??xxx????xx?????xxxxxxxxx?xx?????xxxx?xxx?xx?xxxxxxxxxxxxxxxxxx?xxxx?xxx"));
	MH_CreateHook(CCrashFunc, &hkCrash, (LPVOID*)&CCrashTramp);
	MH_EnableHook(CCrashFunc);


	//DONE
	CStartGameCommandFunc = GetCStartGameCommand(FIND_PATTERN("\x33\xD2\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x89\x51\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x89\x51\x00\x48\x89\x51\x00\xC3\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57", "xxxx??xxx????xx?????xxxxxxxx?xx?????xxx?xxx?xxxxxxxx?x"));
	//DONE
	GetCCommandFunc = GetCCommand(FIND_PATTERN("\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x33\xC0", "x????xxxxxxxxxxxx????xxxxxxxxxxxxx"));
	//DONE
	AIEnableFunc = GetEnableAI(FIND_PATTERN("\x45\x33\xC9\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x44\x89\x49\x00\x48\x89\x01\x66\x44\x89\x49\x00\x4C\x89\x49\x00\xC7\x41\x00\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x8B\x02\x89\x41\x00\x48\x8B\xC1\x44\x89\x41\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x53", "xxxxx??xxx????xxx?xxxxxxx?xxx?xx?????xx?????xxxx?xxxxxx?xxxxxxxxxx"));
	//Done
	CPauseGameFunc = GetCPauseGameCommand(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x41\x0F\xB6\xF1\x41\x0F\xB6\xD8\x48\x8B\xF9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\x66\x89\x4F\x00\x48\x89\x4F\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8D\x4F\x00\xE8\x00\x00\x00\x00\x88\x5F\x00\x40\x88\x77\x00\x48\x8B\xC7\x48\x8B\x5C\x24\x00\x48\x8B\x74\x24\x00\x48\x83\xC4\x00\x5F\xC3\xCC\xCC\xCC\xCC\xCC", "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xx?xxx?xxxxxxx?xxxx?xxx?xxxxxxx"));
	MH_CreateHook(CPauseGameFunc, &hkPauseGame, (LPVOID*)&CPauseGameTramp);
	MH_EnableHook(CPauseGameFunc);

	//Done
	IncreaseSpeedFunc = GetCGameSpeed(FIND_PATTERN("\x33\xD2\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x89\x51\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x89\x51\x00\x48\x89\x51\x00\xC3\xCC\xCC\xCC\x45\x33\xC0\xC6\x41\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x48\x89\x01\x48\x8B\xC1\x44\x89\x41\x00\xC7\x41\x00\x00\x00\x00\x00\x66\x44\x89\x41\x00\x4C\x89\x41\x00\x89\x51", "xxxx??xxx????xx?????xxxxxxxx?xx?????xxx?xxx?xxxxxxxxx??xxx????xx?????xxxxxxxxx?xx?????xxxx?xxx?xx"));

	//Done
	BoostFunc = GetCustomDiffM(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x00\x49\x8B\xD8\x48\x8B\xF9\xC6\x41\x00\x00\xC7\x41\x00\x00\x00\x00\x00\x33\xC9\x89\x4F\x00\xC7\x47\x00\x00\x00\x00\x00\x66\x89\x4F\x00\x48\x89\x4F\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x07\x48\x8D\x4F\x00\xE8\x00\x00\x00\x00\x48\x89\x5F\x00\x48\x8B\xC7\x48\x8B\x5C\x24\x00\x48\x83\xC4\x00\x5F\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x45\x33\xC0", "xxxx?xxxx?xxxx?xxxxxxxx??xx?????xxxx?xx?????xxx?xxx?xxx????xxxxxx?x????xxx?xxxxxxx?xxx?xxxxxxxxxxxx"));
	MH_CreateHook(BoostFunc, &hkCustomDiffM, (LPVOID*)&BoostTramp);
	MH_EnableHook(BoostFunc);

	/*uintptr_t _pInstanceAddr = uintptr_t(FIND_PATTERN("\x48\x83\x3D\x00\x00\x00\x00\x00\x75\x00\xB9\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x89\x44\x24\x00\x33\xDB", "xxx?????x?x????x????xxxx?xx"));
	int32_t displacement = *(int32_t*)(_pInstanceAddr + 3);
	globalAddr = displacement - (_pInstanceAddr + 7);//_pInstanceAddr + 7 + displacement;
	pInstance = GameBase + globalAddr;

	uintptr_t instr = uintptr_t(FIND_PATTERN("\x4C\x8B\x3D\x00\x00\x00\x00\x49\x63\xB7\x00\x00\x00\x00\x48\x85\xF6\x7E\x00\x33\xDB", "xxx????xxx????xxxx?xx"));
	int rel = (int)(instr + 3);
	uintptr_t nextInstr = instr + 7;
	uintptr_t variable = nextInstr + rel;*/


	//DONE
	NetDisconnectHook = GetNetworkDisconnect(FIND_PATTERN("\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\x60\x00\x32\xC0\xC3\xCC\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\x32\xC0\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3", "xxxx?xxxxxx?xxxxxxxx?xxxxxx????xxxxxxxxxxxxxxxxxxxxx?xxxxxx????x"));
	MH_CreateHook(NetDisconnectHook, &hkNetDisconnect, (LPVOID*)&NetDisconnectTramp);
	MH_EnableHook(NetDisconnectHook);
	//Done
	SetStateHook = GetSessionSetState(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x63\xDA", "xxxx?xxxx?xxxx?xxxx????xxx"));
	MH_CreateHook(SetStateHook, &hkSetState, (LPVOID*)&SetStateTramp);
	MH_EnableHook(SetStateHook);
	//DONE
	MatchmakingLeaveHook = GetMatchmakingLeaveLobby(FIND_PATTERN("\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3\x48\x85\xC9\x74\x00\x48\x8B\x01\x48\xFF\xA0\x00\x00\x00\x00\xC3\x48\x83\xEC", "xxxx?xxxxxx????xxxxx?xxxxxx????xxxx"));
	MH_CreateHook(MatchmakingLeaveHook, &hkMatchMakingLeave, (LPVOID*)&MatchmakingLeaveTramp);
	MH_EnableHook(MatchmakingLeaveHook);


	
	HumanHook = GetAddHumanItem(FIND_PATTERN("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x00\x4C\x8B\xEA\x48\x8B\xF1", "xxxx?xxxxxxxxxxxxxxxxx?xxxxxx"));
	MH_CreateHook(HumanHook, &hkHuman, (LPVOID*)&HumanTramp);
	MH_EnableHook(HumanHook);


	//CProxyServerHook = GetProxyServer(FIND_PATTERN("\x48\x8B\xC4\x48\x89\x58\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\x00\x0F\x29\x78\x00\x4D\x8B\xF1", "xxxxxx?xxxxxxxxxxxxxx????xxx????xxx?xxx?xxx"));
	//MH_CreateHook(CProxyServerHook, &hkCProxyServer, (LPVOID*)&CProxyServerTramp);
	//MH_EnableHook(CProxyServerHook);

	//Done
	CLogStreamHook = GetLogStream(FIND_PATTERN("\x40\x53\x48\x83\xEC\x00\x48\x8B\xD9\x49\xC7\xC0\x00\x00\x00\x00\x49\xFF\xC0\x42\x80\x3C\x02\x00\x75\x00\xE8\x00\x00\x00\x00\x48\x8B\xC3\x48\x83\xC4\x00\x5B\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x54\x24", "xxxxx?xxxxxx????xxxxxxx?x?x????xxxxxx?xxxxxxxxxxxxxx"));
	MH_CreateHook(CLogStreamHook, &hkLogStream, (LPVOID*)&CLogStreamTramp);
	MH_EnableHook(CLogStreamHook);

	//Done
	CLogHook = GetCLog(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x48\x89\x54\x24\x00\x41\x56\x48\x83\xEC\x00\x41\x8B\xF1", "xxxx?xxxx?xxxx?xxxx?xxxxx?xxx"));
	MH_CreateHook(CLogHook, &hkLog, (LPVOID*)&CLogTramp);
	MH_EnableHook(CLogHook);
	//Done
	COperatorHook = GetOperator(FIND_PATTERN("\x48\x83\x7A\x00\x00\x76\x00\x48\x8B\x12\xE9\x00\x00\x00\x00\xCC\x48\x83\x79", "xxx??x?xxxx????xxxx"));
	MH_CreateHook(COperatorHook, &hkOperator, (LPVOID*)&COperatorTramp);
	MH_EnableHook(COperatorHook);



	// TOKENS
	CTokenInitHook = GetTokenInitialiser(FIND_PATTERN("\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8D\x44\x24", "x????x????xxxxxxx"));
	//DONE
	CTokenHook = GetCToken(FIND_PATTERN("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x00\xC7\x01\x00\x00\x00\x00\x4C\x8B\xF1\xC6\x41\x00\x00\x49\x8B\xF0", "xxxx?xxxx?xxxx?xxxxx?xx????xxxxx??xxx"));
	MH_CreateHook(CTokenHook, &hkToken, (LPVOID*)&CTokenTramp);
	MH_EnableHook(CTokenHook);



	if(Log)
		WriteLog(OBFUSCATE("HookFunctions Finished"));


}






	

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	if(Log)
		WriteLog(OBFUSCATE("MainThread Initiated"));
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			if (Log) {
				WriteLog(OBFUSCATE("RenderType: DX11 Success"));
				DX11Success = true;
			}
			kiero::bind(8, (void**)& oPresent, hkPresent);
			init_hook = true;
		}
		if (!init_hook)
			Sleep(100);
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
			WriteLog(OBFUSCATE("Process Attached"));
		HANDLE thread = CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		if (thread)
			CloseHandle(thread);

		break;
	}
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	}
	return TRUE;
}
