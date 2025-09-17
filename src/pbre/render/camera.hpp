#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "pbre/base.hpp"

namespace PBRE::Render {
// 3d perspective camera
class Camera {
  public:
    Camera();
    ~Camera();

    void setPosition(vec3 position);
    void setRotation(quat rotation);
    void setAspectRatio(float aspect);
    void setFOV(float fov);
    void setNearPlane(float nearPlane);
    void setFarPlane(float farPlane);
    void setAngle(Axis axis, float degree);
    void lookAt(const vec3& target, const vec3& up = vec3(0.0f, 1.0f, 0.0f));

    vec3 getPosition() const;
    quat getRotation() const;
    mat4 getViewMatrix() const;
    mat4 getProjectionMatrix() const;

  private:
    vec3 position_;
    quat rotation_;
    float aspect_;
    float fov_;
    float nearPlane_;
    float farPlane_;
};
} // namespace PBRE::Render
