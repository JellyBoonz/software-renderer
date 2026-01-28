#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "geometry.h"
#include "point.h"

class file_parser {
public:
    file_parser() = default;


    bool load(const char* path) {
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cout << "File could not be opened" << "\n";
            return false;
        }

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
            else if (line.substr(0, 2) == "f ") {
                std::istringstream stream(line.substr(2));
                std::string token;
                std::vector<int> face;

                while (stream >> token) {
                    std::istringstream token_stream(token);

                    int index;
                    token_stream >> index;

                    face.push_back(index);
                }

                faces.push_back(face);
            }
        }

        return true;
    }

    const std::vector<vec3>& get_vertices() {
        return vertices;
    }

    const std::vector<std::vector<int>>& get_faces() {
        return faces;
    }


private:
    std::vector<vec3> vertices;
    std::vector<std::vector<int>> faces;
};

#endif