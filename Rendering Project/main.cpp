#include <glad/gl.h>       // Includi prima di qualsiasi altro file OpenGL
#define GLFW_INCLUDE_NONE    // Evita che GLFW includa automaticamente gl.h
#include <GLFW/glfw3.h>

#include <iostream>
#include <random>
#include <math.h>
#include <ranges>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Utilities.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "frameBufferObject.h"
#include "Utilities.h"
#include "LightStruct.h"
#include "renderable_object.h"
#include "Skybox.h"
#include <cstdlib> // for rand()
#include <ctime>   // for seeding rand

#define shadowdirlight
#define shadowspotlight



void RenderScene();
void GeometryPass();
void ShadowPass();
void LightingPass();
void LightingPassTest();
void RenderLight();
void RenderSkybox();
void deferredToForward();
void cleanup();

void Init();

GLuint SCR_WIDTH = 1600;
GLuint SCR_HEIGHT = 1000;

// list of object to be rendered
std::vector<renderableObject> renderingObjectList;

// create projection fro prospetic and frustum culling 
glm::mat4 projection;

// create transformations
glm::mat4 view;

// create the model matrix for each object and pass it to shader before drawing
glm::mat4 model;

FXAA fxaa;

GBufferFBO gbuffer;

ShadowMapFBO shadowDirMap{ 3000, 3000 };

ShadowMapArrayFBO shadowSpotMap{ 1024, 1024 };

ShadowMapCubeFBO shadowMapPoint{ 1024 };

Shader shader;
Shader instanceShader;

Shader lightingShader;


DirLight Sunlight;

Skybox skybox;


std::vector<PointLight> pointLightList;

std::vector<SpotLight> spotLightList;


Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

unsigned int lightVAO;


int main()
{

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif



    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dittering black and white", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // OpenGL settings
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoaderLoadGL())
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    glEnable(GL_BLEND);

    // enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // set up GL_DEPTH_TEST
    glEnable(GL_DEPTH_TEST);

    Init();



    // to be deleted 
    lightingShader.load("shader/lightShader.vert", "shader/lightShader.frag");

    unsigned int VBO;
    {
        glGenVertexArrays(1, &lightVAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(lightVAO);
        // we only need to bind to the VBO, the container's VBO's data already contains the data.
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubenDC), cubenDC, GL_STATIC_DRAW);
        // set the vertex attribute 
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

    }

    double lastTime{ 0.0 };
    int frameCount{ 0 };
    double fps{ 0.0 };


    // render loop
    // -----------
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        double currentTime = glfwGetTime();
        frameCount++;

        // Update FPS counter once per second
        if (currentTime - lastTime >= 1.0) {
            fps = frameCount / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;

            // Update window title with FPS
            char title[64];
            sprintf_s(title, "Dittering black and white | FPS: %.1f", fps);
            glfwSetWindowTitle(window, title);
        }
        // input
        // -----
        processInput(window);

        // update world matrix
        projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);

        // update transformations
        view = camera.GetViewMatrix();

        // calculate the model matrix for each object and pass it to shader before drawing
        model = glm::mat4(1.0f);

        // render
        // ------

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);


        // render call
        RenderScene();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // cleanup all the 
    cleanup();

    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (lightVAO != 0)
    {
        glDeleteVertexArrays(1, &lightVAO);
    }

    glfwSetWindowShouldClose(window, true);
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();


    return 0;
}

