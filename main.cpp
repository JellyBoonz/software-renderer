#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include <OpenGL/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl2.h"

#include "color.h"
#include "file_parser.h"
#include "pipeline.h"

constexpr int width = 800;
constexpr int height = 800;

constexpr Color white = { 255, 255, 255 }; // attention, BGRA order
constexpr Color green = { 0, 255, 0 };
constexpr Color red = { 255, 0, 0 };
constexpr Color blue = { 64, 128, 255 };
constexpr Color yellow = { 255, 200, 0 };

struct Shader : Pipeline::IShader {
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    Color color;
    vec3 tri_pos[3];
    vec3 tri_norm[3];
    vec3 eye;
    vec3 lightPos;

    Shader(const std::vector<vec3>& vertices) : vertices(vertices) {}

    Pipeline::VertexOutput vertex(const vec3& v, const vec3& n, const mat<4, 4>& modelview, const mat<4, 4>& perspective, const mat<4, 4>& normalMatrix) override {
        // Transform vertex to clip space
        vec4 clipPos = perspective * modelview * vec4{ v.x, v.y, v.z, 1.0 };

        // Transform normal by inverse transpose
        vec4 normalTransformed = normalMatrix * vec4{ n.x, n.y, n.z, 0.0 };
        vec3 normalVec = normalize(vec3{ normalTransformed.x, normalTransformed.y, normalTransformed.z });

        // Return all outputs
        Pipeline::VertexOutput output;
        output.clipPos = clipPos;
        output.worldPos = v;  // Store world-space position (after model rotation, before ModelView)
        output.normal = normalVec;
        return output;
    }

    void setup_triangle(const vec3 pos[3], const vec3 norm[3]) override {
        tri_pos[0] = pos[0];
        tri_pos[1] = pos[1];
        tri_pos[2] = pos[2];
        tri_norm[0] = norm[0];
        tri_norm[1] = norm[1];
        tri_norm[2] = norm[2];
    }

    std::pair<bool, Color> fragment(const vec3& bar) const override {
        Color baseColor = this->color;

        vec3 normal = bar[0] * tri_norm[0] + bar[1] * tri_norm[1] + bar[2] * tri_norm[2];
        vec3 fragPos = bar[0] * tri_pos[0] + bar[1] * tri_pos[1] + bar[2] * tri_pos[2];

        vec3 lightDir = normalize(lightPos - fragPos);
        vec3 viewDir = normalize(eye - fragPos);
        vec3 reflectDir = reflect(-lightDir, normal);

        float ambient = 0.1;
        float diff = std::max(dot(normal, lightDir), 0.0);
        float spec = std::pow(std::max(dot(viewDir, reflectDir), 0.0), 32);

        float intensity = ambient + diff + spec;
        Color result = baseColor * intensity;

        return { false, result };
    }
};

