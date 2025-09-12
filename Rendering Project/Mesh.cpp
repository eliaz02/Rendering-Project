#include "Mesh.h"

BasicMesh::BasicMesh() :
    m_VAO{ 0 },
    m_Buffers{ 0 },
    m_InstanceBuffer{ 0 },
    m_FileFormat{ INVALID_FORMAT }
{
};

bool BasicMesh::LoadMesh(const std::string& Filename)
{
    // Release the previously loaded mesh (if it exists)
    Clear();

    // Create the VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // Create the buffers for the vertices attributes
    glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

    // Determine file format from extension
    m_FileFormat = GetFormatFromFilename(Filename);

    bool Ret = false;
    Assimp::Importer Importer;

    pScene = Importer.ReadFile(Filename.c_str(), ASSIMP_LOAD_FLAGS);


    if (pScene) {
        std::cout << Filename << std::endl;
        Ret = InitFromScene(pScene, Filename);
    }
    else {
        std::cout << "Error parsing " << Filename.c_str() << ": " << Importer.GetErrorString() << "\n";
    }

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
    GL_CHECK();

    return Ret;

}

bool BasicMesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{
    // since usualy there is more than one meshes in an object we need to store them and each meshes has is own texture we have to store them too
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    //update NumIndices and NumVertices
    CountVerticesAndIndices(pScene, NumVertices, NumIndices);

    //setup our space for the cpu temp
    ReserveSpace(NumVertices, NumIndices);

    InitAllMeshes(pScene);

    if (!InitMaterials(pScene, Filename)) {
        return false;
    }

    PopulateBuffers();
    GL_CHECK();
    return GLCheckError();
}

void BasicMesh::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices)
{
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3; // triangolation in the ASSIMP_LOAD_FLAGS make this consistent 
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices += m_Meshes[i].NumIndices;
    }
}

void BasicMesh::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices)
{
    m_Positions.reserve(NumVertices);
    m_Normals.reserve(NumVertices);
    m_TexCoords.reserve(NumVertices);
    m_Tangents.reserve(NumVertices);
    m_Bitangents.reserve(NumVertices);
    m_Indices.reserve(NumIndices);
}

void BasicMesh::InitAllMeshes(const aiScene* pScene)
{
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitSingleMesh(paiMesh);
    }
    GL_CHECK();
}

void BasicMesh::InitSingleMesh(const aiMesh* paiMesh)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    // Populate the vertex attribute vectors
    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D& pPos = paiMesh->mVertices[i];
        const aiVector3D& pNormal = paiMesh->mNormals[i];
        const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
        const aiVector3D& pTangent = paiMesh->HasTangentsAndBitangents() ? paiMesh->mTangents[i] : Zero3D;
        const aiVector3D& pBitangent = paiMesh->HasTangentsAndBitangents() ? paiMesh->mBitangents[i] : Zero3D;

        m_Positions.push_back(glm::vec3(pPos.x, pPos.y, pPos.z));
        m_Normals.push_back(glm::vec3(pNormal.x, pNormal.y, pNormal.z));
        m_TexCoords.push_back(glm::vec2(pTexCoord.x, pTexCoord.y));
        m_Tangents.push_back(glm::vec3(pTangent.x, pTangent.y, pTangent.z));
        m_Bitangents.push_back(glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z));
    }

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
    {
        const aiFace& Face = paiMesh->mFaces[i];

        if (Face.mNumIndices >= 3)
        {
            m_Indices.push_back(Face.mIndices[0]);
            m_Indices.push_back(Face.mIndices[1]);
            m_Indices.push_back(Face.mIndices[2]);
        }
        else {
            // Log warning or handle unexpected case
            std::cout << "Warning: Face " << i << " has " << Face.mNumIndices << " indices instead of 3" << std::endl;
        }
    }
}

bool BasicMesh::InitMaterials(const aiScene* pScene, const std::string& Filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex = Filename.find_last_of("/");
    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;

    //printf("Num materials: %d\n", pScene->mNumMaterials);

    // Initialize the materials
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        LoadTextures(Dir, pMaterial, i);

        // i load the color to in the case the object don't use the texture, 
        // i don't think that this case will ever occur in the final project but i writen this for the first testing could be usefull in some case 
        LoadColors(pMaterial, i);
    }

    return Ret;
}

void BasicMesh::LoadDiffuseTexture(const std::string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pDiffuse = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString Path;
        std::cout << "diffuse texture number " << pMaterial->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
        if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            std::string FullPath = GetFullPath(Dir, Path);
            std::cout << " Name : " << FullPath.c_str() << std::endl;
            m_Materials[MaterialIndex].pDiffuse = new Texture(FullPath.c_str(), COLOR_TEXTURE_UNIT);
        }
    }
}

void BasicMesh::LoadSpecularTexture(const std::string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pSpecularExponent = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString Path;
        std::cout << "specular texture number " << pMaterial->GetTextureCount(aiTextureType_SHININESS) << std::endl;
        if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
        {
            std::string FullPath = GetFullPath(Dir, Path);
            std::cout << " Name : " << FullPath.c_str() << std::endl;
            m_Materials[MaterialIndex].pSpecularExponent = new Texture(FullPath.c_str(), SPECULAR_EXPONENT_UNIT);
        }
    }
}

