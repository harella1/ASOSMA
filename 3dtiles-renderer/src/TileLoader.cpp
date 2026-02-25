#include "TileLoader.h"
#include <fstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

std::unique_ptr<Tile> TileLoader::loadTileset(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open tileset: " << path << std::endl;
        return nullptr;
    }

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return nullptr;
    }

    std::string basePath = std::filesystem::path(path).parent_path().string();
    if (!basePath.empty()) basePath += "/";

    if (!json.contains("root")) {
        std::cerr << "Tileset missing root" << std::endl;
        return nullptr;
    }

    return parseTile(json["root"], basePath);
}

std::unique_ptr<Tile> TileLoader::parseTile(const nlohmann::json& tileJson, const std::string& basePath) {
    auto tile = std::make_unique<Tile>();

    if (tileJson.contains("transform")) {
        auto t = tileJson["transform"].get<std::vector<double>>();
        if (t.size() == 16) {
            tile->transform = glm::make_mat4(t.data());
        }
    }

    if (tileJson.contains("geometricError")) {
        tile->geometricError = tileJson["geometricError"].get<double>();
    }

    if (tileJson.contains("content") && tileJson["content"].contains("uri")) {
        std::string uri = tileJson["content"]["uri"].get<std::string>();
        tile->contentUri = basePath + uri;
    } else if (tileJson.contains("content") && tileJson["content"].contains("url")) {
        // older spec used 'url'
        std::string url = tileJson["content"]["url"].get<std::string>();
        tile->contentUri = basePath + url;
    }

    if (tileJson.contains("boundingVolume")) {
        tile->boundingVolume = tileJson["boundingVolume"];
    }

    if (tileJson.contains("children")) {
        for (const auto& childJson : tileJson["children"]) {
            auto child = parseTile(childJson, basePath);
            if (child) {
                tile->children.push_back(std::move(child));
            }
        }
    }

    return tile;
}
