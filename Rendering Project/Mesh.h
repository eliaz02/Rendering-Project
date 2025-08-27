#pragma once

#ifndef MESH_H
#define MESH_H

#include <map>
#include <vector>
#include <glad/gl.h> // holds all OpenGL type declarations
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "Texture.h"
#include "Utilities.h"
#include "Shader.h"

// use to keep sync with the shaders
constexpr int POSITION_LOCATION = 0;
constexpr int TEX_COORD_LOCATION = 1;
constexpr int NORMAL_LOCATION = 2;
constexpr int TANGENT_LOCATION = 3;
constexpr int BITANGENT_LOCATION = 4;

#define ASSIMP_LOAD_FLAGS aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace

#define INVALID_MATERIAL 0xFFFFFFFF

class BasicMesh
{
public:
    BasicMesh();
    ~BasicMesh();

    bool LoadMesh(const std::string& Filename);
    void SetupInstancedArrays(const std::vector<glm::mat4>& instanceMatrices);
    void Render(const std::shared_ptr<Shader> shader);
    void Render(const Shader& shader);
    void RenderInstanced(const Shader& shader, unsigned int instanceCount = 0);
    void RenderInstanced(const std::shared_ptr<Shader> shader, unsigned int instanceCount = 0);

    class Shape
    {
    public:
        Shape(std::string name) : Name{ name } {}
        virtual ~Shape() {}
        std::string Name;
    };

    class Square : public Shape
    {
    public:
        Square(int size, int textureRepeat) :
            Shape("Square"),
            TextureRepeat{ textureRepeat },
            Size{ size }
        {
        }
        int TextureRepeat;
        int Size;
    };

    class Circle : public Shape
    {
    public:
        Circle(int size, int textureRepeat, float resolution) :
            Shape("Circle"),
            TextureRepeat{ textureRepeat },
            Resolution{ resolution },
            Size{ size }
        {
        }
        int TextureRepeat;
        float Resolution;
        int Size;
    };

    class Bezier : public Shape
    {
    public:
        Bezier(int size, std::vector<glm::vec3>& points, float offset) :
            Shape("Bezier"),
            Offset{ offset },
            Points{ points }
        {
        }
        std::vector<glm::vec3> Points;
        float Offset;
    };

    class BSpline :public Shape
    {
    public:
        BSpline(std::vector<glm::vec3>& points, float offset, float textureScale, bool isColsed = false) :
            Shape("B-Spline"),
            Offset{ offset },
            Points{ points },
            TextureScale{ textureScale },
            IsColsed{ isColsed }
        {
        }
        std::vector<glm::vec3> Points;
        float Offset;
        float TextureScale;
        bool IsColsed;
    };

    void PrintBezier(const Bezier& bezier);
    bool CreatePrimitive(Shape* shape);
    void SetTextures(const std::string& diffusePath, const std::string& specularPath = "",
        const std::string& normalPath = "", const std::string& alphaPath = "");
    void Clear();
private:

    void ClearBuffer();
    bool InitFromScene(const aiScene* pScene, const std::string& Filename);
    void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);
    void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);
    void InitAllMeshes(const aiScene* pScene);
    void InitSingleMesh(const aiMesh* paiMesh);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename);
    void LoadTextures(const std::string& Dir, const aiMaterial* pMaterial, int Index);
    void LoadColors(const aiMaterial* pMaterial, int index);
    void LoadDiffuseTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadSpecularTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadNormalTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadAlphaTexture(const std::string& Dir, const aiMaterial* pMaterial, int MaterialIndex);
    bool CreateSquare(Square square);
    bool CreateCircle(Circle cicle);
    bool CreateBezier(Bezier bezier);
    bool CreateBSpline(BSpline bspline);
    void InitPrimitiveMaterial();
    void PopulateBuffers();

    enum FORMAT_TYPE {
        OBJ = 0,
        GLTF = 1,
        INVALID_FORMAT = 2
    };

    FORMAT_TYPE GetFormatFromFilename(const std::string& Filename) {
        std::string extension = "";
        size_t lastDot = Filename.find_last_of(".");

        if (lastDot != std::string::npos) {
            extension = Filename.substr(lastDot + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (extension == "obj") {
                return FORMAT_TYPE::OBJ;
            }
            else if (extension == "gltf" || extension == "glb") {
                return FORMAT_TYPE::GLTF;
            }
        }
        return FORMAT_TYPE::INVALID_FORMAT;
    }

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB = 1,
        TEXCOORD_VB = 2,
        NORMAL_VB = 3,
        TANGENT_VB = 4,
        BITANGENT_VB = 5,
        NUM_BUFFERS = 6
    };

    struct BasicMeshEntry {
        BasicMeshEntry()
        {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };

    FORMAT_TYPE m_FileFormat;
    GLuint m_VAO;
    GLuint m_Buffers[NUM_BUFFERS];
    GLuint m_InstanceBuffer;
    unsigned int m_InstanceMatricesSize;


    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<Material> m_Materials;
    std::vector<glm::vec3> m_Positions;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<glm::vec3> m_Tangents;
    std::vector<glm::vec3> m_Bitangents;
    std::vector<unsigned int> m_Indices;

    const aiScene* pScene;

    friend std::ostream& operator<< (std::ostream& out, const BasicMesh& point);
};

#endif