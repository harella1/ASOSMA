#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Tile {
    glm::dmat4 transform = glm::dmat4(1.0);
    double geometricError = 0.0;
    std::string contentUri;

    // Bounding volume data could be stored here.
    // For simplicity, we just store the raw JSON for now or specific fields if needed.
    nlohmann::json boundingVolume;

    std::vector<std::unique_ptr<Tile>> children;
};

class TileLoader {
public:
    std::unique_ptr<Tile> loadTileset(const std::string& path);

private:
    std::unique_ptr<Tile> parseTile(const nlohmann::json& tileJson, const std::string& basePath);
};
