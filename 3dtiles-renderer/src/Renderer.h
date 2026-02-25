#pragma once

#include <string>
#include <map>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "TileLoader.h"

struct Renderable {
    GLuint vao = 0;
    std::vector<GLuint> vbos;
    GLuint ebo = 0;
    size_t indexCount = 0;
    GLenum mode = GL_TRIANGLES;
    GLenum indexType = GL_UNSIGNED_INT;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(int width, int height, const std::string& title);
    void run(const std::unique_ptr<Tile>& rootTile);

private:
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;

    // Map from file path to a list of renderables (one per primitive)
    std::map<std::string, std::vector<Renderable>> cache;

    void loadTileContent(const Tile* tile);
    void renderTile(const Tile* tile, const glm::mat4& parentTransform, const glm::mat4& viewProj);

    bool loadGLB(const std::string& path, std::vector<Renderable>& outRenderables);
    void createShaderProgram();
};
