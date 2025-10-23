#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_t.h>

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include "gameObject.h"

using namespace std;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// joystick state
bool buttonProcessed[14] = {false}; // To track processed buttons

// Struct to hold game state
struct GameState
{
    GameObject *player;
    vector<unique_ptr<GameObject>> *objects;
    int score = 0;
};

bool checkAABBCollision(const GameObject &a, const GameObject &b)
{
    glm::vec3 aHalf = a.scale * 0.5f;
    glm::vec3 bHalf = b.scale * 0.5f;

    bool overlapX = fabs(a.position.x - b.position.x) <= (aHalf.x + bHalf.x);
    bool overlapY = fabs(a.position.y - b.position.y) <= (aHalf.y + bHalf.y);

    return overlapX && overlapY;
}

void resetGame(GameState &state, Shader &shaderProgram)
{
    // Reset player
    state.player->position = glm::vec3(-0.5f, 0.0f, 0.0f);
    state.player->velocity = glm::vec3(0.0f);
    state.player->alive = true;

    // Clear existing objects
    state.objects->clear();

    // Reset score
    state.score = 0;
}

// ---------------- Input Handling ----------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *state = static_cast<GameState *>(glfwGetWindowUserPointer(window));
    GameObject *player = state->player;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        player->velocity.y = player->jumpImpulse;
    }
}

void processInput(GLFWwindow *window)
{
    auto *state = static_cast<GameState *>(glfwGetWindowUserPointer(window));
    GameObject *player = state->player;

    // Joystick support
    if (glfwJoystickPresent(GLFW_JOYSTICK_1))
    {
        int numAxes;
        const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &numAxes);
        if (numAxes >= 2)
        {
            float xAxis = axes[0];
            float yAxis = axes[1];

            const float deadzone = 0.1f;
            if (fabs(xAxis) < deadzone)
                xAxis = 0.0f;
            if (fabs(yAxis) < deadzone)
                yAxis = 0.0f;

            player->velocity.x = xAxis * 2.0f;
        }

        int buttonCount;
        const unsigned char *buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonCount);
        if (buttons[1] == GLFW_PRESS && !buttonProcessed[1])
        { // Button X
            player->velocity.y = player->jumpImpulse;
            buttonProcessed[1] = true;
        }
        if (buttons[1] == GLFW_RELEASE)
        {
            buttonProcessed[1] = false;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

// ---------------- Pipe Spawning ----------------
void spawnPipes(vector<unique_ptr<GameObject>> &objects, Shader &shaderProgram, float &spawnTimer, float spawnInterval)
{
    spawnTimer += deltaTime;
    if (spawnTimer < spawnInterval || objects.size() >= 20)
        return;
    spawnTimer = 0.0f;

    float gapSize = 0.75f;                           // gap height (adjust as needed)
    float gapCenterY = (rand() % 120 - 60) / 100.0f; // random between -0.6 and 0.6

    float barWidth = 0.25f;
    float barHeight = 2.0f; // tall enough to cover the screen

    float barVelocityX = -0.8f; // Move left at 0.8 units per second

    // Top bar
    objects.push_back(std::make_unique<GameObject>(shaderProgram));
    auto &topBar = *objects.back();
    topBar.position = glm::vec3(1.1f, gapCenterY + (gapSize / 2.0f) + (barHeight / 2.0f), 0.0f);
    topBar.scale = glm::vec3(barWidth, barHeight, 1.0f);
    topBar.objColor = glm::vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f);
    topBar.velocity = glm::vec3(barVelocityX, 0.0f, 0.0f);
    topBar.clampedDirection = glm::bvec3(0, 0, 0); // no clamping
    topBar.gravity = 0.0f;

    // Bottom bar
    objects.push_back(std::make_unique<GameObject>(shaderProgram));
    auto &bottomBar = *objects.back();
    bottomBar.position = glm::vec3(1.1f, gapCenterY - (gapSize / 2.0f) - (barHeight / 2.0f), 0.0f);
    bottomBar.scale = glm::vec3(barWidth, barHeight, 1.0f);
    bottomBar.objColor = glm::vec3((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f);
    bottomBar.velocity = glm::vec3(barVelocityX, 0.0f, 0.0f);
    bottomBar.clampedDirection = glm::bvec3(0, 0, 0);
    bottomBar.gravity = 0.0f;
}

// ---------------- Main ----------------
int main()
{
    // glfw: init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
    if (!window)
    {
        cerr << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // Setup VAO/VBO/EBO
    GLfloat vertices[] = {
        -0.5f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f};
    GLuint indices[] = {0, 1, 2, 1, 2, 3};

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // build and compile shader
    Shader shaderProgram("shader.vs", "shader.fs");

    // Player setup
    GameObject player(shaderProgram);
    player.position = glm::vec3(-0.5f, 0.0f, 0.0f);
    player.scale = glm::vec3(0.125f, 0.125f, 0.125f);
    player.objColor = glm::vec3(0.02f, 0.3f, 0.8f);
    player.jumpImpulse = 2.25f;

    int score = 0;
    // Game state
    vector<unique_ptr<GameObject>> objects;
    GameState state{&player, &objects, score};
    glfwSetWindowUserPointer(window, &state);

    // Pipe spawning
    float spawnTimer = 0.0f;
    float spawnInterval = 1.25f;

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // logic
        spawnPipes(objects, shaderProgram, spawnTimer, spawnInterval);
        player.rotation -= 50.0f * deltaTime;
        player.update(deltaTime);

        for (auto &obj : objects)
        {
            obj->update(deltaTime);
        }

        for (size_t i = 0; i < objects.size(); i += 2)
        {
            auto &topBar = *objects[i];
            auto &bottomBar = *objects[i + 1];
            if (checkAABBCollision(player, topBar) || checkAABBCollision(player, bottomBar))
            {
                player.alive = false;
                resetGame(state, shaderProgram);
                cout << "Game Over!" << endl;
                break;
            }

            if (!topBar.scored && player.position.x > topBar.position.x + (topBar.scale.x / 2.0f))
            {
                topBar.scored = true;
                state.score += 1;
                cout << "Score: " << state.score << endl;
                break;
            }
        }

        // render
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        player.draw();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        for (auto &obj : objects)
        {
            obj->draw();
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // cleanup old objects
        objects.erase(remove_if(objects.begin(), objects.end(),
                                [](const unique_ptr<GameObject> &obj)
                                { return !obj->alive; }),
                      objects.end());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
