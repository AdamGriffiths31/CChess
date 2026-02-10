#ifndef CCHESS_ERROR_H
#define CCHESS_ERROR_H

#include <stdexcept>
#include <string>

namespace cchess {

class ChessError : public std::runtime_error {
public:
    explicit ChessError(const std::string& message)
        : std::runtime_error(message) {}
};

class FenParseError : public ChessError {
public:
    explicit FenParseError(const std::string& message)
        : ChessError("FEN Parse Error: " + message) {}
};

class FenValidationError : public ChessError {
public:
    explicit FenValidationError(const std::string& message)
        : ChessError("FEN Validation Error: " + message) {}
};

} // namespace cchess

#endif // CCHESS_ERROR_H
