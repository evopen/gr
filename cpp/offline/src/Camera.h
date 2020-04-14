#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace dhh::camera
{
    // Default camera values
    const float kYaw         = -90.0f;
    const float kPitch       = 0.0f;
    const float kSpeed       = 1.f;
    const float kSensitivity = 0.1f;
    const float kZoom        = 45.f;

    enum CameraMovement
    {
        kForward,
        kBackward,
        kLeft,
        kRight,
        kUp,
        kDown
    };

    class Camera
    {
    public:
        // Defines several possible options for camera movement. Used as abstraction to stay away from window-system
        // specific input methods


        // Camera Attributes
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 world_up;
        // Euler Angles
        float yaw;
        float pitch;
        // Camera options
        float movement_speed;
        float mouse_sensitivity;
        float zoom;

        // Constructor with vectors
        Camera(glm::vec3 position = glm::vec3(0.0f, 1.0f, 10.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
            float yaw = kYaw, float pitch = kPitch)
            : front(glm::vec3(0.0f, 0.0f, -1.0f)), movement_speed(kSpeed), mouse_sensitivity(kSensitivity), zoom(kZoom)
        {
            this->position = position;
            this->up       = up;
            this->yaw      = yaw;
            this->pitch    = pitch;
            world_up       = glm::vec3(0, 1, 0);
            UpdateCameraVectors();
        }

        // Constructor with scalar values
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
            : front(glm::vec3(0.0f, 0.0f, -1.0f)), movement_speed(kSpeed), mouse_sensitivity(kSensitivity), zoom(kZoom)
        {
            this->position = glm::vec3(posX, posY, posZ);
            this->up       = glm::vec3(upX, upY, upZ);
            this->yaw      = yaw;
            this->pitch    = pitch;
            world_up       = glm::vec3(0, 1, 0);

            UpdateCameraVectors();
        }

        // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 GetViewMatrix() { return glm::lookAt(position, position + front, up); }

        // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera
        // defined ENUM (to abstract it from windowing systems)
        void ProcessMove(CameraMovement direction, float deltaTime)
        {
            const float kVelocity = movement_speed * deltaTime;
            if (direction == kForward)
                position += front * kVelocity;
            if (direction == kBackward)
                position -= front * kVelocity;
            if (direction == kLeft)
                position -= right * kVelocity;
            if (direction == kRight)
                position += right * kVelocity;
            if (direction == kUp)
                position += world_up * kVelocity;
            if (direction == kDown)
                position -= world_up * kVelocity;
        }

        // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
        {
            xoffset *= mouse_sensitivity;
            yoffset *= mouse_sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // Make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrainPitch)
            {
                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;
            }

            // Update Front, Right and Up Vectors using the updated Euler angles
            UpdateCameraVectors();
        }

        // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void ProcessMouseScroll(float yoffset)
        {
            if (zoom >= 1.0f && zoom <= 45.0f)
                zoom -= yoffset;
            if (zoom <= 1.0f)
                zoom = 1.0f;
            if (zoom >= 45.0f)
                zoom = 45.0f;
        }


    private:
        // Calculates the front vector from the Camera's (updated) Euler Angles
        void UpdateCameraVectors()
        {
            // Calculate the new Front vector
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            this->front   = glm::normalize(front);
            // Also re-calculate the Right and Up vector
            right = glm::normalize(glm::cross(front, world_up));
            // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results
            // in slower movement.
            up = glm::normalize(glm::cross(right, front));
        }
    };

    glm::dvec3 GetTexCoord(int row, int col, int width, int height, Camera cam)
    {
        glm::dvec3 tex_coord;

        double center_x = double(width) / 2;
        double center_y = double(height) / 2;
        double bend_x   = (double(col) - center_x) / center_x * glm::radians(cam.zoom / 2);
        double bend_y   = -(double(row) - center_y) / center_y * glm::radians(cam.zoom / 2);

        double up_length    = glm::length(cam.front) * tan(bend_y);
        double right_length = glm::length(cam.front) * tan(bend_x);
        tex_coord           = cam.front + glm::normalize(cam.up) * float(up_length);
        tex_coord           = glm::vec3(tex_coord) + glm::normalize(cam.right) * float(right_length);
        

        return tex_coord;
    }
}