void Init()
{
    // setup world matrix
    projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);

    // create transformations
    view = camera.GetViewMatrix();

    // calculate the model matrix for each object and pass it to shader before drawing
    model = glm::mat4(1.0f);

    //framebufferMSSA.innit();
    fxaa.init();

    std::shared_ptr<Shader> shadowMapShader{ std::make_shared<Shader>() };
    std::shared_ptr<Shader> shadowMapShaderPoint{ std::make_shared<Shader>() };


    gbuffer.Init();

    // setup shades
    shader.load("shader/phong.vert", "shader/phong.frag");
    instanceShader.load("shader/instancePhong.vert", "shader/phong.frag");

    shadowMapShader->load("shader/shadowMap.vert", "shader/shadowMap.frag");
    shadowMapShaderPoint->load("shader/shadowMapPoint.vert", "shader/shadowMapPoint.frag", "shader/shadowMapPoint.geom");

    // setup light
    {
        float near = 1.0f;
        float far = 25.0f;
        for (int i = 0; i < 10; ++i) {
            float angle = glm::radians(i * 36.0f); // Spread around 360 degrees
            float radius = 10.0f + (i % 3); // Vary radius a bit
            float height = 3.0f + (i % 5);  // Keep in positive Y

            glm::vec3 position = glm::vec3(
                radius * cos(angle),
                height,
                radius * sin(angle)
            );

            glm::vec3 ambient = 0.1f * colors[i]; // 
            glm::vec3 diffuse = 0.8f * colors[i]; // 
            glm::vec3 specular = glm::vec3(colors[i].y * (0.587f / 0.299f) + colors[i].x); // White specular highlight


            // Add Point Light
            pointLightList.emplace_back(position, ambient, diffuse, specular, near, far);


        }
        // setup Sunlight 
        Sunlight = DirLight{ glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.3f), 0.1f, 100.f };



        // Seed the random number generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        // Define common attenuation and cutoff values
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::vec2 cut = glm::vec2(glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)));
        glm::vec3 attenuation = glm::vec3(1.0f, 0.09f, 0.032f); // constant, linear, quadratic


        // Loop over each color and generate a spotlight

        for (size_t i = 0; i < colors.size(); i++)
        {
            const auto& color = colors[i];
            float angle = static_cast<int>(i) * glm::radians(36.0f);  // 360/10 degrees per step
            float radius = 5.0f;
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;
            // Random position in range vec3([-10, 10],[-10, 10],[0, 10])
            glm::vec3 pos = glm::vec3(
                x,
                static_cast<float>(std::rand() % 2000) / 100.0f,
                z
            );

            // Random direction in range [-1, 1]
            glm::vec3 dir = glm::normalize(glm::vec3(
                0.f, -1.f, 0.f
            ));

            // Add the spotlight to the list
            spotLightList.emplace_back(
                pos, dir,
                color * 0.1f, // Ambient
                color,        // Diffuse
                color * 0.5f, // Specular
                near_plane,
                far_plane,
                cut,
                attenuation
            );
        }
    }

    shadowDirMap.Init(shadowMapShader);

    shadowMapPoint.Init(static_cast<int>(pointLightList.size()), shadowMapShaderPoint);

    shadowSpotMap.Init(static_cast<unsigned int>(spotLightList.size()), shadowMapShader);



    // setup object
    auto coso{ std::make_shared<BasicMesh>() };  // WIP: for later change this to unique pointer should be fine if all the changes are done befor passig it to the renderingObjectList
    auto curva{ std::make_shared<BasicMesh>() };
    auto cerchio{ std::make_shared<BasicMesh>() };
    auto terrein{ std::make_shared<BasicMesh>() };
    auto barrel{ std::make_shared<BasicMesh>() };


    //
    // coso->LoadMesh("Assets/coast_sand_rocks_02_1k/coast_sand_rocks_02_1k.gltf");
    std::vector<glm::vec3> curvePoints =
    {
        glm::vec3{0.0f,0.0f,0.0f}*10.f,
        glm::vec3{1.5f,0.0f,-1.0f}*10.f,
        glm::vec3{2.0f,0.0f,-3.0f}*10.f,
        glm::vec3{4.5f,0.0f,-1.0f}*10.f,
        glm::vec3{7.0f,0.0f,-1.5f}*10.f,
        glm::vec3{8.0f,0.0f,3.5f}*10.f,
        glm::vec3{8.0f,0.0f,5.0f}*10.f,
        glm::vec3{6.25f,0.0f,4.0f}*10.f,
        glm::vec3{5.5f,0.0f,6.5f}*10.f,
        glm::vec3{3.5f,0.0f,2.5f}*10.f,
        glm::vec3{2.5f,0.0f,1.5f}*10.f,
        glm::vec3{0.0f,0.0f,0.0f}*10.f
    };


    BasicMesh::BSpline bs{ curvePoints,1, 1,true };
    curva->CreatePrimitive(&bs);

    curva->SetTextures("Assets/rock_wall/textures/rock_wall_13_diff_1k.jpg", "", "Assets/rock_wall/textures/rock_wall_13_nor_gl_1k.jpg");
    //curva->SetTextures("Assets/laminate_floor_03_1k/textures/laminate_floor_03_diff_1k.jpg","","Assets/laminate_floor_03_1k/textures/laminate_floor_03_nor_gl_1k.jpg" );

    BasicMesh::Circle cerchietto{ 10,32,10.f };
    cerchio->CreatePrimitive(&cerchietto);
    cerchio->SetTextures("Assets/laminate_floor_03_1k/textures/laminate_floor_03_diff_1k.jpg", "", "Assets/laminate_floor_03_1k/textures/laminate_floor_03_nor_gl_1k.jpg");

    BasicMesh::Square quadrato{ 500,100 };
    terrein->CreatePrimitive(&quadrato);
    terrein->SetTextures("Assets/laminate_floor_03_1k/textures/laminate_floor_03_diff_1k.jpg", "", "Assets/laminate_floor_03_1k/textures/laminate_floor_03_nor_gl_1k.jpg");
    //cerchio->CreatePrimitive(&cerchietto); 
    //cerchio->SetTextures("Assets/rock_wall/textures/rock_wall_13_diff_1k.jpg", "", "Assets/rock_wall/textures/rock_wall_13_nor_gl_1k.jpg");

    std::vector<float> timeAnimation = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f };

    auto truckAnimation = std::make_unique<BSplineAnimation>(
        curvePoints,
        timeAnimation,
        true
    );
    // coso->LoadMesh("Assets/Post_Apocalyptic_truck/Post_Apocalyptic_truck.obj");


    //coso->LoadMesh("Assets/jacaranda_tree_1k/jacaranda_tree_1k.gltf");
    //coso->LoadMesh("Assets/dead_tree_trunk_02_1k/dead_tree_trunk_02_1k.gltf");

