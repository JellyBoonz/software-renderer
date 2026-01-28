#pragma once
#include <cmath>
#include <cassert>
#include <iostream>
#include <initializer_list>
#include <type_traits>

template<int n> struct vec {
    double data[n] = { 0 };
    double& operator[](const int i) { assert(i >= 0 && i < n); return data[i]; }
    double  operator[](const int i) const { assert(i >= 0 && i < n); return data[i]; }
};

template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i = 0; i < n; i++) out << v[i] << " ";
    return out;
}

template<> struct vec<2> {
    double x = 0, y = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 2); return i ? y : x; }
    double  operator[](const int i) const { assert(i >= 0 && i < 2); return i ? y : x; }
};

template<> struct vec<3> {
    double x = 0, y = 0, z = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 3); return i ? (1 == i ? y : z) : x; }
    double  operator[](const int i) const { assert(i >= 0 && i < 3); return i ? (1 == i ? y : z) : x; }
};

template<> struct vec<4> {
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i) { assert(i >= 0 && i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    double  operator[](const int i) const { assert(i >= 0 && i < 4); return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    vec<2> xy() const { return vec<2>{x, y}; }
};

typedef vec<3> vec3;

template<int n>
vec<n> operator+(const vec<n>& a, const vec<n>& b) {
    vec<n> result;
    for (int i = 0; i < n; i++) {
        result[i] = a[i] + b[i];
    }
    return result;
}

template<int n>
vec<n>operator-(const vec<n>& a, const vec<n>& b) {
    vec<n> result;
    for (int i = 0; i < n; i++) {
        result[i] = a[i] - b[i];
    }
    return result;
}

template<int n>
vec<n>operator*(const vec<n>& v, double scalar) {
    vec<n> result;
    for (int i = 0; i < n; i++) {
        result[i] = v[i] * scalar;
    }
    return result;
}

template<int n>
vec<n> operator*(double scalar, const vec<n>& v) {
    return v * scalar;
}

template<int n>
vec<n>operator/(const vec<n>& v, double scalar) {
    vec<n> result;
    for (int i = 0; i < n; i++) {
        result[i] = v[i] / scalar;
    }
    return result;
}

template<int n>
double operator*(const vec<n>& a, const vec<n>& b) {
    return dot(a, b);
}

template<int n>
vec<n> operator-(const vec<n>& v) {
    vec<n> result;
    for (int i = 0; i < n; i++) {
        result[i] = -v[i];
    }
    return result;
}

template<int n>
double dot(const vec<n>& a, const vec<n>& b) {
    double result = 0.0;
    for (int i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

template<int n>
double magnitude(const vec<n>& v) {
    return std::sqrt(dot(v, v));
}

template<int n>
vec<n> normalize(const vec<n>& v) {
    double mag = magnitude(v);
    if (mag > 0.0) {
        return v / mag;
    }
    return vec<n>{};
}

template<int n>
vec<n> reflect(const vec<n>& v, const vec<n>& norm) {
    return v - 2 * dot(v, norm) * norm;
}

typedef vec<2> vec2;
typedef vec<4> vec4;

template<int rows, int cols> struct mat {
    double data[rows * cols] = { 0 };

    // Default constructor
    mat() = default;

    // Constructor from nested initializer list
    constexpr mat(std::initializer_list<std::initializer_list<double>> init) {
        auto row_it = init.begin();
        for (int i = 0; i < rows && row_it != init.end(); i++, row_it++) {
            auto col_it = row_it->begin();
            for (int j = 0; j < cols && col_it != row_it->end(); j++, col_it++) {
                data[i * cols + j] = *col_it;
            }
        }
    }

    // Access element at row i, column j
    double& operator()(int i, int j) {
        assert(i >= 0 && i < rows && j >= 0 && j < cols);
        return data[i * cols + j];
    }

    double operator()(int i, int j) const {
        assert(i >= 0 && i < rows && j >= 0 && j < cols);
        return data[i * cols + j];
    }

    // Access by index (row-major)
    double& operator[](int i) {
        assert(i >= 0 && i < rows * cols);
        return data[i];
    }

    double operator[](int i) const {
        assert(i >= 0 && i < rows * cols);
        return data[i];
    }

    // Determinant (only for square matrices)
    template<int n = rows>
    typename std::enable_if<n == cols && n == rows, double>::type det() const {
        return determinant_impl(*this);
    }

    // Inverse transpose (only for square matrices)
    template<int n = rows>
    typename std::enable_if<n == cols && n == rows, mat<n, n>>::type invert_transpose() const {
        return transpose(inverse(*this));
    }
};

// Helper function to compute determinant recursively
template<int n>
double determinant_impl(const mat<n, n>& m) {
    if constexpr (n == 1) {
        return m(0, 0);
    }
    else if constexpr (n == 2) {
        return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
    }
    else if constexpr (n == 3) {
        return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
            m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
            m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
    }
    else {
        // Recursive case: use cofactor expansion along first row
        double result = 0.0;
        for (int j = 0; j < n; j++) {
            // Create submatrix by removing row 0 and column j
            mat<n - 1, n - 1> submatrix;
            for (int i = 1; i < n; i++) {
                for (int k = 0, sub_k = 0; k < n; k++) {
                    if (k != j) {
                        submatrix(i - 1, sub_k) = m(i, k);
                        sub_k++;
                    }
                }
            }
            double cofactor = determinant_impl(submatrix);
            if (j % 2 == 0) {
                result += m(0, j) * cofactor;
            }
            else {
                result -= m(0, j) * cofactor;
            }
        }
        return result;
    }
}

template<int rows, int cols>
std::ostream& operator<<(std::ostream& out, const mat<rows, cols>& m) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            out << m(i, j) << " ";
        }
        out << "\n";
    }
    return out;
}

template<int A, int B, int C>
mat<A, C> operator*(const mat<A, B>& a, const mat<B, C>& b) {
    mat<A, C> result;
    for (int i = 0; i < A; i++) {
        for (int j = 0; j < C; j++) {
            double sum = 0.0;
            for (int k = 0; k < B; k++) {
                sum += a(i, k) * b(k, j);
            }
            result(i, j) = sum;
        }
    }
    return result;
}

template<int rows, int cols>
vec<rows> operator*(const mat<rows, cols>& m, const vec<cols>& v) {
    vec<rows> result;
    for (int i = 0; i < rows; i++) {
        double sum = 0.0;
        for (int j = 0; j < cols; j++) {
            sum += m(i, j) * v[j];
        }
        result[i] = sum;
    }
    return result;
}

template<int rows, int cols>
mat<rows, cols> operator*(const mat<rows, cols>& m, double scalar) {
    mat<rows, cols> result;
    for (int i = 0; i < rows * cols; i++) {
        result[i] = m[i] * scalar;
    }
    return result;
}

template<int rows, int cols>
mat<rows, cols> operator*(double scalar, const mat<rows, cols>& m) {
    return m * scalar;
}

template<int rows, int cols>
mat<cols, rows> transpose(const mat<rows, cols>& m) {
    mat<cols, rows> result;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result(j, i) = m(i, j);
        }
    }
    return result;
}

