// main.cpp
// HW5 Q1: Transformations + Software Rasterizer + Z-Buffer
// Single file, using only the specified headers.

#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>

#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// ------------------------------------------------------------------
// Generate a unit-sphere mesh (vertex list + triangle index list)
// ------------------------------------------------------------------
static int     gNumVertices = 0;
static int     gNumTriangles = 0;
static float* gVertexBuffer = nullptr; // 3 floats per vertex
static int* gIndexBuffer = nullptr; // 3 indices per triangle

void create_sphere(int width = 32, int height = 16) {
    gNumVertices = (height - 2) * width + 2;
    gNumTriangles = (height - 2) * (width - 1) * 2 + 2 * (width - 1);

    gVertexBuffer = new float[3 * gNumVertices];
    gIndexBuffer = new int[3 * gNumTriangles];

    int v = 0;
    for (int j = 1; j < height - 1; ++j) {
        float theta = float(j) / float(height - 1) * pi<float>();
        for (int i = 0; i < width; ++i) {
            float phi = float(i) / float(width - 1) * two_pi<float>();
            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);
            gVertexBuffer[3 * v + 0] = x;
            gVertexBuffer[3 * v + 1] = y;
            gVertexBuffer[3 * v + 2] = z;
            ++v;
        }
    }
    // top pole
    gVertexBuffer[3 * v + 0] = 0.0f;
    gVertexBuffer[3 * v + 1] = 1.0f;
    gVertexBuffer[3 * v + 2] = 0.0f;
    int topIndex = v++;
    // bottom pole
    gVertexBuffer[3 * v + 0] = 0.0f;
    gVertexBuffer[3 * v + 1] = -1.0f;
    gVertexBuffer[3 * v + 2] = 0.0f;
    int botIndex = v++;

    int idx = 0;
    // sides
    for (int j = 0; j < height - 2; ++j) {
        for (int i = 0; i < width - 1; ++i) {
            int v0 = j * width + i;
            int v1 = j * width + (i + 1);
            int v2 = (j + 1) * width + (i + 1);
            int v3 = (j + 1) * width + i;
            // tri 1
            gIndexBuffer[idx++] = v0;
            gIndexBuffer[idx++] = v2;
            gIndexBuffer[idx++] = v1;
            // tri 2
            gIndexBuffer[idx++] = v0;
            gIndexBuffer[idx++] = v3;
            gIndexBuffer[idx++] = v2;
        }
    }
    // top cap
    for (int i = 0; i < width - 1; ++i) {
        gIndexBuffer[idx++] = topIndex;
        gIndexBuffer[idx++] = i;
        gIndexBuffer[idx++] = i + 1;
    }
    // bottom cap
    int ringStart = (height - 2) * width;
    for (int i = 0; i < width - 1; ++i) {
        gIndexBuffer[idx++] = botIndex;
        gIndexBuffer[idx++] = ringStart + (i + 1);
        gIndexBuffer[idx++] = ringStart + i;
    }
}

// ------------------------------------------------------------------
// Software rasterizer + Z-buffer
// ------------------------------------------------------------------
static int Width = 512, Height = 512;
static std::vector<float> FrameBuffer;
static std::vector<float> ZBuffer;

void renderSoftware() {
    FrameBuffer.assign(Width * Height * 3, 0.5f);
    ZBuffer.assign(Width * Height, std::numeric_limits<float>::infinity());

    mat4 Model = translate(mat4(1.0f), vec3(0, 0, -7)) * scale(mat4(1.0f), vec3(2.0f));
    mat4 View = lookAt(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0));
    mat4 Projection = frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.0f);
    mat4 MVP = Projection * View * Model;

    auto toScreen = [&](const vec4& v) {
        vec4 p = v / v.w;
        return vec3(
            (p.x * 0.5f + 0.5f) * Width,
            (p.y * 0.5f + 0.5f) * Height,
            p.z
        );
        };
    auto edge = [&](const vec3& A, const vec3& B, const vec2& P) {
        return (P.x - A.x) * (B.y - A.y) - (P.y - A.y) * (B.x - A.x);
        };

    for (int t = 0; t < gNumTriangles; ++t) {
        int i0 = gIndexBuffer[3 * t + 0],
            i1 = gIndexBuffer[3 * t + 1],
            i2 = gIndexBuffer[3 * t + 2];
        vec3 P[3] = {
            toScreen(MVP * vec4(gVertexBuffer[3 * i0 + 0], gVertexBuffer[3 * i0 + 1], gVertexBuffer[3 * i0 + 2], 1.0f)),
            toScreen(MVP * vec4(gVertexBuffer[3 * i1 + 0], gVertexBuffer[3 * i1 + 1], gVertexBuffer[3 * i1 + 2], 1.0f)),
            toScreen(MVP * vec4(gVertexBuffer[3 * i2 + 0], gVertexBuffer[3 * i2 + 1], gVertexBuffer[3 * i2 + 2], 1.0f))
        };

        float area = edge(P[0], P[1], vec2(P[2]));
        if (fabs(area) < 1e-6f) continue;

        int minX = std::max(0, int(std::floor(std::min({ P[0].x,P[1].x,P[2].x }))));
        int maxX = std::min(Width - 1, int(std::ceil(std::max({ P[0].x,P[1].x,P[2].x }))));
        int minY = std::max(0, int(std::floor(std::min({ P[0].y,P[1].y,P[2].y }))));
        int maxY = std::min(Height - 1, int(std::ceil(std::max({ P[0].y,P[1].y,P[2].y }))));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                vec2 p(x + 0.5f, y + 0.5f);
                float w0 = edge(P[1], P[2], p) / area;
                float w1 = edge(P[2], P[0], p) / area;
                float w2 = edge(P[0], P[1], p) / area;
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    float z = w0 * P[0].z + w1 * P[1].z + w2 * P[2].z;
                    int idx = y * Width + x;
                    if (z < ZBuffer[idx]) {
                        ZBuffer[idx] = z;
                        int off = idx * 3;
                        FrameBuffer[off + 0] = 1.0f;
                        FrameBuffer[off + 1] = 1.0f;
                        FrameBuffer[off + 2] = 1.0f;
                    }
                }
            }
        }
    }

    // flip vertically when drawing
    for (int row = 0; row < Height; ++row) {
        float* ptr = FrameBuffer.data() + ((Height - 1 - row) * Width * 3);
        glRasterPos2i(0, row);
        glDrawPixels(Width, 1, GL_RGB, GL_FLOAT, ptr);
    }
}

// window resize callback
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    Width = w; Height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, Width, 0, Height, 1, -1);
    renderSoftware();
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(Width, Height, "HW5 Q1", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    create_sphere();
    renderSoftware();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        renderSoftware();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete[] gVertexBuffer;
    delete[] gIndexBuffer;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