/*
    barca ->LoadMesh("Assets/Barca olandese/dutch_ship_1k.obj");
    barrel->LoadMesh("Assets/barrel_vino/wine_barrel_01_4k.obj");
    table->LoadMesh("Assets/wooden_picnic_table_4k/wooden_picnic_table_4k.obj");
    floorBrick->LoadMesh("Assets/Birck_wall_4k/Wall.obj");
    floorWood->LoadMesh("Assets/floor by me/floor.obj"); */
    //plant2->LoadMesh("D:/repos/_Assets/weed_plant_4k/weed_plant_02_4k.obj");
    //plant3->LoadMesh("D:/repos/_Assets/weed_plant_4k/weed_plant_03_4k.obj");
    //plant4->LoadMesh("D:/repos/_Assets/weed_plant_4k/weed_plant_04_4k.obj");
    //plant4->LoadMesh("D:/repos/_Assets/house/house.obj");

    barrel->LoadMesh("Assets/barrel_vino/wine_barrel_01_4k.obj");

    glm::mat4 ModelMatrixCurve = glm::mat4(1.f);
    ModelMatrixCurve = glm::translate(ModelMatrixCurve, glm::vec3(-50.f, 0.01f - 2.f, -0.f));
    //ModelMatrix = glm::rotate(ModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    renderingObjectList.push_back(renderableObject{ curva, &shader,ModelMatrixCurve });

    glm::mat4 ModelMatrixTerrein = glm::mat4(1.f);
    ModelMatrixTerrein = glm::translate(ModelMatrixTerrein, glm::vec3(0.f, -2.f, 0.f));
    ModelMatrixTerrein = glm::rotate(ModelMatrixTerrein, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    renderingObjectList.push_back(renderableObject{ terrein , &shader, ModelMatrixTerrein });


    for (int i = 0; i < 10; ++i) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        float angle = i * glm::radians(36.0f);  // 360/10 degrees per step
        float radius = 5.0f;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        float y = -2.1f;  // rising spiral (ensures y > 0)

        modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));

        // Optional: random rotation (e.g., for variation)
        // modelMatrix = glm::rotate(modelMatrix, glm::radians(i * 10.0f), glm::vec3(0.f, 1.f, 0.f));

        renderingObjectList.push_back(renderableObject{ barrel, &shader, modelMatrix });
    }

    //renderingObjectList.push_back(renderableObject{ coso,&shader,ModelMatrixCurve}); //,std::move(truckAnimation) 
    //renderingObjectList.push_back(renderableObject{ curva, &shader, ModelMatrix });

    /*
    renderingObjectList.push_back(renderableObject{ barrel, &shader, glm::mat4(1.f) });


    renderingObjectList.push_back(renderableObject{ floorWood, &shader,glm::vec3(0.f, -5.f, 0.f) ,glm::mat4(1.0f) });

    renderingObjectList.push_back(renderableObject{ house , &shader,glm::vec3(0.f, 0.f, 0.f) ,glm::mat4(1.0f) });

    glm::mat4 backModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.f, -5.f));
    backModelMatrix = glm::rotate(backModelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    renderingObjectList.push_back(renderableObject{ floorBrick,  &shader, backModelMatrix });

    glm::mat4 leftModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(5.f, 0.f, 0.f));
    leftModelMatrix = glm::rotate(leftModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    leftModelMatrix = glm::rotate(leftModelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    renderingObjectList.push_back(renderableObject{ floorBrick,  &shader,leftModelMatrix });

    glm::mat4 rightModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.f, 0.f, 0.f));
    rightModelMatrix = glm::rotate(rightModelMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    rightModelMatrix = glm::rotate(rightModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    renderingObjectList.push_back(renderableObject{ floorBrick, &shader,rightModelMatrix });

    loadInstanceRenderingObjectList(plant2);
    loadInstanceRenderingObjectList(plant3);
    loadInstanceRenderingObjectList(plant4);
    */



    const char* skyboxfolder = "Assets/skybox/";
    skybox.load(skyboxfolder, faces);
}