void BasicMesh::LoadNormalTexture(const std::string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    /// i use aiTextureType_internal instaed of aiTextureType_normal since obj format its a bit wierd behavior when interact with assimp
    m_Materials[MaterialIndex].pNormal = nullptr;
    aiTextureType aiTextureType_internal = aiTextureType_UNKNOWN;
    if (m_FileFormat == OBJ)
        aiTextureType_internal = aiTextureType_HEIGHT;
    if (m_FileFormat == GLTF)
        aiTextureType_internal = aiTextureType_NORMALS;

    if (pMaterial->GetTextureCount(aiTextureType_internal) > 0) {
        aiString Path;
        std::cout << "height texture number " << pMaterial->GetTextureCount(aiTextureType_internal) << std::endl;
        if (pMaterial->GetTexture(aiTextureType_internal, 0, &Path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
        {
            std::string FullPath = GetFullPath(Dir, Path);
            std::cout << " Name : " << FullPath.c_str() << std::endl;
            m_Materials[MaterialIndex].pNormal = new Texture(FullPath.c_str(), NORMAL_TEXTURE_UNIT);
        }
    }
}

void BasicMesh::LoadAlphaTexture(const std::string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pAlpha = nullptr;
    if (pMaterial->GetTextureCount(aiTextureType_OPACITY) > 0) {
        std::cout << "alpha texture number " << pMaterial->GetTextureCount(aiTextureType_OPACITY) << std::endl;
        aiString Path;
        if (pMaterial->GetTexture(aiTextureType_OPACITY, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            std::string FullPath = GetFullPath(Dir, Path);
            std::cout << " Name : " << FullPath.c_str() << std::endl;
            m_Materials[MaterialIndex].pAlpha = new Texture(FullPath.c_str(), ALPHA_TEXTURE_UNIT);
        }
    }
}

void BasicMesh::LoadColors(const aiMaterial* pMaterial, int index)
{
    aiColor4D AmbientColor(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 AllOnes(1.0f);

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        //printf("Shading model %d\n", ShadingModel); 
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
        //printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b); 
        m_Materials[index].AmbientColor.r = AmbientColor.r;
        m_Materials[index].AmbientColor.g = AmbientColor.g;
        m_Materials[index].AmbientColor.b = AmbientColor.b;
    }
    else {
        m_Materials[index].AmbientColor = AllOnes;
    }

    aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
        //printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
        m_Materials[index].DiffuseColor.r = DiffuseColor.r;
        m_Materials[index].DiffuseColor.g = DiffuseColor.g;
        m_Materials[index].DiffuseColor.b = DiffuseColor.b;
    }

    aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
        //printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g, SpecularColor.b);
        m_Materials[index].SpecularColor.r = SpecularColor.r;
        m_Materials[index].SpecularColor.g = SpecularColor.g;
        m_Materials[index].SpecularColor.b = SpecularColor.b;
    }
    float Shininess = 0.0f;
    if (pMaterial->Get(AI_MATKEY_SHININESS, Shininess) == AI_SUCCESS) {
        m_Materials[index].Shininess = std::max(Shininess, 16.0f);
    }
}

void BasicMesh::LoadTextures(const std::string& Dir, const aiMaterial* pMaterial, int Index)
{
    LoadDiffuseTexture(Dir, pMaterial, Index);
    LoadSpecularTexture(Dir, pMaterial, Index);
    LoadNormalTexture(Dir, pMaterial, Index);
    LoadAlphaTexture(Dir, pMaterial, Index);
    // PBR
    /*LoadAlbedoTexture(Dir, pMaterial, Index);
    LoadMetalnessTexture(Dir, pMaterial, Index);
    LoadRoughnessTexture(Dir, pMaterial, Index);
    */
}

// use struct of array for populating the buffer 
void BasicMesh::PopulateBuffers()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Positions[0]) * m_Positions.size(), &m_Positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords[0]) * m_TexCoords.size(), &m_TexCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Normals[0]) * m_Normals.size(), &m_Normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TANGENT_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Tangents[0]) * m_Tangents.size(), &m_Tangents[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TANGENT_LOCATION);
    glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BITANGENT_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_Bitangents[0]) * m_Bitangents.size(), &m_Bitangents[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(BITANGENT_LOCATION);
    glVertexAttribPointer(BITANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_Indices.size(), &m_Indices[0], GL_STATIC_DRAW);
}

void BasicMesh::Clear()
{
    // This is now the key part. Clearing the vectors triggers all the destructors.
    m_Materials.clear();
    m_Meshes.clear();

    // The rest of your cleanup is for VAO / VBOs
    if (m_Buffers[0] != 0) {
        glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
        // Make it idempotent
        for (int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(m_Buffers); ++i) m_Buffers[i] = 0;
    }

    if (m_InstanceBuffer != 0) {
        glDeleteBuffers(1, &m_InstanceBuffer);
        m_InstanceBuffer = 0;
    }

    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}
void BasicMesh::ClearBuffer()
{
    m_Positions.clear();
    m_Normals.clear();
    m_TexCoords.clear();
    m_Tangents.clear();
    m_Bitangents.clear();
    m_Indices.clear();
}

BasicMesh::~BasicMesh()
{
    Clear();
}

void BasicMesh::Render(const std::shared_ptr<Shader> shader)
{
    Render(*shader);
}

