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

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_juce.h"

#include <juce_core/system/juce_TargetPlatform.h>

thread_local ImGuiContext* MyImGuiTLS = nullptr;

// Note: Since we dispatch x functions to the main thread (JUCE message thread) with juce::MessageManager::callAsync
// It's possible for the ImGui_Juce_Backend class to be destructed before the JUCE message thread executes the dispatched function
// Which crashes as it attempts to access ImGui_Juce_Backend class data which has already been destroyed.
// Therefore, we use a global variable which lives outside of the scope of theImGui_Juce_Backend class
// Setting to true on Construction, false on Destruction
// Allowing the dispatched functions on the main thread to check this before accessing the data
// Note that we therefore MUST always create/destroy the ImGui_Juce_Backend class on the main thread too
// We ensure this is the case with JUCE_ASSERT_MESSAGE_THREAD in our main thread functions
namespace JuceImGuiBackend
{
    bool g_juceImguiBackendActive = false;
}

//==============================================================================
static constexpr ImGuiMouseSource ImGui_ImplJuce_MouseInputSource_ToImGuiMouseSource
(
    juce::MouseInputSource::InputSourceType const i_mouseInputSourceType
)
{
    switch (i_mouseInputSourceType)
    {
        case juce::MouseInputSource::InputSourceType::mouse: return ImGuiMouseSource_Mouse;
        case juce::MouseInputSource::InputSourceType::touch: return ImGuiMouseSource_TouchScreen;
        case juce::MouseInputSource::InputSourceType::pen: return ImGuiMouseSource_Pen;
    }
}

//==============================================================================
static ImGuiMouseButton_ ImGui_ImplJuce_MouseModifierKeys_ToImGuiMouseButton
(
    juce::ModifierKeys const& i_mouseModifierKeys
)
{
    if(i_mouseModifierKeys.isLeftButtonDown())
    {
        return ImGuiMouseButton_Left;
    }

    if(i_mouseModifierKeys.isRightButtonDown())
    {
        return ImGuiMouseButton_Right;
    }

    if(i_mouseModifierKeys.isMiddleButtonDown())
    {
        return ImGuiMouseButton_Middle;
    }

    return ImGuiMouseButton_COUNT;
}

//==============================================================================
static constexpr juce::MouseCursor::StandardCursorType ImGui_ImplJuce_ImGuiMouseCursor_ToJuceStandardCursorType
(
    ImGuiMouseCursor const i_imGuiMouseCursor
)
{
    switch(i_imGuiMouseCursor)
    {
        case ImGuiMouseCursor_None: return juce::MouseCursor::StandardCursorType::NoCursor;
        case ImGuiMouseCursor_Arrow: return juce::MouseCursor::StandardCursorType::NormalCursor;
        case ImGuiMouseCursor_TextInput: return juce::MouseCursor::StandardCursorType::IBeamCursor;
        case ImGuiMouseCursor_ResizeAll: return juce::MouseCursor::StandardCursorType::UpDownLeftRightResizeCursor;
        case ImGuiMouseCursor_ResizeNS: return juce::MouseCursor::StandardCursorType::UpDownResizeCursor;
        case ImGuiMouseCursor_ResizeEW: return juce::MouseCursor::StandardCursorType::LeftRightResizeCursor;
        case ImGuiMouseCursor_ResizeNESW: return juce::MouseCursor::StandardCursorType::BottomLeftCornerResizeCursor;
        case ImGuiMouseCursor_ResizeNWSE: return juce::MouseCursor::StandardCursorType::BottomRightCornerResizeCursor;
        case ImGuiMouseCursor_Hand: return juce::MouseCursor::StandardCursorType::PointingHandCursor;

        default: return juce::MouseCursor::StandardCursorType::NormalCursor;
    }

    /**
     * Juce doesn't currently support:
     * ImGuiMouseCursor_NotAllowed
     * */ 

    /**
     * ImGui doesn't currently support:
     * juce::MouseCursor::StandardCursorType::ParentCursor
     * juce::MouseCursor::StandardCursorType::WaitCursor
     * juce::MouseCursor::StandardCursorType::CrosshairCursor
     * juce::MouseCursor::StandardCursorType::CopyingCursor
     * juce::MouseCursor::StandardCursorType::DraggingHandCursor
     * juce::MouseCursor::StandardCursorType::TopEdgeResizeCursor
     * juce::MouseCursor::StandardCursorType::BottomEdgeResizeCursor
     * juce::MouseCursor::StandardCursorType::LeftEdgeResizeCursor
     * juce::MouseCursor::StandardCursorType::RightEdgeResizeCursor
     * juce::MouseCursor::StandardCursorType::TopLeftCornerResizeCursor
     * juce::MouseCursor::StandardCursorType::TopRightCornerResizeCursor
     * */
}

