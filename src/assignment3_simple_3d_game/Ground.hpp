#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader_m.h>

class Ground {
public:
    Ground(float size = 100.0f);
    ~Ground();

    void draw(Shader &shader);
    float getHeightAt(float x, float z) const { return 0.0f; } // flat plane
    float getHalfSize() const { return size * 0.5f; }

private:
    unsigned int VAO, VBO, EBO;
    float size;
};