void BasicMesh::Render(const Shader& shader)
{
    glBindVertexArray(m_VAO);

    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        unsigned int MaterialIndex = m_Meshes[i].MaterialIndex;

        if (m_Materials[MaterialIndex].pDiffuse != nullptr)
        {
            m_Materials[MaterialIndex].pDiffuse->Bind();
            shader.setInt("material.diffuse", COLOR_TEXTURE_UNIT);
        }

        if (m_Materials[MaterialIndex].pSpecularExponent != nullptr)
        {
            m_Materials[MaterialIndex].pSpecularExponent->Bind();
            shader.setInt("material.specular", SPECULAR_EXPONENT_UNIT);
        }

        if (m_Materials[MaterialIndex].pNormal != nullptr)
        {
            m_Materials[MaterialIndex].pNormal->Bind();
            shader.setInt("material.normal", NORMAL_TEXTURE_UNIT);
        }

        if (m_Materials[MaterialIndex].pAlpha != nullptr)
        {
            m_Materials[MaterialIndex].pAlpha->Bind();
            shader.setInt("material.alpha", ALPHA_TEXTURE_UNIT);
        }

        shader.setBool("hasDiffuseTexture", m_Materials[MaterialIndex].pDiffuse != nullptr);
        shader.setBool("hasSpecularTexture", m_Materials[MaterialIndex].pSpecularExponent != nullptr);
        shader.setBool("hasNormalTexture", m_Materials[MaterialIndex].pNormal != nullptr);
        shader.setBool("hasAlphaTexture", m_Materials[MaterialIndex].pAlpha != nullptr);

        shader.setVec3("material.diffuseColor", m_Materials[MaterialIndex].DiffuseColor);
        shader.setVec3("material.ambientColor", m_Materials[MaterialIndex].AmbientColor);
        shader.setVec3("material.specularColor", m_Materials[MaterialIndex].SpecularColor);
        shader.setFloat("material.shininess", m_Materials[MaterialIndex].Shininess);


        glDrawElementsBaseVertex(GL_TRIANGLES,
            m_Meshes[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * m_Meshes[i].BaseIndex),
            m_Meshes[i].BaseVertex);

        if (m_Materials[MaterialIndex].pDiffuse != nullptr)
            m_Materials[MaterialIndex].pDiffuse->Unbind();

        if (m_Materials[MaterialIndex].pSpecularExponent != nullptr)
            m_Materials[MaterialIndex].pSpecularExponent->Unbind();

        if (m_Materials[MaterialIndex].pNormal != nullptr)
            m_Materials[MaterialIndex].pNormal->Unbind();

        if (m_Materials[MaterialIndex].pAlpha != nullptr)
            m_Materials[MaterialIndex].pAlpha->Unbind();

    }

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
}

void BasicMesh::SetupInstancedArrays(const std::vector<glm::mat4>& instanceMatrices) {
    if (instanceMatrices.empty()) return;

    if (m_InstanceBuffer == 0) {
        glGenBuffers(1, &m_InstanceBuffer);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_STATIC_DRAW);

    // Set up vertex attributes for each column of the mat4
    GLsizei vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i) {
        GLuint location = NUM_BUFFERS - 1 + i; // Use attribute locations 5-8
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, NUM_BUFFERS - 2, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(uintptr_t)(i * vec4Size));
        glVertexAttribDivisor(location, 1); // Update once per instance
    }

    glBindVertexArray(0);
    m_InstanceMatricesSize = static_cast<unsigned int>(instanceMatrices.size()); // Optional: Store if needed later

    GL_CHECK();
}

void BasicMesh::RenderInstanced(const Shader& shader, unsigned int instanceCount) {
    if (instanceCount == 0)
        instanceCount = m_InstanceMatricesSize;

    if (instanceCount > m_InstanceMatricesSize) // avoid render object without a model matrix 
        instanceCount = m_InstanceMatricesSize;

    glBindVertexArray(m_VAO);

    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        unsigned int MaterialIndex = m_Meshes[i].MaterialIndex;

        if (m_Materials[MaterialIndex].pDiffuse != nullptr) {
            m_Materials[MaterialIndex].pDiffuse->Bind();
            shader.setInt("material.diffuse", COLOR_TEXTURE_UNIT);
        }

        if (m_Materials[MaterialIndex].pSpecularExponent != nullptr) {
            m_Materials[MaterialIndex].pSpecularExponent->Bind();
            shader.setInt("material.specular", SPECULAR_EXPONENT_UNIT);
        }

        if (m_Materials[MaterialIndex].pNormal)
        {
            m_Materials[MaterialIndex].pNormal->Bind();
            shader.setInt("material.normal", NORMAL_TEXTURE_UNIT);
        }

        if (m_Materials[MaterialIndex].pAlpha)
        {
            m_Materials[MaterialIndex].pAlpha->Bind();
            shader.setInt("material.alpha", ALPHA_TEXTURE_UNIT);
        }

        shader.setBool("hasDiffuseTexture", m_Materials[MaterialIndex].pDiffuse != nullptr);
        shader.setBool("hasSpecularTexture", m_Materials[MaterialIndex].pSpecularExponent != nullptr);
        shader.setBool("hasNormalTexture", m_Materials[MaterialIndex].pNormal != nullptr);
        shader.setBool("hasAlphaTexture", m_Materials[MaterialIndex].pAlpha != nullptr);

        shader.setVec3("material.diffuseColor", m_Materials[MaterialIndex].DiffuseColor);
        shader.setVec3("material.ambientColor", m_Materials[MaterialIndex].AmbientColor);
        shader.setVec3("material.specularColor", m_Materials[MaterialIndex].SpecularColor);
        shader.setFloat("material.shininess", m_Materials[MaterialIndex].Shininess);

        // Instanced draw call
        glDrawElementsInstancedBaseVertex(
            GL_TRIANGLES,
            m_Meshes[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * m_Meshes[i].BaseIndex),
            instanceCount,
            m_Meshes[i].BaseVertex
        );

        // Unbind textures
        if (m_Materials[MaterialIndex].pDiffuse != nullptr)
            m_Materials[MaterialIndex].pDiffuse->Unbind();

        if (m_Materials[MaterialIndex].pSpecularExponent != nullptr)
            m_Materials[MaterialIndex].pSpecularExponent->Unbind();

        if (m_Materials[MaterialIndex].pNormal != nullptr)
            m_Materials[MaterialIndex].pNormal->Unbind();

        if (m_Materials[MaterialIndex].pAlpha != nullptr)
            m_Materials[MaterialIndex].pAlpha->Unbind();

    }

    glBindVertexArray(0);
    GL_CHECK();
}

