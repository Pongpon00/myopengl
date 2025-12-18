#pragma once

#include <glm/glm.hpp>
#include <random>
#include <vector>

class Ground;
class Shader;
class Model;

struct Collectible {
    glm::vec3 position{0.0f};
    bool active = true;
};

struct CollectibleConfig {
    std::size_t count = 0;
    float radius = 0.0f;
    float scale = 1.0f;
    float yOffset = 0.0f;
    glm::vec3 baseRotationAxis{0.0f, 1.0f, 0.0f};
    float baseRotationDegrees = 0.0f;
    glm::vec3 spinAxis{0.0f, 0.0f, 0.0f};
    float spinSpeedDegrees = 0.0f;
};

class CollectibleSet {
public:
    CollectibleSet(Model* model, CollectibleConfig config);

    void populate(const Ground& ground, std::mt19937& rng);
    void handleCollisions(const glm::vec3& cowPos, float cowRadius, const Ground& ground, std::mt19937& rng);
    void draw(Shader& shader, const Ground& ground, float timeSeconds) const;

private:
    Collectible randomSpawn(const Ground& ground, std::mt19937& rng) const;

    Model* model = nullptr; // non-owning
    CollectibleConfig config{};
    std::vector<Collectible> items;
};
