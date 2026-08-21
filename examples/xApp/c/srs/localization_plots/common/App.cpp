/*

Copyright (c) 2022 Evan Pezent & ImPlot Community

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "App.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

App::App(std::string title, int w, int h)
{
  title += " - OpenGL";

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    abort();
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // Maximize window size at start
  // Create window with graphics context
  Window = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
  if (Window == NULL) {
    fprintf(stderr, "Failed to initialize GLFW window!\n");
    abort();
  }
  glfwMakeContextCurrent(Window);
  glfwSwapInterval(1); // vsync option
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(Window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  ClearColor = ImVec4(0.15f, 0.16f, 0.21f, 1.00f);
}

App::~App()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(Window);
  glfwTerminate();
}

void App::Run()
{
  Start();
  // Main loop
  while (!glfwWindowShouldClose(Window)) {
    glfwPollEvents();
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    Update();
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(Window);
  }
}

ImVec2 App::GetWindowSize() const
{
  int w, h;
  glfwGetWindowSize(Window, &w, &h);
  return ImVec2(w, h);
}
