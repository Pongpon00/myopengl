#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

#include "sphere.hpp"

// ---------- Utility ----------
std::string readFile(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs)
    {
        std::cerr << "❌ Failed to open: " << path << std::endl;
        exit(1);
    }
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

GLuint compileShader(GLenum type, const std::string &src)
{
    GLuint s = glCreateShader(type);
    const char *c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << std::endl;
    }
    return s;
}

GLuint makeProgram(const std::string &vsPath, const std::string &fsPath)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, readFile(vsPath));
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, readFile(fsPath));
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

struct Trail
{
    GLuint vao = 0;
    GLuint vbo = 0;
    std::vector<glm::vec4> points;
    int maxPoints = 200; // number of points kept
};

void initTrail(Trail &trail, int maxPoints = 200)
{
    trail.maxPoints = maxPoints;

    glGenVertexArrays(1, &trail.vao);
    glGenBuffers(1, &trail.vbo);

    glBindVertexArray(trail.vao);
    glBindBuffer(GL_ARRAY_BUFFER, trail.vbo);
    glBufferData(GL_ARRAY_BUFFER, maxPoints * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

    // layout (location = 0) → vec3 aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);

    // layout (location = 3) → float aAlpha
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void updateTrail(Trail &trail, glm::vec3 newPos)
{
    trail.points.push_back(glm::vec4(newPos, 1.0f)); // latest = opaque
    if ((int)trail.points.size() > trail.maxPoints)
        trail.points.erase(trail.points.begin());

    // update alpha fade (0.0–1.0)
    int n = trail.points.size();
    for (int i = 0; i < n; ++i)
    {
        float t = float(i) / float(n);
        trail.points[i].a = pow(t, 1.5f); // tail = 0, head = 1
    }
}

struct Planet
{
    std::string name;
    GLuint tex;
    float radius;
    float orbitRadius;
    float orbitSpeed;
    float selfSpeed;
    Trail trail;
};

// ---------- Globals ----------
int SCR_WIDTH = 1280, SCR_HEIGHT = 720;

// ---------- Camera ----------
glm::vec3 camPos = glm::vec3(0.0f, 15.0f, 35.0f);
glm::vec3 camFront = glm::normalize(glm::vec3(0.0f, -0.3f, -1.0f));
glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = -15.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float cameraSpeedBase = 20.0f;

void handleInput(GLFWwindow *window)
{
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    float cameraSpeed = cameraSpeedBase * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPos += cameraSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPos -= cameraSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camFront, camUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camPos += camUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camPos -= camUp * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Simulation", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
                             {
    static float sensitivity = 0.1f;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir); });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);

    GLuint program = makeProgram("shader.vs", "shader.fs");

    GLuint bgVAO, bgVBO;
    float bgVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f};
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glBindVertexArray(0);

    SphereMesh sphere = createSphere(1.0f, 40, 80);

    // ---------- Load Textures ----------
    GLuint sunTex = loadTexture("textures/sun.jpg");
    GLuint mercuryTex = loadTexture("textures/mercury.jpg");
    GLuint venusTex = loadTexture("textures/venus.jpg");
    GLuint earthTex = loadTexture("textures/earth.jpg");
    GLuint marsTex = loadTexture("textures/mars.jpg");
    GLuint jupiterTex = loadTexture("textures/jupiter.jpg");
    GLuint saturnTex = loadTexture("textures/saturn.jpg");
    GLuint uranusTex = loadTexture("textures/uranus.jpg");
    GLuint neptuneTex = loadTexture("textures/neptune.jpg");

    GLuint saturnRingTex = loadTexture("textures/saturn_ring.png");

    std::vector<Planet> planets = {
        {"Mercury", mercuryTex, 0.5f, 8.0f, 1.607f, 6.14f},
        {"Venus", venusTex, 0.7f, 11.0f, 1.174f, -1.48f},
        {"Earth", earthTex, 0.75f, 14.0f, 1.0f, 6.28f},
        {"Mars", marsTex, 0.6f, 17.0f, 0.802f, 6.28f},
        {"Jupiter", jupiterTex, 1.5f, 22.0f, 0.434f, 17.8f},
        {"Saturn", saturnTex, 1.2f, 27.0f, 0.323f, 15.0f},
        {"Uranus", uranusTex, 1.0f, 32.0f, 0.228f, -14.97f},
        {"Neptune", neptuneTex, 1.0f, 37.0f, 0.182f, 16.11f},
    };

    for (int i = 0; i < (int)planets.size(); ++i)
    {
        initTrail(planets[i].trail, 200 * (i + 1)); // longer trails for outer planets
    }

    // ---------- Uniforms ----------
    glUseProgram(program);
    GLint uModel = glGetUniformLocation(program, "uModel");
    GLint uView = glGetUniformLocation(program, "uView");
    GLint uProj = glGetUniformLocation(program, "uProj");
    GLint uLightPos = glGetUniformLocation(program, "uLightPos");
    GLint uViewPos = glGetUniformLocation(program, "uViewPos");
    GLint uRenderMode = glGetUniformLocation(program, "uRenderMode");

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), float(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 200.0f);
    glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

    double startTime = glfwGetTime();

    // ---------- Render Loop ----------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        handleInput(window);

        // --- Time update ---
        double currentTime = glfwGetTime() - startTime;

        // --- Camera & matrices ---
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        // --- Clear the screen ---
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        // Draw Background Gradient
        glDepthMask(GL_FALSE);       // don't write to depth buffer
        glUniform1i(uRenderMode, 0); // background mode
        glBindVertexArray(bgVAO);    // fullscreen quad VAO
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDepthMask(GL_TRUE); // re-enable depth writes

        // Common uniforms for 3D
        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(uLightPos, 1, glm::value_ptr(glm::vec3(0.0f)));
        glUniform3fv(uViewPos, 1, glm::value_ptr(camPos));
        glBindVertexArray(sphere.vao);
        glActiveTexture(GL_TEXTURE0);

        // Draw the Sun (emissive)
        glUniform1i(uRenderMode, 1); // emissive mode
        glBindTexture(GL_TEXTURE_2D, sunTex);
        glm::mat4 sunModel = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
        sunModel = glm::rotate(sunModel, float(currentTime * 0.2f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(sunModel));
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

        // Draw Planets
        glUniform1i(uRenderMode, 2); // normal lighting

        for (auto &p : planets)
        {
            // --- Planet position ---
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, float(currentTime * p.orbitSpeed), glm::vec3(0, 1, 0));
            model = glm::translate(model, glm::vec3(p.orbitRadius, 0, 0));
            model = glm::rotate(model, float(currentTime * p.selfSpeed), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(p.radius));

            glm::vec3 planetWorldPos = glm::vec3(model[3]); // world-space position
            updateTrail(p.trail, planetWorldPos);           // update its orbit trail

            // --- Draw the planet sphere ---
            glBindTexture(GL_TEXTURE_2D, p.tex);
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        // draw orbit trails
        glUniform1i(uRenderMode, 3);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (auto &p : planets)
        {
            if (p.trail.points.size() < 2)
                continue;

            glBindBuffer(GL_ARRAY_BUFFER, p.trail.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            p.trail.points.size() * sizeof(glm::vec4),
                            p.trail.points.data());

            glm::mat4 I = glm::mat4(1.0f);
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(I));

            GLint uColor = glGetUniformLocation(program, "uColor");
            glUniform3f(uColor, 0.6f, 0.8f, 1.0f);

            glBindVertexArray(p.trail.vao);
            glLineWidth(1.5f);
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)p.trail.points.size());
        }
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // swap buffers
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