//==============================================================================
static ImGuiKey ImGui_ImplJuce_KeyPress_ToImGuiKey
(
    juce::KeyPress const& i_keyPress
)
{
    // Note: Modifier keys are handled within UpdateModifierKeys()

    switch(i_keyPress.getKeyCode())
    {
        case '0': return ImGuiKey_0;
        case '1': return ImGuiKey_1;
        case '2': return ImGuiKey_2;
        case '3': return ImGuiKey_3;
        case '4': return ImGuiKey_4;
        case '5': return ImGuiKey_5;
        case '6': return ImGuiKey_6;
        case '7': return ImGuiKey_7;
        case '8': return ImGuiKey_8;
        case '9': return ImGuiKey_9;

        case 'A': return ImGuiKey_A;
        case 'B': return ImGuiKey_B;
        case 'C': return ImGuiKey_C;
        case 'D': return ImGuiKey_D;
        case 'E': return ImGuiKey_E;
        case 'F': return ImGuiKey_F;
        case 'G': return ImGuiKey_G;
        case 'H': return ImGuiKey_H;
        case 'I': return ImGuiKey_I;
        case 'J': return ImGuiKey_J;
        case 'K': return ImGuiKey_K;
        case 'L': return ImGuiKey_L;
        case 'M': return ImGuiKey_M;
        case 'N': return ImGuiKey_N;
        case 'O': return ImGuiKey_O;
        case 'P': return ImGuiKey_P;
        case 'Q': return ImGuiKey_Q;
        case 'R': return ImGuiKey_R;
        case 'S': return ImGuiKey_S;
        case 'T': return ImGuiKey_T;
        case 'U': return ImGuiKey_U;
        case 'V': return ImGuiKey_V;
        case 'W': return ImGuiKey_W;
        case 'X': return ImGuiKey_X;
        case 'Y': return ImGuiKey_Y;
        case 'Z': return ImGuiKey_Z;

        case '\'': return ImGuiKey_Apostrophe;
        case ',': return ImGuiKey_Comma;
        case '-': return ImGuiKey_Minus;
        case '.': return ImGuiKey_Period;
        case '/': return ImGuiKey_Slash;
        case ';': return ImGuiKey_Semicolon;
        case '=': return ImGuiKey_Equal;
        case '[': return ImGuiKey_LeftBracket;
        case ']': return ImGuiKey_RightBracket;
        case '\\': return ImGuiKey_Backslash;
        case '`': return ImGuiKey_GraveAccent;
    }

    // Note: Juce keypress values are not constant expressions:
    if(i_keyPress.isKeyCode(juce::KeyPress::spaceKey)) return ImGuiKey_Space;
    if(i_keyPress.isKeyCode(juce::KeyPress::escapeKey)) return ImGuiKey_Escape;
    if(i_keyPress.isKeyCode(juce::KeyPress::returnKey)) return ImGuiKey_Enter;
    if(i_keyPress.isKeyCode(juce::KeyPress::tabKey)) return ImGuiKey_Tab;
    if(i_keyPress.isKeyCode(juce::KeyPress::deleteKey)) return ImGuiKey_Delete;
    if(i_keyPress.isKeyCode(juce::KeyPress::backspaceKey)) return ImGuiKey_Backspace;
    if(i_keyPress.isKeyCode(juce::KeyPress::insertKey)) return ImGuiKey_Insert;
    if(i_keyPress.isKeyCode(juce::KeyPress::upKey)) return ImGuiKey_UpArrow;
    if(i_keyPress.isKeyCode(juce::KeyPress::downKey)) return ImGuiKey_DownArrow;
    if(i_keyPress.isKeyCode(juce::KeyPress::leftKey)) return ImGuiKey_LeftArrow;
    if(i_keyPress.isKeyCode(juce::KeyPress::rightKey)) return ImGuiKey_RightArrow;
    if(i_keyPress.isKeyCode(juce::KeyPress::pageUpKey)) return ImGuiKey_PageUp;
    if(i_keyPress.isKeyCode(juce::KeyPress::pageDownKey)) return ImGuiKey_PageDown;
    if(i_keyPress.isKeyCode(juce::KeyPress::homeKey)) return ImGuiKey_Home;
    if(i_keyPress.isKeyCode(juce::KeyPress::endKey)) return ImGuiKey_End;

    if(i_keyPress.isKeyCode(juce::KeyPress::F1Key)) return ImGuiKey_F1;
    if(i_keyPress.isKeyCode(juce::KeyPress::F2Key)) return ImGuiKey_F2;
    if(i_keyPress.isKeyCode(juce::KeyPress::F3Key)) return ImGuiKey_F3;
    if(i_keyPress.isKeyCode(juce::KeyPress::F4Key)) return ImGuiKey_F4;
    if(i_keyPress.isKeyCode(juce::KeyPress::F5Key)) return ImGuiKey_F5;
    if(i_keyPress.isKeyCode(juce::KeyPress::F6Key)) return ImGuiKey_F6;
    if(i_keyPress.isKeyCode(juce::KeyPress::F7Key)) return ImGuiKey_F7;
    if(i_keyPress.isKeyCode(juce::KeyPress::F8Key)) return ImGuiKey_F8;
    if(i_keyPress.isKeyCode(juce::KeyPress::F9Key)) return ImGuiKey_F9;
    if(i_keyPress.isKeyCode(juce::KeyPress::F10Key)) return ImGuiKey_F10;
    if(i_keyPress.isKeyCode(juce::KeyPress::F11Key)) return ImGuiKey_F11;
    if(i_keyPress.isKeyCode(juce::KeyPress::F12Key)) return ImGuiKey_F12;
    if(i_keyPress.isKeyCode(juce::KeyPress::F13Key)) return ImGuiKey_F13;
    if(i_keyPress.isKeyCode(juce::KeyPress::F14Key)) return ImGuiKey_F14;
    if(i_keyPress.isKeyCode(juce::KeyPress::F15Key)) return ImGuiKey_F15;
    if(i_keyPress.isKeyCode(juce::KeyPress::F16Key)) return ImGuiKey_F16;
    if(i_keyPress.isKeyCode(juce::KeyPress::F17Key)) return ImGuiKey_F17;
    if(i_keyPress.isKeyCode(juce::KeyPress::F18Key)) return ImGuiKey_F18;
    if(i_keyPress.isKeyCode(juce::KeyPress::F19Key)) return ImGuiKey_F19;
    if(i_keyPress.isKeyCode(juce::KeyPress::F20Key)) return ImGuiKey_F20;
    if(i_keyPress.isKeyCode(juce::KeyPress::F21Key)) return ImGuiKey_F21;
    if(i_keyPress.isKeyCode(juce::KeyPress::F22Key)) return ImGuiKey_F22;
    if(i_keyPress.isKeyCode(juce::KeyPress::F23Key)) return ImGuiKey_F23;
    if(i_keyPress.isKeyCode(juce::KeyPress::F24Key)) return ImGuiKey_F24;

    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad0)) return ImGuiKey_Keypad0;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad1)) return ImGuiKey_Keypad1;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad2)) return ImGuiKey_Keypad2;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad3)) return ImGuiKey_Keypad3;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad4)) return ImGuiKey_Keypad4;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad5)) return ImGuiKey_Keypad5;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad6)) return ImGuiKey_Keypad6;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad7)) return ImGuiKey_Keypad7;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad8)) return ImGuiKey_Keypad8;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPad9)) return ImGuiKey_Keypad9;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadAdd)) return ImGuiKey_KeypadAdd;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadSubtract)) return ImGuiKey_KeypadSubtract;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadMultiply)) return ImGuiKey_KeypadMultiply;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadDivide)) return ImGuiKey_KeypadDivide;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadDecimalPoint)) return ImGuiKey_KeypadDecimal;
    if(i_keyPress.isKeyCode(juce::KeyPress::numberPadEquals)) return ImGuiKey_KeypadEqual;

    /**
     * Juce doesn't currently support:
     * ImGuiKey_Menu
     * ImGuiKey_NumLock
     * ImGuiKey_PrintScreen
     * ImGuiKey_Pause
     * ImGuiKey_KeypadEnter
     * ImGuiKey_AppBack
     * ImGuiKey_AppForward
     * ImGuiKey_MouseX2, ImGuiKey_MouseWheelX, ImGuiKey_MouseWheelY,
     * ImGuiMod_Shortcut
     * ImGuiKey_Left..., ImGuiKey_Right...
     * ImGuiKey_Gamepad...
     *  */ 

    /**
     * ImGui doesn't currently support:
     * juce::KeyPress::F25Key -> juce::KeyPress::F35Key
     * juce::KeyPress::numberPadSeparator
     * juce::KeyPress::numberPadDelete
     * juce::KeyPress::playKey
     * juce::KeyPress::stopKey
     * juce::KeyPress::fastForwardKey
     * juce::KeyPress::rewindKey
     * */

    // Unsupported key code
    return ImGuiKey_None;
}

