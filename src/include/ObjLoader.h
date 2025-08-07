#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class Mesh;
struct Vertex;

class ObjLoader {
public:
    // Load OBJ file with uniform scale
    static std::shared_ptr<Mesh> loadObjFile(const std::string& filepath, float scale);

    // Load OBJ file with per-axis scale
    static std::shared_ptr<Mesh> loadObjFile(const std::string& filepath, const glm::vec3& scale);

    // Load OBJ file without scaling (backward compatibility)
    static std::shared_ptr<Mesh> loadObjFile(const std::string& filepath);

private:
    // Parse OBJ file with uniform scale
    static bool parseObjFile(const std::string& filepath,
                           std::vector<Vertex>& vertices,
                           std::vector<unsigned int>& indices,
                           float scale);

    // Parse OBJ file with per-axis scale
    static bool parseObjFile(const std::string& filepath,
                           std::vector<Vertex>& vertices,
                           std::vector<unsigned int>& indices,
                           const glm::vec3& scale);

    // Parse OBJ file without scaling (backward compatibility)
    static bool parseObjFile(const std::string& filepath,
                           std::vector<Vertex>& vertices,
                           std::vector<unsigned int>& indices);
    
    static void calculateNormals(std::vector<Vertex>& vertices,
                               const std::vector<unsigned int>& indices);
};