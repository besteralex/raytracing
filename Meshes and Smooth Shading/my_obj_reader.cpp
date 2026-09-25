// my_obj_reader.cpp
#include "glm/glm.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

class Reader {
public:
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> vertex_indices;
  std::string file_path;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec3> normal_indices;
  bool has_normals; // True when faces include separate normal indices.

  Reader(string file_path, bool has_normals) {
    this->has_normals = has_normals;
    this->file_path = file_path;
    print_file_test();
  }

  void print_file_test() {
    // Read the mesh from the path passed to the constructor.
    const char* file_path_cstr = file_path.c_str();

    FILE *obj_file = fopen(file_path_cstr, "r");

    char record_type[200];

    // OBJ records use v for vertices, vn for normals, and f for faces.
    while (fscanf(obj_file, "%s", record_type) != EOF) {

      if (strcmp(record_type, "v") == 0) {

        glm::vec3 vertex;

        fscanf(obj_file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);

        vertices.push_back(vertex);

      } else if (strcmp(record_type, "f") == 0) {
        if (has_normals) {

            
            // Store the vertex and normal indices for one triangular face.
          int face_indices[6];

          fscanf(obj_file, "%d//%d %d//%d %d//%d \n", &face_indices[0], &face_indices[1], &face_indices[2],&face_indices[3], &face_indices[4], &face_indices[5]);

          // Convert OBJ indices from one-based to zero-based indexing.
          vertex_indices.push_back(glm::vec3((face_indices[0] - 1), (face_indices[2] - 1), (face_indices[4] - 1)));

          normal_indices.push_back(glm::vec3((face_indices[1] - 1), (face_indices[3] - 1), (face_indices[5] - 1)));

        } else {
          int face_indices[3];
          fscanf(obj_file, "%d %d %d \n", &face_indices[0], &face_indices[1], &face_indices[2]);
          vertex_indices.push_back(glm::vec3((face_indices[0] - 1), (face_indices[1] - 1), (face_indices[2] - 1)));
        }
      } else if (strcmp(record_type, "vn") == 0) {
        glm::vec3 normal;
        fscanf(obj_file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
        normals.push_back(normal);
      }
    }

    for(glm::vec3 face_vertices : vertex_indices) {
      cout << face_vertices.x << face_vertices.y << face_vertices.z << endl;
    }

    fclose(obj_file);
  }
};
