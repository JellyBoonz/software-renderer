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

    struct IShader {
        virtual vec4 vertex(const vec3& v, const mat<4, 4>& modelview, const mat<4, 4>& perspective) const = 0;
        virtual std::pair<bool, Color> fragment(const vec3 bar) const = 0;
    };

    typedef std::array<vec4, 3> Triangle; // a triangle primitive is made of three ordered points

    Triangle transform_triangle(const IShader& shader, const vec3& v0, const vec3& v1, const vec3& v2) const {
        Triangle clip;
        clip[0] = shader.vertex(v0, ModelView, Perspective);
        clip[1] = shader.vertex(v1, ModelView, Perspective);
        clip[2] = shader.vertex(v2, ModelView, Perspective);
        return clip;
    }

    void rasterize(const Triangle& clip, const IShader& shader);

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