// Identity matrix
template<int n>
mat<n, n> identity() {
    mat<n, n> result;
    for (int i = 0; i < n; i++) {
        result(i, i) = 1.0;
    }
    return result;
}

template<int n>
mat<n, n> inverse(const mat<n, n>& m) {
    mat<n, n> result;
    mat<n, n> temp = m;

    for (int i = 0; i < n; i++) {
        result(i, i) = 1.0;
    }

    // Forward elimination with partial pivoting
    for (int i = 0; i < n; i++) {
        // Find pivot
        int max_row = i;
        double max_val = std::abs(temp(i, i));
        for (int k = i + 1; k < n; k++) {
            if (std::abs(temp(k, i)) > max_val) {
                max_val = std::abs(temp(k, i));
                max_row = k;
            }
        }

        // Swap rows
        if (max_row != i) {
            for (int j = 0; j < n; j++) {
                std::swap(temp(i, j), temp(max_row, j));
                std::swap(result(i, j), result(max_row, j));
            }
        }

        // Check for singular matrix
        if (std::abs(temp(i, i)) < 1e-10) {
            // Matrix is singular or nearly singular
            return mat<n, n>{}; // Return zero matrix
        }

        // Make diagonal element 1
        double pivot = temp(i, i);
        for (int j = 0; j < n; j++) {
            temp(i, j) /= pivot;
            result(i, j) /= pivot;
        }

        // Eliminate column
        for (int k = 0; k < n; k++) {
            if (k != i) {
                double factor = temp(k, i);
                for (int j = 0; j < n; j++) {
                    temp(k, j) -= factor * temp(i, j);
                    result(k, j) -= factor * result(i, j);
                }
            }
        }
    }

    return result;
}

typedef mat<2, 2> mat2;
typedef mat<3, 3> mat3;
typedef mat<4, 4> mat4;
typedef mat<3, 4> mat3x4;
typedef mat<4, 3> mat4x3;