//==============================================================================
static void ImGui_ImplJuce_SetClipboardText
(
    [[maybe_unused]] void* i_userData
    , char const* i_text
)
{
    juce::SystemClipboard::copyTextToClipboard(juce::String(i_text));
}

//==============================================================================
static char const* ImGui_ImplJuce_GetClipboardText
(
    void* i_userData
)
{
    ImGui_Juce_Backend* const imguiJuceBackend = static_cast<ImGui_Juce_Backend*>(i_userData);
    jassert(imguiJuceBackend);

    imguiJuceBackend->SetClipboardText(juce::SystemClipboard::getTextFromClipboard());
    return imguiJuceBackend->GetClipboardText().toRawUTF8();
}

//==============================================================================
ImGui_Juce_Backend::ImGui_Juce_Backend
(
    juce::Component& i_owningComponent
    , juce::OpenGLContext& i_openGLContext
    , ImGuiContext& i_imGuiContext
    , float const i_mouseWheelSensitivity /* = 1.0f */
    , bool const i_consumeKeyPresses /* = true */
)   : m_owningComponent(i_owningComponent)
    , m_openGLContext(i_openGLContext)
    , m_imGuiContext(i_imGuiContext)
    , m_mouseWheelSensitivity(i_mouseWheelSensitivity)
    , m_consumeKeyPresses(i_consumeKeyPresses)
{
    JUCE_ASSERT_MESSAGE_THREAD

    // Adding mouse / key listeners must occur on the message thread
    m_owningComponent.addMouseListener(this, false);
    m_owningComponent.addKeyListener(this);

    ImGuiIO& io = GetContextSpecificImGuiIO();
    IMGUI_CHECKVERSION();

    io.SetClipboardTextFn = ImGui_ImplJuce_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplJuce_GetClipboardText;
    io.ClipboardUserData = this;
    io.BackendPlatformName = "imgui_impl_juce";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    JuceImGuiBackend::g_juceImguiBackendActive = true;
}

