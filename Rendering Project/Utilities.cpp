#include "Utilities.h"


GLuint loadTexture(const char* path);


// light  
glm::vec3 lightPos(0.5f, 0.0f, 1.8f);


#ifdef shadowdirlight
extern ShadowMapFBO shadowMap;
#endif
// camera
extern Camera camera;

extern bool firstMouse = true;



float randf()
{
    return -1.0f + (rand() / (RAND_MAX / 2.0f));
}

glm::mat4 AssimpToGLMMatrix(const aiMatrix4x4& assimpMatrix)
{
    // Assimp matrices are row-major, while GLM matrices are column-major
    // We need to transpose during the conversion
    return glm::mat4(
        assimpMatrix.a1, assimpMatrix.b1, assimpMatrix.c1, assimpMatrix.d1,
        assimpMatrix.a2, assimpMatrix.b2, assimpMatrix.c2, assimpMatrix.d2,
        assimpMatrix.a3, assimpMatrix.b3, assimpMatrix.c3, assimpMatrix.d3,
        assimpMatrix.a4, assimpMatrix.b4, assimpMatrix.c4, assimpMatrix.d4
    );
}




std::vector <glm::mat4> genInstanceMatrix(int number = 5000, float radius = 10.0f, float radiusDeviation = 2.0f)
{
    std::vector <glm::mat4> instanceMatrix;
    for (int i = 0; i < number; i++)
    {
        // Generates x and y for the function x^2 + y^2 = radius^2 which is a circle
        float x = randf();
        float finalRadius = radius + randf() * radiusDeviation;
        float y = ((rand() % 2) * 2 - 1) * sqrt(1.0f - x * x);

        // Holds transformations before multiplying them
        glm::vec3 tempTranslation;
        glm::quat tempRotation;
        glm::vec3 tempScale;

        // Makes the random distribution more even
        if (randf() > 0.5f)
        {
            // Generates a translation near a circle of radius "radius"
            tempTranslation = glm::vec3(y * finalRadius, randf(), x * finalRadius);
        }
        else
        {
            // Generates a translation near a circle of radius "radius"
            tempTranslation = glm::vec3(x * finalRadius, randf(), y * finalRadius);
        }
        // Generates random rotations
        tempRotation = glm::quat(1.0f, randf(), randf(), randf());
        // Generates random scales
        tempScale = 0.1f * glm::vec3(randf(), randf(), randf());


        // Initialize matrices
        glm::mat4 trans = glm::mat4(1.0f);


        // Transform the matrices to their correct form
        trans = glm::translate(trans, tempTranslation);


        // Push matrix transformation
        instanceMatrix.push_back(trans);
    }
    return instanceMatrix;
}

void printInstanceMatrices(std::vector<glm::mat4>* instanceMatrix) {
    if (instanceMatrix == nullptr) {
        std::cout << "Instance matrix is null" << std::endl;
        return;
    }

    std::cout << "Instance Matrices (" << instanceMatrix->size() << " matrices):" << std::endl;
    for (size_t i = 0; i < instanceMatrix->size(); ++i) {
        std::cout << "Matrix " << i << ":" << std::endl;
        glm::mat4 matrix = (*instanceMatrix)[i];
        std::cout << glm::io::precision(2) << glm::io::width(10); // Formatting
        std::cout << matrix << std::endl;
        std::cout << std::endl;
    }
}


void printMat4(const glm::mat4& matrix, const std::string& name ) {
    std::cout << "\n" << name << ":\n";
    std::cout << "--------------------------------------------------------\n";

    for (int i = 0; i < 4; ++i) {
        std::cout << "|";
        for (int j = 0; j < 4; ++j) {
            std::cout << std::setw(11) << std::fixed << std::setprecision(6)
                << matrix[j][i] << " |"; // Note: GLM uses column-major order
        }
        std::cout << "\n";
        if (i < 3) {
            std::cout << "                                                        \n";
        }
    }
    std::cout << "--------------------------------------------------------\n";
}


std::string GetFullPath(const std::string& Dir, const aiString& Path)
{
    std::string p(Path.data);

    if (p == "C:\\\\") {
        p = "";
    }
    else if (p.substr(0, 2) == ".\\") {
        p = p.substr(2, p.size() - 2);
    }

    std::string FullPath = Dir + "/" + p;

    return FullPath;
};

unsigned int skyboxIndices[] =
{
    // Right
    1, 2, 6,
    6, 5, 1,
    // Left
    0, 4, 7,
    7, 3, 0,
    // Top
    4, 5, 6,
    6, 7, 4,
    // Bottom
    0, 3, 2,
    2, 1, 0,
    // Back
    0, 1, 5,
    5, 4, 0,
    // Front
    3, 7, 6,
    6, 2, 3
};


// base structure
struct MaterialPhong
{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    GLfloat shininess;
};

