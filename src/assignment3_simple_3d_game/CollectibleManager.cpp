#include "CollectibleManager.hpp"
#include "Ground.hpp"

#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

#include <glm/gtc/matrix_transform.hpp>

namespace {
glm::vec3 randomGroundPosition(const Ground& ground, std::mt19937& rng)
{
    float half = ground.getHalfSize() - 1.0f; // keep some margin from the edges
    std::uniform_real_distribution<float> dist(-half, half);
    glm::vec3 pos;
    pos.x = dist(rng);
    pos.z = dist(rng);
    pos.y = ground.getHeightAt(pos.x, pos.z);
    return pos;
}
} // namespace

CollectibleSet::CollectibleSet(Model* model, CollectibleConfig config)
    : model(model), config(config)
{
}

Collectible CollectibleSet::randomSpawn(const Ground& ground, std::mt19937& rng) const
{
    Collectible collectible;
    collectible.position = randomGroundPosition(ground, rng);
    collectible.active = true;
    return collectible;
}

void CollectibleSet::populate(const Ground& ground, std::mt19937& rng)
{
    items.clear();
    items.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i)
        items.push_back(randomSpawn(ground, rng));
}

void CollectibleSet::handleCollisions(const glm::vec3& cowPos, float cowRadius, const Ground& ground, std::mt19937& rng)
{
    for (auto& collectible : items)
    {
        float distanceXZ = glm::length(glm::vec2(cowPos.x - collectible.position.x, cowPos.z - collectible.position.z));
        if (distanceXZ < (cowRadius + config.radius))
            collectible = randomSpawn(ground, rng); // eat then immediately respawn elsewhere
    }
}

void CollectibleSet::draw(Shader& shader, const Ground& ground, float timeSeconds) const
{
    if (model == nullptr)
        return;

    for (const auto& collectible : items)
    {
        if (!collectible.active)
            continue;

        glm::mat4 modelMat = glm::mat4(1.0f);
        float collectibleY = ground.getHeightAt(collectible.position.x, collectible.position.z);
        modelMat = glm::translate(modelMat, glm::vec3(collectible.position.x, collectibleY + config.yOffset, collectible.position.z));

        if (glm::length(config.baseRotationAxis) > 0.0f && std::abs(config.baseRotationDegrees) > 0.0f)
            modelMat = glm::rotate(modelMat, glm::radians(config.baseRotationDegrees), glm::normalize(config.baseRotationAxis));

        if (glm::length(config.spinAxis) > 0.0f && std::abs(config.spinSpeedDegrees) > 0.0f)
            modelMat = glm::rotate(modelMat, glm::radians(config.spinSpeedDegrees * timeSeconds), glm::normalize(config.spinAxis));

        modelMat = glm::scale(modelMat, glm::vec3(config.scale));
        shader.setMat4("model", modelMat);
        model->Draw(shader);
    }
}
