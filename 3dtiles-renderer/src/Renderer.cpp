#include "Renderer.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Helper to get component count
int getComponentCount(int type) {
    switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        default: return 1;
    }
}

Renderer::Renderer() {}

Renderer::~Renderer() {
    for (auto& entry : cache) {
        for (auto& r : entry.second) {
            glDeleteVertexArrays(1, &r.vao);
            if (!r.vbos.empty()) glDeleteBuffers(r.vbos.size(), r.vbos.data());
            if (r.ebo > 0) glDeleteBuffers(1, &r.ebo);
        }
    }
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool Renderer::init(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Headless for test env?

    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    createShaderProgram();

    return true;
}

void Renderer::createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 u_Model;
        uniform mat4 u_ViewProj;

        out vec3 Normal;

        void main() {
            gl_Position = u_ViewProj * u_Model * vec4(aPos, 1.0);
            Normal = mat3(transpose(inverse(u_Model))) * aNormal;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 Normal;

        void main() {
            vec3 norm = normalize(Normal);
            // Simple lighting
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            float diff = max(dot(norm, lightDir), 0.2);
            FragColor = vec4(vec3(0.8) * diff, 1.0);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::run(const std::unique_ptr<Tile>& rootTile) {
    if (!rootTile) return;

    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        if (height == 0) height = 1; // Prevent div by zero
        float aspect = (float)width / (float)height;

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
        glm::mat4 viewProj = proj * view;

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_ViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

        renderTile(rootTile.get(), glm::mat4(1.0f), viewProj);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Renderer::renderTile(const Tile* tile, const glm::mat4& parentTransform, const glm::mat4& viewProj) {
    glm::mat4 currentTransform = parentTransform * glm::mat4(tile->transform);

    if (!tile->contentUri.empty()) {
        loadTileContent(tile);

        if (cache.find(tile->contentUri) != cache.end()) {
            const auto& renderables = cache[tile->contentUri];
            for (const auto& r : renderables) {
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_Model"), 1, GL_FALSE, glm::value_ptr(currentTransform));
                glBindVertexArray(r.vao);
                if (r.ebo > 0) {
                    glDrawElements(r.mode, r.indexCount, r.indexType, 0);
                } else {
                    glDrawArrays(r.mode, 0, r.indexCount);
                }
            }
        }
    }

    for (const auto& child : tile->children) {
        renderTile(child.get(), currentTransform, viewProj);
    }
}

void Renderer::loadTileContent(const Tile* tile) {
    if (cache.find(tile->contentUri) != cache.end()) return;

    std::vector<Renderable> renderables;
    if (loadGLB(tile->contentUri, renderables)) {
        cache[tile->contentUri] = renderables;
    } else {
        cache[tile->contentUri] = {};
    }
}

bool Renderer::loadGLB(const std::string& path, std::vector<Renderable>& outRenderables) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!warn.empty()) std::cout << "Warn: " << warn << std::endl;
    if (!err.empty()) std::cout << "Err: " << err << std::endl;
    if (!ret) return false;

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            Renderable r;

            glGenVertexArrays(1, &r.vao);
            glBindVertexArray(r.vao);
            r.mode = primitive.mode; // Usually GL_TRIANGLES (4)
            if (r.mode == -1) r.mode = GL_TRIANGLES;

            // Position
            if (primitive.attributes.count("POSITION")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                GLuint vbo;
                glGenBuffers(1, &vbo);
                r.vbos.push_back(vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

                int size = getComponentCount(accessor.type);
                glVertexAttribPointer(0, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, bufferView.byteStride, (void*)(uintptr_t)accessor.byteOffset);
                glEnableVertexAttribArray(0);

                if (r.indexCount == 0 && primitive.indices < 0) {
                     r.indexCount = accessor.count;
                }
            }

            // Normal
            if (primitive.attributes.count("NORMAL")) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                GLuint vbo;
                glGenBuffers(1, &vbo);
                r.vbos.push_back(vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

                int size = getComponentCount(accessor.type);
                glVertexAttribPointer(1, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, bufferView.byteStride, (void*)(uintptr_t)accessor.byteOffset);
                glEnableVertexAttribArray(1);
            }

            // Indices
            if (primitive.indices >= 0) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                glGenBuffers(1, &r.ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

                r.indexCount = accessor.count;
                r.indexType = accessor.componentType;
            }

            glBindVertexArray(0);
            outRenderables.push_back(r);
        }
    }
    return true;
}
