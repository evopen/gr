#pragma once

#include "Camera.h"
#include <GLFW/glfw3.h>

namespace dhh::input
{
    static dhh::camera::Camera* camera;

    inline void CursorPosCallback(GLFWwindow* window, double x, double y)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float offset_x = x - (float) width / 2;
        float offset_y = (float) height / 2 - y;

        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            camera->ProcessMouseMovement(offset_x, offset_y);
            glfwSetCursorPos(window, width / 2, height / 2);
        }
    }

    inline void ScrollCallback(GLFWwindow* window, double offsetX, double offsetY)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            camera->ProcessMouseScroll(offsetY);
        }
    }

    inline void ProcessKeyboard(GLFWwindow* window, dhh::camera::Camera& camera)
    {
        static float last_time   = glfwGetTime();
        const float kCurrentTime = glfwGetTime();
        const float kDeltaTime   = kCurrentTime - last_time;
        last_time                = kCurrentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kForward, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kBackward, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kRight, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kLeft, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kUp, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::kDown, kDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS)
        {
            glfwMaximizeWindow(window);
        }
        if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS)
        {
            glfwRestoreWindow(window);
        }
    }
}
