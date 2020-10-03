// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <limits>
#include <vector>

#if defined(_WIN32)
#include <conio.h>
#include <windows.h>
#endif

// GL related.
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>

// See Embree readme.pdf.
#include <pmmintrin.h>
#include <xmmintrin.h>

/*
 * A minimal tutorial.
 *
 * It demonstrates how to intersect a ray with a single triangle. It is
 * meant to get you started as quickly as possible, and does not output
 * an image.
 *
 * For more complex examples, see the other tutorials.
 *
 * Compile this file using
 *
 *   gcc -std=c99 \
 *       -I<PATH>/<TO>/<EMBREE>/include \
 *       -o minimal \
 *       minimal.c \
 *       -L<PATH>/<TO>/<EMBREE>/lib \
 *       -lembree3
 *
 * You should be able to compile this using a C or C++ compiler.
 */

// Copied from readme.pdf.
struct Vertex {
  float x, y, z, a;
};
struct Triangle {
  int v0, v1, v2;
};

/*
 * We will register this error handler with the device in initializeDevice(),
 * so that we are automatically informed on errors.
 * This is extremely helpful for finding bugs in your code, prevents you
 * from having to add explicit error checking to each Embree API call.
 */
void errorFunction(void* userPtr, enum RTCError error, const char* str) {
  printf("error %d: %s\n", error, str);
}

/*
 * Embree has a notion of devices, which are entities that can run
 * raytracing kernels.
 * We initialize our device here, and then register the error handler so that
 * we don't miss any errors.
 *
 * rtcNewDevice() takes a configuration string as an argument. See the API docs
 * for more information.
 *
 * Note that RTCDevice is reference-counted.
 */
RTCDevice initializeDevice() {
  RTCDevice device = rtcNewDevice(NULL);

  if (!device)
    printf("error %d: cannot create device\n", rtcDeviceGetError(NULL));

  rtcDeviceSetErrorFunction2(device, errorFunction, NULL);
  return device;
}

/*
 * Create a scene, which is a collection of geometry objects. Scenes are
 * what the intersect / occluded functions work on. You can think of a
 * scene as an acceleration structure, e.g. a bounding-volume hierarchy.
 *
 * Scenes, like devices, are reference-counted.
 */
RTCScene initializeScene(RTCDevice device) {
  RTCScene scene = rtcDeviceNewScene(device, RTC_SCENE_DYNAMIC, RTC_INTERSECT1);

  /*
   * Create a triangle mesh geometry, and initialize a single triangle.
   * You can look up geometry types in the API documentation to
   * find out which type expects which buffers.
   *
   * We create buffers directly on the device, but you can also use
   * shared buffers. For shared buffers, special care must be taken
   * to ensure proper alignment and padding. This is described in
   * more detail in the API documentation.
   */
  unsigned int mesh = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, 1, 3);

  Vertex* vertices = (Vertex*)rtcMapBuffer(scene, mesh, RTC_VERTEX_BUFFER);
  vertices[0].x = 0.f;
  vertices[0].y = 0.f;
  vertices[0].z = 0.f;
  vertices[1].x = 1.f;
  vertices[1].y = 0.f;
  vertices[1].z = 0.f;
  vertices[2].x = 0.f;
  vertices[2].y = 1.f;
  vertices[2].z = 0.f;
  rtcUnmapBuffer(scene, mesh, RTC_VERTEX_BUFFER);

  Triangle* triangles = (Triangle*)rtcMapBuffer(scene, mesh, RTC_INDEX_BUFFER);
  triangles[0].v0 = 0;
  triangles[0].v1 = 1;
  triangles[0].v2 = 2;
  rtcUnmapBuffer(scene, mesh, RTC_INDEX_BUFFER);

  rtcCommit(scene);

  return scene;
}

/*
 * Cast a single ray with origin (ox, oy, oz) and direction
 * (dx, dy, dz).
 */
bool CastRay(RTCScene scene, float ox, float oy, float oz, float dx, float dy,
             float dz) {
  RTCRay ray;
  ray.org[0] = ox;
  ray.org[1] = oy;
  ray.org[2] = oz;
  ray.dir[0] = dx;
  ray.dir[1] = dy;
  ray.dir[2] = dz;
  ray.tnear = 0.0f;
  ray.tfar = std::numeric_limits<float>::infinity();
  ray.instID = RTC_INVALID_GEOMETRY_ID;
  ray.geomID = RTC_INVALID_GEOMETRY_ID;
  ray.primID = RTC_INVALID_GEOMETRY_ID;
  ray.mask = 0xFFFFFFFF;
  ray.time = 0.0f;
  rtcIntersect(scene, ray);

  return ray.geomID != RTC_INVALID_GEOMETRY_ID;

  // printf("%f, %f, %f: ", ox, oy, oz);
  // if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
  //  /* Note how geomID and primID identify the geometry we just hit.
  //   * We could use them here to interpolate geometry information,
  //   * compute shading, etc.
  //   * Since there is only a single triangle in this scene, we will
  //   * get geomID=0 / primID=0 for all hits.
  //   * There is also instID, used for instancing. See
  //   * the instancing tutorials for more information */
  //  printf("Found intersection on geometry %d, primitive %d at tfar=%f\n",
  //         ray.geomID, ray.primID, ray.tfar);
  //} else
  //  printf("Did not find any intersection.\n");
}

/* -------------------------------------------------------------------------- */

// On OSX, the actual framebuffer seems to be double this size.
constexpr int kScreenWidth = 512;
constexpr int kScreenHeight = 384;
constexpr const char* kWindowName = "Minimal Test";

void GlfwErrorFunc(int error, const char* description) {
  std::cerr << "GLFW error: " << description;
}

int main(int argc, char** argv) {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  /* Initialization. All of this may fail, but we will be notified by
   * our errorFunction. */
  RTCDevice device = initializeDevice();
  RTCScene scene = initializeScene(device);

  glfwSetErrorCallback(GlfwErrorFunc);
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow* window = glfwCreateWindow(kScreenWidth, kScreenHeight,
                                        kWindowName, nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  std::cout << "GLFW framebuffer size: " << width << ", " << height << "\n";
  glViewport(0, 0, width, height);

  std::vector<uint8_t> pixels(width * height * 4);
  int frame = 0;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    uint8_t c = static_cast<uint8_t>(++frame % 128) + 128;
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if (CastRay(scene, -0.1f + 1.2f * x / (width - 1),
                    -0.1f + 1.2f * y / (height - 1), -1.f, 0.f, 0.f, 1.f)) {
          pixels[4 * (y * width + x)] = c;
        } else {
          pixels[4 * (y * width + x)] = 0;
        }
        pixels[4 * (y * width + x) + 1] = 0;
        pixels[4 * (y * width + x) + 2] = 0;
        pixels[4 * (y * width + x) + 3] = 255;
      }
    }

    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glfwSwapBuffers(window);
  }

  /* Though not strictly necessary in this example, you should
   * always make sure to release resources allocated through Embree. */
  rtcDeleteScene(scene);
  rtcDeleteDevice(device);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
