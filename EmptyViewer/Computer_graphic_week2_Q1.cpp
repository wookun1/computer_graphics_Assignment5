// q1_phong_shading/main.cpp
#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <cmath>
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

// Material structure
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

    Ray generateRay(int i, int j, int nx, int ny) const {
        float u = l + (r - l) * ((i + 0.5f) / float(nx));
        float v = b + (t - b) * ((j + 0.5f) / float(ny));
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

        // Shadow ray
        vec3 toLight = normalize(lightPos - hitPos);
        Ray shadowRay(hitPos + 1e-4f * normal, toLight);
        for (auto obj : objects) {
            vec3 dummy;
            float t = obj->intersect(shadowRay, dummy);
            if (t > 0.0f) return hitObj->material.ka * lightColor;
        }

        // Lighting
        vec3 viewDir = normalize(-ray.direction);
        vec3 halfVec = normalize(toLight + viewDir);
        float diff = max(dot(normal, toLight), 0.0f);
        float spec = pow(max(dot(normal, halfVec), 0.0f), hitObj->material.specPower);
        vec3 color = hitObj->material.ka +
            hitObj->material.kd * diff +
            hitObj->material.ks * spec;
        return clamp(color * lightColor, 0.0f, 1.0f);
    }
};

int Width = 512, Height = 512;
std::vector<float> OutputImage;
Camera* camera = nullptr;
Scene* scene = nullptr;

void render() {
    OutputImage.resize(Width * Height * 3);
    for (int j = 0; j < Height; ++j) {
        for (int i = 0; i < Width; ++i) {
            Ray ray = camera->generateRay(i, j, Width, Height);
            vec3 color = scene->trace(ray);
            int idx = (j * Width + i) * 3;
            OutputImage[idx + 0] = color.r;
            OutputImage[idx + 1] = color.g;
            OutputImage[idx + 2] = color.b;
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
    window = glfwCreateWindow(Width, Height, "Q1: Phong Shading", NULL, NULL);
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
