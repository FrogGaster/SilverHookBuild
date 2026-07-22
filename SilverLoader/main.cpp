#include <Windows.h>
#include <TlHelp32.h>
#include <winhttp.h>

#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <string>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")

namespace
{
    constexpr int kEmbeddedDllResourceId = 101;

    class Handle
    {
    public:
        explicit Handle(HANDLE value = nullptr) : value_(value) {}
        ~Handle() { if (value_ && value_ != INVALID_HANDLE_VALUE) CloseHandle(value_); }

        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;

        HANDLE get() const { return value_; }
        explicit operator bool() const { return value_ && value_ != INVALID_HANDLE_VALUE; }

    private:
        HANDLE value_;
    };

    class WinHttpHandle
    {
    public:
        explicit WinHttpHandle(HINTERNET value = nullptr) : value_(value) {}
        ~WinHttpHandle() { if (value_) WinHttpCloseHandle(value_); }

        WinHttpHandle(const WinHttpHandle&) = delete;
        WinHttpHandle& operator=(const WinHttpHandle&) = delete;

        HINTERNET get() const { return value_; }
        explicit operator bool() const { return value_ != nullptr; }

    private:
        HINTERNET value_;
    };

    enum class LaunchNotificationResult
    {
        NotConfigured,
        Sent,
        Failed
    };

    std::wstring ReadEnvironmentVariable(const wchar_t* name)
    {
        const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
        if (required)
        {
            std::wstring value(required, L'\0');
            const DWORD written = GetEnvironmentVariableW(name, value.data(), required);
            if (written && written < required)
            {
                value.resize(written);
                return value;
            }
        }

        DWORD bytes = 0;
        if (RegGetValueW(
                HKEY_CURRENT_USER,
                L"Environment",
                name,
                RRF_RT_REG_SZ,
                nullptr,
                nullptr,
                &bytes) != ERROR_SUCCESS ||
            bytes < sizeof(wchar_t))
            return {};

        std::wstring value(bytes / sizeof(wchar_t), L'\0');
        if (RegGetValueW(
                HKEY_CURRENT_USER,
                L"Environment",
                name,
                RRF_RT_REG_SZ,
                nullptr,
                value.data(),
                &bytes) != ERROR_SUCCESS)
            return {};

        while (!value.empty() && value.back() == L'\0')
            value.pop_back();
        return value;
    }

    std::string ToUtf8(const std::wstring& value)
    {
        if (value.empty())
            return {};

        const int required = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
            nullptr, 0, nullptr, nullptr);
        if (required <= 0)
            return {};

