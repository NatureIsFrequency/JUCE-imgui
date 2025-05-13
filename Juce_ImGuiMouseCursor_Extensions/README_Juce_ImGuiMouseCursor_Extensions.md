# This patch extends ImGui.h ImGuiMouseCursor_ with additional Juce mouse cursors  
Note the define is within JUCE-imgui/source/imgui_impl_juce_config.h  
To disable set the define to 0

# Apply the patch to your Juce source
Copy the Juce_ImGuiMouseCursor_Extensions.patch to your ImGui source  
```
cd <imguiSource>  
git apply Juce_ImGuiMouseCursor_Extensions.patch
```
