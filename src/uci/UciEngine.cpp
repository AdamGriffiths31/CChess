#include "uci/UciEngine.h"

#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace cchess {

#ifdef _WIN32

UciEngine::UciEngine(const std::string& path) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE childStdinRd, childStdinWr;
    HANDLE childStdoutRd, childStdoutWr;

    if (!CreatePipe(&childStdinRd, &childStdinWr, &sa, 0))
        throw std::runtime_error("UciEngine: failed to create stdin pipe");
    SetHandleInformation(childStdinWr, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&childStdoutRd, &childStdoutWr, &sa, 0)) {
        CloseHandle(childStdinRd);
        CloseHandle(childStdinWr);
        throw std::runtime_error("UciEngine: failed to create stdout pipe");
    }
    SetHandleInformation(childStdoutRd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = childStdinRd;
    si.hStdOutput = childStdoutWr;
    si.hStdError = childStdoutWr;

    PROCESS_INFORMATION pi{};
    std::string cmdLine = path;

    if (!CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        CloseHandle(childStdinRd);
        CloseHandle(childStdinWr);
        CloseHandle(childStdoutRd);
        CloseHandle(childStdoutWr);
        throw std::runtime_error("UciEngine: failed to start engine: " + path);
    }

    // Close child-side handles (the child process has them)
    CloseHandle(childStdinRd);
    CloseHandle(childStdoutWr);

    processHandle_ = pi.hProcess;
    threadHandle_ = pi.hThread;

    // Wrap handles in FILE* for easy line-based I/O
    int writeFd = _open_osfhandle(reinterpret_cast<intptr_t>(childStdinWr), 0);
    int readFd = _open_osfhandle(reinterpret_cast<intptr_t>(childStdoutRd), 0);

    toEngine_ = _fdopen(writeFd, "w");
    fromEngine_ = _fdopen(readFd, "r");

    if (!toEngine_ || !fromEngine_)
        throw std::runtime_error("UciEngine: failed to open FILE streams");
}

UciEngine::~UciEngine() {
    if (toEngine_) {
        fputs("quit\n", toEngine_);
        fflush(toEngine_);
        fclose(toEngine_);
    }
    if (fromEngine_) {
        fclose(fromEngine_);
    }
    if (processHandle_) {
        WaitForSingleObject(static_cast<HANDLE>(processHandle_), 3000);
        CloseHandle(static_cast<HANDLE>(processHandle_));
    }
    if (threadHandle_) {
        CloseHandle(static_cast<HANDLE>(threadHandle_));
    }
}

#else  // POSIX

UciEngine::UciEngine(const std::string& path) {
    int toChild[2], fromChild[2];
    if (pipe(toChild) < 0 || pipe(fromChild) < 0)
        throw std::runtime_error("UciEngine: pipe failed");

    pid_ = fork();
    if (pid_ < 0)
        throw std::runtime_error("UciEngine: fork failed");

    if (pid_ == 0) {
        // Child process
        close(toChild[1]);
        close(fromChild[0]);
        dup2(toChild[0], STDIN_FILENO);
        dup2(fromChild[1], STDOUT_FILENO);
        dup2(fromChild[1], STDERR_FILENO);
        close(toChild[0]);
        close(fromChild[1]);
        execl(path.c_str(), path.c_str(), nullptr);
        _exit(1);
    }

    // Parent process
    close(toChild[0]);
    close(fromChild[1]);

    toEngine_ = fdopen(toChild[1], "w");
    fromEngine_ = fdopen(fromChild[0], "r");

    if (!toEngine_ || !fromEngine_)
        throw std::runtime_error("UciEngine: failed to open FILE streams");
}

UciEngine::~UciEngine() {
    if (toEngine_) {
        fputs("quit\n", toEngine_);
        fflush(toEngine_);
        fclose(toEngine_);
    }
    if (fromEngine_) {
        fclose(fromEngine_);
    }
    if (pid_ > 0) {
        int status;
        waitpid(pid_, &status, WNOHANG);
    }
}

#endif  // _WIN32

void UciEngine::send(const std::string& cmd) {
    if (!toEngine_)
        throw std::runtime_error("UciEngine: engine not running");
    fputs((cmd + "\n").c_str(), toEngine_);
    fflush(toEngine_);
}

std::string UciEngine::readLine() {
    if (!fromEngine_)
        throw std::runtime_error("UciEngine: engine not running");

    char buf[4096];
    if (!fgets(buf, sizeof(buf), fromEngine_))
        throw std::runtime_error("UciEngine: engine closed connection");

    std::string line(buf);
    // Strip trailing newline/carriage return
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        line.pop_back();
    return line;
}

std::string UciEngine::readUntil(const std::string& token) {
    while (true) {
        std::string line = readLine();
        if (line.substr(0, token.size()) == token)
            return line;
    }
}

void UciEngine::initUci() {
    send("uci");
    readUntil("uciok");
}

void UciEngine::setOption(const std::string& name, const std::string& value) {
    send("setoption name " + name + " value " + value);
}

void UciEngine::newGame() {
    send("ucinewgame");
    send("isready");
    readUntil("readyok");
}

std::string UciEngine::go(const std::string& params) {
    send("go " + params);
    std::string line = readUntil("bestmove");
    // "bestmove e2e4 ponder d7d5" -> extract "e2e4"
    auto pos = line.find(' ');
    if (pos == std::string::npos)
        return "";
    auto end = line.find(' ', pos + 1);
    if (end == std::string::npos)
        return line.substr(pos + 1);
    return line.substr(pos + 1, end - pos - 1);
}

}  // namespace cchess
