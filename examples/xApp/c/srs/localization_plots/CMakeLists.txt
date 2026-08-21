include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

###################################################################
# Dependencies
###################################################################
# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 REQUIRED)

# GLAD
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

add_subdirectory(third_party)
###############################################################################
# APP FRAMEWORK
###############################################################################

add_library(app
  common/App.h
  common/App.cpp
)
target_include_directories(app PUBLIC common third_party/glad/include)
target_link_libraries(app ImPlot::ImPlot glad)

###################################################################
# DEMO APP
###################################################################
# main ImPlot demo
#add_executable(demo "demo/demo.cpp")

#target_link_libraries(demo PRIVATE app)

######################################################################
# INTERFACE library for the localization GUI
######################################################################
add_executable(srs_gui inc/cc_gui_app.h src/cc_gui_app.cpp "${GENERATED_HEADER}")
target_include_directories(srs_gui PRIVATE inc "${GENERATED_DIR}")

target_link_libraries(srs_gui PRIVATE app cppzmq flatbuffers)
