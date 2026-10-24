#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include "Camera.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"
#include "Settings.hpp"

#ifdef _WIN32
// Pede ao driver a GPU dedicada em sistemas híbridos (Optimus / PowerXpress).
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

struct App {
    Settings s;
    Camera cam;
    Renderer renderer;
    bool mouseCaptured = true;
    bool firstMouse = true;
    double lastX = 0, lastY = 0;
    bool reloadRequested = false;
    bool graphicsMenuOpen = false;
    bool mouseCapturedBeforeGraphicsMenu = true;
    int graphicsMenuSelection = 0;
};
static App* gApp = nullptr;

static void printSetting(const char* name, float v) { std::printf("[ajuste] %s = %.3f\n", name, v); }

static void setGraphicsMenuOpen(GLFWwindow* window, App& app, bool open) {
    if (open) app.mouseCapturedBeforeGraphicsMenu = app.mouseCaptured;
    app.graphicsMenuOpen = open;
    app.mouseCaptured = open ? false : app.mouseCapturedBeforeGraphicsMenu;
    if (app.mouseCaptured) app.firstMouse = true;
    glfwSetInputMode(window, GLFW_CURSOR,
        app.mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

static void adjustGraphicsSetting(App& app, int direction) {
    switch (app.graphicsMenuSelection) {
    case 0:
        app.s.renderScale = std::clamp(
            std::round((app.s.renderScale + direction * 0.05f) * 100.0f) / 100.0f,
            0.25f, 1.5f);
        break;
    case 1:
        app.s.antiAliasing = !app.s.antiAliasing;
        break;
    case 2:
        app.s.bloomEnabled = !app.s.bloomEnabled;
        break;
    case 3:
        app.s.vsync = !app.s.vsync;
        glfwSwapInterval(app.s.vsync ? 1 : 0);
        break;
    case 4:
        app.s.blackHoleWanderSpeed = std::clamp(
            std::round((app.s.blackHoleWanderSpeed + direction * 0.1f) * 10.0f) / 10.0f,
            0.0f, 3.0f);
        break;
    }
}

static void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    App& app = *gApp;
    Settings& s = app.s;
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        setGraphicsMenuOpen(w, app, !app.graphicsMenuOpen);
        return;
    }
    if (app.graphicsMenuOpen) {
        if (key == GLFW_KEY_UP) {
            app.graphicsMenuSelection = (app.graphicsMenuSelection + 4) % 5;
        } else if (key == GLFW_KEY_DOWN) {
            app.graphicsMenuSelection = (app.graphicsMenuSelection + 1) % 5;
        } else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
            if (action == GLFW_REPEAT && app.graphicsMenuSelection != 0) return;
            adjustGraphicsSetting(app, key == GLFW_KEY_RIGHT ? 1 : -1);
        }
        return;
    }
    switch (key) {
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); break;
    case GLFW_KEY_TAB:
        if (action != GLFW_PRESS) break;
        gApp->mouseCaptured = !gApp->mouseCaptured;
        glfwSetInputMode(w, GLFW_CURSOR, gApp->mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        gApp->firstMouse = true; break;
    case GLFW_KEY_1: s.exposure = std::max(0.05f, s.exposure / 1.1f); printSetting("exposicao", s.exposure); break;
    case GLFW_KEY_2: s.exposure = std::min(20.0f, s.exposure * 1.1f); printSetting("exposicao", s.exposure); break;
    case GLFW_KEY_B: if (action == GLFW_PRESS) { s.bloomEnabled = !s.bloomEnabled; std::printf("[ajuste] bloom %s\n", s.bloomEnabled ? "ligado" : "desligado"); } break;
    case GLFW_KEY_3: s.bloomIntensity = std::max(0.0f, s.bloomIntensity - 0.05f); printSetting("bloom", s.bloomIntensity); break;
    case GLFW_KEY_4: s.bloomIntensity = std::min(2.0f, s.bloomIntensity + 0.05f); printSetting("bloom", s.bloomIntensity); break;
    case GLFW_KEY_5: s.animSpeed = std::max(0.0f, s.animSpeed - 0.25f); printSetting("animacao", s.animSpeed); break;
    case GLFW_KEY_6: s.animSpeed = std::min(10.0f, s.animSpeed + 0.25f); printSetting("animacao", s.animSpeed); break;
    case GLFW_KEY_7: s.renderScale = std::max(0.25f, s.renderScale - 0.05f); printSetting("escala", s.renderScale); break;
    case GLFW_KEY_8: s.renderScale = std::min(1.5f, s.renderScale + 0.05f); printSetting("escala", s.renderScale); break;
    case GLFW_KEY_9: s.quality = std::max(0, s.quality - 1); std::printf("[ajuste] qualidade %s\n", s.preset().name); break;
    case GLFW_KEY_0: s.quality = std::min(kNumQualityPresets - 1, s.quality + 1); std::printf("[ajuste] qualidade %s\n", s.preset().name); break;
    case GLFW_KEY_Z: s.diskIntensity = std::max(0.05f, s.diskIntensity / 1.1f); printSetting("disco", s.diskIntensity); break;
    case GLFW_KEY_X: s.diskIntensity = std::min(20.0f, s.diskIntensity * 1.1f); printSetting("disco", s.diskIntensity); break;
    case GLFW_KEY_C: s.dopplerStrength = std::max(0.0f, s.dopplerStrength - 0.1f); printSetting("doppler", s.dopplerStrength); break;
    case GLFW_KEY_V: s.dopplerStrength = std::min(1.0f, s.dopplerStrength + 0.1f); printSetting("doppler", s.dopplerStrength); break;
    case GLFW_KEY_N: s.artisticMix = std::max(0.0f, s.artisticMix - 0.05f); printSetting("mistura artistica", s.artisticMix); break;
    case GLFW_KEY_M: s.artisticMix = std::min(1.0f, s.artisticMix + 0.05f); printSetting("mistura artistica", s.artisticMix); break;
    case GLFW_KEY_P: if (action == GLFW_PRESS) s.paused = !s.paused; break;
    case GLFW_KEY_F2: if (action == GLFW_PRESS) { s.vsync = !s.vsync; glfwSwapInterval(s.vsync ? 1 : 0); std::printf("[ajuste] vsync %s\n", s.vsync ? "ligado" : "desligado"); } break;
    case GLFW_KEY_F5: if (action == GLFW_PRESS) gApp->reloadRequested = true; break;
    }
}

