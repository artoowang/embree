// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <stdio.h>
#include <math.h>
#include <limits>
#include <stdio.h>

#if defined(_WIN32)
#  include <conio.h>
#  include <windows.h>
#endif

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

// TODO: Should we do the following from the readme.pdf?
//#include <xmmintrin.h>
//#include <pmmintrin.h>
//...
//_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
//_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

/* 
 * This is only required to make the tutorial compile even when
 * a custom namespace is set.
 */
//#if defined(RTC_NAMESPACE_OPEN)
//RTC_NAMESPACE_OPEN
//#endif

// Copied from readme.pdf.
struct Vertex { float x, y, z, a; };
struct Triangle { int v0, v1, v2; };

/*
 * We will register this error handler with the device in initializeDevice(),
 * so that we are automatically informed on errors.
 * This is extremely helpful for finding bugs in your code, prevents you
 * from having to add explicit error checking to each Embree API call.
 */
void errorFunction(void* userPtr, enum RTCError error, const char* str)
{
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
RTCDevice initializeDevice()
{
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
RTCScene initializeScene(RTCDevice device)
{
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

  Vertex* vertices = (Vertex*) rtcMapBuffer(scene, mesh, RTC_VERTEX_BUFFER);
  vertices[0].x = 0.f; vertices[0].y = 0.f; vertices[0].z = 0.f;
  vertices[1].x = 1.f; vertices[1].y = 0.f; vertices[1].z = 0.f;
  vertices[2].x = 0.f; vertices[2].y = 1.f; vertices[2].z = 0.f;
  rtcUnmapBuffer(scene, mesh, RTC_VERTEX_BUFFER);

  Triangle* triangles = (Triangle*) rtcMapBuffer(scene, mesh, RTC_INDEX_BUFFER);
  triangles[0].v0 = 0; triangles[0].v1 = 1; triangles[0].v2 = 2;
  rtcUnmapBuffer(scene, mesh, RTC_INDEX_BUFFER);

  rtcCommit(scene);

  return scene;
}

/*
 * Cast a single ray with origin (ox, oy, oz) and direction
 * (dx, dy, dz).
 */
void castRay(RTCScene scene, 
             float ox, float oy, float oz,
             float dx, float dy, float dz)
{
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

  printf("%f, %f, %f: ", ox, oy, oz);
  if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
  {
    /* Note how geomID and primID identify the geometry we just hit.
     * We could use them here to interpolate geometry information,
     * compute shading, etc.
     * Since there is only a single triangle in this scene, we will
     * get geomID=0 / primID=0 for all hits.
     * There is also instID, used for instancing. See
     * the instancing tutorials for more information */
    printf("Found intersection on geometry %d, primitive %d at tfar=%f\n", 
           ray.geomID,
           ray.primID,
           ray.tfar);
  }
  else
    printf("Did not find any intersection.\n");

  ///*
  // * The intersect context can be used to set intersection
  // * filters or flags, and it also contains the instance ID stack
  // * used in multi-level instancing.
  // */
  //struct RTCIntersectContext context;
  //rtcInitIntersectContext(&context);

  ///*
  // * The ray hit structure holds both the ray and the hit.
  // * The user must initialize it properly -- see API documentation
  // * for rtcIntersect1() for details.
  // */
  //struct RTCRayHit rayhit;
  //rayhit.ray.org_x = ox;
  //rayhit.ray.org_y = oy;
  //rayhit.ray.org_z = oz;
  //rayhit.ray.dir_x = dx;
  //rayhit.ray.dir_y = dy;
  //rayhit.ray.dir_z = dz;
  //rayhit.ray.tnear = 0;
  //rayhit.ray.tfar = std::numeric_limits<float>::infinity();
  //rayhit.ray.mask = -1;
  //rayhit.ray.flags = 0;
  //rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  //rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

  ///*
  // * There are multiple variants of rtcIntersect. This one
  // * intersects a single ray with the scene.
  // */
  //rtcIntersect1(scene, &context, &rayhit);

  //printf("%f, %f, %f: ", ox, oy, oz);
  //if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
  //{
  //  /* Note how geomID and primID identify the geometry we just hit.
  //   * We could use them here to interpolate geometry information,
  //   * compute shading, etc.
  //   * Since there is only a single triangle in this scene, we will
  //   * get geomID=0 / primID=0 for all hits.
  //   * There is also instID, used for instancing. See
  //   * the instancing tutorials for more information */
  //  printf("Found intersection on geometry %d, primitive %d at tfar=%f\n", 
  //         rayhit.hit.geomID,
  //         rayhit.hit.primID,
  //         rayhit.ray.tfar);
  //}
  //else
  //  printf("Did not find any intersection.\n");
}

void waitForKeyPressedUnderWindows()
{
#if defined(_WIN32)
  HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(hStdOutput, &csbi)) {
    printf("GetConsoleScreenBufferInfo failed: %d\n", GetLastError());
    return;
  }
  
  /* do not pause when running on a shell */
  if (csbi.dwCursorPosition.X != 0 || csbi.dwCursorPosition.Y != 0)
    return;
  
  /* only pause if running in separate console window. */
  printf("\n\tPress any key to exit...\n");
  int ch = getch();
#endif
}


/* -------------------------------------------------------------------------- */

int main()
{
  /* Initialization. All of this may fail, but we will be notified by
   * our errorFunction. */
  RTCDevice device = initializeDevice();
  RTCScene scene = initializeScene(device);

  /* This will hit the triangle at t=1. */
  castRay(scene, 0, 0, -1, 0, 0, 1);

  /* This will not hit anything. */
  castRay(scene, 1, 1, -1, 0, 0, 1);

  /* Though not strictly necessary in this example, you should
   * always make sure to release resources allocated through Embree. */
  rtcDeleteScene(scene);
  rtcDeleteDevice(device);
  
  /* wait for user input under Windows when opened in separate window */
  waitForKeyPressedUnderWindows();
  
  return 0;
}
