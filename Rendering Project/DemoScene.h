#pragma once

#include "EntityComponentSysetm.h"
#include "Utilities.h"
#include <ctime>
#include <cstdlib>
#include <random>
#include "PathConfig.h"


// A concrete implementation of a simple scene
class MyDemoScene : public Scene {
public:
    MyDemoScene(std::unique_ptr<IRenderer> renderer) : Scene(std::move(renderer)) {}

protected:
    // All scene setup, previously in Init(), now happens here.
    void loadScene() override {
        // Seed for random positions
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        createLights();
        createWorldObjects();
        createSkybox();
    }

private:
    void createLights() {
        // --- Create Sun Light ---
        EntityID sun = createEntity();
        DirLight sunLight = {
            glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.0f, 10.f, 0.0f),
            glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.3f),
            0.1f, 100.f
        };
        addComponent(sun, sunLight);

        // --- Create Point Lights ---
        // These will also get a visual representation 
        auto cubePrimitive = std::make_unique<BasicMesh::Cube>((double)1);
        auto lightCubeMesh = std::make_shared<BasicMesh>(); 
        
        lightCubeMesh->CreatePrimitive(cubePrimitive.get());

        float near = 1.0f;
        float far = 25.0f;
        for (int i = 0; i < 10; ++i) {
            EntityID lightEntity = createEntity();

            float angle = glm::radians(i * 36.0f);
            float radius = 10.0f + (i % 3);
            float height = 3.0f + (i % 5);
            glm::vec3 position = glm::vec3(radius * cos(angle), height, radius * sin(angle));

            // Define light properties
            glm::vec3 ambient = 0.1f * colors[i];
            glm::vec3 diffuse = 0.8f * colors[i];
            glm::vec3 specular = glm::vec3(1.0f);

            // Add the light component to the entity
            PointLight pointLight(position, ambient, diffuse, specular, near, far);
            addComponent(lightEntity, pointLight);

            // Add components to make the light visible as a small cube
            Transform transform;
            transform.position = position;
            transform.scale = glm::vec3(0.2f);
           // transform.matrix = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), transform.scale);
            addComponent(lightEntity, transform);

            MeshRenderer Renderer;
            Renderer.mesh = lightCubeMesh;
            Renderer.castShadows = false; // Light cube should not cast shadows
            addComponent(lightEntity, Renderer);
        }

        // --- Create Spot Lights ---
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::vec2 cut = glm::vec2(glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)));
        glm::vec3 attenuation = glm::vec3(1.0f, 0.09f, 0.032f);

        for (size_t i = 0; i < colors.size(); i++) {
            EntityID lightEntity = createEntity();

            float angle = static_cast<int>(i) * glm::radians(36.0f);
            float radius = 5.0f;
            glm::vec3 pos = glm::vec3(
                cos(angle) * radius,
                static_cast<float>(std::rand() % 2000) / 100.0f,
                sin(angle) * radius
            );
            glm::vec3 dir = glm::normalize(glm::vec3(0.f, -1.f, 0.f));

            // Add the spotlight component to the entity
            SpotLight spotLight(
                pos, dir,
                colors[i] * 0.1f, colors[i], colors[i] * 0.5f,
                near_plane, far_plane, cut, attenuation
            );
            addComponent(lightEntity, spotLight);
        }
    }

    void createWorldObjects() {
        // --- Create Curved pathway ---
        {
            EntityID curveEntity = createEntity();

            Transform curveTransform;
            curveTransform.position = glm::vec3(-0.f, 1.f, 0.f);
            addComponent(curveEntity, curveTransform);

            MeshRenderer curveRenderer;
            auto curveMesh = std::make_shared<BasicMesh>();
            std::vector<glm::vec3> curvePoints = {
                 glm::vec3{ -0.5f, 0.8f, -2.5f} *5.f, // Start of long straightaway
                 glm::vec3{ 3.0f, 0.5f, -6.0f} *5.f, // End of long straightaway
                 glm::vec3{  5.5f, .0f, -3.5f}*5.f, // Apex of tight corner
                 glm::vec3{ 3.0f, 0.0f, 0.0f}*5.f, // Start of sweeping curve
                 glm::vec3{ 6.0f, -0.5f, 3.0f}*5.f,
                 glm::vec3{  4.5f, -1.2f, 5.0f}*5.f,
                 glm::vec3{  1.0f, -1.0f, 3.0f}*5.f,
                 glm::vec3{  -2.0f, 0.0f, 5.0f}*5.f,
                 glm::vec3{  -4.5f, 0.0f, 1.0f}*5.f,
                 glm::vec3{  -5, 0.0f, -2.0f}*5.f,
                 glm::vec3{  -4, 0.0f, -5.0f}*5.f
            };
            BasicMesh::BSpline bs{ curvePoints, 1, 1, true };
            curveMesh->CreatePrimitive(&bs);
            curveMesh->SetTextures(getAssetFullPath("rock_wall/textures/rock_wall_13_diff_1k.jpg").c_str(), "", getAssetFullPath("rock_wall/textures/rock_wall_13_nor_gl_1k.jpg").c_str());
            curveRenderer.mesh = curveMesh;
            addComponent(curveEntity, curveRenderer);
        }

        // --- Create Terrain ---
        {
            EntityID terrainEntity = createEntity();

            Transform terrainTransform;
            terrainTransform.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
            terrainTransform.position = glm::vec3(0.f, -0.2, 0.f);
                glm::translate(glm::mat4(1.f), glm::vec3(0.f, -2.f, 0.f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
           // addComponent(terrainEntity, terrainTransform);

            MeshRenderer terrainRenderer;
            auto terrainMesh = std::make_shared<BasicMesh>();
            BasicMesh::Square square{ 500, 100 };
            terrainMesh->CreatePrimitive(&square);
           // terrainMesh->SetTextures(getAssetFullPath("laminate_floor_03_1k/textures/laminate_floor_03_diff_1k.jpg").c_str(), "", getAssetFullPath("laminate_floor_03_1k/textures/laminate_floor_03_nor_gl_1k.jpg").c_str() );
            terrainRenderer.mesh = terrainMesh;
           // addComponent(terrainEntity, terrainRenderer);
        }


        // --- Moving Cube ---
        {
            EntityID movingCubeEntity = createEntity();

            auto cubePrimitive = std::make_unique<BasicMesh::Cube>((double)2);
            auto CubeMesh = std::make_shared<BasicMesh>();
            CubeMesh->CreatePrimitive(cubePrimitive.get());

            MeshRenderer cubeRenderer;
            cubeRenderer.mesh = CubeMesh;
            addComponent(movingCubeEntity, cubeRenderer);
            Transform movingCubTransform;
           // movingCubTransform.matrix = glm::translate(glm::mat4(1.f), glm::vec3(-50.f, -1.99f, 0.f));;
            addComponent(movingCubeEntity, movingCubTransform);

            std::vector<glm::vec3> curvePoints = {
                 glm::vec3{ -0.5f, 0.8f, -2.5f} *2.f, 
                 glm::vec3{ 3.0f, 0.5f, -6.0f} *2.f, 
                 glm::vec3{  5.5f, .0f, -3.5f}*2.f,
                 glm::vec3{ 3.0f, 0.0f, 0.0f}*2.f, 
                 glm::vec3{ 6.0f, -0.5f, 3.0f}*2.f,
                 glm::vec3{  4.5f, -1.2f, 5.0f}*2.f,
                 glm::vec3{  1.0f, -1.0f, 3.0f}*2.f,
                 glm::vec3{  -2.0f, 0.0f, 5.0f}*2.f,
                 glm::vec3{  -4.5f, 0.0f, 1.0f}*2.f,
                 glm::vec3{  -5, 0.0f, -2.0f}*2.f,
                 glm::vec3{  -4, 0.0f, -5.0f}*2.f
            };
            std::vector<float> timestamp = {
                1.0 , 1.0 , 1.0 , 1.0 , 1.0 , 1.0 , 1.0 , 1.0 ,1.0, 1.0,1.0, 1.0
            };
            Animation cubeAniComponent;
            std::unique_ptr<BSplineAnimation> movment = std::make_unique<BSplineAnimation>(curvePoints, timestamp, true);
            cubeAniComponent.animation= std::move(movment);
            addComponent(movingCubeEntity, std::move(cubeAniComponent));
        }
        // --- Moving Cube ---
        {
            EntityID movingCubeEntity = createEntity();

            auto cubePrimitive = std::make_unique<BasicMesh::Cube>((double)2);
            auto CubeMesh = std::make_shared<BasicMesh>();
            CubeMesh->CreatePrimitive(cubePrimitive.get());

            MeshRenderer cubeRenderer;
            cubeRenderer.mesh = CubeMesh;
            addComponent(movingCubeEntity, cubeRenderer);
            Transform movingCubTransform;
            // movingCubTransform.matrix = glm::translate(glm::mat4(1.f), glm::vec3(-50.f, -1.99f, 0.f));;
            addComponent(movingCubeEntity, movingCubTransform);

            std::vector<glm::vec3> curvePoints = {
                 glm::vec3{ -2.f, 0.0f, -2.f} *4.f,
                 glm::vec3{ 2.5f, 0.5f, -7.0f} *4.f,
                 glm::vec3{  5.0f, .0f, -3.0f}*4.f,
                 glm::vec3{ 3.0f, 0.0f, 0.0f}*4.f,
                 glm::vec3{ 6.0f, -0.5f, 1.0f}*4.f,
                 glm::vec3{  4.5f, -1.2f, 5.0f}*4.f,
                 glm::vec3{  1.0f, -1.0f, 3.0f}*4.f,
                 glm::vec3{  -2.0f, 0.0f, 5.0f}*4.f,
                 glm::vec3{  -4.5f, 0.0f, 1.0f}*4.f,
                 glm::vec3{  -5, 0.0f, -2.0f}*4.f,
                 glm::vec3{  -4, 0.0f, -5.0f}*4.f
            };
            std::vector<float> timestamp = {
                2.0 , 2.0 ,2.0 , 2.0 , 2.0 , 2.0 , 2.0 , 2.0 ,2.0, 2.0,2.0, 2.0
            };
            Animation cubeAniComponent;
            std::unique_ptr<BSplineAnimation> movment = std::make_unique<BSplineAnimation>(curvePoints, timestamp, true);
            cubeAniComponent.animation = std::move(movment);
            addComponent(movingCubeEntity, std::move(cubeAniComponent));
        }


        // --- Instanced cube ---
        {
            std::vector<glm::mat4> instanceMatrices;
            const int numberOfInstances = 100;
            std::random_device rd;
            std::mt19937 generator(rd());

            std::uniform_real_distribution<float> positionXZ_dist(-80.0f, 80.0f);
            std::uniform_real_distribution<float> positionY_dist(0.1f, 80.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, glm::radians(360.0f));
            std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f);
            
            for (int i = 0; i < numberOfInstances; ++i) {
                // --- Generate random transformation values ---
                glm::vec3 position(
                    positionXZ_dist(generator),
                    positionY_dist(generator),
                    positionXZ_dist(generator)
                );
                float angle = rotation_dist(generator);
                glm::vec3 axis = glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f)); 

                float scale = scale_dist(generator);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, position);
                model = glm::rotate(model, angle, axis);
                model = glm::scale(model, glm::vec3(scale));
                instanceMatrices.push_back(model);
            }
            EntityID instanceCubes = createEntity();

            auto cubePrimitive = std::make_unique<BasicMesh::Cube>((double)1);
            auto cubeMesh = std::make_shared<BasicMesh>();
            cubeMesh->CreatePrimitive(cubePrimitive.get()); 
            cubeMesh->SetupInstancedArrays(instanceMatrices);
            InstancedMeshRenderer instancedCubeRenderer; 
            instancedCubeRenderer.mesh = cubeMesh;
            instancedCubeRenderer.instanceMatrices = instanceMatrices;
            addComponent(instanceCubes, std::move(instancedCubeRenderer));
        }
    }

    void createSkybox() {
        // Set the skybox for the scene's renderer
        setSkybox(getAssetFullPath("skybox/").c_str(), faces);
    }
};  