void visualize_zbuffer(const std::vector<float>& zbuffer, const std::string& filename) {
    // Find min and max depth values
    float min_depth = std::numeric_limits<float>::max();
    float max_depth = std::numeric_limits<float>::lowest();

    for (float z : zbuffer) {
        if (z > std::numeric_limits<float>::lowest() + 1e-6) { // Check if pixel was drawn
            min_depth = std::min(min_depth, z);
            max_depth = std::max(max_depth, z);
        }
    }

    if (max_depth <= min_depth) {
        min_depth = 0.0f;
        max_depth = 1.0f;
    }

    float depth_range = max_depth - min_depth;
    if (depth_range < 1e-6) depth_range = 1.0f;

    // Convert to grayscale image
    std::vector<Color> zbuffer_image(width * height);
    for (int i = 0; i < width * height; i++) {
        if (zbuffer[i] > std::numeric_limits<float>::lowest() + 1e-6) {
            float normalized = (zbuffer[i] - min_depth) / depth_range;
            unsigned char gray = static_cast<unsigned char>(normalized * 255.0f);
            zbuffer_image[i] = { gray, gray, gray };
        }
    }

    // Write PPM file
    std::ofstream ofs(filename, std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write(reinterpret_cast<char*>(zbuffer_image.data()), zbuffer_image.size() * sizeof(Color));
    ofs.close();
}

void realtime_render() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    // Create window with RGB framebuffer
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(width, height, "Software Rasterizer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Set up OpenGL for displaying the framebuffer
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera and model transform parameters
    vec3 eye{ -1, 0, 2 };
    vec3 center{ 0, 0, 0 };
    vec3 up{ 0, 1, 0 };
    float rotation = 0.0f;
    float zoom = 2.0f;

    // Load model once
    file_parser fp;
    fp.load("./test2.obj");
    std::vector<vec3> vertices = fp.get_vertices();
    std::vector<vec3> normals = fp.get_normals();
    std::vector<Face> faces = fp.get_faces();

    std::cout << "Controls:" << std::endl;
    std::cout << "  Left/Right Arrow: Rotate model" << std::endl;
    std::cout << "  Up/Down Arrow: Zoom in/out" << std::endl;
    std::cout << "  W/S: Move camera up/down" << std::endl;
    std::cout << "  A/D: Move camera left/right" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;

    double lastTime = glfwGetTime();
    int frameCount = 0;
    int currentFPS = 0;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate FPS
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0) {
            currentFPS = frameCount;
            frameCount = 0;
            lastTime = currentTime;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Handle input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            rotation -= 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            rotation += 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            zoom -= 0.02f;
            if (zoom < 0.5f) zoom = 0.5f;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            zoom += 0.02f;
            if (zoom > 5.0f) zoom = 5.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            eye.y += 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            eye.y -= 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            eye.x -= 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            eye.x += 0.02f;
        }

        // Update camera position with zoom
        eye.z = zoom;

        // Create pipeline for this frame
        Pipeline pipeline(width, height);
        pipeline.lookat(eye, center, up);
        pipeline.init_perspective(magnitude(eye - center));  // Smaller focal length = wider FOV = larger model
        pipeline.init_viewport(0, 0, width, height);

        // Create shader
        Shader shader(vertices);
        shader.eye = eye;
        shader.lightPos = vec3{ 2, 2, 3 };
        shader.color = Color{ 200, 200, 200 };

        // Render all faces
        for (size_t f = 0; f < faces.size(); f++) {
            const Face& face = faces[f];

            vec3 v0 = vertices[face[0].v_idx - 1];
            vec3 v1 = vertices[face[1].v_idx - 1];
            vec3 v2 = vertices[face[2].v_idx - 1];

            bool hasVertexNormals =
                !normals.empty() &&
                face[0].n_idx > 0 && face[1].n_idx > 0 && face[2].n_idx > 0 &&
                face[0].n_idx <= (int)normals.size() &&
                face[1].n_idx <= (int)normals.size() &&
                face[2].n_idx <= (int)normals.size();

            vec3 n0, n1, n2;
            if (hasVertexNormals) {
                n0 = normals[face[0].n_idx - 1];
                n1 = normals[face[1].n_idx - 1];
                n2 = normals[face[2].n_idx - 1];
            }
            else {
                // Fallback: geometric face normal
                vec3 faceNormal = normalize(cross(v1 - v0, v2 - v0));
                n0 = n1 = n2 = faceNormal;
            }

            // Apply rotation around Y axis
            float cosR = cos(rotation);
            float sinR = sin(rotation);
            auto rotate_y = [&](vec3 v) {
                return vec3{ v.x * cosR + v.z * sinR, v.y, -v.x * sinR + v.z * cosR };
                };
            v0 = rotate_y(v0);
            v1 = rotate_y(v1);
            v2 = rotate_y(v2);
            n0 = normalize(rotate_y(n0));
            n1 = normalize(rotate_y(n1));
            n2 = normalize(rotate_y(n2));

            // Transform triangle (vertex shader handles ModelView/Perspective and normal transformation)
            auto clip = pipeline.transform_triangle(shader, v0, v1, v2, n0, n1, n2);
            pipeline.rasterize(clip, shader);
        }

        // Display framebuffer using OpenGL
        glClear(GL_COLOR_BUFFER_BIT);
        glRasterPos2f(-1.0f, 1.0f);
        glPixelZoom(2.0f, -2.0f);
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pipeline.get_framebuffer_data());

        // Render ImGui on top
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("FPS: %d", currentFPS);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}

void file_load() {
    const vec3    eye{ -1,0,2 }; // camera position
    const vec3 center{ 0,0,0 };  // camera direction
    const vec3     up{ 0,1,0 };  // camera up vector

    Pipeline pipeline(width, height);

    pipeline.lookat(eye, center, up);
    pipeline.init_perspective(magnitude(eye - center));
    pipeline.init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);

    file_parser fp;

    fp.load("./test.obj");

    Shader shader(fp.get_vertices());
    shader.eye = eye;
    shader.lightPos = vec3{ 0, 0.5, 1 };

    std::vector<vec3> vertices = fp.get_vertices();
    std::vector<vec3> normals = fp.get_normals();
    std::vector<Face> faces = fp.get_faces();

    for (size_t f = 0; f < faces.size(); f++) {
        shader.color = Color{ 150, 150, 150 };

        vec3 v0 = vertices[faces[f][0].v_idx - 1];
        vec3 v1 = vertices[faces[f][1].v_idx - 1];
        vec3 v2 = vertices[faces[f][2].v_idx - 1];

        // Get normals (or compute face normal if not available)
        vec3 n0, n1, n2;
        bool hasVertexNormals = !normals.empty() &&
            faces[f][0].n_idx > 0 && faces[f][1].n_idx > 0 && faces[f][2].n_idx > 0 &&
            faces[f][0].n_idx <= (int)normals.size() &&
            faces[f][1].n_idx <= (int)normals.size() &&
            faces[f][2].n_idx <= (int)normals.size();

        if (hasVertexNormals) {
            n0 = normals[faces[f][0].n_idx - 1];
            n1 = normals[faces[f][1].n_idx - 1];
            n2 = normals[faces[f][2].n_idx - 1];
        }
        else {
            vec3 faceNormal = normalize(cross(v1 - v0, v2 - v0));
            n0 = n1 = n2 = faceNormal;
        }

        auto clip = pipeline.transform_triangle(shader, v0, v1, v2, n0, n1, n2);
        pipeline.rasterize(clip, shader);
    }

    std::ofstream ofs("framebuffer.ppm", std::ios::binary);
    ofs << "P6\n"
        << width << " " << height << "\n255\n";
    ofs.write(reinterpret_cast<const char*>(pipeline.get_framebuffer_data()), pipeline.get_framebuffer_size() * sizeof(Color));
    ofs.close();

    visualize_zbuffer(pipeline.get_zbuffer(), "zbuffer.ppm");
}

int main() {
    switch (2) {
    case 1:
        file_load();
        break;
    case 2:
        realtime_render();
        break;
    }
}