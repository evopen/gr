#pragma once

#include "Camera.h"
#include <GLFW/glfw3.h>

namespace dhh::input
{
    static dhh::camera::Camera* camera;

    inline void cursorPosCallback(GLFWwindow* window, double x, double y)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float offsetX = x - (float) width / 2;
        float offsetY = (float) height / 2 - y;

        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            camera->ProcessMouseMovement(offsetX, offsetY);
            glfwSetCursorPos(window, width / 2, height / 2);
        }
    }

    inline void scrollCallback(GLFWwindow* window, double offsetX, double offsetY)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            camera->ProcessMouseScroll(offsetY);
        }
    }

    inline void processKeyboard(GLFWwindow* window, dhh::camera::Camera& camera)
    {
        static float lastTime   = glfwGetTime();
        const float currentTime = glfwGetTime();
        const float deltaTime   = currentTime - lastTime;
        lastTime                = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::FORWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::BACKWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::RIGHT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::LEFT, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::UP, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            camera.ProcessMove(dhh::camera::DOWN, deltaTime);
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