MaterialPhong materialValues[] = {
    { glm::vec3(0.0215,0.1745,0.0215) ,glm::vec3(0.07568,0.61424,0.07568),glm::vec3(0.633,0.727811,0.633) ,0.6f}, //emerald
    { glm::vec3(0.135,0.2225,0.1575) ,glm::vec3(0.54,0.89,0.63) ,glm::vec3(0.316228,0.316228,0.316228),0.1f}, //jade
    { glm::vec3(0.05375,0.05,0.06625) ,glm::vec3(0.18275,0.17,0.22525) ,glm::vec3(0.332741,0.328634,0.346435),0.3f}, //obsidian
    { glm::vec3(0.25,0.20725,0.20725) ,glm::vec3(1,0.829,0.829) ,glm::vec3(0.296648,0.296648,0.296648),0.088f}, //pearl
    { glm::vec3(0.1745,0.01175,0.01175),glm::vec3(0.61424,0.04136,0.04136),glm::vec3(0.727811,0.626959,0.626959),0.6f}, //ruby
    { glm::vec3(0.1,0.18725,0.1745) ,glm::vec3(0.396,0.74151,0.69102) ,glm::vec3(0.297254,0.30829,0.306678) ,0.1f}, //turquoise
    { glm::vec3(0.329412,0.223529,0.027451),glm::vec3(0.780392,0.568627,0.113725), glm::vec3(0.992157,0.941176,0.807843),0.21794872f}, //brass
    { glm::vec3(0.2125,0.1275,0.054), glm::vec3(0.714,0.4284,0.18144) ,glm::vec3(0.393548,0.271906,0.166721), 0.2f}, //bronze
    { glm::vec3(0.25,0.25,0.25) ,glm::vec3(0.4,0.4,0.4) ,glm::vec3(0.774597,0.774597,0.774597), 0.6f}, //chrome
    { glm::vec3(0.19125,0.0735,0.0225) ,glm::vec3(0.7038,0.27048,0.0828) ,glm::vec3(0.256777,0.137622,0.086014),0.1f}, //copper
    { glm::vec3(0.24725,0.1995,0.0745) ,glm::vec3(0.75164,0.60648,0.22648) ,glm::vec3(0.628281,0.555802,0.366065),0.4f}, //gold
    { glm::vec3(0.19225,0.19225,0.19225),glm::vec3(0.50754,0.50754,0.50754) ,glm::vec3(0.508273,0.508273,0.508273),0.4f}, //silver
    { glm::vec3(0.0,0.0,0.0) ,glm::vec3(0.01,0.01,0.01) ,glm::vec3(0.50,0.50,0.50), .25f }, //black plastic
    { glm::vec3(0.0,0.1,0.06) ,glm::vec3(0.0,0.50980392,0.50980392),glm::vec3(0.50196078,0.50196078,0.50196078), .25f}, //cyan plastic
    { glm::vec3(0.0,0.0,0.0) ,glm::vec3(0.1 ,0.35,0.1) ,glm::vec3(0.45,0.55,0.45),.25f }, //green plastic
    { glm::vec3(0.0,0.0,0.0) ,glm::vec3(0.5 ,0.0,0.0) ,glm::vec3(0.7,0.6 ,0.6),.25f }, //red plastic
    { glm::vec3(0.0,0.0,0.0) ,glm::vec3(0.55 ,0.55,0.55) ,glm::vec3(0.70,0.70,0.70),.25f }, //white plastic
    { glm::vec3(0.0,0.0,0.0) ,glm::vec3(0.5 ,0.5,0.0) ,glm::vec3(0.60,0.60,0.50),.25f }, //yellow plastic
    { glm::vec3(0.02,0.02,0.02) ,glm::vec3(0.01,0.01,0.01) ,glm::vec3(0.4,0.4 ,0.4),.078125f}, //black rubber
    { glm::vec3(0.0,0.05,0.05) ,glm::vec3(0.4 ,0.5,0.5) ,glm::vec3(0.04,0.7,0.7) ,.078125f}, //cyan rubber
    { glm::vec3(0.0,0.05,0.0) ,glm::vec3(0.4 ,0.5,0.4) ,glm::vec3(0.04,0.7,0.04) ,.078125f}, //green rubber
    { glm::vec3(0.05,0.0,0.0) ,glm::vec3(0.5 ,0.4,0.4) ,glm::vec3(0.7,0.04,0.04) ,.078125f}, //red rubber
    { glm::vec3(0.05,0.05,0.05) ,glm::vec3(0.5,0.5,0.5) ,glm::vec3(0.7,0.7,0.7) ,.078125f}, //white rubber
    { glm::vec3(0.05,0.05,0.0) ,glm::vec3(0.5,0.5,0.4) ,glm::vec3(0.7,0.7,0.04) ,.078125f } //yellow rubber
};


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------



GLuint loadTexture(char const* path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
GLuint loadCubemap(char const* path, std::vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    std::string strpath = std::string(path);
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load((strpath + std::string(faces[i].c_str())).c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


std::vector<std::string> faces = {
            "right.jpg",
            "left.jpg",
            "top.jpg",
            "bottom.jpg",
            "front.jpg",
            "back.jpg"
};
GLuint indicesCubeNDC[] = {
    // Front face
    0, 1, 2,    // First triangle
    3, 4, 5,    // Second triangle

    // Back face
    8, 7, 6,    // First triangle
    11, 10, 9,  // Second triangle

    // Left face
    14, 13, 12, // First triangle
    17, 16, 15, // Second triangle

    // Right face
    18, 19, 20, // First triangle
    21, 22, 23, // Second triangle

    // Bottom face
    26, 25, 24, // First triangle
    29, 28, 27, // Second triangle

    // Top face
    30, 31, 32, // First triangle
    33, 34, 35  // Second triangle
};
float cubenDC[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};
