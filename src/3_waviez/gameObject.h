#pragma once
#include <glm/glm.hpp>
#include <learnopengl/shader_t.h>

class GameObject
{
public:
    Shader shader;
    glm::vec3 position, scale;
    float rotation; // in degrees
    glm::vec3 objColor;
    glm::mat4 transformMatrix;
    glm::bvec3 clampedDirection;

    float jumpImpulse; 
    bool alive;
    bool scored = false; // For scoring when passing pipes

    glm::vec3 velocity;
    float gravity;     

    GameObject(Shader shaderProgram);
    ~GameObject() = default;

    void updateTransform(glm::mat4 mat) { transformMatrix = mat; }
    void draw();

    void update(float deltaTime);
};
