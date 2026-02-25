#include "Renderer.h"
#include "TileLoader.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string tilesetPath = "3dtiles-renderer/data/tileset.json";
    if (argc > 1) {
        tilesetPath = argv[1];
    }

    Renderer renderer;
    if (!renderer.init(800, 600, "3D Tiles Renderer")) {
        return -1;
    }

    TileLoader loader;
    std::cout << "Loading tileset: " << tilesetPath << std::endl;
    auto rootTile = loader.loadTileset(tilesetPath);

    if (!rootTile) {
        std::cerr << "Failed to load tileset." << std::endl;
    } else {
        std::cout << "Tileset loaded successfully." << std::endl;
    }

    renderer.run(rootTile);

    return 0;
}
