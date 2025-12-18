#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <stb_image.h>

#include <iostream>
#include <cmath>
#include <random>
#include <vector>
#include "CollectibleManager.hpp"
#include "Ground.hpp"

struct CowController {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    float yawDegrees = 0.0f;
    float moveSpeed = 3.5f;
    float turnSpeed = 90.0f;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, CowController &cow, const Ground &ground, const std::vector<glm::vec3>& treePositions, float treeRadius);
void updateCowControls(GLFWwindow *window, CowController &cow, const Ground &ground, const std::vector<glm::vec3>& treePositions, float treeRadius);
void updateChaseCamera(const CowController &cow);
glm::mat4 buildCowModelMatrix(const CowController &cow, const Ground &ground);
glm::vec3 randomGroundPosition(const Ground& ground, std::mt19937& rng);
std::vector<glm::vec3> generateTreePositions(std::size_t count, const Ground& ground, std::mt19937& rng, float minSpacing, const glm::vec3& avoidCenter, float avoidRadius);
bool collidesWithTrees(const glm::vec3& pos, float cowRadius, const std::vector<glm::vec3>& treePositions, float treeRadius);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float cowHeightOffset = 0.375f;
float cowScale = 0.1f;
float cameraDistance = 6.0f;
float cameraHeight = 2.5f;
float cowCollisionRadius = 0.6f;
float chocolateScale = 0.001f;
float chocolateRadius = 0.09f;
float orangeScale = 0.05f;
float orangeRadius = 0.45f;
float groundSize = 50.0f;
std::size_t chocolateCount = 25;
std::size_t orangeCount = 10;
std::size_t treeCount = 25;
float treeScale = 1.5f;
float treeCollisionRadius = 0.05f;
float treeSpacing = 3.0f;

