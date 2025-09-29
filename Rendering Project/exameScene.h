#pragma once

#include "EntityComponentSysetm.h"
#include "Utilities.h"
#include <ctime>
#include <cstdlib>
#include <random>
#include "PathConfig.h"


// A concrete implementation of a simple scene
class ExameScene : public Scene {
public:
    ExameScene(std::unique_ptr<IRenderer> renderer) : Scene(std::move(renderer)) {}

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
    void createLights() 
    {
        // --- Create Sun Light ---
        EntityID sun = createEntity();
        DirLight sunLight = {
            glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.0f, 10.f, 0.0f),
            glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.3f),
            0.1f, 100.f
        };
        addComponent(sun, sunLight);

        

        // --- Create Point Lights ---
        auto starLanternMesh = std::make_shared<BasicMesh>();

        starLanternMesh->LoadMesh(getAssetFullPath("star_lantern/scene.gltf").c_str());
       
        std::vector<glm::vec3> posPoints = {
                glm::vec3{ -2.f, 0.0f, -2.f} *2.f,
                glm::vec3{ 2.5f, 0.0f, -7.0f} *2.f,
                glm::vec3{  5.0f, 0.0f, -3.0f}*2.f,
                glm::vec3{ 3.0f, 0.0f, 0.0f}*2.f,
                glm::vec3{ 6.0f, 0.0f, 1.0f}*2.f,
                glm::vec3{  4.5f, 0.0f, 5.0f}*2.f,
                glm::vec3{  1.0f, 0.0f, 3.0f}*2.f,
                glm::vec3{  -2.0f, 0.0f, 5.0f}*2.f,
                glm::vec3{  -4.5f, 0.0f, 1.0f}*2.f,
                glm::vec3{  -5, 0.0f, -2.0f}*2.f,
                glm::vec3{  -4, 0.0f, -5.0f}*2.f
        };

        std::random_device rd; 
        std::mt19937 generator(rd()); 

        std::uniform_real_distribution<float> position_Y(-5.0f, 5.0f); 

        // Define light properties
        glm::vec3 ambient = 0.1f * Colors::Blue;
        glm::vec3 diffuse = 0.8f * Colors::Blue;
        glm::vec3 specular = glm::vec3(1.0f);
        float near = 1.0f;
        float far = 25.0f;

        for (size_t i = 0; i < posPoints.size(); i++) 
        {
            EntityID lightEntity = createEntity();

            glm::vec3 position = posPoints[i] + glm::vec3(0.f,10.f + position_Y(generator), 0.f); 
            // Add the light component to the entity
            PointLight pointLight(position, ambient, diffuse, specular, near, far); 
            addComponent(lightEntity, pointLight); 
             
            Transform transform; 
            transform.position = position; 
            transform.scale = glm::vec3(0.1f); 
            addComponent(lightEntity, transform); 

            MeshRenderer renderer; 
            renderer.mesh = starLanternMesh; 
            renderer.receiveShadows = false;  
            addComponent(lightEntity, renderer);  
        }

        // --- Create Spot Lights ---
        auto purpleLanternMesh = std::make_shared<BasicMesh>(); 
        purpleLanternMesh->LoadMesh(getAssetFullPath("spherical_japanese_paper_lantern/scene.gltf").c_str()); 

        float near_plane = 1.0f;  
        float far_plane = 25.0f;   
        glm::vec2 cut = glm::vec2(glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)));  
        glm::vec3 attenuation = glm::vec3(1.0f, 0.09f, 0.032f);  
        float numberlight{ 10.f };
        float angleStep{ (2.f * 3.14159f) / numberlight };
        for (size_t i = 0 ; i< numberlight; i++ )
        {

            EntityID lightEntity = createEntity(); 
            glm::vec3 position{cos(angleStep * i),10.f,sin(angleStep * i)} ;
            position = ((5.f * glm::vec3(1.f, 0.f, 1.f)) + glm::vec3(0.f, 1.f, 0.f)) * position;
            SpotLight spotLight(
                position, glm::vec3(0.f, -1.f, 0.f),
                Colors::Purple * 0.1f, Colors::Purple, Colors::Purple * 0.5f,
                near_plane, far_plane, cut, attenuation
            );
            addComponent(lightEntity, spotLight);

            Transform transform; 
            transform.position = position; 
            transform.scale = glm::vec3(0.1f); 
            addComponent(lightEntity, transform); 

            MeshRenderer renderer; 
            renderer.mesh = purpleLanternMesh;
            renderer.receiveShadows = false; 
            addComponent(lightEntity, renderer); 
        }
    }

    void createWorldObjects() {

        // --- Central island ---
        {
            EntityID centralIslandID = createEntity();

            Transform centralIslandTR;
            
            centralIslandTR.scale = glm::vec3(0.01f);

            addComponent(centralIslandID, centralIslandTR);

            auto centralIslandMS = std::make_shared<BasicMesh>(); 
            centralIslandMS->LoadMesh(getAssetFullPath("stylized_mini_floating_island/scene.gltf").c_str()); 
            MeshRenderer centralIslandMR;
            centralIslandMR.mesh = centralIslandMS;
            addComponent(centralIslandID, centralIslandMR); 
        }

        // --- Inner ship ---
        auto ShipMS = std::make_shared<BasicMesh>();
        ShipMS->LoadMesh(getAssetFullPath("peachy_balloon_gift/scene.gltf").c_str());
        {
            EntityID innerShipID = createEntity();

            Transform innerShipTR;
            glm::quat rotationX = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.f, 0.f, 0.f));
            glm::quat rotationY = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.f, -1.f, 0.f));
            glm::quat rotationZ = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.f));
            innerShipTR.rotation = (rotationY * rotationX * rotationZ);
            innerShipTR.scale = glm::vec3(0.1f);
            addComponent(innerShipID, innerShipTR);

            
            MeshRenderer innerShipMR;
            innerShipMR.mesh = ShipMS;
            addComponent(innerShipID, innerShipMR);

            std::vector<glm::vec3> curvePoints = {
                 glm::vec3{ -2.f, 0.0f, -2.f} *2.f,
                 glm::vec3{ 2.5f, 0.0f, -7.0f} *2.f,
                 glm::vec3{  5.0f, 0.0f, -3.0f}*2.f,
                 glm::vec3{ 3.0f, 0.0f, 0.0f}*2.f,
                 glm::vec3{ 6.0f, 0.0f, 1.0f}*2.f,
                 glm::vec3{  4.5f, 0.0f, 5.0f}*2.f,
                 glm::vec3{  1.0f, 0.0f, 3.0f}*2.f,
                 glm::vec3{  -2.0f, 0.0f, 5.0f}*2.f,
                 glm::vec3{  -4.5f, 0.0f, 1.0f}*2.f,
                 glm::vec3{  -5, 0.0f, -2.0f}*2.f,
                 glm::vec3{  -4, 0.0f, -5.0f}*2.f
            };
            std::vector<float> timestamp = {
                2.0 , 2.0 , 2.0 , 2.0 , 2.0 , 2.0 , 2.0 , 2.0 , 2.0, 2.0, 2.0, 2.0
            } ;
            Animation AniComponent;
            std::unique_ptr<BSplineAnimation> movment = std::make_unique<BSplineAnimation>(curvePoints, timestamp, true);
            AniComponent.animation = std::move(movment);
            addComponent(innerShipID, std::move(AniComponent));
        }
        // --- Outer ship ---
        {
            EntityID outerShipID = createEntity();

            Transform outerShipTR;
            glm::quat rotationX = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.f, 0.f, 0.f));
            glm::quat rotationY = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.f, -1.f, 0.f));
            glm::quat rotationZ = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.f));
            outerShipTR.rotation = (rotationY * rotationX * rotationZ);
            outerShipTR.scale = glm::vec3(0.1f);
            addComponent(outerShipID, outerShipTR);

            MeshRenderer outerShipMR;
            outerShipMR.mesh = ShipMS;
            addComponent( outerShipID, outerShipMR );

            std::vector<glm::vec3> curvePoints = {
                 glm::vec3{ -2.f, 0.0f, -2.f} *4.f,
                 glm::vec3{ 2.5f, 0.0f, -7.0f} *4.f,
                 glm::vec3{  5.0f, 0.0f, -3.0f}*4.f,
                 glm::vec3{ 3.0f, 0.0f, 0.0f}*4.f,
                 glm::vec3{ 6.0f, 0.f, 1.0f}*4.f,
                 glm::vec3{  4.5f, 0.0f, 5.0f}*4.f,
                 glm::vec3{  1.0f, 0.0f, 3.0f}*4.f,
                 glm::vec3{  -2.0f, 0.0f, 5.0f}*4.f,
                 glm::vec3{  -4.5f, 0.0f, 1.0f}*4.f,
                 glm::vec3{  -5, 0.0f, -2.0f}*4.f,
                 glm::vec3{  -4, 0.0f, -5.0f}*4.f
            };
            std::vector<float> timestamp = {
                5.0 , 5.0 , 5.0 , 5.0 , 5.0 , 5.0 , 5.0 , 5.0 , 5.0, 5.0, 5.0, 5.0
            };
            Animation cubeAniComponent;
            std::unique_ptr<BSplineAnimation> movment = std::make_unique<BSplineAnimation>(curvePoints, timestamp, true);
            cubeAniComponent.animation = std::move(movment);
            addComponent(outerShipID, std::move(cubeAniComponent));
        }

        // --- Many island ---
        {
            std::vector<glm::mat4> instanceMatrices;
            const int numberOfInstances = 100;
            std::random_device rd;
            std::mt19937 generator(rd());

            std::uniform_real_distribution<float> positionXZ_dist(-40.0f, 40.0f);
            std::uniform_real_distribution<float> positionY_dist(-40.f, 40.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, glm::radians(360.0f));
            std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f);

            for (int i = 0; i < numberOfInstances; ++i)
            {
                // --- Generate random transformation values ---
                glm::vec3 position(
                    positionXZ_dist(generator),
                    positionY_dist(generator),
                    positionXZ_dist(generator)
                );
                float angle = 0.f;
                glm::vec3 axis = glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f));

                float scale = 0.01f;
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, position);
                model = glm::rotate(model, angle, axis);
                model = glm::scale(model, glm::vec3(scale));
                instanceMatrices.push_back(model);
            }
            EntityID smallSiland2ID = createEntity();


            auto smallSiland2MS = std::make_shared<BasicMesh>();
            smallSiland2MS->LoadMesh(getAssetFullPath("flying_island/scene.gltf").c_str());
            InstancedMeshRenderer instancedCubeRenderer;
            instancedCubeRenderer.mesh = smallSiland2MS;
            instancedCubeRenderer.instanceMatrices = instanceMatrices;
            addComponent(smallSiland2ID, std::move(instancedCubeRenderer));
        }

        // --- Many small island 2 ---
        {
            std::vector<glm::mat4> instanceMatrices;
            const int numberOfInstances = 100;
            std::random_device rd;
            std::mt19937 generator(rd()); 

            std::uniform_real_distribution<float> positionXZ_dist(-40.0f, 40.0f); 
            std::uniform_real_distribution<float> positionY_dist(-40.0f, 40.0f); 
            std::uniform_real_distribution<float> rotation_dist(0.0f, glm::radians(360.0f)); 
            std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f); 

            for (int i = 0; i < numberOfInstances; ++i) 
            {
                // --- Generate random transformation values ---
                glm::vec3 position(
                    positionXZ_dist(generator), 
                    positionY_dist(generator), 
                    positionXZ_dist(generator) 
                ); 


                float scale = 0.01;
                glm::mat4 model = glm::mat4(1.0f); 
                model = glm::translate(model, position); 
                model = glm::scale(model, glm::vec3(scale));
                instanceMatrices.push_back(model);
            }
            EntityID smallSiland2ID = createEntity();


            auto smallSiland2MS = std::make_shared<BasicMesh>();
            smallSiland2MS->LoadMesh(getAssetFullPath("flying_island_2/scene.gltf").c_str());
            InstancedMeshRenderer instancedCubeRenderer;
            instancedCubeRenderer.mesh = smallSiland2MS;
            instancedCubeRenderer.instanceMatrices = instanceMatrices;
            addComponent(smallSiland2ID, std::move(instancedCubeRenderer));
        }

        // --- Moving ship ---
        {
            std::vector<glm::mat4> instanceMatrices;
            const int numberOfInstances = 100;
            std::random_device rd;
            std::mt19937 generator(rd());

            std::uniform_real_distribution<float> positionXZ_dist(-20.0f, 20.0f);
            std::uniform_real_distribution<float> positionY_dist(0.1f, 10.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, glm::radians(360.0f));
            std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f);

            for (int i = 0; i < numberOfInstances; ++i)
            {
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
            EntityID smallSiland2ID = createEntity();


            auto smallSiland2MS = std::make_shared<BasicMesh>();
            //smallSiland2MS->LoadMesh(getAssetFullPath("the_last_stronghold_animated/scene.gltf").c_str());
            InstancedMeshRenderer instancedCubeRenderer;
            instancedCubeRenderer.mesh = smallSiland2MS;
            instancedCubeRenderer.instanceMatrices = instanceMatrices;
            //addComponent(smallSiland2ID, std::move(instancedCubeRenderer));
        }


        

        {
            EntityID BarrelsID = createEntity();

            Transform barrelTransform; 
            barrelTransform.scale = glm::vec3(0.1f);
            addComponent(BarrelsID , barrelTransform);

            auto barrelMesh = std::make_shared<BasicMesh>();
            //barrelMesh->LoadMesh(getAssetFullPath("the_last_stronghold_animated/scene.gltf").c_str());
            MeshRenderer barrelRenderer; 
            barrelRenderer.mesh = barrelMesh;
      
            //addComponent(BarrelsID, barrelRenderer); 
        }

        // --- Instanced barrel ---
        //{
        //    std::vector<glm::mat4> instanceMatrices;
        //    const int numberOfInstances = 100;
        //    std::random_device rd;
        //    std::mt19937 generator(rd());

        //    std::uniform_real_distribution<float> positionXZ_dist(-20.0f, 20.0f);
        //    std::uniform_real_distribution<float> positionY_dist(0.1f, 10.0f);
        //    std::uniform_real_distribution<float> rotation_dist(0.0f, glm::radians(360.0f));
        //    std::uniform_real_distribution<float> scale_dist(0.5f, 1.5f);

        //    for (int i = 0; i < numberOfInstances; ++i) {
        //        // --- Generate random transformation values ---
        //        glm::vec3 position(
        //            positionXZ_dist(generator),
        //            positionY_dist(generator),
        //            positionXZ_dist(generator)
        //        );
        //        float angle = rotation_dist(generator);
        //        glm::vec3 axis = glm::normalize(glm::vec3(0.2f, 1.0f, 0.1f));

        //        float scale = scale_dist(generator);
        //        glm::mat4 model = glm::mat4(1.0f);
        //        model = glm::translate(model, position);
        //        model = glm::rotate(model, angle, axis);
        //        model = glm::scale(model, glm::vec3(scale));
        //        instanceMatrices.push_back(model);
        //    }
        //    EntityID instanceBarrelsID = createEntity();

        //    
        //    auto barrelMesh = std::make_shared<BasicMesh>();
        //    barrelMesh->LoadMesh("Assets/lion_head/lion_head_4k.obj");
        //    barrelMesh->SetupInstancedArrays(instanceMatrices);
        //    InstancedMeshRenderer instancedCubeRenderer;
        //    instancedCubeRenderer.mesh = barrelMesh;
        //    instancedCubeRenderer.instanceMatrices = instanceMatrices;
        //    addComponent(instanceBarrelsID, std::move(instancedCubeRenderer));
        //}
    }

    void createSkybox() {
        // Set the skybox for the scene's renderer
        setSkybox(getAssetFullPath("skybox/").c_str(), faces);
    }
};