#include <windows.h>

#include <iostream>
#include <string>

namespace {

struct TempPaths
{
    std::wstring original;
    std::wstring replacement;

    ~TempPaths()
    {
        DeleteFileW(original.c_str());
        DeleteFileW(replacement.c_str());
    }
};

bool fail(const char *message)
{
    std::cerr << "FAIL: " << message << " (GetLastError=" << GetLastError() << ")\n";
    return false;
}

std::wstring temporaryPath()
{
    wchar_t directory[MAX_PATH];
    wchar_t fileName[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, directory) || !GetTempFileNameW(directory, L"pfs", 0, fileName)) {
        fail("could not create a temporary path");
        return {};
    }
    return fileName;
}

bool writeFile(const std::wstring &path, const char value)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return fail("could not write temporary file");
    }

    DWORD written = 0;
    const bool ok = WriteFile(file, &value, 1, &written, nullptr) && written == 1;
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!ok) {
        SetLastError(error);
        return fail("could not write temporary file contents");
    }
    return true;
}

HANDLE openReader(const std::wstring &path, const DWORD sharing)
{
    return CreateFileW(path.c_str(), GENERIC_READ, sharing, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

struct Result
{
    bool succeeded;
    DWORD error;
};

Result result(const bool succeeded, const DWORD error)
{
    return { succeeded, succeeded ? ERROR_SUCCESS : error };
}

Result testInPlaceOverwrite(const TempPaths &paths, const DWORD sharing, bool *harnessFailed)
{
    if (!writeFile(paths.original, 'a')) {
        *harnessFailed = true;
        return { false, GetLastError() };
    }

    HANDLE reader = openReader(paths.original, sharing);
    if (reader == INVALID_HANDLE_VALUE) {
        *harnessFailed = true;
        return { false, GetLastError() };
    }

    HANDLE writer = CreateFileW(paths.original.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (writer == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        CloseHandle(reader);
        return result(false, error);
    }

    DWORD written = 0;
    const char replacement = 'b';
    const bool wrote = WriteFile(writer, &replacement, 1, &written, nullptr) && written == 1;
    const DWORD error = wrote ? ERROR_SUCCESS : GetLastError();
    CloseHandle(writer);
    CloseHandle(reader);
    return result(wrote, error);
}

Result testAtomicReplacement(const TempPaths &paths, const DWORD sharing, bool *harnessFailed)
{
    if (!writeFile(paths.original, 'a') || !writeFile(paths.replacement, 'b')) {
        *harnessFailed = true;
        return { false, GetLastError() };
    }

    HANDLE reader = openReader(paths.original, sharing);
    if (reader == INVALID_HANDLE_VALUE) {
        *harnessFailed = true;
        return { false, GetLastError() };
    }

    const bool replaced = MoveFileExW(paths.replacement.c_str(), paths.original.c_str(), MOVEFILE_REPLACE_EXISTING);
    const DWORD error = GetLastError();
    CloseHandle(reader);
    return result(replaced, error);
}

struct DeleteThenRecreateResult
{
    Result deleted;
    Result recreatedWhileOpen;
    Result recreatedAfterClose;
};

DeleteThenRecreateResult testDeleteThenRecreate(const TempPaths &paths, const DWORD sharing, bool *harnessFailed)
{
    if (!writeFile(paths.original, 'a')) {
        *harnessFailed = true;
        return { { false, GetLastError() }, {}, {} };
    }

    HANDLE reader = openReader(paths.original, sharing);
    if (reader == INVALID_HANDLE_VALUE) {
        *harnessFailed = true;
        return { { false, GetLastError() }, {}, {} };
    }

    const bool deleted = DeleteFileW(paths.original.c_str());
    const DWORD deleteError = GetLastError();
    if (!deleted) {
        CloseHandle(reader);
        return { result(false, deleteError), {}, {} };
    }

    HANDLE replacement = CreateFileW(paths.original.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    const DWORD whileOpenError = GetLastError();
    const bool recreatedWhileOpen = replacement != INVALID_HANDLE_VALUE;
    if (recreatedWhileOpen) {
        CloseHandle(replacement);
        CloseHandle(reader);
        return { result(true, deleteError), result(true, whileOpenError), {} };
    }

    CloseHandle(reader);
    replacement = CreateFileW(paths.original.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    const DWORD afterCloseError = GetLastError();
    const bool recreatedAfterClose = replacement != INVALID_HANDLE_VALUE;
    if (replacement == INVALID_HANDLE_VALUE) {
        return { result(true, deleteError), result(false, whileOpenError), result(false, afterCloseError) };
    }
    CloseHandle(replacement);
    return { result(true, deleteError), result(recreatedWhileOpen, whileOpenError), result(recreatedAfterClose, afterCloseError) };
}

void printResult(const char *name, const Result result)
{
    std::cout << "  " << name << ": " << (result.succeeded ? "succeeded" : "failed") << " (error=" << result.error << ")\n";
}

bool runCase(const char *name, const DWORD sharing)
{
    TempPaths paths { temporaryPath(), temporaryPath() };
    if (paths.original.empty() || paths.replacement.empty()) {
        return false;
    }

    bool harnessFailed = false;
    const Result inPlace = testInPlaceOverwrite(paths, sharing, &harnessFailed);
    const Result atomic = testAtomicReplacement(paths, sharing, &harnessFailed);
    const DeleteThenRecreateResult deleteThenRecreate = testDeleteThenRecreate(paths, sharing, &harnessFailed);
    std::cout << name << ":\n";
    printResult("in-place overwrite", inPlace);
    printResult("atomic replacement", atomic);
    printResult("delete", deleteThenRecreate.deleted);
    if (deleteThenRecreate.deleted.succeeded) {
        printResult("recreate while old reader is open", deleteThenRecreate.recreatedWhileOpen);
        if (deleteThenRecreate.recreatedWhileOpen.succeeded) {
            std::cout << "  recreate after old reader closes: not attempted (the pathname was already recreated)\n";
        } else {
            printResult("recreate after old reader closes", deleteThenRecreate.recreatedAfterClose);
        }
    }
    return !harnessFailed;
}

}

int main()
{
    const DWORD originalSharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    const DWORD fixedSharing = originalSharing | FILE_SHARE_DELETE;
    return runCase("original sharing", originalSharing) && runCase("FILE_SHARE_DELETE", fixedSharing) ? 0 : 1;
}
