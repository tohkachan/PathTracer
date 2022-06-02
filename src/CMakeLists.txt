cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

# Application source
set(APPLICATION_SOURCE

    # tracer
    tracer/TkBdptTracer.cpp
    tracer/TkLightTracer.cpp
    tracer/TkPathTracer.cpp
    tracer/TkSppmTracer.cpp
    tracer/Photonmap.cpp
    tracer/neighbor.cpp

    # core
    core/camera.cpp
    core/Material.cpp
    core/Object.cpp
    core/sampler.cpp
    core/sampling.cpp
    core/Scene.cpp
    core/TkFilm.cpp
    core/TkFilter.cpp
    core/TkLowdiscrepancy.cpp
    core/TkRayTracer.cpp
    core/TkSpectrum.cpp

    Bounds3.cpp
    BVH.cpp
    Intersection.cpp
    Matrix4.cpp
    PLY_Loader.cpp
    Quaternion.cpp
    rply.cpp
    Sphere.cpp
    sphere_drawing.cpp
    Threads.cpp
    Triangle.cpp
    Light.cpp
    Vector.cpp
    TkLoader.cpp
    TkMath.cpp
    TkRenderer.cpp
    TkViewer.cpp
    tinyxml.cpp
    tinyxmlerror.cpp
    tinyxmlparser.cpp
    main.cpp
)

#-------------------------------------------------------------------------------
# Set include directories
#-------------------------------------------------------------------------------
include_directories(
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/core
)

#-------------------------------------------------------------------------------
# Set link directories
#-------------------------------------------------------------------------------
link_directories(
  ${GLEW_LIBRARY_DIRS}
  ${GLFW_LIBRARY_DIRS}
)

#-------------------------------------------------------------------------------
# Add executable
#-------------------------------------------------------------------------------
add_executable(raytracer ${APPLICATION_SOURCE})

target_link_libraries( raytracer
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${CMAKE_THREADS_INIT}
)

# Put executable in build directory root
set(EXECUTABLE_OUTPUT_PATH ${RayTracer_SOURCE_DIR}/bin)

# Install to project root
install(TARGETS raytracer DESTINATION ${RayTracer_SOURCE_DIR})