void RenderScene()
{
    GeometryPass();

    ShadowPass();

    LightingPass();


    deferredToForward();

    RenderLight();
    RenderSkybox();

    fxaa.render();

}
void GeometryPass()
{

    gbuffer.BindForWriting();
    gbuffer.shaderGeom->use();
    for (auto& obj : renderingObjectList)
    {
        if (obj.isAnimated)
        {
            getAnimatedTransform(obj, static_cast<float>(glfwGetTime()));
            //printMat4(obj.model);
        }

        gbuffer.shaderGeom->setMat4("projection", projection);
        gbuffer.shaderGeom->setMat4("view", view);
        gbuffer.shaderGeom->setMat4("model", obj.model);


        if (!obj.isInstaced)
            obj.mesh->Render(gbuffer.shaderGeom);
        else
            //obj.mesh->RenderInstanced(gbuffer.shader);
            ;
    }

}

void  ShadowPass()
{

    // shadowdirlight
    shadowDirMap.BindForWriting();

    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);

    shadowDirMap.shader->use();

    Sunlight.setPos(camera.Position);
    glm::mat4 lightSpaceMatrix = Sunlight.Projection * Sunlight.View;
    shadowDirMap.shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    for (const auto& obj : renderingObjectList)
    {
        // directiona light 
        shadowDirMap.shader->setMat4("model", obj.model);
        if (!obj.isInstaced) // wip 
        {

            obj.mesh->Render(shadowDirMap.shader);
        }
    }
    glEnable(GL_CULL_FACE);




    // spotLightList

    // decrease peter panning 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    // render all the obbject for all the spot light

    shadowSpotMap.shader->use();

    for (size_t i{ 0 }; i < spotLightList.size(); i++)
    {
        shadowSpotMap.BindLayerForWriting(static_cast<int>(i));

        glClear(GL_DEPTH_BUFFER_BIT);

        const auto light = spotLightList[i];
        glm::mat4 lightSpaceMatrix = light.Projection * light.View;

        shadowSpotMap.shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        for (const auto& obj : renderingObjectList)
        {
            // directiona light 
            shadowSpotMap.shader->setMat4("model", obj.model);
            if (!obj.isInstaced) // wip 
                obj.mesh->Render(shadowSpotMap.shader);
        }
    }
    glCullFace(GL_BACK);

    // shadowPoint light


    shadowMapPoint.BindForWriting(0);
    glEnable(GL_DEPTH_TEST);
    // avoid peter pan effect 
    glDisable(GL_CULL_FACE);
    glViewport(0, 0, shadowMapPoint.s_SIZE, shadowMapPoint.s_SIZE);

    glClear(GL_DEPTH_BUFFER_BIT);

    shadowMapPoint.shader->use();

    for (size_t i = 0; i < pointLightList.size(); ++i)
    {
        // Set current light index
        shadowMapPoint.shader->setInt("lightIndex", static_cast<int>(i));

        // Your existing setupUniformShader call
        shadowMapPoint.setupUniformShader(&pointLightList[i]);

        for (const auto& obj : renderingObjectList)
        {
            // point light 
            shadowMapPoint.shader->setMat4("model", obj.model);
            if (!obj.isInstaced) // wip 
                obj.mesh->Render(shadowMapPoint.shader);
        }
    }

    glEnable(GL_CULL_FACE);
}

