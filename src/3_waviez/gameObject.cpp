#include "gameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/shader_t.h>

GameObject::GameObject(Shader shaderProgram)
    : position(0.0f), scale(1.0f), objColor(1.0f), transformMatrix(1.0f),
      velocity(0.0f), gravity(-9.8f), shader(shaderProgram), clampedDirection(0, 1, 0), alive(true), jumpImpulse(0.1f) {}

void GameObject::draw()
{

    if (!alive) return;

    shader.use();
    shader.setVec3("objColor", objColor);

    GLuint transformLoc = glGetUniformLocation(shader.ID, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformMatrix));
}

void GameObject::update(float deltaTime)
{
    // Apply gravity
    velocity.y += gravity * deltaTime;

    // Update position
    position += velocity * deltaTime;

    // Half size of object (since base quad is 1x1 in NDC)
    glm::vec2 halfSize = glm::vec2(scale.x * 0.5f, scale.y * 0.5f);

    // If outside the screen, destroy
    if (position.x + halfSize.x < -1.0f || position.x - halfSize.x > 1.0f ||
        position.y + halfSize.y < -1.0f || position.y - halfSize.y > 1.0f)
    {
        alive = false;
        return;
    }

    if (clampedDirection.x)
    {
        // Clamp X in range [-1, +1]
        if (position.x - halfSize.x < -1.0f)
        {
            position.x = -1.0f + halfSize.x;
            velocity.x = 0.0f;
        }
        if (position.x + halfSize.x > 1.0f)
        {
            position.x = 1.0f - halfSize.x;
            velocity.x = 0.0f;
        }
    }

    if (clampedDirection.y)
    {
        // Clamp Y in range [-1, +1]
        if (position.y - halfSize.y < -1.0f)
        {
            position.y = -1.0f + halfSize.y;
            velocity.y = 0.0f; // or *= -0.5f for bounce
        }
        if (position.y + halfSize.y > 1.0f)
        {
            position.y = 1.0f - halfSize.y;
            velocity.y = 0.0f;
        }
    }

    // Update transform
    transformMatrix = glm::mat4(1.0f);
    transformMatrix = glm::translate(transformMatrix, position);
    transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    transformMatrix = glm::scale(transformMatrix, scale);
}
