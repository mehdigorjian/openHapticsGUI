/*****************************************************************************

Copyright (c) 2004 SensAble Technologies, Inc. All rights reserved.

OpenHaptics(TM) toolkit. The material embodied in this software and use of
this software is subject to the terms and conditions of the clickthrough
Development License Agreement.

For questions, comments or bug reports, go to forums at:
    http://dsc.sensable.com

Module Name:

  HelloSphere.cpp

Description:

  This example demonstrates basic haptic rendering of a shape.

******************************************************************************/

#include <GLFW/glfw3.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#if defined(WIN32)
#include <windows.h>
#endif
// imgui
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"

#define GL_SILENCE_DEPRECATION
#ifdef _MSC_VER
#pragma warning(disable : 4505)  // unreferenced local function has been removed
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <HDU/hduError.h>
#include <HDU/hduMatrix.h>
#include <HL/hl.h>
#include <HLU/hlu.h>

// imgui variables
static ImVec4 background_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static float b = 0.5f;
static float c = 0.6f;
static float cHeight = 0.6f;

static int selectedPrimitiveItem = 0;

/* Haptic device and rendering context handles. */
static HHD ghHD = HD_INVALID_HANDLE;
static HHLRC ghHLRC = 0;

/* Shape id for shape we will render haptically. */
HLuint gShapeId;

#define CURSOR_SIZE_PIXELS 20
static double gCursorScale;
static GLuint gCursorDisplayList = 0;

/* Function prototypes. */
void drawMenu();
void glutDisplay(void);
void glutReshape(int width, int height);
void glutIdle(void);
void glutMenu(int);

void exitHandler(void);

void initGL();
void initHL();
void initScene();
void drawSceneHaptics();
void drawSceneGraphics();
void drawCursor();
void updateWorkspace();
void drawObject();

/*******************************************************************************
 Initializes GLUT for displaying a simple haptic scene.
*******************************************************************************/
int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
#ifdef __FREEGLUT_EXT_H__
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(500, 500);
    glutCreateWindow("Mehdi's App");

    // Provide a cleanup routine for handling application exit.
    atexit(exitHandler);

    initScene();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs();

    glutDisplayFunc(glutDisplay);
    glutReshapeFunc(glutReshape);
    glutIdleFunc(glutIdle);

    // glutCreateMenu(glutMenu);
    // glutAddMenuEntry("Quit", 0);
    // glutAttachMenu(GLUT_RIGHT_BUTTON);

    ImGui_ImplOpenGL2_Init();

    glutMainLoop();

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

/*******************************************************************************
 Draw an object.
*******************************************************************************/
void drawObject() {
    // glColor3f(1.0, 0.0, 0.0);
    switch (selectedPrimitiveItem) {
        case 0:
            glutSolidCone(c, cHeight, 32, 32);
            break;
        case 1:
            glutSolidSphere(b, 32, 32);

        default:
            break;
    }
    // glutSolidCone(b, .8, 32, 32);
}

/*******************************************************************************
 GLUT callback for redrawing the view.
*******************************************************************************/
void glutDisplay() {
    drawSceneHaptics();

    drawSceneGraphics();

    glutSwapBuffers();
}

/*******************************************************************************
 GLUT callback for reshaping the window.  This is the main place where the
 viewing and workspace transforms get initialized.
*******************************************************************************/
void glutReshape(int width, int height) {
    static const double kPI = 3.1415926535897932384626433832795;
    static const double kFovY = 40;

    double nearDist, farDist, aspect;

    glViewport(0, 0, width, height);

    // Compute the viewing parameters based on a fixed fov and viewing
    // a canonical box centered at the origin.

    nearDist = 1.0 / tan((kFovY / 2.0) * kPI / 180.0);
    farDist = nearDist + 2.0;
    aspect = (double)width / height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(kFovY, aspect, nearDist, farDist);

    // Place the camera down the Z axis looking at the origin.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, nearDist + 1.0,
              0, 0, 0,
              0, 1, 0);

    ImGui_ImplGLUT_ReshapeFunc(width, height);  // this is to solve the issue with ImGui_ImplGLUT_InstallFuncs

    updateWorkspace();
}