void LightingPass()
{

    fxaa.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gbuffer.shaderLighting->use();
    gbuffer.BindForReading(0);

    gbuffer.shaderLighting->setInt("gPosition", 0);
    gbuffer.shaderLighting->setInt("gNormalShininess", 1);
    gbuffer.shaderLighting->setInt("gColorSpec", 2);
    gbuffer.shaderLighting->setInt("gDepth", 3);

    shadowMapPoint.BindForReading(SHADOW_MAP_CUBE_UNIT);
    gbuffer.shaderLighting->setInt("shadowCubeArray", SHADOW_MAP_CUBE_UNIT);


    shadowDirMap.BindForReading(SHADOW_MAP_DIR_UNIT);
    gbuffer.shaderLighting->setInt("shadowDir", SHADOW_MAP_DIR_UNIT);
    glm::mat4 lightSpaceMatrix = Sunlight.Projection * Sunlight.View;
    gbuffer.shaderLighting->setMat4("dirLightSpaceMatrice", lightSpaceMatrix);
    // shadowdirlight


    shadowSpotMap.BindForReading(SHADOW_MAP_SPOT_UNIT);
    gbuffer.shaderLighting->setInt("shadowSpotArray", SHADOW_MAP_SPOT_UNIT);
    // shadowspotlight




    gbuffer.shaderLighting->setVec3("viewPos", camera.Position);


    //      Set unifrom point lights   //
    gbuffer.shaderLighting->setInt("numPointLights", static_cast<int>(pointLightList.size()));
    for (size_t i = 0; i < pointLightList.size(); ++i)
    {
        const auto& light = pointLightList[i];
        std::string idx = "pointLights[" + std::to_string(i) + "]";

        gbuffer.shaderLighting->setVec3(idx + ".position", light.Pos);
        gbuffer.shaderLighting->setVec3(idx + ".light.ambient", light.Ambient);
        gbuffer.shaderLighting->setVec3(idx + ".light.diffuse", light.Diffuse);
        gbuffer.shaderLighting->setVec3(idx + ".light.specular", light.Specular);

        // You may define attenuation terms per light
        gbuffer.shaderLighting->setFloat(idx + ".constant", 1.0f);
        gbuffer.shaderLighting->setFloat(idx + ".linear", 0.09f);
        gbuffer.shaderLighting->setFloat(idx + ".quadratic", 0.032f);

        gbuffer.shaderLighting->setFloat(idx + ".far_plane", light.far_plane);
        gbuffer.shaderLighting->setInt(idx + ".shadowID", static_cast<int>(i));
    }

    //      Set uniform spot lights     //
    gbuffer.shaderLighting->setInt("numSpotLights", static_cast<int>(spotLightList.size()));
    for (size_t i = 0; i < spotLightList.size(); ++i)
    {
        const auto& light = spotLightList[i];
        std::string idx = "spotLights[" + std::to_string(i) + "]";

        glm::mat4 lightSpaceMatrix = light.Projection * light.View;
        gbuffer.shaderLighting->setMat4(idx + ".SpaceMatrices", lightSpaceMatrix);

        // Position and direction
        gbuffer.shaderLighting->setVec3(idx + ".position", light.Pos);
        gbuffer.shaderLighting->setVec3(idx + ".direction", light.Dir);

        // Light properties
        gbuffer.shaderLighting->setVec3(idx + ".light.ambient", light.Ambient);
        gbuffer.shaderLighting->setVec3(idx + ".light.diffuse", light.Diffuse);
        gbuffer.shaderLighting->setVec3(idx + ".light.specular", light.Specular);

        // Attenuation
        gbuffer.shaderLighting->setFloat(idx + ".constant", light.constant);
        gbuffer.shaderLighting->setFloat(idx + ".linear", light.linear);
        gbuffer.shaderLighting->setFloat(idx + ".quadratic", light.quadratic);

        // Spotlight specific
        gbuffer.shaderLighting->setFloat(idx + ".cutOff", light.cutOff);
        gbuffer.shaderLighting->setFloat(idx + ".outerCutOff", light.outerCutOff);

        // Shadow mapping
        gbuffer.shaderLighting->setFloat(idx + ".far_plane", light.far_plane);
        gbuffer.shaderLighting->setInt(idx + ".shadowID", static_cast<int>(i));
    }

    //      Set unifrom point lights    //
    {
        std::string idx = "dirLight";

        gbuffer.shaderLighting->setVec3(idx + ".position", Sunlight.Position);
        gbuffer.shaderLighting->setVec3(idx + ".direction", Sunlight.Direction);
        gbuffer.shaderLighting->setVec3(idx + ".light.ambient", Sunlight.Ambient);
        gbuffer.shaderLighting->setVec3(idx + ".light.diffuse", Sunlight.Diffuse);
        gbuffer.shaderLighting->setVec3(idx + ".light.specular", Sunlight.Specular);

    }



    gbuffer.Render();

    fxaa.unbind();
}
void LightingPassTest()
{
    //framebufferMSSA.bind(); 

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    shadowDirMap.BindForReading(SHADOW_MAP_DIR_UNIT);


    shadowMapPoint.BindForReading(SHADOW_MAP_CUBE_UNIT);
    for (auto& obj : renderingObjectList)
    {
        if (obj.isAnimated)
        {
            getAnimatedTransform(obj, static_cast<float>(glfwGetTime()));
            //printMat4(obj.model);
        }

        obj.shader->use();

        obj.shader->setInt("shadowMapCube", SHADOW_MAP_CUBE_UNIT);
        obj.shader->setMat4("projection", projection);
        obj.shader->setMat4("view", view);
        obj.shader->setMat4("model", obj.model);

        // sunlight shader setup
        {
            obj.shader->setInt("shadowMap", SHADOW_MAP_DIR_UNIT);
            // Set light properties
            obj.shader->setVec3("dirLight.direction", Sunlight.Direction);

            obj.shader->setVec3("dirLight.light.ambient", Sunlight.Ambient);
            obj.shader->setVec3("dirLight.light.diffuse", Sunlight.Diffuse);
            obj.shader->setVec3("dirLight.light.specular", Sunlight.Specular);


            glm::mat4 lightSpaceMatrix = Sunlight.Projection * Sunlight.View;
            obj.shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
            obj.shader->setFloat("near_plane", Sunlight.near_plane);
            obj.shader->setFloat("far_plane", Sunlight.far_plane);
        }


        // Set view position for specular calculation
        obj.shader->setVec3("viewPos", camera.Position);
        if (!obj.isInstaced)
            obj.mesh->Render(*obj.shader);
        else
            obj.mesh->RenderInstanced(*obj.shader);
    }
}

