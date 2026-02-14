#include "mode/OpponentList.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace cchess {

std::vector<Opponent> loadOpponents(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open opponents config: " + jsonPath);

    nlohmann::json root = nlohmann::json::parse(file);
    if (!root.is_array())
        throw std::runtime_error("opponents.json must be a JSON array");

    // Determine base directory of the JSON file for resolving relative engine paths
    std::string baseDir;
    auto lastSlash = jsonPath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        baseDir = jsonPath.substr(0, lastSlash + 1);

    std::vector<Opponent> opponents;
    for (const auto& entry : root) {
        Opponent opp;
        opp.name = entry.at("name").get<std::string>();

        std::string relPath = entry.at("engine").get<std::string>();
        opp.enginePath = baseDir + relPath;

        if (entry.contains("options")) {
            for (auto& [key, val] : entry["options"].items()) {
                opp.options[key] = val.is_string() ? val.get<std::string>() : val.dump();
            }
        }

        opponents.push_back(std::move(opp));
    }

    return opponents;
}

}  // namespace cchess
