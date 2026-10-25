#pragma once
#include <glm/glm.hpp>

struct GLFWwindow;

class Camera {
public:
    Camera();
    void update(GLFWwindow* window, float dt, const glm::vec3& blackHolePosition, bool orbitMode);
    void setOrbitMode(bool enabled, const glm::vec3& blackHolePosition);
    void onMouseDelta(float dx, float dy);
    void onScroll(float yoffset);

    glm::vec3 position;
    float yaw;    // radianos; yaw = 0 olha para -Z
    float pitch;  // radianos
    float fovDeg;
    float orbitDistance = 30.0f;

    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    float mouseSensitivity = 0.0022f;
    float baseSpeed = 6.0f; // unidades de M por segundo
};
