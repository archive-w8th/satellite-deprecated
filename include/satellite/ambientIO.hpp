#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <vector>
#include <functional>
#include <iostream>
#include <string>

namespace ambientIO {

    // listeners for GLFW
    namespace Listeners {
        std::vector<std::function<void(GLFWwindow*, double, double)>> mouseMoveFunctions;
        std::vector<std::function<void(GLFWwindow*, int32_t, int32_t, int32_t)>> mouseActionFunctions;
        std::vector<std::function<void(GLFWwindow*, int32_t, int32_t, int32_t, int32_t)>> keyActionFunctions;
        std::vector<std::function<void(int32_t, const char*)>> errorFunctions;
        std::vector<std::function<void(ImDrawData* data)>> drawListFunctions;
    };

    // for GLFW usage
    void _glfwErrorHandler(int32_t error, const char* description) {
        std::cerr << "GLFW Error: \n" + std::string(description) << std::endl;
        for (auto const& fn : Listeners::errorFunctions) fn(error, description);
    }

    void _glfwKeyAction(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        for (auto const& fn : Listeners::keyActionFunctions) fn(window, key, scancode, action, mods);
    }

    void _glfwMouseAction(GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
        for (auto const& fn : Listeners::mouseActionFunctions) fn(window, button, action, mods);
    }

    void _glfwMouseMove(GLFWwindow* window, double x, double y) {
        for (auto const& fn : Listeners::mouseMoveFunctions) fn(window, x, y);
    }

    void _ImGuiDrawListHandler(ImDrawData* data) {
        for (auto const& fn : Listeners::drawListFunctions) fn(data);
    }










    // user (coder) friend functions
    void addMouseMoveCallback(std::function<void(GLFWwindow*, double, double)> cb) {
        Listeners::mouseMoveFunctions.push_back(cb);
    }

    void addMouseActionCallback(std::function<void(GLFWwindow*, int32_t, int32_t, int32_t)> cb) {
        Listeners::mouseActionFunctions.push_back(cb);
    }

    void addKeyboardCallback(std::function<void(GLFWwindow*, int32_t, int32_t, int32_t, int32_t)> cb) {
        Listeners::keyActionFunctions.push_back(cb);
    }

    void addErrorHandler(std::function<void(int32_t, const char*)> cb) {
        Listeners::errorFunctions.push_back(cb);
    }

    void addGuiDrawListCallback(std::function<void(ImDrawData*)> cb) {
        Listeners::drawListFunctions.push_back(cb);
    }

    void handleGlfw(GLFWwindow* window) {
        glfwSetErrorCallback(_glfwErrorHandler);
        glfwSetKeyCallback(window, _glfwKeyAction);
        glfwSetMouseButtonCallback(window, _glfwMouseAction);
        glfwSetCursorPosCallback(window, _glfwMouseMove);

        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                         // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
        io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
        io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
        io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
        io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
        io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
        io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
        io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
        io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

        io.RenderDrawListsFn = _ImGuiDrawListHandler;
        io.SetClipboardTextFn = [](void* user_data, const char* text) { glfwSetClipboardString((GLFWwindow*)user_data, text); };
        io.GetClipboardTextFn = [](void* user_data) { return glfwGetClipboardString((GLFWwindow*)user_data); };
        io.ClipboardUserData = window;
        io.ImeWindowHandle = glfwGetWindowUserPointer(window);

        addMouseMoveCallback([&](GLFWwindow* window, double x, double y) {
            io.MousePos = ImVec2(x / io.DisplayFramebufferScale.x, y / io.DisplayFramebufferScale.y);
        });

        addMouseActionCallback([&](GLFWwindow*, int button, int action, int /*mods*/) {
            if (button >= 0 && button < 3) io.MouseDown[button] = (action == GLFW_PRESS);
        });

        addKeyboardCallback([&](GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
            if (action == GLFW_PRESS) io.KeysDown[key] = true;
            if (action == GLFW_RELEASE) io.KeysDown[key] = false;
            io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
            io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
            io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
            io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
        });

        // TODO: 
        //  mousewheel support
        //  

    }
};