// Overload for shared_ptr<Shader>
void BasicMesh::RenderInstanced(const std::shared_ptr<Shader> shader, unsigned int instanceCount) {
    RenderInstanced(*shader, instanceCount);
}

bool BasicMesh::CreatePrimitive(Shape* shape)
{
    // Release the previously loaded mesh (if it exists)
    Clear();

    // Create the VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    // Create the buffers for the vertices attributes
    glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

    // Create a single mesh entry for our primitive
    m_Meshes.resize(1);
    m_Meshes[0].MaterialIndex = 0;
    m_Meshes[0].BaseVertex = 0;
    m_Meshes[0].BaseIndex = 0;

    // Create a single material entry
    m_Materials.resize(1);
    InitPrimitiveMaterial();

    if (shape->Name == "Square") {
        ;
        Square* square = dynamic_cast<Square*>(shape);
        CreateSquare(*square);  // Pass the Square object
    }
    else if (shape->Name == "Circle") {
        Circle* circle = dynamic_cast<Circle*>(shape);
        CreateCircle(*circle);  // Pass the Circle object
    }
    else if (shape->Name == "Bezier") {
        Bezier* curve = dynamic_cast<Bezier*>(shape);
        PrintBezier(*curve);
        CreateBezier(*curve);   // Pass the bezier object
    }
    else if (shape->Name == "B-Spline") {
        BSpline* curve = dynamic_cast<BSpline*>(shape);
        CreateBSpline(*curve);   // Pass the bezier object
    }
    else if (shape->Name == "Cube") { // <-- ADDED
        Cube* cube = dynamic_cast<Cube*>(shape);
        CreateCube(*cube);   // Pass the cube object
    }
    else {
        // Handle unknown shape type
        return false;
    }


    // Update the number of indices in our mesh entry
    m_Meshes[0].NumIndices = static_cast<unsigned int>(m_Indices.size());

    // Load data to GPU
    PopulateBuffers();

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
    GL_CHECK();

    return true;
}

bool BasicMesh::CreateSquare(Square square)
{
    // Clear existing data
    ClearBuffer();

    // Half size for vertex positioning
    float halfSize = square.Size / 2.0f;

    // Vertex positions (4 corners of a square)
    m_Positions = {
        glm::vec3(-halfSize, -halfSize, 0.0f), // bottom-left
        glm::vec3(halfSize, -halfSize, 0.0f), // bottom-right
        glm::vec3(halfSize,  halfSize, 0.0f), // top-right
        glm::vec3(-halfSize,  halfSize, 0.0f)  // top-left
    };

    // Texture coordinates with repetition
    m_TexCoords = {
        glm::vec2(0.0f, 0.0f),                   // bottom-left
        glm::vec2(square.TextureRepeat, 0.0f),          // bottom-right
        glm::vec2(square.TextureRepeat, square.TextureRepeat), // top-right
        glm::vec2(0.0f, square.TextureRepeat)           // top-left
    };

    // Normal vectors (all pointing in +Z direction for a flat square)
    m_Normals = {
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };

    // Tangents (X direction for standard tangent space)
    m_Tangents = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    };

    // Bitangents (Y direction for standard tangent space)
    m_Bitangents = {
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    };

    // Indices for two triangles forming the square
    m_Indices = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };
    return true;
}