//==============================================================================
ImGui_Juce_Backend::~ImGui_Juce_Backend()
{
    JUCE_ASSERT_MESSAGE_THREAD

    JuceImGuiBackend::g_juceImguiBackendActive = false;

    // Removing mouse / key listeners must occur on the message thread
    m_owningComponent.removeMouseListener(this);
    m_owningComponent.removeKeyListener(this);

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.BackendPlatformName = nullptr;
}

//==============================================================================
void ImGui_Juce_Backend::NewFrame()
{
    // Note: Valid to / Expected to call NewFrame() from render thread

    ImGuiIO& io = GetContextSpecificImGuiIO();

    io.DisplaySize = ImVec2(static_cast<float>(m_owningComponent.getWidth()), static_cast<float>(m_owningComponent.getHeight()));

    float const renderingScale = static_cast<float>(m_openGLContext.getRenderingScale());
    io.DisplayFramebufferScale = ImVec2(renderingScale, renderingScale);

    // Ensuring thread safety by dispatching key update functions on the main thread (Juce message thread)
    // Note: UpdateModifierKeys() called first, in order to apply modifier to current key presses
    juce::MessageManager::callAsync([this](){ UpdateModifierKeys(); } );
    juce::MessageManager::callAsync([this](){ UpdateKeyPresses(); } );
    juce::MessageManager::callAsync([this](){ UpdateKeyReleases(); } );
    juce::MessageManager::callAsync([this](){ UpdateMouseCursor(); } );

    double const currentTimeSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    io.DeltaTime = static_cast<float>(currentTimeSeconds - m_currentTimeSeconds);
    m_currentTimeSeconds = currentTimeSeconds;

    // Fix for occasional crash in ImGui::NewFrame when (deltaTime == 0)
    // Using proposed fixes referenced here:
    // https://github.com/ocornut/imgui/issues/4680
    // https://github.com/ocornut/imgui/issues/4680
    if (io.DeltaTime <= 0.0f)
    {
        io.DeltaTime = 0.00001f;
    }
}

