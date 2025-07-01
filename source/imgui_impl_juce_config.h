/*
* ==============================================================================
* This file is part of the Nature Is Frequency - JUCE-imgui distribution (https://github.com/NatureIsFrequency/JUCE-imgui).
* Copyright(c) 2024 Nature Is Frequency
* ==============================================================================
*/

/**
 * Imgui Config:
 * Per Imgui recommendation, we define our own imgui_config.h
 * 
 * The following statement is copied from imgui_config.h to provide reasoning of thread_local
 * Essentially this allows for thread-safe Multiple ImGuiContexts...
 * 
 * Important: Dear ImGui functions are not thread-safe because of this pointer.
 *  If you want thread-safety to allow N threads to access N different contexts:
 *      Change this variable to use thread local storage so each thread can refer to a different context, in your imconfig.h:
 *          struct ImGuiContext;
 *          extern thread_local ImGuiContext* MyImGuiTLS;
 *          #define GImGui MyImGuiTLS
 *      And then define MyImGuiTLS in one of your cpp files. Note that thread_local is a C++11 keyword, earlier C++ uses compiler-specific keyword.
 * 
 * Below we use thread_local storage
 * And we define MyImGuiTLS in imgui_impl_juce.cpp
 * */ 

#pragma once

struct ImGuiContext;
extern thread_local ImGuiContext* MyImGuiTLS;
#define GImGui MyImGuiTLS

// Juce_ImGuiMouseCursor_Extensions: Enable with 1, Disable with 0
#define Juce_ImGuiMouseCursor_Extensions 1