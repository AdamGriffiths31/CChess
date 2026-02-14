#ifndef CCHESS_UCI_ENGINE_H
#define CCHESS_UCI_ENGINE_H

#include <cstdio>
#include <map>
#include <string>

namespace cchess {

// Wraps a child process for communicating with an external UCI engine.
class UciEngine {
public:
    explicit UciEngine(const std::string& path);
    ~UciEngine();

    UciEngine(const UciEngine&) = delete;
    UciEngine& operator=(const UciEngine&) = delete;

    void send(const std::string& cmd);
    std::string readLine();
    std::string readUntil(const std::string& token);

    void initUci();
    void setOption(const std::string& name, const std::string& value);
    void newGame();
    std::string go(const std::string& params);

private:
    FILE* toEngine_ = nullptr;
    FILE* fromEngine_ = nullptr;
#ifdef _WIN32
    void* processHandle_ = nullptr;
    void* threadHandle_ = nullptr;
#else
    pid_t pid_ = -1;
#endif
};

}  // namespace cchess

#endif  // CCHESS_UCI_ENGINE_H
