#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "geometry.h"
#include "point.h"

struct FaceVertex {
    int v_idx;  // vertex index
    int n_idx;  // normal index (0 if none)
};

using Face = std::vector<FaceVertex>;

class file_parser {
public:
    file_parser() = default;


    bool load(const char* path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cout << "File could not be opened" << "\n";
            return false;
        }

        std::cout << "file opened" << std::endl;

        std::string line;

        while (std::getline(file, line)) {
            if (!(line.size() >= 2)) {
                continue;
            }
            if (line.substr(0, 2) == "v ") {
                float x, y, z;
                std::istringstream stream(line.substr(2));

                if (stream >> x >> y >> z) {
                    vertices.push_back(vec3{ x, y, z });
                }
            }
            else if (line.substr(0, 3) == "vn ") {
                float x, y, z;
                std::istringstream stream(line.substr(3));

                if (stream >> x >> y >> z) {
                    normals.push_back(normalize(vec3{ x, y, z }));
                }
            }
            else if (line.substr(0, 2) == "f ") {
                std::istringstream stream(line.substr(2));
                Face face;
                int v, t, n;
                char trash;

                while (stream >> v >> trash >> t >> trash >> n) {
                    FaceVertex fv;
                    fv.v_idx = v;
                    fv.n_idx = n;
                    face.push_back(fv);
                }

                faces.push_back(face);
            }
        }

        return true;
    }

    const std::vector<Face>& get_faces() {
        return faces;
    }
    const std::vector<vec3>& get_normals() {
        return normals;
    }
    const std::vector<vec3>& get_vertices() {
        return vertices;
    }



private:
    std::vector<Face> faces;
    std::vector<vec3> normals;
    std::vector<vec3> vertices;
};

#endif