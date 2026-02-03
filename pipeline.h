#pragma once
#include <array>
#include "color.h"
#include "geometry.h"

class Pipeline {
public:
    Pipeline(int w, int h) : width(w), height(h) {
        framebuffer.resize(width * height, { 0, 0, 0 });
        zbuffer.resize(width * height, std::numeric_limits<float>::lowest());
    }

    void lookat(const vec3 eye, const vec3 center, const vec3 up);
    void init_perspective(const double f);
    void init_viewport(const int x, const int y, const int w, const int h);
    void init_zbuffer(const int width, const int height);

    struct VertexOutput {
        vec4 clipPos;
        vec3 worldPos;
        vec3 normal;
    };

    struct IShader {
        virtual Pipeline::VertexOutput vertex(const vec3& v, const vec3& n, const mat<4, 4>& modelview, const mat<4, 4>& perspective, const mat<4, 4>& normalMatrix) = 0;
        virtual void setup_triangle(const vec3 pos[3], const vec3 norm[3]) = 0;
        virtual std::pair<bool, Color> fragment(const vec3& bar) const = 0;
    };

    typedef std::array<vec4, 3> Triangle; // a triangle primitive is made of three ordered points

    Triangle transform_triangle(IShader& shader, const vec3& v0, const vec3& v1, const vec3& v2,
        const vec3& n0, const vec3& n1, const vec3& n2) {
        mat<4, 4> normalMatrix = ModelView.invert_transpose();

        // Process all three vertices and collect outputs
        VertexOutput out0 = shader.vertex(v0, n0, ModelView, Perspective, normalMatrix);
        VertexOutput out1 = shader.vertex(v1, n1, ModelView, Perspective, normalMatrix);
        VertexOutput out2 = shader.vertex(v2, n2, ModelView, Perspective, normalMatrix);

        // Build clip space triangle
        Triangle clip;
        clip[0] = out0.clipPos;
        clip[1] = out1.clipPos;
        clip[2] = out2.clipPos;

        // Pass transformed positions and normals to shader
        vec3 pos[3] = { out0.worldPos, out1.worldPos, out2.worldPos };
        vec3 norm[3] = { out0.normal, out1.normal, out2.normal };
        shader.setup_triangle(pos, norm);

        return clip;
    }

    void rasterize(const Triangle& clip, IShader& shader);

    float get_depth(int x, int y);
    const Color* get_framebuffer_data() const;
    size_t get_framebuffer_size() const;

    std::vector<float>& get_zbuffer();

    mat<4, 4> get_modelview() const;
    mat<4, 4> get_viewport() const;
    mat<4, 4> get_perspective() const;

private:
    int width, height;
    std::vector<Color> framebuffer;
    std::vector<float> zbuffer;
    mat<4, 4> ModelView, Viewport, Perspective;

    void set(int x, int y, Color c);
    void set(int x, int y, float depth);
};