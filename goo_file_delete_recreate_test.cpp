#include "gfile.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

template<typename Char>
struct WinFile;

template<>
struct WinFile<char>
{
    static HANDLE create(const char *path, DWORD access, DWORD creation)
    {
        return CreateFileA(path, access, 0, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    static bool remove(const char *path) { return DeleteFileA(path); }
};

template<>
struct WinFile<wchar_t>
{
    static HANDLE create(const wchar_t *path, DWORD access, DWORD creation)
    {
        return CreateFileW(path, access, 0, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    static bool remove(const wchar_t *path) { return DeleteFileW(path); }
};

template<typename Char>
struct ScopedPath
{
    const std::basic_string<Char> &path;

    ~ScopedPath() { WinFile<Char>::remove(path.c_str()); }
};

template<typename Char>
bool writeByte(const std::basic_string<Char> &path, const char value)
{
    HANDLE file = WinFile<Char>::create(path.c_str(), GENERIC_WRITE, CREATE_ALWAYS);
    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << "could not write test file: " << GetLastError() << "\n";
        return false;
    }

    DWORD written = 0;
    const bool ok = WriteFile(file, &value, 1, &written, nullptr) && written == 1;
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!ok) {
        std::cerr << "could not write test byte: " << error << "\n";
    }
    return ok;
}

std::unique_ptr<GooFile> openGooFile(const std::string &path)
{
    return GooFile::open(path);
}

std::unique_ptr<GooFile> openGooFile(const std::wstring &path)
{
    return GooFile::open(path.c_str());
}

bool readsByte(const GooFile &file, const char expected)
{
    char value = 0;
    return file.read(&value, 1, 0) == 1 && value == expected;
}

std::string narrowTemporaryPath()
{
    char directory[MAX_PATH];
    char path[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, directory) || !GetTempFileNameA(directory, "gfn", 0, path)) {
        return {};
    }
    return path;
}

std::wstring wideTemporaryPath()
{
    wchar_t directory[MAX_PATH];
    wchar_t temporary[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, directory) || !GetTempFileNameW(directory, L"gfw", 0, temporary)) {
        return {};
    }

    const std::wstring path = std::wstring(temporary) + L"-\u6f22";
    if (!MoveFileW(temporary, path.c_str())) {
        DeleteFileW(temporary);
        return {};
    }
    return path;
}

template<typename Char>
bool testDeleteRecreate(const char *name, const std::basic_string<Char> &path)
{
    if (path.empty()) {
        return false;
    }
    const ScopedPath<Char> cleanup { path };
    if (!writeByte(path, 'A')) {
        return false;
    }

    const std::unique_ptr<GooFile> original = openGooFile(path);
    if (!original) {
        std::cerr << name << ": GooFile::open failed\n";
        return false;
    }

    const bool deleted = WinFile<Char>::remove(path.c_str());
    const DWORD deleteError = deleted ? ERROR_SUCCESS : GetLastError();
#if EXPECT_DELETE_SUCCESS
    if (!deleted) {
        std::cerr << name << ": DeleteFile failed: " << deleteError << "\n";
        return false;
    }

    HANDLE replacement = WinFile<Char>::create(path.c_str(), GENERIC_WRITE, CREATE_NEW);
    if (replacement == INVALID_HANDLE_VALUE) {
        std::cerr << name << ": CREATE_NEW failed: " << GetLastError() << "\n";
        return false;
    }
    const char replacementByte = 'B';
    DWORD written = 0;
    const bool wroteReplacement = WriteFile(replacement, &replacementByte, 1, &written, nullptr) && written == 1;
    const DWORD writeError = wroteReplacement ? ERROR_SUCCESS : GetLastError();
    CloseHandle(replacement);
    if (!wroteReplacement) {
        std::cerr << name << ": replacement write failed: " << writeError << "\n";
        return false;
    }

    if (!readsByte(*original, 'A')) {
        std::cerr << name << ": original GooFile did not retain A\n";
        return false;
    }
    const std::unique_ptr<GooFile> replacementFile = openGooFile(path);
    if (!replacementFile || !readsByte(*replacementFile, 'B')) {
        std::cerr << name << ": replacement GooFile did not read B\n";
        return false;
    }
#else
    if (deleted || deleteError != ERROR_SHARING_VIOLATION) {
        std::cerr << name << ": expected DeleteFile error " << ERROR_SHARING_VIOLATION << ", got " << deleteError << "\n";
        return false;
    }
    if (!readsByte(*original, 'A')) {
        std::cerr << name << ": original GooFile did not retain A after failed delete\n";
        return false;
    }
    const std::unique_ptr<GooFile> currentFile = openGooFile(path);
    if (!currentFile || !readsByte(*currentFile, 'A')) {
        std::cerr << name << ": current GooFile did not read A after failed delete\n";
        return false;
    }
#endif

    std::cout << name << ": PASS\n";
    return true;
}

}

int main()
{
    return testDeleteRecreate("narrow ASCII", narrowTemporaryPath()) && testDeleteRecreate("wide Unicode", wideTemporaryPath()) ? 0 : 1;
}