unsigned int loadTexture(const char *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        // Restore default unpack alignment for any subsequent uploads that expect it.
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    else
    {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int createSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    unsigned char data[4] = { r, g, b, a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    return textureID;
}

glm::vec3 getCowWorldPosition(const CowController &cow, const Ground &ground)
{
    float cowY = ground.getHeightAt(cow.position.x, cow.position.z) + cowHeightOffset;
    return glm::vec3(cow.position.x, cowY, cow.position.z);
}

glm::vec3 randomGroundPosition(const Ground& ground, std::mt19937& rng)
{
    float half = ground.getHalfSize() - 1.0f; // keep a small margin from the edges
    std::uniform_real_distribution<float> dist(-half, half);
    glm::vec3 pos;
    pos.x = dist(rng);
    pos.z = dist(rng);
    pos.y = ground.getHeightAt(pos.x, pos.z);
    return pos;
}

std::vector<glm::vec3> generateTreePositions(std::size_t count, const Ground& ground, std::mt19937& rng, float minSpacing, const glm::vec3& avoidCenter, float avoidRadius)
{
    std::vector<glm::vec3> positions;
    positions.reserve(count);
    const int maxAttempts = 50;

    for (std::size_t i = 0; i < count; ++i)
    {
        glm::vec3 candidate(0.0f);
        bool placed = false;
        for (int attempt = 0; attempt < maxAttempts && !placed; ++attempt)
        {
            candidate = randomGroundPosition(ground, rng);
            float distToAvoid = glm::length(glm::vec2(candidate.x - avoidCenter.x, candidate.z - avoidCenter.z));
            if (distToAvoid < avoidRadius)
                continue;

            bool overlaps = false;
            for (const auto& pos : positions)
            {
                float separation = glm::length(glm::vec2(candidate.x - pos.x, candidate.z - pos.z));
                if (separation < minSpacing)
                {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps)
            {
                positions.push_back(candidate);
                placed = true;
            }
        }

        if (!placed)
            positions.push_back(candidate); // last best effort if spacing failed repeatedly
    }

    return positions;
}

bool collidesWithTrees(const glm::vec3& pos, float cowRadius, const std::vector<glm::vec3>& treePositions, float treeRadius)
{
    float minDistance = cowRadius + treeRadius;
    for (const auto& treePos : treePositions)
    {
        float distanceXZ = glm::length(glm::vec2(pos.x - treePos.x, pos.z - treePos.z));
        if (distanceXZ < minDistance)
            return true;
    }
    return false;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
    Shader groundShader("ground.vs", "ground.fs");
    ourShader.use();
    ourShader.setFloat("uvTiling", 1.0f); // ensure model UVs sample the full texture

    // load models
    // -----------
    Model chocolateModel(FileSystem::getPath("resources/objects/chocolate/chocobar.FBX"));
    Model orangeModel(FileSystem::getPath("resources/objects/orange/orange.obj"));
    Model treeModel(FileSystem::getPath("resources/objects/tree/tree.obj"));
    Model ourModel(FileSystem::getPath("resources/objects/cow/cow.dae"));
    CowController cow;
    std::random_device rd;
    std::mt19937 rng(rd());

    // load textures
    // -------------
    unsigned int groundTexture = loadTexture(FileSystem::getPath("textures/grass/grass.jpg").c_str());
    unsigned int fallbackTexture = createSolidTexture(255, 255, 255, 255); // prevent sampling previous bindings if model has no textures
    // keep ground texture on texture unit 1 so we don't override model textures on unit 0
    groundShader.use();
    groundShader.setInt("texture_diffuse1", 1);

    Ground ground(groundSize);
    std::vector<glm::vec3> treePositions = generateTreePositions(
        treeCount,
        ground,
        rng,
        treeSpacing,
        cow.position,
        cowCollisionRadius + treeCollisionRadius + 0.5f);
    CollectibleSet chocolateSet(
        &chocolateModel,
        CollectibleConfig{
            chocolateCount,
            chocolateRadius,
            chocolateScale,
            0.55f,                 // yOffset
            glm::vec3(0.0f, 0.0f, 1.0f), 90.0f, // base rotation
            glm::vec3(1.0f, 0.0f, 0.0f), 50.0f  // spin
        });
    CollectibleSet orangeSet(
        &orangeModel,
        CollectibleConfig{
            orangeCount,
            orangeRadius,
            orangeScale,
            0.05f,                 // yOffset
            glm::vec3(1.0f, 0.0f, 0.0f), -90.0f, // base rotation (Z-up -> Y-up)
            glm::vec3(0.0f, 0.0f, 0.0f), 0.0f    // no spin
        });
    chocolateSet.populate(ground, rng);
    orangeSet.populate(ground, rng);
    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window, cow, ground, treePositions, treeCollisionRadius);
        updateChaseCamera(cow);
        glm::vec3 cowWorldPos = getCowWorldPosition(cow, ground);
        chocolateSet.handleCollisions(cowWorldPos, cowCollisionRadius, ground, rng);
        orangeSet.handleCollisions(cowWorldPos, cowCollisionRadius, ground, rng);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        groundShader.use();
        groundShader.setMat4("projection", projection);
        groundShader.setMat4("view", view);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        ground.draw(groundShader);

        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        for (const auto& treePos : treePositions)
        {
            glm::mat4 treeModelMat = glm::mat4(1.0f);
            float treeY = ground.getHeightAt(treePos.x, treePos.z);
            treeModelMat = glm::translate(treeModelMat, glm::vec3(treePos.x, treeY-0.6f, treePos.z));
            treeModelMat = glm::scale(treeModelMat, glm::vec3(treeScale));
            ourShader.setMat4("model", treeModelMat);
            treeModel.Draw(ourShader);
        }

        // render the loaded model
        glm::mat4 model = buildCowModelMatrix(cow, ground);
        ourShader.setMat4("model", model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fallbackTexture);
        ourShader.setInt("texture_diffuse1", 0);
        ourModel.Draw(ourShader);
        chocolateSet.draw(ourShader, ground, static_cast<float>(glfwGetTime()));
        orangeSet.draw(ourShader, ground, static_cast<float>(glfwGetTime()));


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, CowController &cow, const Ground &ground, const std::vector<glm::vec3>& treePositions, float treeRadius)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    updateCowControls(window, cow, ground, treePositions, treeRadius);
}

void updateCowControls(GLFWwindow *window, CowController &cow, const Ground &ground, const std::vector<glm::vec3>& treePositions, float treeRadius)
{
    glm::vec3 forwardFlat(camera.Front.x, 0.0f, camera.Front.z);
    float forwardLength = glm::length(forwardFlat);
    if (forwardLength < 0.0001f)
        forwardFlat = glm::vec3(0.0f, 0.0f, -1.0f);
    else
        forwardFlat = forwardFlat / forwardLength;

    glm::vec3 right = glm::normalize(glm::cross(forwardFlat, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 moveDir(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir += forwardFlat;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir -= forwardFlat;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDir += right;

    float moveLength = glm::length(moveDir);
    if (moveLength > 0.0f)
    {
        moveDir /= moveLength;
        glm::vec3 candidate = cow.position + moveDir * (cow.moveSpeed * deltaTime);
        float half = ground.getHalfSize() - 0.5f;
        candidate.x = glm::clamp(candidate.x, -half, half);
        candidate.z = glm::clamp(candidate.z, -half, half);

        if (!collidesWithTrees(candidate, cowCollisionRadius, treePositions, treeRadius))
            cow.position = candidate;
    }

    cow.yawDegrees = glm::degrees(std::atan2(forwardFlat.x, forwardFlat.z));
}

void updateChaseCamera(const CowController &cow)
{
    glm::vec3 direction;
    direction.x = glm::cos(glm::radians(camera.Pitch)) * glm::cos(glm::radians(camera.Yaw));
    direction.y = glm::sin(glm::radians(camera.Pitch));
    direction.z = glm::cos(glm::radians(camera.Pitch)) * glm::sin(glm::radians(camera.Yaw));

    glm::vec3 offset = glm::normalize(direction) * cameraDistance;
    glm::vec3 target = cow.position + glm::vec3(0.0f, cowHeightOffset, 0.0f);
    camera.Position = target - offset + glm::vec3(0.0f, cameraHeight, 0.0f);
    camera.Front = glm::normalize(target - camera.Position);
    camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

glm::mat4 buildCowModelMatrix(const CowController &cow, const Ground &ground)
{
    glm::mat4 model = glm::mat4(1.0f);
    float cowY = ground.getHeightAt(cow.position.x, cow.position.z) + cowHeightOffset;
    model = glm::translate(model, glm::vec3(cow.position.x, cowY, cow.position.z));
    model = glm::rotate(model, glm::radians(cow.yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // adjust model orientation
    model = glm::scale(model, glm::vec3(cowScale, cowScale, cowScale));
    return model;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