/*******************************************************************************
 GLUT callback for idle state.  Use this as an opportunity to request a redraw.
 Checks for HLAPI errors that have occurred since the last idle check.
*******************************************************************************/
void glutIdle() {
    HLerror error;

    while (HL_ERROR(error = hlGetError())) {
        fprintf(stderr, "HL Error: %s\n", error.errorCode);

        if (error.errorCode == HL_DEVICE_ERROR) {
            hduPrintError(stderr, &error.errorInfo,
                          "Error during haptic rendering\n");
        }
    }

    glutPostRedisplay();
}

/******************************************************************************
 Popup menu handler.
******************************************************************************/
void glutMenu(int ID) {
    switch (ID) {
        case 0:
            exit(0);
            break;
    }
}

/*******************************************************************************
 Initializes the scene.  Handles initializing both OpenGL and HL.
*******************************************************************************/
void initScene() {
    initGL();
    initHL();
}

/*******************************************************************************
 Sets up general OpenGL rendering properties: lights, depth buffering, etc.
*******************************************************************************/
void initGL() {
    static const GLfloat light_model_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    static const GLfloat light0_diffuse[] = {0.9f, 0.9f, 0.9f, 0.9f};
    static const GLfloat light0_direction[] = {0.0f, -0.4f, 1.0f, 0.0f};

    // Enable depth buffering for hidden surface removal.
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    // Cull back faces.
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    // Setup other misc features.
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    // Setup lighting model.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_direction);
    glEnable(GL_LIGHT0);
}

/*******************************************************************************
 Initialize the HDAPI.  This involves initing a device configuration, enabling
 forces, and scheduling a haptic thread callback for servicing the device.
*******************************************************************************/
void initHL() {
    HDErrorInfo error;

    ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(error = hdGetError())) {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "Press any key to exit");
        getchar();
        exit(-1);
    }

    ghHLRC = hlCreateContext(ghHD);
    hlMakeCurrent(ghHLRC);

    // Enable optimization of the viewing parameters when rendering
    // geometry for OpenHaptics.
    hlEnable(HL_HAPTIC_CAMERA_VIEW);

    // Generate id for the shape.
    gShapeId = hlGenShapes(1);

    hlTouchableFace(HL_FRONT);
}

/*******************************************************************************
 This handler is called when the application is exiting.  Deallocates any state
 and cleans up.
*******************************************************************************/
void exitHandler() {
    // Deallocate the sphere shape id we reserved in initHL.
    hlDeleteShapes(gShapeId, 1);

    // Free up the haptic rendering context.
    hlMakeCurrent(NULL);
    if (ghHLRC != NULL) {
        hlDeleteContext(ghHLRC);
    }

    // Free up the haptic device.
    if (ghHD != HD_INVALID_HANDLE) {
        hdDisableDevice(ghHD);
    }
}