//==============================================================================
void ImGui_Juce_Backend::SetClipboardText
(
    juce::String const& i_clipboardText
)
{
    m_currentClipboardText = i_clipboardText;
}

//==============================================================================
juce::String const& ImGui_Juce_Backend::GetClipboardText() const
{
    return m_currentClipboardText;
}

//==============================================================================
void ImGui_Juce_Backend::SetMouseWheelSensitivity
(
    float const i_mouseWheelSensitivity
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    // Don't support signed values, as this inverts the mouse which conflicts with users platform setup
    jassert(i_mouseWheelSensitivity > 0.0f);
    m_mouseWheelSensitivity = fabsf(i_mouseWheelSensitivity);
}

//==============================================================================
void ImGui_Juce_Backend::mouseMove
(
    juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.AddMouseSourceEvent(ImGui_ImplJuce_MouseInputSource_ToImGuiMouseSource(i_mouseEvent.source.getType()));
    io.AddMousePosEvent(static_cast<float>(i_mouseEvent.x), static_cast<float>(i_mouseEvent.y));
}

//==============================================================================
void ImGui_Juce_Backend::mouseEnter
(
    [[maybe_unused]] juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD
    // Currently unimplemented
}

//==============================================================================
void ImGui_Juce_Backend::mouseExit
(
    [[maybe_unused]] juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD
    // Currently unimplemented
}

//==============================================================================
void ImGui_Juce_Backend::mouseDown
(
    juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.AddMouseSourceEvent(ImGui_ImplJuce_MouseInputSource_ToImGuiMouseSource(i_mouseEvent.source.getType()));
    io.AddMouseButtonEvent(ImGui_ImplJuce_MouseModifierKeys_ToImGuiMouseButton(i_mouseEvent.mods), true);
}

//==============================================================================
void ImGui_Juce_Backend::mouseDrag
(
    juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.AddMouseSourceEvent(ImGui_ImplJuce_MouseInputSource_ToImGuiMouseSource(i_mouseEvent.source.getType()));
    io.AddMousePosEvent(static_cast<float>(i_mouseEvent.x), static_cast<float>(i_mouseEvent.y));
}

//==============================================================================
void ImGui_Juce_Backend::mouseUp
(
    juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.AddMouseSourceEvent(ImGui_ImplJuce_MouseInputSource_ToImGuiMouseSource(i_mouseEvent.source.getType()));
    io.AddMouseButtonEvent(ImGui_ImplJuce_MouseModifierKeys_ToImGuiMouseButton(i_mouseEvent.mods), false);
}

//==============================================================================
void ImGui_Juce_Backend::mouseDoubleClick
(
    [[maybe_unused]] juce::MouseEvent const& i_mouseEvent
)
{
    JUCE_ASSERT_MESSAGE_THREAD
    // Currently unimplemented
}

//==============================================================================
void ImGui_Juce_Backend::mouseWheelMove
(
    [[maybe_unused]] juce::MouseEvent const& i_mouseEvent
    , juce::MouseWheelDetails const& i_mouseWheelDetails
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();
    io.AddMouseWheelEvent(i_mouseWheelDetails.deltaX * m_mouseWheelSensitivity
                        , i_mouseWheelDetails.deltaY * m_mouseWheelSensitivity);
}

//==============================================================================
void ImGui_Juce_Backend::mouseMagnify
(
    [[maybe_unused]] juce::MouseEvent const& i_mouseEvent
    , [[maybe_unused]] float const i_scaleFactor
)
{
    JUCE_ASSERT_MESSAGE_THREAD
    // Currently unimplemented
}

