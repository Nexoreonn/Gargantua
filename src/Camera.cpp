#include "Camera.hpp"
#include "Physics.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

Camera::Camera()
    : position(0.0f, 1.6f, 30.0f), yaw(0.0f), pitch(0.0f), fovDeg(60.0f) {
    // Aponta para o buraco negro na origem
    pitch = -std::atan2(position.y, position.z);
}

glm::vec3 Camera::forward() const {
    return glm::normalize(glm::vec3(std::cos(pitch) * std::sin(yaw),
                                    std::sin(pitch),
                                    -std::cos(pitch) * std::cos(yaw)));
}
glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0, 1, 0)));
}
glm::vec3 Camera::up() const { return glm::cross(right(), forward()); }

void Camera::onMouseDelta(float dx, float dy) {
    yaw   += dx * mouseSensitivity;
    pitch -= dy * mouseSensitivity;
    const float lim = 1.55f;
    pitch = std::clamp(pitch, -lim, lim);
}

void Camera::onScroll(float yoffset) {
    fovDeg = std::clamp(fovDeg - yoffset * 2.5f, 10.0f, 120.0f);
}

void Camera::setOrbitMode(bool enabled, const glm::vec3& blackHolePosition) {
    if (!enabled) return;

    const glm::vec3 offset = position - blackHolePosition;
    orbitDistance = std::max(glm::length(offset), physics::kMinCameraRadius);
    const glm::vec3 lookDirection = glm::normalize(-offset);
    yaw = std::atan2(lookDirection.x, -lookDirection.z);
    pitch = std::asin(std::clamp(lookDirection.y, -1.0f, 1.0f));
}

void Camera::update(GLFWwindow* w, float dt, const glm::vec3& blackHolePosition, bool orbitMode) {
    if (orbitMode) {
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) yaw -= baseSpeed * 0.15f * dt;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) yaw += baseSpeed * 0.15f * dt;
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) orbitDistance -= baseSpeed * dt;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) orbitDistance += baseSpeed * dt;
        if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) orbitDistance -= baseSpeed * dt;
        if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) orbitDistance += baseSpeed * dt;

        orbitDistance = std::max(physics::kMinCameraRadius, orbitDistance);
        position = blackHolePosition - forward() * orbitDistance;
        return;
    }

    glm::vec3 move(0.0f);
    const glm::vec3 f = forward(), r = right(), u(0, 1, 0);
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) move += f;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) move -= f;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) move += r;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) move -= r;
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) move += u;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) move -= u;

    if (glm::dot(move, move) > 0.0f) {
        float speed = baseSpeed;
        if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 4.0f;
        // Desacelera perto do horizonte para facilitar a exploração
        const float rr = glm::length(position - blackHolePosition);
        speed *= std::clamp((rr - physics::kRs) / 10.0f, 0.15f, 3.0f);
        position += glm::normalize(move) * speed * dt;
    }

    // Mantém a câmera fora da região onde um observador estático não existe
    glm::vec3 relativePosition = position - blackHolePosition;
    const float rr = glm::length(relativePosition);
    if (rr < physics::kMinCameraRadius)
        position = blackHolePosition
                 + relativePosition * (physics::kMinCameraRadius / std::max(rr, 1e-4f));
}