/*******************************************************************************
 Use the current OpenGL viewing transforms to initialize a transform for the
 haptic device workspace so that it's properly mapped to world coordinates.
*******************************************************************************/
void updateWorkspace() {
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    hlMatrixMode(HL_TOUCHWORKSPACE);
    hlLoadIdentity();

    // Fit haptic workspace to view volume.
    hluFitWorkspace(projection);

    // Compute cursor scale.
    gCursorScale = hluScreenToModelScale(modelview, projection, viewport);
    gCursorScale *= CURSOR_SIZE_PIXELS;
}
/*******************************************************************************
 Add imgui menu here
*******************************************************************************/
void drawMenu() {
    // static float f = 0.0f;

    ImGui::Begin("Mehdi's App");

    // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("Background Color", (float *)&background_color);
    // dropdown menu
    const static char *primitiveList[]{"Cone", "Sphere"};
    ImGui::Combo("Primitives", &selectedPrimitiveItem, primitiveList, IM_ARRAYSIZE(primitiveList), 0);

    switch (selectedPrimitiveItem) {
        case 0:
            ImGui::SliderFloat("Cone Radius", &c, 0.0f, 1.0f);
            ImGui::SliderFloat("Cone Height", &cHeight, 0.0f, 1.0f);
            break;
        case 1:
            ImGui::SliderFloat("Sphere Radius", &b, 0.0f, 1.0f);
            break;

        default:
            break;
    }

    ImGui::End();
}
/*******************************************************************************
 The main routine for displaying the scene.  Gets the latest snapshot of state
 from the haptic thread and uses it to display a 3D cursor.
*******************************************************************************/
void drawSceneGraphics() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    // menu bottons, sliders, and etc
    drawMenu();

    // Rendering
    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);

    glutPostRedisplay();

    glClearColor(background_color.x * background_color.w, background_color.y * background_color.w, background_color.z * background_color.w, background_color.w);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw 3D cursor at haptic device position.
    drawCursor();

    // Draw a white grid "floor" for the object to sit on.
    // glColor3f(1.0, 1.0, 1.0);
    // glBegin(GL_LINES);
    // for (GLfloat i = -2.5; i <= 2.5; i += 0.25) {
    //     glVertex3f(i, 0, 2.5);
    //     glVertex3f(i, 0, -2.5);
    //     glVertex3f(2.5, 0, i);
    //     glVertex3f(-2.5, 0, i);
    // }
    // glEnd();

    // Draw a sphere using OpenGL.
    drawObject();

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glutPostRedisplay();
}

/*******************************************************************************
 The main routine for rendering scene haptics.
*******************************************************************************/
void drawSceneHaptics() {
    // Start haptic frame.  (Must do this before rendering any haptic shapes.)
    hlBeginFrame();

    // Set material properties for the shapes to be drawn.
    hlMaterialf(HL_FRONT_AND_BACK, HL_STIFFNESS, 0.7f);
    hlMaterialf(HL_FRONT_AND_BACK, HL_DAMPING, 0.1f);
    hlMaterialf(HL_FRONT_AND_BACK, HL_STATIC_FRICTION, 0.2f);
    hlMaterialf(HL_FRONT_AND_BACK, HL_DYNAMIC_FRICTION, 0.3f);

    // Start a new haptic shape.  Use the feedback buffer to capture OpenGL
    // geometry for haptic rendering.
    hlBeginShape(HL_SHAPE_FEEDBACK_BUFFER, gShapeId);

    // Use OpenGL commands to create geometry.
    drawObject();

    // End the shape.
    hlEndShape();

    // End the haptic frame.
    hlEndFrame();
}

/*******************************************************************************
 Draws a 3D cursor for the haptic device using the current local transform,
 the workspace to world transform and the screen coordinate scale.
*******************************************************************************/
void drawCursor() {
    static const double kCursorRadius = 0.5;
    static const double kCursorHeight = 1.5;
    static const int kCursorTess = 15;
    HLdouble proxyxform[16];

    GLUquadricObj *qobj = 0;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();

    if (!gCursorDisplayList) {
        gCursorDisplayList = glGenLists(1);
        glNewList(gCursorDisplayList, GL_COMPILE);
        qobj = gluNewQuadric();

        gluCylinder(qobj, 0.0, kCursorRadius, kCursorHeight,
                    kCursorTess, kCursorTess);
        glTranslated(0.0, 0.0, kCursorHeight);
        gluCylinder(qobj, kCursorRadius, 0.0, kCursorHeight / 5.0,
                    kCursorTess, kCursorTess);

        gluDeleteQuadric(qobj);
        glEndList();
    }

    // Get the proxy transform in world coordinates.
    hlGetDoublev(HL_PROXY_TRANSFORM, proxyxform);
    glMultMatrixd(proxyxform);

    // Apply the local cursor scale factor.
    glScaled(gCursorScale, gCursorScale, gCursorScale);

    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.5, 0.0, 1.0);

    glCallList(gCursorDisplayList);

    glPopMatrix();
    glPopAttrib();
}

/******************************************************************************/