//==============================================================================
void ImGui_Juce_Backend::UpdateModifierKeys()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();

    int const currentModifierFlags = juce::ModifierKeys::getCurrentModifiers().getRawFlags();
    if(currentModifierFlags == m_modifierFlags)
    {
        return;
    }

    juce::ModifierKeys::Flags const currentFlags = static_cast<juce::ModifierKeys::Flags>(currentModifierFlags);
    juce::ModifierKeys::Flags const cachedFlags = static_cast<juce::ModifierKeys::Flags>(m_modifierFlags);

    auto updateModifierStateFtor = [currentFlags, cachedFlags, &io](juce::ModifierKeys::Flags const i_modifierFlag, ImGuiKey const i_imGuiKey)
    {
        bool const modifierIsDown = (currentFlags & i_modifierFlag);
        bool const modifierWasDown = (cachedFlags & i_modifierFlag);

        if(modifierIsDown != modifierWasDown)
        {
            io.AddKeyEvent(i_imGuiKey, modifierIsDown);
        }
    };

    updateModifierStateFtor(juce::ModifierKeys::Flags::shiftModifier, ImGuiMod_Shift);
    updateModifierStateFtor(juce::ModifierKeys::Flags::ctrlModifier, ImGuiMod_Ctrl);
    updateModifierStateFtor(juce::ModifierKeys::Flags::altModifier, ImGuiMod_Alt);

    // Note: Juce commandModifier differs per platform. See: juce/modules/juce_gui_basics/keyboard/juce_ModifierKeys.h
#if JUCE_MAC || JUCE_IOS
    updateModifierStateFtor(juce::ModifierKeys::Flags::commandModifier, ImGuiMod_Super);
#else
    updateModifierStateFtor(juce::ModifierKeys::Flags::commandModifier, ImGuiMod_Ctrl);
#endif

    m_modifierFlags = currentModifierFlags;
}

//==============================================================================
void ImGui_Juce_Backend::UpdateKeyPresses()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    if(m_keyPressesToProcess.empty())
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();

    for(juce::KeyPress const& keyPress : m_keyPressesToProcess)
    {
        io.AddKeyEvent(ImGui_ImplJuce_KeyPress_ToImGuiKey(keyPress), true);

        if(io.WantTextInput)
        {
            io.AddInputCharacter(static_cast<unsigned int>(keyPress.getTextCharacter())); 
        }

        /**
         * Note: keyPressed() is continuously called when a key is held down
         * If we pushed this into the m_pressedKeys array when held, we can easily exceed s_pressedKeyArraySize
         * Therefore ignore this key if already down
         * 
         */

        bool keyAlreadyDown = false;
        for(int i = 0; i < s_pressedKeyArraySize; i++)
        {
            if(m_pressedKeys[i] == keyPress)
            {
                keyAlreadyDown = true;
                break;
            }   
        }

        if(keyAlreadyDown)
        {
            continue; // ignore already down key
        }

        // Ensure there's a free slot (s_pressedKeyArraySize is set to a size where it should be extremely unlikely to reach)
        jassert(m_currentActivePressedKeys < s_pressedKeyArraySize);

        // Note: keyPressed() and UpdateKeyReleases() are both executed on the main thread (Juce message thread) so mutating m_pressedKeys is thread safe 
        for(int i = 0; i < s_pressedKeyArraySize; i++)
        {
            if(!m_pressedKeys[i].isValid())
            {
                // push this pressed key into a free slot, to manage it's release within UpdateKeyReleases()
                m_pressedKeys[i] = keyPress;
                m_currentActivePressedKeys++;

                break;
            }
        }
    }

    m_keyPressesToProcess.clear();
}

//==============================================================================
void ImGui_Juce_Backend::UpdateKeyReleases()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    if(m_currentActivePressedKeys <= 0)
    {
        return; // no pressed keys to check
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();

    /**
     * Note: As stated in keyPressed() and keyStateChanged()
     * Juce provides incorrect information / behaves incorrectly for key presses and key releases
     * Therefore we must take responsibility to determine whether keys are still pressed down or are released
     * 
     * We solve this by using the m_pressedKeys cache array
     * When a key is pressed we cache it in a free array slot [i] = _keyPress (See: UpdateKeyPresses())
     * When a key is released we free up the cached array slot [i] = KeyPress() (See: UpdateKeyReleases() below)
     * KeyPress default constructor creates an invalid KeyPress, so we free a slot with [i] = KeyPress() 
     * Conditional check [KeyPress].IsValid() is not free, vice versa is free
     * 
     * The cache array size is currently set to 256 (s_pressedKeyArraySize)
     * Which is a reasonable assumption that the user won't have 256 keys pressed simultaneously
     * 
     * Since keyPressed() and UpdateKeyReleases() are both executed on the main thread (Juce message thread)
     * Mutating m_pressedKeys is thread safe and lock-free
     * */

    for(int i = 0; i < s_pressedKeyArraySize; i++)
    {
        if(!m_pressedKeys[i].isValid())
        {
            continue; // ignore invalid keys
        }

        // push this pressed key into a free slot, to manage it's release within UpdateKeyReleases()
        juce::KeyPress const& keyPress = m_pressedKeys[i];
        jassert(keyPress.isValid());

        if(keyPress.isCurrentlyDown())
        {
            continue; // ignore pressed keys
        }

        io.AddKeyEvent(ImGui_ImplJuce_KeyPress_ToImGuiKey(keyPress), false);

        m_pressedKeys[i] = juce::KeyPress(); // reset keyPress, to free slot
        m_currentActivePressedKeys--;

        jassert(m_currentActivePressedKeys >= 0);
    }
}

