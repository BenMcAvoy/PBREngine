#include "camera.hpp"

using namespace PBRE;

Render::Camera::Camera()
    : position_(0.0f, 0.0f, 5.0f),
      rotation_(1.0f, 0.0f, 0.0f, 0.0f),
      aspect_(4.0f / 3.0f),
      fov_(45.0f),
      nearPlane_(0.1f),
      farPlane_(100.0f) {}
Render::Camera::~Camera() {}

void Render::Camera::setPosition(vec3 position) {
    position_ = position;
}
void Render::Camera::setRotation(quat rotation) {
    rotation_ = rotation;
}
void Render::Camera::setAspectRatio(float aspect) {
    aspect_ = aspect;
}
void Render::Camera::setFOV(float fov) {
    fov_ = fov;
}
void Render::Camera::setNearPlane(float nearPlane) {
    nearPlane_ = nearPlane;
}
void Render::Camera::setFarPlane(float farPlane) {
    farPlane_ = farPlane;
}
void Render::Camera::setAngle(Axis axis, float degree) {
    float rad = glm::radians(degree);
    quat q;
    switch (axis) {
    case Axis::X:
        q = glm::angleAxis(rad, vec3(1.0f, 0.0f, 0.0f));
        break;
    case Axis::Y:
        q = glm::angleAxis(rad, vec3(0.0f, 1.0f, 0.0f));
        break;
    case Axis::Z:
        q = glm::angleAxis(rad, vec3(0.0f, 0.0f, 1.0f));
        break;
    }
    rotation_ = q * rotation_;
    rotation_ = glm::normalize(rotation_);
}
void Render::Camera::lookAt(const vec3& target, const vec3& up) {
    mat4 view = glm::lookAt(position_, target, up);
    rotation_ = glm::quat_cast(glm::inverse(view));
    rotation_ = glm::normalize(rotation_);
}
vec3 Render::Camera::getPosition() const {
    return position_;
}
quat Render::Camera::getRotation() const {
    return rotation_;
}
mat4 Render::Camera::getViewMatrix() const {
    mat4 translation = glm::translate(mat4(1.0f), -position_);
    mat4 rotation = glm::mat4_cast(glm::conjugate(rotation_));
    return rotation * translation;
}
mat4 Render::Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov_), aspect_, nearPlane_, farPlane_);
}