static void cursorPositionCallback(GLFWwindow* w, double x, double y) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (!app || !app->mouseCaptured) return;

    if (app->firstMouse) {
        app->lastX = x;
        app->lastY = y;
        app->firstMouse = false;
        return;
    }

    app->cam.onMouseDelta(float(x - app->lastX), float(y - app->lastY));
    app->lastX = x;
    app->lastY = y;
}

static void scrollCallback(GLFWwindow* w, double, double yoffset) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->cam.onScroll(float(yoffset));
}

static void glfwErrorCallback(int, const char* description) {
    std::fprintf(stderr, "[glfw] %s\n", description);
}

static void updateWindowTitle(GLFWwindow* window, const App& app, int fps) {
    char title[384];
    if (app.graphicsMenuOpen) {
        const char* selected = "";
        char scaleValue[16];
        std::snprintf(scaleValue, sizeof(scaleValue), "%.2fx", app.s.renderScale);
        switch (app.graphicsMenuSelection) {
        case 0: selected = "Escala"; break;
        case 1: selected = "Anti-aliasing"; break;
        case 2: selected = "Bloom"; break;
        case 3: selected = "VSync"; break;
        case 4: selected = "Deriva do buraco negro"; break;
        }
        std::snprintf(title, sizeof(title),
            "Gargantua | %d FPS | GRAFICOS: %s | Escala %s | AA %s | Bloom %s | VSync %s | Deriva %.1fx | Setas: navegar/alterar | F3: fechar",
            fps, selected, scaleValue, app.s.antiAliasing ? "ON" : "OFF",
            app.s.bloomEnabled ? "ON" : "OFF", app.s.vsync ? "ON" : "OFF",
            app.s.blackHoleWanderSpeed);
    } else {
        std::snprintf(title, sizeof(title), "Gargantua | %d FPS | F3: graficos", fps);
    }
    glfwSetWindowTitle(window, title);
}

int main(int, char* argv[]) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "[glfw] falha ao inicializar\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Gargantua", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "[glfw] falha ao criar janela OpenGL 4.3\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::fprintf(stderr, "[glew] %s\n", glewGetErrorString(glewStatus));
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glGetError();

    App app;
    gApp = &app;
    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(app.s.vsync ? 1 : 0);

    const std::filesystem::path executablePath = std::filesystem::absolute(argv[0]);
    const std::filesystem::path shaderDir = executablePath.parent_path() / "shaders";
    if (!app.renderer.init(shaderDir.string())) {
        std::fprintf(stderr, "[renderer] nao foi possivel inicializar; verifique os shaders em %s\n",
                     shaderDir.string().c_str());
        gApp = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    double previousTime = glfwGetTime();
    double fpsWindowStart = previousTime;
    double titleUpdateTime = previousTime;
    int framesInWindow = 0;
    int displayedFps = 0;
    float diskTime = 0.0f;
    float wanderTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        const float dt = float(std::clamp(currentTime - previousTime, 0.0, 0.1));
        previousTime = currentTime;

        if (!app.s.paused) {
            diskTime += dt * app.s.animSpeed;
            wanderTime += dt;
        }
        const float wanderPhase = wanderTime * app.s.blackHoleWanderSpeed;
        const glm::vec3 blackHolePosition(
            2.2f * std::sin(wanderPhase * 0.23f),
            0.45f * std::sin(wanderPhase * 0.37f),
            1.6f * (std::cos(wanderPhase * 0.19f) - 1.0f));
        app.cam.update(window, dt, blackHolePosition);
        if (app.reloadRequested) {
            app.renderer.reloadShaders();
            app.reloadRequested = false;
        }

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        app.renderer.render(app.cam, app.s, diskTime, blackHolePosition, width, height);
        glfwSwapBuffers(window);

        framesInWindow++;
        if (currentTime - fpsWindowStart >= 0.5) {
            displayedFps = int(std::lround(framesInWindow / (currentTime - fpsWindowStart)));
            framesInWindow = 0;
            fpsWindowStart = currentTime;
        }
        if (currentTime - titleUpdateTime >= 0.25) {
            updateWindowTitle(window, app, displayedFps);
            titleUpdateTime = currentTime;
        }
    }

    app.renderer.shutdown();
    gApp = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}