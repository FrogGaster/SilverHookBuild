#include <windows.h>
#include <iostream>

enum ERemovalReason
{
	REMOVE_PLAYER_DEFAULT = 0,
	REMOVE_PLAYER_CONNECTION_LOST = 1,
	REMOVE_PLAYER_KICKED = 2,
	REMOVE_PLAYER_BANNED = 3,
};

struct _ENetHost;

struct CPdxArrayCBlob
{
	char* _pData;
	int _nCapacity;
	int _nSize;
};

struct CBlob
{
	CPdxArrayCBlob _Data;
};

const struct CID {
	unsigned int _Type;
	unsigned int _ID;
};

const struct CString
{
	std::basic_string<char, std::char_traits<char>, std::allocator<char>> _str;

	template <class InputIterator>
	std::string& assign(InputIterator first, InputIterator last);

	std::string& assign(const char* str, size_t len);
};



template<class InputIterator>
inline std::string& CString::assign(InputIterator first, InputIterator last)
{
	_str.assign(first, last); // Use the assign method of std::basic_string
	return _str;              // Return the modified string
}

inline std::string& CString::assign(const char* str, size_t len)
{
	_str.assign(str, len);    // Assign the string using length
	return _str;              // Return the modified string
}


const struct CCountryTag
{
	int _nLinkIndex;
};

struct COption
{
	bool _bIsSelected;
};

struct CPersistentVtbl
{
	/*void* (__fastcall* __vecDelDtor)(unsigned int);
	void(__fastcall* Write)(CWriter*);
	void(__fastcall* WriteMembers)(CWriter*);
	void(__fastcall* Read)(CReader*);
	void(__fastcall* ReadMember)(CReader*, int);
	void(__fastcall* InitPostRead)(CString*, int, int);
	void(__fastcall* InitPostRead)();*/
};

struct CPersistent
{
	CPersistentVtbl* vfptr;
	int _TypeToken; //357
};


struct CReferenceObject : CPersistent {
	CID _ID;
	bool _bIsCreated;
};

struct CCommand : CPersistent
{
	bool _bLogged; //0
	int _nCommandSender; //-1
	unsigned int _nRecipient; //0
	unsigned __int16 _nRecipientPort; //-65536
	unsigned __int16 _nTickStamp;
	bool _bTargetedAsynchronous; //0
	bool _bMayBeRemoved;
	unsigned int _nIdentity; //0
};

struct CAddPlayerCommand : CCommand
{
	CString* _User; //hoi
	CString* _Name;
	
	int _nMachineId;
	bool _bHotjoin;
	__int64 Social;
};

struct CRemovePlayerCommand : CCommand
{
	int _nMachineId;
	int unknown;
	int eReason;
};

struct CNameChange : CCommand
{
	CString* _Name;
};

struct CCrash : CCommand
{
	unsigned int _nCrash;
};

struct CChatMessage : CCommand
{
	CString* _Message;
};

struct CSetSpeed : CCommand
{
	int speed;
};

struct CMultiplayerConfiguration {

};
// All these empty structs might aswell just be (void*)... something something CCommand inheritance


struct CPauseGameCommand : CCommand
{

};

struct CStartGameCommand : CCommand
{

};

struct CPostHotJoinCommand : CCommand
{

};

struct CAiEnableCommand : CCommand
{

};

struct CChatLeaveFake : CCommand
{
	int _machineID;
};

struct CCreateEquipmentVariant : CCommand
{

};

struct CGameSpeed : CCommand
{

};

struct EmptyTest : CCommand
{

};


struct CSessionConfig {
	__int64 _NetAddress;
	CString* _GameName;
	int _Type;
	int _BasicType;
	int _Status;
	__int64 _Version;
	bool _IsResetAllowed;
};

struct CMultiplayerConfig {
	CString* _Name;
	DWORD* _a2;
	void* _SessionConfig;
	bool _IsGameOwner;

};

enum Menu {
	eEmpty,
	eBoostMenu,
	eIngameMenu,
	eLobbyMenu,
	eDebugMenu
};

struct CDate
{
	int _nUniversalDate;
};




struct CGregorianDateVtbl
{
	//void* (__fastcall* CGregorianDate)(CGregorianDate* this);
	//void(__fastcall* AddHours)(CGregorianDate* this, int);
	//void(__fastcall* AddDays)(CGregorianDate* this, int);
	//void(__fastcall* AddMonths)(CGregorianDate* this, int);
	//void(__fastcall* AddYears)(CGregorianDate* this, int);
};

struct CGregorianDate : CDate
{
	CGregorianDateVtbl* vfptr;
};

