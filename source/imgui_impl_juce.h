/*
* ==============================================================================
* This file is part of the Nature Is Frequency - JUCE-imgui distribution (https://github.com/NatureIsFrequency/JUCE-imgui).
* Copyright(c) 2024 Nature Is Frequency
*
* This program is free software: you can redistribute it and/or modify  
* it under the terms of the GNU General Public License as published by  
* the Free Software Foundation, version 3.
*
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License 
* along with this program. If not, see <http://www.gnu.org/licenses/>.
* ==============================================================================
*/

// dear imgui: Platform Backend for Juce: https://github.com/juce-framework/JUCE
// This needs to be used along with the OpenGL 3 Renderer (imgui_impl_opengl3) as Juce uses OpenGL
// E.g ImGui_ImplOpenGL3_Init(juce::OpenGLHelpers::getGLSLVersionString().toUTF8())
// Note: Juce handles platform responsibilities. So do NOT use with other platform backends (e.g. win32, osx)
// TODO: Example Juce project using this backend: <TODO ADD GIT LINK HERE>

// Implemented features:
// [x] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.  
// [x] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.  
// [x] Platform: Override Mouse wheel scroll sensitivity.  
// [x] Platform: Keyboard support.  
// [x] Platform: Clipboard support.  
// [x] Platform: Multiple ImGuiContexts support. Use case: multiple plugin instances within a DAW.  
//     (Implementations must follow thread_local guidance in imconfig.h / imgui_impl_juce_config.h)

// Unsupported features:
// [ ] Gamepad input.
// [ ] Certain key presses: Details see ImGui_ImplJuce_KeyPress_ToImGuiKey().

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

//==============================================================================
class ImGui_Juce_Backend    : public juce::MouseListener
                            , public juce::KeyListener
{
public:
    ImGui_Juce_Backend(juce::Component& i_owningComponent
                    , juce::OpenGLContext& i_openGLContext
                    , ImGuiContext& i_imGuiContext
                    , float i_mouseWheelSensitivity = 1.0f
                    , bool i_consumeKeyPresses = true);
    ~ImGui_Juce_Backend() override;
    ImGui_Juce_Backend(ImGui_Juce_Backend const&) = delete;

    ImGui_Juce_Backend& operator=(ImGui_Juce_Backend const&) = delete;
    bool operator==(ImGui_Juce_Backend const&) = delete;
    bool operator!=(ImGui_Juce_Backend const&) = delete;

    //==============================================================================
    // Decrease (slow down) the mouse wheel sensitivity with 0.0 - 1.0
    // Increase (Speed up) the mouse wheel sensitivity with > 1.0
    void SetMouseWheelSensitivity(float i_mouseWheelSensitivity);

    //==============================================================================
    void NewFrame();

    //==============================================================================
    // Used internally via ImGui_ImplJuce_SetClipboardText() and ImGui_ImplJuce_GetClipboardText()
    // (Marked public for callback user data access)
    void SetClipboardText(juce::String const& i_clipboardText);
    juce::String const& GetClipboardText() const;

private:
    //==============================================================================
    // juce::MouseListener overrides: juce/modules/juce_gui_basics/mouse/juce_MouseListener.h
    void mouseMove(juce::MouseEvent const& i_mouseEvent) override;
    void mouseEnter(juce::MouseEvent const& i_mouseEvent) override;
    void mouseExit(juce::MouseEvent const& i_mouseEvent) override;
    void mouseDown(juce::MouseEvent const& i_mouseEvent) override;
    void mouseDrag(juce::MouseEvent const& i_mouseEvent) override;
    void mouseUp(juce::MouseEvent const& i_mouseEvent) override;
    void mouseDoubleClick(juce::MouseEvent const& i_mouseEvent) override;
    void mouseWheelMove(juce::MouseEvent const& i_mouseEvent
                        , juce::MouseWheelDetails const& i_mouseWheelDetails) override;
    void mouseMagnify(juce::MouseEvent const& i_mouseEvent
                        , float i_scaleFactor) override;

    //==============================================================================
    // juce::KeyListener overrides: juce/modules/juce_gui_basics/keyboard/juce_KeyListener.h
    bool keyPressed(juce::KeyPress const& i_keyPress
                        , juce::Component* i_originatingComponent) override;
    bool keyStateChanged(bool i_isKeyDown
                        , juce::Component* i_originatingComponent) override;

    //==============================================================================
    void UpdateModifierKeys();
    void UpdateKeyPresses();
    void UpdateKeyReleases();
    void UpdateMouseCursor();

    //==============================================================================
    // Returns the ImGuiContext associated to this backend instance (Supporting multiple ImGuiContexts)
    ImGuiIO& GetContextSpecificImGuiIO();

    //==============================================================================
    // Constructor initialisation order:
    juce::Component& m_owningComponent;
    juce::OpenGLContext& m_openGLContext;
    ImGuiContext& m_imGuiContext;
    float m_mouseWheelSensitivity;
    bool m_consumeKeyPresses;

    //==============================================================================
    static constexpr int s_pressedKeyArraySize = 256;
    int m_currentActivePressedKeys = 0;
    juce::KeyPress m_pressedKeys[s_pressedKeyArraySize];
    std::vector<juce::KeyPress> m_keyPressesToProcess;

    juce::String m_currentClipboardText;
    double m_currentTimeSeconds = 0.0;
    int m_modifierFlags = 0;
    ImGuiMouseCursor m_currentImGuiMouseCursor = ImGuiMouseCursor_Arrow;
};

#endif // #ifndef IMGUI_DISABLE