//==============================================================================
void ImGui_Juce_Backend::UpdateMouseCursor()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if(!JuceImGuiBackend::g_juceImguiBackendActive)
    {
        return;
    }

    ImGuiIO& io = GetContextSpecificImGuiIO();

    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
    {
        return;
    }

    ImGuiMouseCursor const imGuiMouseCursor = ImGui::GetMouseCursor();
    
    if(imGuiMouseCursor == m_currentImGuiMouseCursor)
    {
        return;
    }

    m_currentImGuiMouseCursor = imGuiMouseCursor;

    if (io.MouseDrawCursor || imGuiMouseCursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        m_owningComponent.setMouseCursor(juce::MouseCursor::StandardCursorType::NoCursor);
        return;
    }

    m_owningComponent.setMouseCursor(ImGui_ImplJuce_ImGuiMouseCursor_ToJuceStandardCursorType(imGuiMouseCursor));
}

//==============================================================================
ImGuiIO& ImGui_Juce_Backend::GetContextSpecificImGuiIO()
{
    ImGui::SetCurrentContext(&m_imGuiContext);
    jassert(ImGui::GetCurrentContext() == &m_imGuiContext);

    return ImGui::GetIO();
}

//==============================================================================
bool ImGui_Juce_Backend::keyPressed
(
    juce::KeyPress const& i_keyPress
    , [[maybe_unused]] juce::Component* i_originatingComponent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    /**
     * Note: This function is not called for key releases. Handled within: UpdateKeyReleases())
     * Note: This function is not called for key modifiers. Handled within: UpdateModifierKeys())
     * */ 

    /**
     * Note: Key modifiers (shift/alt/ctrl/cmd etc) must be processed before key presses
     * In order to apply the modifier to the pressed key, e.g. cmd+a to select all text
     * 
     * Therefore we cache the current key presses we receive in this function
     * Process them in the UpdateKeyPresses() after UpdateModifierKeys()
     * And clear the m_keyPressesToProcess vector once we've processed the current cached key presses
     */

    m_keyPressesToProcess.push_back(i_keyPress);

    // Consume the key presses (See: juce/modules/juce_gui_basics/keyboard/juce_KeyListener.h keyPressed())
    return m_consumeKeyPresses;
}

//==============================================================================
bool ImGui_Juce_Backend::keyStateChanged
(
    [[maybe_unused]] bool const i_isKeyDown
    , [[maybe_unused]] juce::Component* i_originatingComponent
)
{
    JUCE_ASSERT_MESSAGE_THREAD

    /**
     * Note: from testing, this function keyStateChanged() provides incorrect information
     * Note: from testing, key release functionality can also behave incorrectly
     * E.g. stating the shift key is down when it's actually released and other odd behaviour
     * Juce forum posts suggest Plugin Hosts (i.e. DAWs) can "do things like steal key presses"
     * https://forum.juce.com/t/modifier-keys-in-plugin-builds-bug/49488/4
     * Additional forum post: "In a plugin, all bets are off because hosts can do silly things to the incoming events."
     * https://forum.juce.com/t/component-keystatechanged-bug/10049/3
     * 
     * Therefore to mitigate these issues:
     * key presses are handled within: keyPressed() and UpdateKeyPresses()
     * key releases are handled within: UpdateKeyReleases()
     * key modifiers are handled within: UpdateModifierKeys()
     * All called from within NewFrame() and dispatched on the main thread
     * */

    // Consume the key presses (See: juce/modules/juce_gui_basics/keyboard/juce_KeyListener.h keyPressed())
    // Additionally this prevents the MacOS alert beep playing on key presses
    return true;
}

#endif // #ifndef IMGUI_DISABLE