struct CGameDate : CGregorianDate, CPersistent
{
};

struct CSaveFile
{
	CString* _Path;
	CString* _SaveName;
	__int64 _ModifiedDate;
	int _Version;
	int _FileSize;
	bool _IsValid;
	bool _IsRemote;
	bool _IsSynced;
};



template<typename Key, typename Value>
class CPdxArray {
	// Internal structure - maybe similar to std::map<Key, Value> or std::vector<std::pair<Key, Value>>
	int* _pData;
	int _nCapacity;
	int _nSize;
};

template<typename TEnum, typename TStorage = unsigned int>
struct CSimpleBitMask
{
	TStorage _eFlagMask;

	// Example utility methods (optional, inferred from name)
	bool HasFlag(TEnum flag) const {
		return (_eFlagMask & static_cast<TStorage>(flag)) != 0;
	}

	void AddFlag(TEnum flag) {
		_eFlagMask |= static_cast<TStorage>(flag);
	}

	void RemoveFlag(TEnum flag) {
		_eFlagMask &= ~static_cast<TStorage>(flag);
	}

	void Clear() {
		_eFlagMask = 0;
	}
};

struct /*VFT*/ CLogger_vtbl
{
	/*void(__fastcall * ~CLogger)(CLogger* this);
	void(__fastcall* Log)(CLogger* this, int, const CString*, int, const CString*);
	CLogger::ELoggerType(__fastcall* GetType)(CLogger* this);
	unsigned int(__fastcall* GetLogCount)(CLogger* this, int);*/
};

enum EFileMode : __int32
{
	OPEN_READ = 0x0,
	OPEN_WRITE = 0x1,
	OPEN_WRITE_APPEND = 0x2,
};

enum EHumanFlagsType : __int32
{
	HUMAN_FLAGS_NONE = 0x0,
	HUMAN_FLAGS_COUNTRYLEADER = 0x1,
	HUMAN_FLAGS_ASKINGFORHOTJOIN = 0x2,
	HUMAN_FLAGS_READY = 0x4,
	HUMAN_FLAGS_INSYNC = 0x8,
	HUMAN_FLAGS_DEFAULT = 0x9,
};

struct __declspec(align(4)) CLogger
{
	CLogger_vtbl* __vftable /*VFT*/;
	CString* _pOpenLogCommand;
	CString* _pOpenLogCommandPostfix;
	unsigned int _nLogCount;
	bool _bIsNewLogCommandFormat;
};
struct __declspec(align(8)) SVirtualFile_PIMPL
{
	//PHYSFS_File* _pFile;
	bool _bIsWriteMode;
};

struct /*VFT*/ CVirtualFile_vtbl
{
	//void(__fastcall * ~CVirtualFile)(CVirtualFile* this);
};

struct __declspec(align(8)) CVirtualFile
{
	CVirtualFile_vtbl* __vftable /*VFT*/;
	SVirtualFile_PIMPL* _hFile;
	CString _FileName;
	EFileMode _Mode;
};



struct CVirtualFilePtr
{
	CVirtualFile* _Myptr;
};

enum ELoggerSetting : __int32
{
	LOGGER_NONE = 0x0,
	LOGGER_DISABLE_FLUSH = 0x1,
	LOGGER_ENABLE_TIMESTAMP = 0x2,
	LOGGER_ENABLE_FILENAME = 0x4,
	LOGGER_CLEAR_WHEN_FILE_SIZE = 0x8,
	LOGGER_CLEAR_WHEN_NUM_ENTRIES = 0x10,
};



struct CFileLogger : CLogger
{
	CVirtualFilePtr _pFile;
	CSimpleBitMask<enum ELoggerSetting, unsigned int> _Settings;
	unsigned __int64 _nUnprunedFileSize;
	unsigned __int64 _nUnprunedNumEntries;
	unsigned __int64 _nExtraArg;
	unsigned __int64 _nNumEntries;
};


const struct CHuman : CPersistent
{
	CPdxArray<int, int> _PinnedStrategicRegions;
	CString* _User;
	CString* _Name;
	CCountryTag _SelectedCountry;
	CGameDate _LastPingedDate;
	CSimpleBitMask<enum EHumanFlagsType, unsigned char> _HumanFlags;
	int _nMachineId;
};



struct CSaveGameMeta : CSaveFile
{
	CCountryTag _CountryTag;
	CString* _CosmeticTag;
	CGameDate _GameDate;
	void* _PlayerCountries;
	void* _FlagsMask;
	void* _pIdeology;
	int _nDifficulty;
	unsigned int _nDlcMask;
	void* _Mods;
};