bool BasicMesh::CreateCircle(Circle circle)
{
    // Clear existing data
    ClearBuffer();

    float radius = circle.Size / 2.0f;

    // Generate vertices around the circle
    for (int i = 0; i <= circle.Resolution; ++i) {
        float angle1 = 2.0f * glm::pi<float>() * i / circle.Resolution;
        float angle2 = 2.0f * glm::pi<float>() * (i + 1) / circle.Resolution;

        // Current and next point on the circle's circumference
        float x1 = radius * cos(angle1);
        float y1 = radius * sin(angle1);
        float x2 = radius * cos(angle2);
        float y2 = radius * sin(angle2);

        // Texture coordinates for current segment
        float tx1 = (cos(angle1) * 0.5f + 0.5f) * circle.TextureRepeat;
        float ty1 = (sin(angle1) * 0.5f + 0.5f) * circle.TextureRepeat;
        float tx2 = (cos(angle2) * 0.5f + 0.5f) * circle.TextureRepeat;
        float ty2 = (sin(angle2) * 0.5f + 0.5f) * circle.TextureRepeat;

        // Vertices for the rectangular part of the segment
        glm::vec3 bottomLeft(0.0f, 0.0f, 0.0f);
        glm::vec3 bottomRight(x1, y1, 0.0f);
        glm::vec3 topRight(x2, y2, 0.0f);

        // Add rectangular segment vertices
        m_Positions.push_back(bottomLeft);
        m_Positions.push_back(bottomRight);
        m_Positions.push_back(topRight);

        // Texture coordinates for rectangular segment
        m_TexCoords.push_back(glm::vec2(0.5f, 0.5f)); // Center
        m_TexCoords.push_back(glm::vec2(tx1, ty1));   // First point
        m_TexCoords.push_back(glm::vec2(tx2, ty2));   // Second point

        // Normals (all pointing in +Z direction)
        m_Normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        m_Normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        m_Normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));

        // Tangents and Bitangents
        m_Tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
        m_Tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
        m_Tangents.push_back(glm::vec3(1.0f, 0.0f, 0.0f));

        m_Bitangents.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        m_Bitangents.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        m_Bitangents.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

        // Indices for the rectangular segment
        m_Indices.push_back(i * 3);     // Center vertex
        m_Indices.push_back(i * 3 + 1); // First point
        m_Indices.push_back(i * 3 + 2); // Second point
    }
    return true;
}

