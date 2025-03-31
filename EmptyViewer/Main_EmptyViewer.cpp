// q3_antialiasing/main.cpp
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

struct Material {
    vec3 ka, kd, ks;
    float specPower;
};

class Ray {
public:
    vec3 origin, direction;
    Ray(const vec3& o, const vec3& d) : origin(o), direction(normalize(d)) {}
};

class Surface {
public:
    Material material;
    virtual ~Surface() {}
    virtual float intersect(const Ray& ray, vec3& normalOut) const = 0;
    virtual vec3 getPosition(const Ray& ray, float t) const = 0;
};

class Sphere : public Surface {
public:
    vec3 center;
    float radius;
    Sphere(const vec3& c, float r, const Material& m) : center(c), radius(r) { material = m; }

    float intersect(const Ray& ray, vec3& normalOut) const override {
        vec3 oc = ray.origin - center;
        float b = 2.0f * dot(ray.direction, oc);
        float c = dot(oc, oc) - radius * radius;
        float disc = b * b - 4.0f * c;
        if (disc < 0.0f) return -1.0f;
        float sqrtDisc = sqrtf(disc);
        float t1 = (-b - sqrtDisc) / 2.0f;
        float t2 = (-b + sqrtDisc) / 2.0f;
        float t = (t1 > 0.001f) ? t1 : (t2 > 0.001f) ? t2 : -1.0f;
        if (t > 0.0f) {
            vec3 pos = ray.origin + t * ray.direction;
            normalOut = normalize(pos - center);
        }
        return t;
    }

    vec3 getPosition(const Ray& ray, float t) const override {
        return ray.origin + t * ray.direction;
    }
};

class Plane : public Surface {
public:
    float y;
    Plane(float yVal, const Material& m) : y(yVal) { material = m; }

    float intersect(const Ray& ray, vec3& normalOut) const override {
        if (fabs(ray.direction.y) < 1e-6f) return -1.0f;
        float t = (y - ray.origin.y) / ray.direction.y;
        if (t > 0.001f) {
            normalOut = vec3(0, 1, 0);
            return t;
        }
        return -1.0f;
    }

    vec3 getPosition(const Ray& ray, float t) const override {
        return ray.origin + t * ray.direction;
    }
};

class Camera {
public:
    vec3 eye;
    float l, r, b, t, d;
    Camera(const vec3& e, float l_, float r_, float b_, float t_, float d_)
        : eye(e), l(l_), r(r_), b(b_), t(t_), d(d_) {}

    Ray generateRay(float i, float j, int nx, int ny) const {
        float u = l + (r - l) * (i / float(nx));
        float v = b + (t - b) * (j / float(ny));
        vec3 imagePoint(u, v, -d);
        return Ray(eye, imagePoint - eye);
    }
};

class Scene {
public:
    std::vector<Surface*> objects;
    vec3 lightPos = vec3(-4, 4, -3);
    vec3 lightColor = vec3(1.0f);

    ~Scene() {
        for (auto obj : objects) delete obj;
    }

    vec3 trace(const Ray& ray) const {
        float tMin = 1e20;
        Surface* hitObj = nullptr;
        vec3 normal;
        for (auto obj : objects) {
            vec3 tempNormal;
            float t = obj->intersect(ray, tempNormal);
            if (t > 0.0f && t < tMin) {
                tMin = t;
                normal = tempNormal;
                hitObj = obj;
            }
        }
        if (!hitObj) return vec3(0);

        vec3 hitPos = ray.origin + tMin * ray.direction;

        vec3 toLight = normalize(lightPos - hitPos);
        Ray shadowRay(hitPos + 1e-4f * normal, toLight);
        for (auto obj : objects) {
            vec3 dummy;
            float t = obj->intersect(shadowRay, dummy);
            if (t > 0.0f) return hitObj->material.ka * lightColor;
        }

        vec3 viewDir = normalize(-ray.direction);
        vec3 halfVec = normalize(toLight + viewDir);
        float diff = max(dot(normal, toLight), 0.0f);
        float spec = pow(max(dot(normal, halfVec), 0.0f), hitObj->material.specPower);
        vec3 color = hitObj->material.ka + hitObj->material.kd * diff + hitObj->material.ks * spec;
        return clamp(color * lightColor, 0.0f, 1.0f);
    }
};

int Width = 512, Height = 512;
std::vector<float> OutputImage;
Camera* camera = nullptr;
Scene* scene = nullptr;
const float gammaValue = 2.2f;
const int NUM_SAMPLES = 64;

vec3 applyGammaCorrection(const vec3& color) {
    return vec3(pow(color.r, 1.0f / gammaValue),
        pow(color.g, 1.0f / gammaValue),
        pow(color.b, 1.0f / gammaValue));
}

void render() {
    OutputImage.resize(Width * Height * 3);
    for (int j = 0; j < Height; ++j) {
        for (int i = 0; i < Width; ++i) {
            vec3 colorSum(0.0f);
            for (int s = 0; s < NUM_SAMPLES; ++s) {
                float dx = static_cast<float>(rand()) / RAND_MAX;
                float dy = static_cast<float>(rand()) / RAND_MAX;
                Ray ray = camera->generateRay(i + dx, j + dy, Width, Height);
                vec3 sampleColor = scene->trace(ray);
                colorSum += sampleColor;
            }
            vec3 finalColor = colorSum / float(NUM_SAMPLES);
            finalColor = applyGammaCorrection(finalColor);
            int idx = (j * Width + i) * 3;
            OutputImage[idx + 0] = finalColor.r;
            OutputImage[idx + 1] = finalColor.g;
            OutputImage[idx + 2] = finalColor.b;
        }
    }
}

void resize_callback(GLFWwindow*, int nw, int nh) {
    Width = nw;
    Height = nh;
    glViewport(0, 0, nw, nh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Width, 0.0, Height, 1.0, -1.0);
    OutputImage.reserve(Width * Height * 3);
    render();
}

int main(int argc, char* argv[]) {
    GLFWwindow* window;
    if (!glfwInit()) return -1;
    window = glfwCreateWindow(Width, Height, "Q3: Anti-aliasing", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    camera = new Camera(vec3(0.0f, 0.0f, 0.0f), -0.1f, 0.1f, -0.1f, 0.1f, 0.1f);
    scene = new Scene();

    scene->objects.push_back(new Plane(-2.0f, { {0.2f,0.2f,0.2f}, {1,1,1}, {0,0,0}, 0 }));
    scene->objects.push_back(new Sphere(vec3(-4, 0, -7), 1, { {0.2f,0,0}, {1,0,0}, {0,0,0}, 0 }));
    scene->objects.push_back(new Sphere(vec3(0, 0, -7), 2, { {0,0.2f,0}, {0,0.5f,0}, {0.5f,0.5f,0.5f}, 32 }));
    scene->objects.push_back(new Sphere(vec3(4, 0, -7), 1, { {0,0,0.2f}, {0,0,1}, {0,0,0}, 0 }));

    resize_callback(NULL, Width, Height);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    }

    delete camera;
    delete scene;
    glfwTerminate();
    return 0;
}
