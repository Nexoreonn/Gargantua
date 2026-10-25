#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui.h"
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
    bool cameraOrbitMode = false;
    glm::vec3 blackHolePosition{0.0f};
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

static void setupGraphicsMenuStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(20.0f, 18.0f);
    style.FramePadding = ImVec2(12.0f, 9.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.035f, 0.030f, 0.96f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.34f, 0.39f, 0.24f, 0.85f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.88f, 0.89f, 0.81f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.57f, 0.59f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.095f, 0.105f, 0.090f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.19f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.23f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.67f, 0.72f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.82f, 0.86f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.84f, 0.48f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.18f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.29f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.38f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.24f, 0.14f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.33f, 0.18f, 0.85f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.40f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.39f, 0.25f, 0.75f);
}

static void drawSettingsSection(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.75f, 0.47f, 1.0f));
    ImGui::Text("%s", title);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    const float remaining = ImGui::GetContentRegionAvail().x;
    if (remaining > 8.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetTextLineHeight() * 0.55f);
        ImGui::SetNextItemWidth(remaining);
        ImGui::Separator();
    }
}

static void drawSettingLabel(const char* label) {
    ImGui::TableNextRow(ImGuiTableRowFlags_None, 42.0f);
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1);
}

static void drawGraphicsMenu(App& app, int fps) {
    if (!app.graphicsMenuOpen) return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = std::clamp(viewport->WorkSize.x * 0.58f, 480.0f, 720.0f);
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 28.0f, viewport->WorkPos.y + 28.0f),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(std::min(width, viewport->WorkSize.x - 40.0f), 0.0f),
                             ImGuiCond_Always);
    ImGui::Begin("##GargantuaGraphicsPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.61f, 1.0f));
    ImGui::TextUnformatted("GARGANTUA");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("// VIDEO SETTINGS");
    ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);
    ImGui::Text("%d FPS", fps);
    ImGui::Separator();
    ImGui::TextDisabled("CONFIGURACAO DE IMAGEM E SIMULACAO");

    drawSettingsSection("DISPLAY");
    if (ImGui::BeginTable("DisplaySettings", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, 0.58f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.42f);
        drawSettingLabel("Escala da resolucao");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##RenderScale", &app.s.renderScale, 0.25f, 1.5f, "%.2fx");
        drawSettingLabel("Anti-aliasing (FXAA)");
        ImGui::Checkbox("Ativo##FXAA", &app.s.antiAliasing);
        drawSettingLabel("Sincronizacao vertical");
        if (ImGui::Checkbox("Ativo##VSync", &app.s.vsync))
            glfwSwapInterval(app.s.vsync ? 1 : 0);
        ImGui::EndTable();
    }

    drawSettingsSection("LIGHT & LENSING");
    if (ImGui::BeginTable("LightingSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, 0.58f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.42f);
        drawSettingLabel("Bloom");
        ImGui::Checkbox("Ativo##Bloom", &app.s.bloomEnabled);
        drawSettingLabel("Intensidade do bloom");
        ImGui::BeginDisabled(!app.s.bloomEnabled);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##BloomIntensity", &app.s.bloomIntensity, 0.0f, 2.0f, "%.2f");
        ImGui::EndDisabled();
        drawSettingLabel("Lente gravitacional");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##LensingStrength", &app.s.lensingStrength, 0.0f, 1.5f, "%.2fx");
        ImGui::EndTable();
    }

    drawSettingsSection("SIMULATION");
    if (ImGui::BeginTable("SimulationSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, 0.58f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.42f);
        drawSettingLabel("Qualidade do tracado");
        const char* qualityNames[] = { "Rapido", "Equilibrado", "Alto", "Referencia" };
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##RayQuality", &app.s.quality, qualityNames, kNumQualityPresets);
        drawSettingLabel("Velocidade do disco");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##DiskSpeed", &app.s.animSpeed, 0.0f, 10.0f, "%.2fx");
        drawSettingLabel("Deriva do buraco negro");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##WanderSpeed", &app.s.blackHoleWanderSpeed, 0.0f, 3.0f, "%.1fx");
        drawSettingLabel("Camera em orbita");
        if (ImGui::Checkbox("Ativa##OrbitCamera", &app.cameraOrbitMode))
            app.cam.setOrbitMode(app.cameraOrbitMode, app.blackHolePosition);
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextDisabled("F3 fechar  |  Mouse ou setas para ajustar  |  P pausar simulacao");
    ImGui::End();
}

static void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    App& app = *gApp;
    Settings& s = app.s;
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        setGraphicsMenuOpen(w, app, !app.graphicsMenuOpen);
        return;
    }
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS && !app.graphicsMenuOpen) {
        app.cameraOrbitMode = !app.cameraOrbitMode;
        app.cam.setOrbitMode(app.cameraOrbitMode, app.blackHolePosition);
        return;
    }
    if (app.graphicsMenuOpen) return;
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
    if (app && !app->graphicsMenuOpen) app->cam.onScroll(float(yoffset));
}

static void glfwErrorCallback(int, const char* description) {
    std::fprintf(stderr, "[glfw] %s\n", description);
}

static void updateWindowTitle(GLFWwindow* window, int fps) {
    char title[128];
    std::snprintf(title, sizeof(title), "Gargantua | %d FPS | F3: graficos", fps);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imguiIo = ImGui::GetIO();
    imguiIo.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    setupGraphicsMenuStyle();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::fprintf(stderr, "[imgui] falha ao inicializar backend GLFW\n");
        ImGui::DestroyContext();
        gApp = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 430")) {
        std::fprintf(stderr, "[imgui] falha ao inicializar backend OpenGL\n");
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        gApp = nullptr;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const std::filesystem::path executablePath = std::filesystem::absolute(argv[0]);
    const std::filesystem::path shaderDir = executablePath.parent_path() / "shaders";
    if (!app.renderer.init(shaderDir.string())) {
        std::fprintf(stderr, "[renderer] nao foi possivel inicializar; verifique os shaders em %s\n",
                     shaderDir.string().c_str());
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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
    const glm::vec3 driftDirection = glm::normalize(glm::vec3(1.0f, 0.2f, -0.4f));
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double currentTime = glfwGetTime();
        const float dt = float(std::clamp(currentTime - previousTime, 0.0, 0.1));
        previousTime = currentTime;

        if (!app.s.paused) {
            diskTime += dt * app.s.animSpeed;
            wanderTime += dt;
        }
        app.blackHolePosition =
            driftDirection * (wanderTime * 0.35f * app.s.blackHoleWanderSpeed);
        if (!app.graphicsMenuOpen)
            app.cam.update(window, dt, app.blackHolePosition, app.cameraOrbitMode);
        if (app.reloadRequested) {
            app.renderer.reloadShaders();
            app.reloadRequested = false;
        }

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        app.renderer.render(app.cam, app.s, diskTime, app.blackHolePosition, width, height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawGraphicsMenu(app, displayedFps);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        framesInWindow++;
        if (currentTime - fpsWindowStart >= 0.5) {
            displayedFps = int(std::lround(framesInWindow / (currentTime - fpsWindowStart)));
            framesInWindow = 0;
            fpsWindowStart = currentTime;
        }
        if (currentTime - titleUpdateTime >= 0.25) {
            updateWindowTitle(window, displayedFps);
            titleUpdateTime = currentTime;
        }
    }

    app.renderer.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    gApp = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}