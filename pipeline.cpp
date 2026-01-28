#include <algorithm>
#include <limits>
#include "pipeline.h"

void Pipeline::lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalize(eye - center);
    vec3 l = normalize(cross(up, n));
    vec3 m = normalize(cross(n, l));
    this->ModelView = mat<4, 4>{ {{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}} } *
        mat<4, 4>{{{1, 0, 0, -center.x}, { 0,1,0,-center.y }, { 0,0,1,-center.z }, { 0,0,0,1 }}};
}

void Pipeline::init_perspective(const double f) {
    this->Perspective = { {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1 / f,1}} };
}

void Pipeline::init_viewport(const int x, const int y, const int w, const int h) {
    this->Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
}

void Pipeline::init_zbuffer(const int width, const int height) {
    this->zbuffer.resize(width * height, -1000.);
}

void Pipeline::rasterize(const Triangle& clip, const IShader& shader) {
    vec4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };                // normalized device coordinates
    vec2 screen[3] = { (this->Viewport * ndc[0]).xy(), (this->Viewport * ndc[1]).xy(), (this->Viewport * ndc[2]).xy() }; // screen coordinates

    mat<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} } };
    if (ABC.det() < 1) return; // backface culling + discarding triangles that cover less than a pixel

    auto [bbminx, bbmaxx] = std::minmax({ screen[0].x, screen[1].x, screen[2].x }); // bounding box for the triangle
    auto [bbminy, bbmaxy] = std::minmax({ screen[0].y, screen[1].y, screen[2].y }); // defined by its top left and bottom right corners

    mat<3, 3> ABC_inv = ABC.invert_transpose();
#pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, this->width - 1); x++) {         // clip the bounding box by the screen
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, this->height - 1); y++) {
            vec3 bc = ABC_inv * vec3{ static_cast<double>(x), static_cast<double>(y), 1. }; // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                    // negative barycentric coordinate => the pixel is outside the triangle
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };  // linear interpolation of the depth
            if (z <= get_depth(x, y)) continue;
            auto [discard, color] = shader.fragment(bc);
            if (discard) continue;
            set(x, y, static_cast<float>(z));
            set(x, y, color);
        }
    }
}

mat<4, 4> Pipeline::get_modelview() const {
    return this->ModelView;
}

mat<4, 4> Pipeline::get_viewport() const {
    return this->Viewport;
}

mat<4, 4> Pipeline::get_perspective() const {
    return this->Perspective;
}

const Color* Pipeline::get_framebuffer_data() const {
    return this->framebuffer.data();
}

size_t Pipeline::get_framebuffer_size() const {
    return this->framebuffer.size();
}

std::vector<float>& Pipeline::get_zbuffer() {
    return this->zbuffer;
}

// Helper Functions
void Pipeline::set(int x, int y, Color c) {
    int idx = (this->height - 1 - y) * this->width + x;
    if (idx >= 0 && idx < this->width * this->height) {
        this->framebuffer[idx] = c;
    }
};

void Pipeline::set(int x, int y, float depth) {
    int idx = (this->height - 1 - y) * this->width + x;
    if (idx >= 0 && idx < this->width * this->height) {
        this->zbuffer[idx] = depth;
    }
};

float Pipeline::get_depth(int x, int y) {
    int idx = (this->height - 1 - y) * this->width + x;
    if (idx >= 0 && idx < this->width * this->height) {
        return this->zbuffer[idx];
    }
    return std::numeric_limits<float>::lowest();
};