        std::string result(required, '\0');
        if (!WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                result.data(), required, nullptr, nullptr))
            return {};

        return result;
    }

    std::string UrlEncode(const std::string& value)
    {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(value.size() * 3);

        for (const unsigned char character : value)
        {
            if ((character >= 'a' && character <= 'z') ||
                (character >= 'A' && character <= 'Z') ||
                (character >= '0' && character <= '9') ||
                character == '-' || character == '_' || character == '.' || character == '~')
            {
                result.push_back(static_cast<char>(character));
            }
            else
            {
                result.push_back('%');
                result.push_back(hex[character >> 4]);
                result.push_back(hex[character & 0x0F]);
            }
        }

        return result;
    }

    bool IsValidBotToken(const std::wstring& token)
    {
        if (token.empty())
            return false;

        for (const wchar_t character : token)
        {
            if (!((character >= L'a' && character <= L'z') ||
                  (character >= L'A' && character <= L'Z') ||
                  (character >= L'0' && character <= L'9') ||
                  character == L':' || character == L'-' || character == L'_'))
                return false;
        }

        return true;
    }

    LaunchNotificationResult SendLaunchNotification()
    {
        const std::wstring botToken = ReadEnvironmentVariable(L"ZA_PUTINA_TELEGRAM_BOT_TOKEN");
        const std::wstring chatId = ReadEnvironmentVariable(L"ZA_PUTINA_TELEGRAM_CHAT_ID");
        if (botToken.empty() && chatId.empty())
            return LaunchNotificationResult::NotConfigured;
        if (!IsValidBotToken(botToken) || chatId.empty())
            return LaunchNotificationResult::Failed;

        WinHttpHandle session(WinHttpOpen(
            L"ZA_PUTINA_Loader/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0));
        if (!session)
            return LaunchNotificationResult::Failed;

        WinHttpSetTimeouts(session.get(), 2000, 2000, 2000, 2000);

        WinHttpHandle connection(WinHttpConnect(
            session.get(), L"api.telegram.org", INTERNET_DEFAULT_HTTPS_PORT, 0));
        if (!connection)
            return LaunchNotificationResult::Failed;

        const std::wstring requestPath = L"/bot" + botToken + L"/sendMessage";
        WinHttpHandle request(WinHttpOpenRequest(
            connection.get(),
            L"POST",
            requestPath.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE));
        if (!request)
            return LaunchNotificationResult::Failed;

        const std::string body =
            "chat_id=" + UrlEncode(ToUtf8(chatId)) +
            "&text=" + UrlEncode("ZA_PUTINA Loader started. No personal data was collected.");
        const wchar_t* headers = L"Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n";

        if (!WinHttpSendRequest(
                request.get(),
                headers,
                static_cast<DWORD>(-1L),
                const_cast<char*>(body.data()),
                static_cast<DWORD>(body.size()),
                static_cast<DWORD>(body.size()),
                0) ||
            !WinHttpReceiveResponse(request.get(), nullptr))
            return LaunchNotificationResult::Failed;

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        if (!WinHttpQueryHeaders(
                request.get(),
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusCodeSize,
                WINHTTP_NO_HEADER_INDEX))
            return LaunchNotificationResult::Failed;

        return statusCode >= 200 && statusCode < 300
            ? LaunchNotificationResult::Sent
            : LaunchNotificationResult::Failed;
    }

    std::wstring LastErrorMessage(DWORD error = GetLastError())
    {
        wchar_t* buffer = nullptr;
        const DWORD length = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            0,
            reinterpret_cast<wchar_t*>(&buffer),
            0,
            nullptr);

        std::wstring message = length && buffer ? buffer : L"Неизвестная ошибка";
        if (buffer)
            LocalFree(buffer);
        while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n'))
            message.pop_back();
        return message;
    }

    DWORD FindHoi4Process()
    {
        Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!snapshot)
            return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (!Process32FirstW(snapshot.get(), &entry))
            return 0;

        do
        {
            if (_wcsicmp(entry.szExeFile, L"hoi4.exe") == 0)
                return entry.th32ProcessID;
        } while (Process32NextW(snapshot.get(), &entry));

        return 0;
    }

    uintptr_t FindRemoteModule(DWORD processId, const wchar_t* moduleName)
    {
        Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId));
        if (!snapshot)
            return 0;

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (!Module32FirstW(snapshot.get(), &entry))
            return 0;

        do
        {
            if (_wcsicmp(entry.szModule, moduleName) == 0)
                return reinterpret_cast<uintptr_t>(entry.modBaseAddr);
        } while (Module32NextW(snapshot.get(), &entry));

        return 0;
    }

    bool IsX64Dll(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        IMAGE_DOS_HEADER dos{};
        input.read(reinterpret_cast<char*>(&dos), sizeof(dos));
        if (!input || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0)
            return false;

        input.seekg(dos.e_lfanew);
        DWORD signature = 0;
        IMAGE_FILE_HEADER fileHeader{};
        input.read(reinterpret_cast<char*>(&signature), sizeof(signature));
        input.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        return input && signature == IMAGE_NT_SIGNATURE && fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64;
    }

    class EmbeddedDllExtraction
    {
    public:
        ~EmbeddedDllExtraction()
        {
            if (path_.empty())
                return;

            if (!DeleteFileW(path_.c_str()))
                MoveFileExW(path_.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
            if (!directory_.empty() && !RemoveDirectoryW(directory_.c_str()))
                MoveFileExW(directory_.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        EmbeddedDllExtraction(const EmbeddedDllExtraction&) = delete;
        EmbeddedDllExtraction& operator=(const EmbeddedDllExtraction&) = delete;
        EmbeddedDllExtraction() = default;

        const std::filesystem::path& path() const { return path_; }

        bool Extract()
        {
            HMODULE module = GetModuleHandleW(nullptr);
            HRSRC resource = FindResourceW(
                module,
                MAKEINTRESOURCEW(kEmbeddedDllResourceId),
                RT_RCDATA);
            if (!resource)
                return false;

            const DWORD resourceSize = SizeofResource(module, resource);
            HGLOBAL resourceData = LoadResource(module, resource);
            const void* bytes = resourceData ? LockResource(resourceData) : nullptr;
            if (!bytes || !resourceSize)
                return false;

            std::error_code pathError;
            const std::filesystem::path tempRoot = std::filesystem::temp_directory_path(pathError);
            if (pathError)
                return false;

            directory_ = tempRoot /
                (L"ZA_PUTINA-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                 std::to_wstring(GetTickCount64()));
            if (!std::filesystem::create_directory(directory_, pathError) || pathError)
            {
                directory_.clear();
                return false;
            }

            path_ = directory_ / L"ZA_PUTINA.dll";
            std::ofstream output(path_, std::ios::binary | std::ios::trunc);
            output.write(static_cast<const char*>(bytes), resourceSize);
            output.close();
            if (!output)
            {
                std::filesystem::remove(path_, pathError);
                std::filesystem::remove(directory_, pathError);
                path_.clear();
                directory_.clear();
                return false;
            }

            return true;
        }

    private:
        std::filesystem::path path_;
        std::filesystem::path directory_;
    };

    std::filesystem::path DefaultDllPath()
    {
        std::wstring executable(MAX_PATH, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
        executable.resize(length);
        return std::filesystem::path(executable).parent_path() / L"ZA_PUTINA.dll";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    const LaunchNotificationResult notificationResult = SendLaunchNotification();
    if (notificationResult == LaunchNotificationResult::Sent)
        std::wcout << L"Уведомление о запуске отправлено в Telegram (без персональных данных).\n";
    else if (notificationResult == LaunchNotificationResult::Failed)
        std::wcerr << L"Не удалось отправить уведомление в Telegram; продолжаем запуск.\n";

    if (argc > 2)
    {
        std::wcerr << L"Использование: ZA_PUTINA_Loader.exe [путь-к-ZA_PUTINA.dll]\n";
        return 1;
    }

    const DWORD processId = FindHoi4Process();
    if (!processId)
    {
        std::wcerr << L"hoi4.exe не запущен. Сначала запустите игру в режиме DirectX 11.\n";
        return 3;
    }
    if (FindRemoteModule(processId, L"ZA_PUTINA.dll"))
    {
        std::wcout << L"ZA_PUTINA.dll уже загружена в hoi4.exe.\n";
        return 0;
    }

    EmbeddedDllExtraction embeddedDll;
    std::filesystem::path requested;
    if (argc == 2)
    {
        requested = argv[1];
    }
    else if (embeddedDll.Extract())
    {
        requested = embeddedDll.path();
    }
    else
    {
        requested = DefaultDllPath();
    }

    std::error_code pathError;
    const std::filesystem::path dllPath = std::filesystem::weakly_canonical(requested, pathError);
    if (pathError || !std::filesystem::is_regular_file(dllPath))
    {
        std::wcerr << L"ZA_PUTINA.dll не найдена: " << requested << L"\n";
        return 2;
    }
    if (_wcsicmp(dllPath.filename().c_str(), L"ZA_PUTINA.dll") != 0 || !IsX64Dll(dllPath))
    {
        std::wcerr << L"Загрузка отменена: ожидалась 64-битная ZA_PUTINA.dll.\n";
        return 2;
    }

    Handle process(OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE,
        processId));
    if (!process)
    {
        std::wcerr << L"Не удалось открыть hoi4.exe: " << LastErrorMessage() << L"\nПопробуйте запустить ZA_PUTINA_Loader от имени администратора.\n";
        return 4;
    }

    HMODULE localKernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC localLoadLibrary = localKernel ? GetProcAddress(localKernel, "LoadLibraryW") : nullptr;
    HMODULE ownerModule = nullptr;
    if (!localLoadLibrary || !GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(localLoadLibrary),
            &ownerModule))
    {
        std::wcerr << L"Не удалось определить адрес LoadLibraryW.\n";
        return 5;
    }

    wchar_t ownerPath[MAX_PATH]{};
    if (!GetModuleFileNameW(ownerModule, ownerPath, MAX_PATH))
    {
        std::wcerr << L"Не удалось определить модуль, содержащий LoadLibraryW.\n";
        return 5;
    }
    const std::wstring ownerName = std::filesystem::path(ownerPath).filename().wstring();
    const uintptr_t remoteOwner = FindRemoteModule(processId, ownerName.c_str());
    if (!remoteOwner)
    {
        std::wcerr << L"Не удалось найти " << ownerName << L" в hoi4.exe.\n";
        return 5;
    }

    const uintptr_t loadLibraryOffset = reinterpret_cast<uintptr_t>(localLoadLibrary) - reinterpret_cast<uintptr_t>(ownerModule);
    const auto remoteLoadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteOwner + loadLibraryOffset);
    const std::wstring pathText = dllPath.wstring();
    const SIZE_T pathBytes = (pathText.size() + 1) * sizeof(wchar_t);

    void* remotePath = VirtualAllocEx(process.get(), nullptr, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath)
    {
        std::wcerr << L"Не удалось выделить память в hoi4.exe: " << LastErrorMessage() << L"\n";
        return 6;
    }

    int result = 0;
    bool remoteThreadMayBeRunning = false;
    if (!WriteProcessMemory(process.get(), remotePath, pathText.c_str(), pathBytes, nullptr))
    {
        std::wcerr << L"Не удалось записать путь к DLL: " << LastErrorMessage() << L"\n";
        result = 6;
    }
    else
    {
        Handle thread(CreateRemoteThread(process.get(), nullptr, 0, remoteLoadLibrary, remotePath, 0, nullptr));
        if (!thread)
        {
            std::wcerr << L"Не удалось запустить поток загрузчика: " << LastErrorMessage() << L"\n";
            result = 7;
        }
        else
        {
            remoteThreadMayBeRunning = true;
            if (WaitForSingleObject(thread.get(), 15000) != WAIT_OBJECT_0)
            {
                std::wcerr << L"Превышено время ожидания загрузки DLL.\n";
                result = 8;
            }
            else
            {
                remoteThreadMayBeRunning = false;
            }
        }
    }

    if (!remoteThreadMayBeRunning)
        VirtualFreeEx(process.get(), remotePath, 0, MEM_RELEASE);
    if (result != 0)
        return result;

    if (!FindRemoteModule(processId, L"ZA_PUTINA.dll"))
    {
        std::wcerr << L"LoadLibraryW завершилась, но DLL отсутствует в целевом процессе.\n";
        return 9;
    }

    std::wcout << L"Файл " << dllPath << L" загружен в hoi4.exe (PID " << processId << L").\n";
    std::wcout << L"Нажмите Insert в игре, чтобы открыть или закрыть меню.\n";
    return 0;
}