bool BasicMesh::CreateBezier(Bezier bezier)
{
    // Check for valid number of control points
    if (!((bezier.Points.size() - 1) / 3 >= 1))
    {
        std::cout << "Error invalid point number in the Bezier surface" << std::endl;
        return false;
    }

    // Clear existing data
    ClearBuffer();

    std::vector<float> sample{ 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };

    unsigned int curveNumber = static_cast<unsigned int>(bezier.Points.size() - 2) / 2;
    // Calculate Bézier curve points, tangent vectors, and normal vectors
    std::vector<glm::vec3> bezierPoints(sample.size() * curveNumber, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> tangentVectors(sample.size() * curveNumber, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> normalVectors(sample.size() * curveNumber, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> binormalVectors(sample.size() * curveNumber, glm::vec3(0.0f, 0.0f, 0.0f));

    // World up vector (parallel to most of your tangents - typically world up is a good choice)
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    for (int i = 0; i < sample.size(); ++i)
    {
        // STEP 1: Calculate the point on the Bézier curve
        float t = sample[i];
        float t2 = t * t;
        float t3 = t2 * t;
        float coef0 = -t3 + 3 * t2 - 3 * t + 1;
        float coef1 = 3 * t3 - 6 * t2 + 3 * t;
        float coef2 = -3 * t3 + 3 * t2;
        float coef3 = t3;

        bezierPoints[i] =
            bezier.Points[0] * coef0 +
            bezier.Points[1] * coef1 +
            bezier.Points[2] * coef2 +
            bezier.Points[3] * coef3;

        // STEP 2: Calculate the tangent vector (derivative)
        float tcoef0 = -3 * t2 + 6 * t - 3;       // Derivative of coef0
        float tcoef1 = 9 * t2 - 12 * t + 3;       // Derivative of coef1
        float tcoef2 = -9 * t2 + 6 * t;           // Derivative of coef2
        float tcoef3 = 3 * t2;                    // Derivative of coef3

        tangentVectors[i] =
            bezier.Points[0] * tcoef0 +
            bezier.Points[1] * tcoef1 +
            bezier.Points[2] * tcoef2 +
            bezier.Points[3] * tcoef3;

        // Normalize the tangent vector
        if (glm::length(tangentVectors[i]) > 0.0001f) {
            tangentVectors[i] = glm::normalize(tangentVectors[i]);
        }

        // STEP 3: Calculate the normal vector
        // First, create a binormal by crossing the world up with the tangent
        binormalVectors[i] = glm::cross(worldUp, tangentVectors[i]);

        // Handle the case where tangent and worldUp are parallel
        if (glm::length(binormalVectors[i]) < 0.0001f) {
            // Try a different up vector
            binormalVectors[i] = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), tangentVectors[i]);
        }

        // Normalize the binormal 
        if (glm::length(binormalVectors[i]) > 0.0001f) {
            binormalVectors[i] = glm::normalize(binormalVectors[i]);
        }

        // The normal is the cross product of the tangent and binormal
        normalVectors[i] = glm::normalize(glm::cross(tangentVectors[i], binormalVectors[i]));
    }

    // Build the mesh data
    for (size_t i = 0; i < sample.size(); i++)
    {
        // the first 2 vertices of the new curve are the last 2 of the previus one 

        // For each point on the curve, create two vertices (left and right of the curve)
        // point - offset * normal 
        m_Positions.push_back(bezierPoints[i] - bezier.Offset * binormalVectors[i]);
        // point + offset * normal 
        m_Positions.push_back(bezierPoints[i] + bezier.Offset * binormalVectors[i]);

        // Normals (pointing outward from the curve)
        m_Normals.push_back(normalVectors[i]);
        m_Normals.push_back(normalVectors[i]);

        // Tangents (along the curve)
        m_Tangents.push_back(tangentVectors[i]);
        m_Tangents.push_back(tangentVectors[i]);

        // Bitangents (perpendicular to curve and normal)
        m_Bitangents.push_back(binormalVectors[i]);
        m_Bitangents.push_back(binormalVectors[i]);

        // Texture coordinates (U varies along the curve, V goes from 0 to 1 across the width)
        m_TexCoords.push_back(glm::vec2((i) / float(sample.size() - 1), 0.0f));
        m_TexCoords.push_back(glm::vec2((i) / float(sample.size() - 1), 1.0f));
    }

    // Create indices for triangles
    for (int i = 0; i < sample.size() - 1; i++)
    {
        int baseIdx = i * 2;

        // First triangle (bottom-left, bottom-right, top-right)
        m_Indices.push_back(baseIdx);
        m_Indices.push_back(baseIdx + 2);
        m_Indices.push_back(baseIdx + 3);

        // Second triangle (bottom-left, top-right, top-left)
        m_Indices.push_back(baseIdx);
        m_Indices.push_back(baseIdx + 3);
        m_Indices.push_back(baseIdx + 1);
    }

    return true;
}

bool BasicMesh::CreateBSpline(BSpline bspline)
{
    // Check for valid number of control points
    if (bspline.Points.size() < 4)
    {
        std::cout << "Error: not enough point for a B-spline" << std::endl;
        return false;
    }

    // Clear existing data
    ClearBuffer();

    std::vector<float> sample{ 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };

    // Prepare control points for closed or open curve
    std::vector<glm::vec3> workingPoints;

    if (bspline.IsColsed) {
        // For closed B-spline, we need to extend the control points array
        // Remove duplicate last point if it exists (same as first point)
        std::vector<glm::vec3> uniquePoints = bspline.Points;
        if (glm::length(bspline.Points.front() - bspline.Points.back()) < 0.0001f) {
            uniquePoints.pop_back(); // Remove duplicate last point
        }

        // Add last 3 points to the beginning
        int n = static_cast<unsigned int> (uniquePoints.size());
        workingPoints.push_back(uniquePoints[n - 3]);
        workingPoints.push_back(uniquePoints[n - 2]);
        workingPoints.push_back(uniquePoints[n - 1]);

        // Add all original points
        for (const auto& point : uniquePoints) {
            workingPoints.push_back(point);
        }

        // Add first 3 points to the end
        workingPoints.push_back(uniquePoints[0]);
        workingPoints.push_back(uniquePoints[1]);
        workingPoints.push_back(uniquePoints[2]);
    }
    else {
        // For open B-spline, use points as-is
        workingPoints = bspline.Points;
    }

    // Calculate number of curves
    unsigned int curveNumber = static_cast<unsigned int> (workingPoints.size()) - 3;

    // Calculate total points needed
    unsigned int totalSamplePoints = static_cast<unsigned int> (sample.size()) * curveNumber;

    // Calculate B-spline curve points, tangent vectors, and normal vectors
    std::vector<glm::vec3> curvePoints(totalSamplePoints, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> tangentVectors(totalSamplePoints, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> normalVectors(totalSamplePoints, glm::vec3(0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> binormalVectors(totalSamplePoints, glm::vec3(0.0f, 0.0f, 0.0f));

    // World up vector
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    for (unsigned int basePointIdx = 0; basePointIdx < curveNumber; ++basePointIdx)
    {
        // Get the 4 control points for this curve
        glm::vec3 P0 = workingPoints[basePointIdx];
        glm::vec3 P1 = workingPoints[basePointIdx + 1];
        glm::vec3 P2 = workingPoints[basePointIdx + 2];
        glm::vec3 P3 = workingPoints[basePointIdx + 3];

        for (int i = 0; i < sample.size(); ++i)
        {
            unsigned int arrayIdx = basePointIdx * static_cast<unsigned int>(sample.size()) + i;

            // STEP 1: Calculate the point on the B-spline curve
            float t = sample[i];
            float t2 = t * t;
            float t3 = t2 * t;
            float coef0 = (-t3 + 3 * t2 - 3 * t + 1) / 6.0f;
            float coef1 = (3 * t3 - 6 * t2 + 4) / 6.0f;
            float coef2 = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0f;
            float coef3 = t3 / 6.0f;

            //compute the polynomial for the position in the sample point
            curvePoints[arrayIdx] = P0 * coef0 + P1 * coef1 + P2 * coef2 + P3 * coef3;

            // STEP 2: Calculate the tangent vector (derivative)
            float dcoef0 = (-3 * t2 + 6 * t - 3) / 6.0f;
            float dcoef1 = (9 * t2 - 12 * t) / 6.0f;
            float dcoef2 = (-9 * t2 + 6 * t + 3) / 6.0f;
            float dcoef3 = (3 * t2) / 6.0f;

            //compute the polynomial for the tangent in the sample point
            tangentVectors[arrayIdx] = P0 * dcoef0 + P1 * dcoef1 + P2 * dcoef2 + P3 * dcoef3;

            // Normalize the tangent vector
            if (glm::length(tangentVectors[arrayIdx]) > 0.0001f) {
                tangentVectors[arrayIdx] = glm::normalize(tangentVectors[arrayIdx]);
            }

            // STEP 3: Calculate the normal vector
            // First, create a binormal by crossing the world up with the tangent
            binormalVectors[arrayIdx] = glm::cross(worldUp, tangentVectors[arrayIdx]);

            // Handle the case where tangent and worldUp are parallel
            if (glm::length(binormalVectors[arrayIdx]) < 0.0001f) {
                // Try a different up vector
                binormalVectors[arrayIdx] = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), tangentVectors[arrayIdx]);
            }

            // Calculate normal as cross product of binormal and tangent
            normalVectors[arrayIdx] = glm::cross(binormalVectors[arrayIdx], tangentVectors[arrayIdx]);

            // Normalize normal
            if (glm::length(normalVectors[arrayIdx]) > 0.0001f) {
                normalVectors[arrayIdx] = glm::normalize(normalVectors[arrayIdx]);
            }
        }
    }

    // Build the mesh data
    float totalLength = 0.0f;
    std::vector<float> cumulativeLength(totalSamplePoints, 0.0f);

    // Calculate cumulative length for proper texture coordinate mapping
    for (unsigned int i = 1; i < totalSamplePoints; i++) {
        float segmentLength = glm::length(curvePoints[i] - curvePoints[i - 1]);
        totalLength += segmentLength;
        cumulativeLength[i] = totalLength;
    }

    for (unsigned int i = 0; i < totalSamplePoints; i++)
    {
        // For each point on the curve, create two vertices (left and right of the curve)
        // point - offset * binormal 
        m_Positions.push_back(curvePoints[i] - bspline.Offset * binormalVectors[i]);
        // point + offset * binormal 
        m_Positions.push_back(curvePoints[i] + bspline.Offset * binormalVectors[i]);

        // Normals (pointing outward from the curve)
        m_Normals.push_back(normalVectors[i]);
        m_Normals.push_back(normalVectors[i]);

        // Tangents (along the curve)
        m_Tangents.push_back(tangentVectors[i]);
        m_Tangents.push_back(tangentVectors[i]);

        // Bitangents (perpendicular to curve and normal)
        m_Bitangents.push_back(binormalVectors[i]);
        m_Bitangents.push_back(binormalVectors[i]);

        // Texture coordinates for tileable texture
        // Use actual distance for U coordinate to maintain consistent texture scale
        float u = cumulativeLength[i] / bspline.TextureScale;
        m_TexCoords.push_back(glm::vec2(u, 0.0f));
        m_TexCoords.push_back(glm::vec2(u, 1.0f));
    }

    // Create indices for triangles
    int indexLimit = totalSamplePoints - 1;

    // For closed curves, we need to connect the last segment back to the first
    if (bspline.IsColsed) {
        indexLimit = totalSamplePoints; // Include the wraparound connection
    }

    for (int i = 0; i < indexLimit; i++)
    {
        int baseIdx = i * 2;
        int nextBaseIdx = ((i + 1) % totalSamplePoints) * 2; // Wrap around for closed curves

        // First triangle (bottom-left, bottom-right, top-right)
        m_Indices.push_back(baseIdx);
        m_Indices.push_back(nextBaseIdx);
        m_Indices.push_back(nextBaseIdx + 1);

        // Second triangle (bottom-left, top-right, top-left)
        m_Indices.push_back(baseIdx);
        m_Indices.push_back(nextBaseIdx + 1);
        m_Indices.push_back(baseIdx + 1);
    }

    return true;
}

bool BasicMesh::CreateCube(Cube cube)
{
    ClearBuffer();

    float halfSize = static_cast<float>(cube.size) / 2.0f;

    // Define the 8 vertices of the cube
    glm::vec3 v0(-halfSize, -halfSize, halfSize);  // Front bottom left
    glm::vec3 v1(halfSize, -halfSize, halfSize);   // Front bottom right
    glm::vec3 v2(halfSize, halfSize, halfSize);    // Front top right
    glm::vec3 v3(-halfSize, halfSize, halfSize);   // Front top left
    glm::vec3 v4(-halfSize, -halfSize, -halfSize); // Back bottom left
    glm::vec3 v5(halfSize, -halfSize, -halfSize);  // Back bottom right
    glm::vec3 v6(halfSize, halfSize, -halfSize);   // Back top right
    glm::vec3 v7(-halfSize, halfSize, -halfSize);  // Back top left

    // Define texture coordinates
    glm::vec2 t0(0.0f, 0.0f); // Bottom-left
    glm::vec2 t1(1.0f, 0.0f); // Bottom-right
    glm::vec2 t2(1.0f, 1.0f); // Top-right
    glm::vec2 t3(0.0f, 1.0f); // Top-left

    // Each face has 4 vertices
    m_Positions = {
        // Front face
        v0, v1, v2, v3,
        // Back face
        v5, v4, v7, v6,
        // Left face
        v4, v0, v3, v7,
        // Right face
        v1, v5, v6, v2,
        // Top face
        v3, v2, v6, v7,
        // Bottom face
        v4, v5, v1, v0
    };

    m_TexCoords = {
        // Front face
        t0, t1, t2, t3,
        // Back face
        t0, t1, t2, t3,
        // Left face
        t0, t1, t2, t3,
        // Right face
        t0, t1, t2, t3,
        // Top face
        t0, t1, t2, t3,
        // Bottom face
        t0, t1, t2, t3
    };

    // Define normals for each face
    glm::vec3 frontNormal(0.0f, 0.0f, 1.0f);
    glm::vec3 backNormal(0.0f, 0.0f, -1.0f);
    glm::vec3 leftNormal(-1.0f, 0.0f, 0.0f);
    glm::vec3 rightNormal(1.0f, 0.0f, 0.0f);
    glm::vec3 topNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);

    m_Normals = {
        // Front face
        frontNormal, frontNormal, frontNormal, frontNormal,
        // Back face
        backNormal, backNormal, backNormal, backNormal,
        // Left face
        leftNormal, leftNormal, leftNormal, leftNormal,
        // Right face
        rightNormal, rightNormal, rightNormal, rightNormal,
        // Top face
        topNormal, topNormal, topNormal, topNormal,
        // Bottom face
        bottomNormal, bottomNormal, bottomNormal, bottomNormal
    };

    // Define tangents for each face
    glm::vec3 frontTangent(1.0f, 0.0f, 0.0f);
    glm::vec3 backTangent(-1.0f, 0.0f, 0.0f);
    glm::vec3 leftTangent(0.0f, 0.0f, -1.0f);
    glm::vec3 rightTangent(0.0f, 0.0f, 1.0f);
    glm::vec3 topTangent(1.0f, 0.0f, 0.0f);
    glm::vec3 bottomTangent(1.0f, 0.0f, 0.0f);

    m_Tangents = {
        // Front face
        frontTangent, frontTangent, frontTangent, frontTangent,
        // Back face
        backTangent, backTangent, backTangent, backTangent,
        // Left face
        leftTangent, leftTangent, leftTangent, leftTangent,
        // Right face
        rightTangent, rightTangent, rightTangent, rightTangent,
        // Top face
        topTangent, topTangent, topTangent, topTangent,
        // Bottom face
        bottomTangent, bottomTangent, bottomTangent, bottomTangent
    };

    // Calculate bitangents by crossing normals and tangents
    for (size_t i = 0; i < m_Normals.size(); ++i) {
        m_Bitangents.push_back(glm::cross(m_Normals[i], m_Tangents[i]));
    }

    // Define indices for the 12 triangles of the cube
    m_Indices = {
        // Front face
        0, 1, 2,  0, 2, 3,
        // Back face
        4, 5, 6,  4, 6, 7,
        // Left face
        8, 9, 10, 8, 10, 11,
        // Right face
        12, 13, 14, 12, 14, 15,
        // Top face
        16, 17, 18, 16, 18, 19,
        // Bottom face
        20, 21, 22, 20, 22, 23
    };

    return true;
}

void BasicMesh::InitPrimitiveMaterial()
{
    // Initialize with default values for materials
    m_Materials[0].AmbientColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    m_Materials[0].DiffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Materials[0].SpecularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Initialize pointers to NULL
    m_Materials[0].pDiffuse = nullptr;
    m_Materials[0].pSpecularExponent = nullptr;
    m_Materials[0].pNormal = nullptr;
    m_Materials[0].pAlpha = nullptr;
}

void BasicMesh::SetTextures(const std::string& diffusePath, const std::string& specularPath,
    const std::string& normalPath, const std::string& alphaPath)
{
    // Make sure we have materials initialized
    if (m_Materials.empty()) {
        m_Materials.resize(1);
        InitPrimitiveMaterial();
    }

    // Set diffuse texture if provided
    if (!diffusePath.empty()) {
        m_Materials[0].pDiffuse = new Texture(diffusePath.c_str(), COLOR_TEXTURE_UNIT);
    }

    // Set specular texture if provided
    if (!specularPath.empty()) {
        m_Materials[0].pSpecularExponent = new Texture(specularPath.c_str(), SPECULAR_EXPONENT_UNIT);
    }

    // Set normal texture if provided
    if (!normalPath.empty()) {
        m_Materials[0].pNormal = new Texture(normalPath.c_str(), NORMAL_TEXTURE_UNIT);
    }

    // Set alpha texture if provided
    if (!alphaPath.empty()) {
        m_Materials[0].pAlpha = new Texture(alphaPath.c_str(), ALPHA_TEXTURE_UNIT);
    }
}

void BasicMesh::PrintBezier(const Bezier& bezier) {
    std::cout << "Shape Name: " << bezier.Name << "\n";
    std::cout << "Offset: " << bezier.Offset << "\n";
    std::cout << "Control Points:\n";

    for (size_t i = 0; i < bezier.Points.size(); ++i) {
        const glm::vec3& p = bezier.Points[i];
        std::cout << "  Point " << i << ": (" << p.x << ", " << p.y << ", " << p.z << ")\n";
    }
}


// overload of the operator << for a clean cout usefull for debugging
std::ostream& operator<< (std::ostream& out, const BasicMesh& mesh)
{
    for (size_t i{ 0 }; i < mesh.m_Meshes.size(); ++i)
    {
        out << "Mesh number: " << i <<
            std::endl << "BaseIndex " << mesh.m_Meshes.at(i).BaseIndex <<
            std::endl << "BaseVertex " << mesh.m_Meshes.at(i).BaseVertex <<
            std::endl << "MaterialIndex " << mesh.m_Meshes.at(i).MaterialIndex <<
            std::endl << "NumIndices " << mesh.m_Meshes.at(i).NumIndices << std::endl;
        // << "hasdiffuse " << mesh.m_Materials[mesh.m_Meshes.at(i).NumIndices].pDiffuse;

    }
    return out;
}
