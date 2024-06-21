# JUCE-imgui  
imgui platform backend for JUCE  
This needs to be used along with the OpenGL 3 Renderer (imgui_impl_opengl3) as JUCE uses OpenGL  
https://github.com/ocornut/imgui  
https://github.com/juce-framework/JUCE  
https://github.com/ocornut/imgui/wiki/Bindings#frameworkengine-backends  

# Usage  
Download and add imgui_impl_juce.h, imgui_impl_juce.cpp, imgui_impl_juce_config.h to your projects source code  
This needs to be used along with the OpenGL 3 Renderer (imgui_impl_opengl3) as Juce supports OpenGL  
See the provided example  

# Implemented Features  
- [x] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen.  
- [x] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.  
- [x] Platform: Override Mouse wheel scroll sensitivity.  
- [x] Platform: Keyboard support.  
- [x] Platform: Clipboard support.  
- [x] Platform: Multiple ImGuiContexts support. Use case: multiple plugin instances within a DAW.  
      (Implementations must follow thread_local guidance in imconfig.h / imgui_impl_juce_config.h)

# Unsupported Features  
- [ ] Gamepad input.
- [ ] Certain key presses: Details see ImGui_ImplJuce_KeyPress_ToImGuiKey().  

# Notable existing juce backend implementations  
https://github.com/Krasjet/imgui_juce  
## Differences:  
1. Lock-free implementation
2. No patch required to edit the imgui source  
    -> imgui now provides user config.h functionality for thread_local imguiContexts    
3. Fixes MacOS alert sound playing when keys are pressed
4. Fixes MacOS missing text input support for:  
    -> command key support  
    -> cut (Command+X)  
    -> copy (Command+C)  
    -> paste (Command+V)  
    -> undo (Command+Z)  
    -> redo (Shift+Command+Z)  
    -> move the insertion point to the beginning of the document (Command+UpArrow)  
    -> move the insertion point to the end of the document (Command+DownArrow)  
    -> move the insertion point to the beginning of the current line (Command+LeftArrow)  
    -> move the insertion point to the end of the current line (Command+RightArrow)  
    -> select all (Command+A)  
    -> select the text between the insertion point and the beginning of the document (Shift+Command+UpArrow)  
    -> select the text between the insertion point and the end of the document (Shift+Command+DownArrow)  
    -> select the text between the insertion point and the beginning of the current line (Shift+Command+LeftArrow)  
    -> select the text between the insertion point and the end of the current line (Shift+Command+RightArrow)  
    -> delete line (Command+delete/backspace)  
    -> Additional command functionality and any user functionality using the command key  
5. Fixes clipboard functions on MacOS  
    -> Which required command key support for copy and paste to and from clipboard  
6. Extra mouse wheel scroll sensitivity  
    -> Allow user to speed up or slow down the mouse wheel scroll

# Testing  
- [x] imgui master branch https://github.com/ocornut/imgui  
- [x] imgui docking branch https://github.com/ocornut/imgui/tree/docking  
- [x] JUCE version 7.0.9  
- [x] Platform: MacOS Silicon (Arm64 architecture)  
- [ ] Platform: Windows 10 (x86_64 architecture)

# Why use this JUCE imgui platform backend over existing dedicated platform backends e.g. osx/win32?  
## MacOS  
- JUCE renders openGL on it's own background render thread (not the main thread)  
  - The imgui_impl_osx.mm file has NS (appkit) function calls which are only safe to execute on the main thread
  - From testing this can lead to crashes, bugs and other undefined behaviour  
  - Related imgui issues with Mac and the main thread can be found here:  
    - "Can only use ImGui OSX backend from main thread #6527"  
    - https://github.com/ocornut/imgui/issues/6527  
    - https://github.com/ocornut/imgui/compare/master...marzent:imgui:osx-threading-issue#diff-75678f5ba3b98ce72c6b9051d86e452a653fccb131cce2ed305dda38daaee24f  
  - The proposed fix is to use dispatch_sync() to ensure NS calls execute on the main thread...  
  - BUT this can deadlock with JUCE's message thread
- When an audio plugin is destructed (e.g. plugin deleted from DAW track)...we call ImGui::DestroyContext()
  - When the plugin destruction is completed, JUCE informs the NS system to update (run) which calls onApplicationBecomeInactive():
  - Where the call to ImGui::GetIO() can fail and crash since the context has already been destroyed
## Platforms in general  
- JUCE handles platform responsibilities like mouse/keyboard input  
- So leveraging this allows for one single cross-platform backend rather than multiple platform backends
- Therefore do NOT use this with other platform backends (e.g. win32, osx)  