void deferredToForward()
{

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);

    // Write to the FXAA FBO (you'll need a getter for this)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fxaa.framebuffer);

    // Blit the depth buffer contents
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void RenderLight()
{

    // rendere the light 

    for (size_t i = 0; i < pointLightList.size(); i++)
    {
        glDisable(GL_CULL_FACE);
        lightingShader.use();

        glm::mat4 lightModel = model;
        lightModel = glm::translate(lightModel, pointLightList[i].Pos);
        lightModel = glm::scale(lightModel, glm::vec3(0.1f));

        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setMat4("model", lightModel);

        lightingShader.setVec3("color", colors[i]);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glEnable(GL_CULL_FACE);
    }

}

void RenderSkybox()
{

    //render the skybox
    {
        glDepthFunc(GL_LEQUAL);     // Allow depth values equal to the far plane
        glDepthMask(GL_FALSE);      // Don't write to depth buffer
        skybox.shader.use();
        skybox.shader.setMat4("projection", projection);
        skybox.shader.setMat4("view", view);
        skybox.Draw();
        glDepthFunc(GL_LESS);       // Restore default
        glDepthMask(GL_TRUE);
    }
}

void cleanup()
{

    gbuffer.clean();
    fxaa.clean();
    shadowMapPoint.clean();
    shader.clean();
    instanceShader.clean();
    lightingShader.clean();
    gbuffer.shaderLighting->clean();
    renderingObjectList.clear();

    shadowDirMap.clean();
    // shadowdirlight

    shadowSpotMap.clean();
    // shadowdirlight
    skybox.clear();
    pointLightList.clear();
};
