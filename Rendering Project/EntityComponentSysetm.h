    #pragma once

#ifndef SCENE_H
#define SCENE_H 

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <typeindex>
#include <any>
#include <functional>

#include "Utilities.h"
#include "Shader.h"
#include "Mesh.h"
#include "frameBufferObject.h"
#include "LightStruct.h"
#include "Camera.h"
#include "Skybox.h"



class WindowContext;  //avoid circular declaratio #include "WindowContext.h"

// Entity Component System forms the foundation of the entire architecture.
// Rather than using traditional object - oriented inheritance hierarchies,
// this approach treats entities as simple numeric identifiers that serve as keys to access various components.

using EntityID = uint32_t;

// ComponentStorage class acts as the central repository, 
// using a nested mapping system where the outer map is keyed by component type and the inner map associates entity IDs
class ComponentStorage {
private:
    std::unordered_map
        <std::type_index, // identify the Position component type
        std::unordered_map<EntityID, std::any>> components; // list of component 

public:
    template<typename T>
    void addComponent(EntityID entity, T component) {
        components[std::type_index(typeid(T))][entity] = std::move(component);
    }

    template<typename T>
    T* getComponent(EntityID entity) {
        // Find the drawer for type T
        auto typeIt = components.find(std::type_index(typeid(T)));
        if (typeIt == components.end()) return nullptr; // No components of this type exist

        //  Find the folder for the entity in that drawer
        auto entityIt = typeIt->second.find(entity);
        if (entityIt == typeIt->second.end()) return nullptr; // This entity doesn't have this component

        // Get the component out of its std::any wrapper
        return std::any_cast<T>(&entityIt->second);
    }

    template<typename T>
    bool hasComponent(EntityID entity) {
        return getComponent<T>(entity) != nullptr;
    }

    template<typename T>
    void removeComponent(EntityID entity) {
        auto typeIt = components.find(std::type_index(typeid(T)));
        if (typeIt != components.end()) {
            typeIt->second.erase(entity);
        }
    }

    template<typename T>
    std::vector<EntityID> getEntitiesWith() {
        std::vector<EntityID> result;
        auto typeIt = components.find(std::type_index(typeid(T)));
        if (typeIt != components.end()) {
            for (const auto& pair : typeIt->second) {
                result.push_back(pair.first);
            }
        }
        return result;
    }
};

// Components themselves are pure data structures that describe different aspects of game objects
struct Transform {
    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct MeshRenderer {
    std::shared_ptr<BasicMesh> mesh;
    bool castShadows = true;
    bool receiveShadows = true;
};

using PointLightComponent = PointLight;
using SpotLightComponent = SpotLight;
using DirectionalLightComponent = DirLight;

struct RenderCommand {
    glm::mat4 modelMatrix;
    std::shared_ptr<BasicMesh> mesh;
    bool castShadows{ true };
    bool receiveShadows{ true };
};

struct InstancedRenderCommand {
    std::shared_ptr<BasicMesh> mesh;
    std::vector<glm::mat4> instances;
};

struct LightData {
    std::vector<PointLightComponent> pointLights;
    std::vector<SpotLightComponent> spotLights;
    DirectionalLightComponent sunLight;

};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void initialize() = 0;
    virtual void beginFrame() = 0;
    virtual void submitRenderCommand(const RenderCommand& command) = 0;
    virtual void submitInstancedRenderCommand(const InstancedRenderCommand& command) = 0;
    virtual void setLightData(const LightData& lights) = 0;
    virtual void setSkybox(const std::string& path, const std::vector<std::string>& faces) = 0;
    virtual void endFrame() = 0;
    virtual void resize() = 0;
protected:
    // Prevent creating an IRenderer directly. Only derived classes can.
    IRenderer() = default;
    // Prevent copying/moving the interface itself.
    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;
};


class DeferredRenderer : public IRenderer {
private:
    // Frame Buffer Objects
    std::unique_ptr<FXAA> fxaa;
    std::unique_ptr<GBufferFBO> gbuffer;
    std::unique_ptr<ShadowMapFBO> shadowDirMap;
    std::unique_ptr<ShadowMapArrayFBO> shadowSpotMap;
    std::unique_ptr<ShadowMapCubeFBO> shadowPointMap;
    bool m_spotShadowsInitialized = false;
    bool m_pointShadowsInitialized = false;
    // Render commands for this frame
    std::vector<RenderCommand> renderCommands;
    std::vector<InstancedRenderCommand> instancedCommands;
    LightData lightData;
    std::unique_ptr<Skybox> skybox;

    // Camera data
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;

    WindowContext& m_context;

public:
    DeferredRenderer(WindowContext& conetext) :
        viewMatrix{ glm::mat4(0.f) },
        projectionMatrix{ glm::mat4(0.f) },
        modelMatrix{ glm::mat4(0.f) },
        m_context(conetext)
            {};

    void initialize() override
    {
        // Instance FBOs
        gbuffer = std::make_unique<GBufferFBO>();
        shadowDirMap = std::make_unique<ShadowMapFBO>(3000, 3000);
        shadowSpotMap = std::make_unique<ShadowMapArrayFBO>(1024, 1024);
        shadowPointMap = std::make_unique<ShadowMapCubeFBO>(1024);
        fxaa = std::make_unique<FXAA>();

        // Inizialize shader for shadow casting
        auto shader = std::make_shared<Shader>();
        auto shaderBox = std::make_shared<Shader>();

        shader->load("shader/shadowMap.vert", "shader/shadowMap.frag");

        shaderBox->load("shader/shadowMapPoint.vert", "shader/shadowMapPoint.frag", "shader/shadowMapPoint.geom");

        // Inizialize FBOs
        gbuffer->Init(m_context.getWidth(),m_context.getHeight());
        fxaa->init(m_context.getWidth(), m_context.getHeight());
        shadowDirMap->Init( shader );
        shadowSpotMap->SetupShader( shader );
        shadowPointMap->SetupShader( shaderBox );
    }

    void beginFrame() override 
    {
        // Clear previous frame data
        renderCommands.clear();
        instancedCommands.clear();
        lightData = {};

        // Update camera matrices
        viewMatrix = m_context.getCamera().GetViewMatrix();

        projectionMatrix = glm::perspective(glm::radians(45.0f), (float)m_context.getWidth() / (float)m_context.getHeight(), 0.01f, 100.0f);

        modelMatrix = glm::mat4(1.0f);
    }

    void submitRenderCommand(const RenderCommand& command) override {
        renderCommands.push_back(command);
    }

    void submitInstancedRenderCommand(const InstancedRenderCommand& command) override {
        instancedCommands.push_back(command);
    }

    void setLightData(const LightData& lights) override 
    {
        if (!m_spotShadowsInitialized && !lights.spotLights.empty()) 
        {
            shadowSpotMap->Init(lights.spotLights.size());
            m_spotShadowsInitialized = true;
        }
        if (!m_pointShadowsInitialized && !lights.pointLights.empty())
        {
            shadowPointMap->Init(lights.pointLights.size());
            m_pointShadowsInitialized = true;
        }

        lightData = lights;
    }

    void endFrame() override 
    {
        // Geometry pass
        renderGeometryPass();

        // Shadow pass
        renderShadowMaps();

        // Lighting pass
        renderLightingPass();

        // Forward pass (transparent objects, skybox)
        renderForwardPass();

        // Post-processing
        renderPostProcessing();
    }

    void resize() override 
    {
        gbuffer->Resize(m_context.getWidth(), m_context.getHeight()); // wip remove global SRC 
        fxaa->resize(m_context.getWidth(), m_context.getHeight());
    }

    void setSkybox(const std::string& path, const std::vector<std::string>& faces) override {
        // Crea un nuovo oggetto Skybox e lo carica immediatamente.
        // È l'unico che esiste, quindi non ci saranno problemi di copia/distruzione.
        skybox = std::make_unique<Skybox>();
        skybox->load(path.c_str(), faces);
    }

private:
    void renderShadowMaps()
    {

        // decrease peter panning 
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // Sunlight shadow casting 
        shadowDirMap->BindForWriting();
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowDirMap->shader->use();
        glm::mat4 lightSpaceMatrix = lightData.sunLight.Projection * lightData.sunLight.View;
        shadowDirMap->shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        for (auto [modelMatrix, mesh, castShadows, receiveShadows] : renderCommands)
        {
            if (!castShadows) continue;

            shadowDirMap->shader->setMat4("model", modelMatrix);
            mesh->Render(shadowDirMap->shader);
        }

        // SpotLight shadow casting  
        shadowSpotMap->shader->use();
        for (size_t i{ 0 }; i < lightData.spotLights.size(); i++)
        {
            shadowSpotMap->BindLayerForWriting(static_cast<int>(i));

            glClear(GL_DEPTH_BUFFER_BIT);

            const auto light = lightData.spotLights[i];
            glm::mat4 lightSpaceMatrix = light.Projection * light.View;

            shadowSpotMap->shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

            for (auto [modelMatrix, mesh, castShadows, receiveShadows] : renderCommands)
            {
                if (!castShadows) continue;

                shadowSpotMap->shader->setMat4("model", modelMatrix);
                mesh->Render(shadowSpotMap->shader);
            }
        }

        // Pointlight shadow casting
        if (m_pointShadowsInitialized && !lightData.pointLights.empty())
        {
            shadowPointMap->BindForWriting(0);
            glClear(GL_DEPTH_BUFFER_BIT);

            shadowPointMap->shader->use();
            for (size_t i = 0; i < lightData.pointLights.size(); ++i)
            {
                // Set current light index
                shadowPointMap->shader->setInt("lightIndex", static_cast<int>(i));

                // Your existing setupUniformShader call
                shadowPointMap->setupUniformShader(&lightData.pointLights[i]);

                for (auto [modelMatrix, mesh, castShadows, receiveShadows] : renderCommands)
                {
                    if (!castShadows) continue;

                    shadowPointMap->shader->setMat4("model", modelMatrix);
                    mesh->Render(shadowPointMap->shader);
                }
            }
        }
        glCullFace(GL_BACK);
    }

    void renderGeometryPass() {
        gbuffer->BindForWriting();
        gbuffer->shaderGeom->use();
        // Render normal objects
        for (const auto& cmd : renderCommands) {
            // Set matrices for the mesh's shader

            gbuffer->shaderGeom->setMat4("projection", this->projectionMatrix);
            gbuffer->shaderGeom->setMat4("view", this->viewMatrix);
            gbuffer->shaderGeom->setMat4("model", cmd.modelMatrix);
            // BasicMesh handles its own material and texture binding
            cmd.mesh->Render(gbuffer->shaderGeom);
        }

        // Render instanced objects
        //for (const auto& cmd : instancedCommands) { //// wip totaly differnt handeling 
        //    // Set instance matrices
        //    cmd.mesh->setInstanceMatrices(cmd.instances);
        //    cmd.mesh->renderInstanced(cmd.instances.size());
        //}

        gbuffer->UnBind(); // wip implement un bind 
    }

    void renderLightingPass()
    {
        fxaa->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto shader = gbuffer->shaderLighting;
        shader->use();
        // bind GBuffer for reading and uniform 
        gbuffer->BindForReading(0);
        // wip realy bad magic number
        shader->setInt("gPosition", 0);
        shader->setInt("gNormalShininess", 1);
        shader->setInt("gColorSpec", 2);
        shader->setInt("gDepth", 3);

        // bind shadow map for point light and uniform
        if (m_pointShadowsInitialized) {
            shadowPointMap->BindForReading(SHADOW_MAP_CUBE_UNIT);
            shader->setInt("shadowCubeArray", SHADOW_MAP_CUBE_UNIT);
        }
        

        // bind shadow map for directional light and uniform
        shadowDirMap->BindForReading(SHADOW_MAP_DIR_UNIT);
        shader->setInt("shadowDir", SHADOW_MAP_DIR_UNIT);
        glm::mat4 lightSpaceMatrix = lightData.sunLight.Projection * lightData.sunLight.View;
        shader->setMat4("dirLightSpaceMatrice", lightSpaceMatrix);

        // bind shadow map for spotlight and uniform
        shadowSpotMap->BindForReading(SHADOW_MAP_SPOT_UNIT);
        shader->setInt("shadowSpotArray", SHADOW_MAP_SPOT_UNIT);

        // setup view position in the scene 
        shader->setVec3("viewPos", m_context.getCamera().Position);

        //      Set unifrom point lights   //
        shader->setInt("numPointLights", static_cast<int>(lightData.pointLights.size()));
        for (size_t i = 0; i < lightData.pointLights.size(); ++i)
        {
            const auto& light = lightData.pointLights[i];
            std::string idx = "pointLights[" + std::to_string(i) + "]";

            shader->setVec3(idx + ".position", light.Pos);
            shader->setVec3(idx + ".light.ambient", light.Ambient);
            shader->setVec3(idx + ".light.diffuse", light.Diffuse);
            shader->setVec3(idx + ".light.specular", light.Specular);

            // wip cange this magic number attenuation terms per light
            shader->setFloat(idx + ".constant", 1.0f);
            shader->setFloat(idx + ".linear", 0.09f);
            shader->setFloat(idx + ".quadratic", 0.032f);

            shader->setFloat(idx + ".far_plane", light.far_plane);
            shader->setInt(idx + ".shadowID", static_cast<int>(i));
        }
        //      Set uniform spot lights     //
        shader->setInt("numSpotLights", static_cast<int>(lightData.spotLights.size()));
        for (size_t i = 0; i < lightData.spotLights.size(); ++i)
        {
            const auto& light = lightData.spotLights[i];
            std::string idx = "spotLights[" + std::to_string(i) + "]";

            glm::mat4 lightSpaceMatrix = light.Projection * light.View;
            shader->setMat4(idx + ".SpaceMatrices", lightSpaceMatrix);

            // Position and direction
            shader->setVec3(idx + ".position", light.Pos);
            shader->setVec3(idx + ".direction", light.Dir);

            // Light properties
            shader->setVec3(idx + ".light.ambient", light.Ambient);
            shader->setVec3(idx + ".light.diffuse", light.Diffuse);
            shader->setVec3(idx + ".light.specular", light.Specular);

            // Attenuation
            shader->setFloat(idx + ".constant", light.constant);
            shader->setFloat(idx + ".linear", light.linear);
            shader->setFloat(idx + ".quadratic", light.quadratic);

            // Spotlight specific
            shader->setFloat(idx + ".cutOff", light.cutOff);
            shader->setFloat(idx + ".outerCutOff", light.outerCutOff);

            // Shadow mapping
            shader->setFloat(idx + ".far_plane", light.far_plane);
            shader->setInt(idx + ".shadowID", static_cast<int>(i));
        }

        //      Set unifrom point lights    //
        {
            std::string idx = "dirLight";

            shader->setVec3(idx + ".position", lightData.sunLight.Position); 
            shader->setVec3(idx + ".direction", lightData.sunLight.Direction); 
            shader->setVec3(idx + ".light.ambient", lightData.sunLight.Ambient); 
            shader->setVec3(idx + ".light.diffuse", lightData.sunLight.Diffuse); 
            shader->setVec3(idx + ".light.specular", lightData.sunLight.Specular); 

        }
        gbuffer->Render();
        fxaa->unbind();

    }

    void renderForwardPass() {
        // switch form deferred randering to forward rendering 
        glViewport(0, 0, m_context.getWidth(), m_context.getHeight());

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->fbo);

        // Write to the FXAA FBO (you'll need a getter for this)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fxaa->framebuffer);

        // Blit the depth buffer contents
        glBlitFramebuffer(0, 0, m_context.getWidth(), m_context.getHeight(), 0, 0, m_context.getWidth(), m_context.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, fxaa->framebuffer);

        glEnable(GL_DEPTH_TEST);
        // render skybox
        if (skybox) {
            skybox->Render(projectionMatrix, viewMatrix);
        }
    }

    void renderPostProcessing() {
        fxaa->render();
    }

};

// ============================================================================
// SCENE SYSTEM
// ============================================================================

class Scene {
private:
    ComponentStorage components;
    EntityID nextEntityID = 1;
    std::unique_ptr<IRenderer> renderer;

public:
    Scene(std::unique_ptr<IRenderer> r) : renderer(std::move(r)) {}

    virtual ~Scene() = default;

    // Entity management
    EntityID createEntity() {
        return nextEntityID++;
    }

    template<typename T>
    void addComponent(EntityID entity, T component) {
        components.addComponent(entity, std::move(component));
    }

    template<typename T>
    T* getComponent(EntityID entity) {
        return components.getComponent<T>(entity);
    }

    template<typename T>
    bool hasComponent(EntityID entity) {
        return components.hasComponent<T>(entity);
    }

    void setSkybox(const std::string& path, const std::vector<std::string>& faces) {
        if (renderer) {
            renderer->setSkybox(path, faces);
        }
    }

    // Scene lifecycle
    virtual void initialize() {
        renderer->initialize();
        loadScene(); // Virtual method for derived classes
    }

    void render() {
        renderer->beginFrame();

        // Collect render commands from entities
        collectRenderCommands();

        // Collect light data
        collectLightData();

        renderer->endFrame();
    }

    void resize() {
        renderer->resize();
    }

protected:
    virtual void loadScene() = 0; // Pure virtual - derived classes implement

    ComponentStorage& getComponents() { return components; }

private:
    void collectRenderCommands() {
        // Collect regular mesh renderers
        auto meshEntities = components.getEntitiesWith<MeshRenderer>();
        for (EntityID entity : meshEntities) {
            auto* transform = components.getComponent<Transform>(entity);
            auto* meshRenderer = components.getComponent<MeshRenderer>(entity);

            if (transform && meshRenderer) {
                RenderCommand cmd;
                cmd.modelMatrix = transform->matrix;
                cmd.mesh = meshRenderer->mesh;
                cmd.castShadows = meshRenderer->castShadows;
                cmd.receiveShadows = meshRenderer->receiveShadows;

                renderer->submitRenderCommand(cmd);
            }
        }

        // wip instance rendering
        // Collect instanced renderers 
        /*auto instancedEntities = components.getEntitiesWith<InstancedRenderer>();
        for (EntityID entity : instancedEntities) {
            auto* instancedRenderer = components.getComponent<InstancedRenderer>(entity);
            if (instancedRenderer) {
                InstancedRenderCommand cmd;
                cmd.mesh = instancedRenderer->mesh;
                cmd.instances = instancedRenderer->instances;

                renderer->submitInstancedRenderCommand(cmd);
            }
        }*/
    }

    void collectLightData() {
        LightData lights;

        // --- Collect Point Lights ---
        auto pointLightEntities = components.getEntitiesWith<PointLightComponent>();
        for (EntityID entity : pointLightEntities) {
            auto* light = components.getComponent<PointLightComponent>(entity);
            if (light) {
                lights.pointLights.push_back(*light);
            }
        }

        // --- Collect Spot Lights ---
        auto spotLightEntities = components.getEntitiesWith<SpotLightComponent>();
        for (EntityID entity : spotLightEntities) {
            // We only need the light component itself.
            auto* light = components.getComponent<SpotLightComponent>(entity);
            if (light) {
                lights.spotLights.push_back(*light);
            }
        }

        // --- Collect Directional Light (The Sun) ---
        // The renderer expects only one primary directional light.
        // We will find the first entity with a DirLight component and use that.
        auto dirLightEntities = components.getEntitiesWith<DirectionalLightComponent>();
        if (!dirLightEntities.empty()) {
            EntityID sunEntity = dirLightEntities[0]; // Get the first one
            auto* light = components.getComponent<DirectionalLightComponent>(sunEntity);
            if (light) {
                lights.sunLight = *light;
            }
            // Log a warning if more than one sun is found, as only the first is used.
            if (dirLightEntities.size() > 1) {
                std::cout << "Warning: Multiple DirectionalLights found, but only one is supported. Using the first one." << std::endl;
            }
        }

        renderer->setLightData(lights);
    }
};

// ============================================================================
// CONCRETE SCENE IMPLEMENTATION
// ============================================================================
/*
class CarScene : public Scene {
public:
    CarScene(std::unique_ptr<IRenderer> renderer) : Scene(std::move(renderer)) {}

protected:
    void loadScene() override {
        // Create car entity
        EntityID car = createEntity();

        Transform carTransform;
        carTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        addComponent(car, carTransform);

        MeshRenderer carRenderer;
        // Create BasicMesh directly - it handles material and texture internally
        carRenderer.mesh = std::make_shared<BasicMesh>("car.obj");
        addComponent(car, carRenderer);

        // Create ground with instanced grass
        //EntityID grassField = createEntity();
        //InstancedRenderer grassRenderer;
        //grassRenderer.mesh = std::make_shared<BasicMesh>("grass_blade.obj");

        // Generate grass instances
        //for (int x = -50; x < 50; x++) {
        //    for (int z = -50; z < 50; z++) {
        //        glm::mat4 instanceMatrix = glm::mat4(1.0f);
        //        instanceMatrix = glm::translate(instanceMatrix, glm::vec3(x, 0, z));
        //        grassRenderer.instances.push_back(instanceMatrix);
        //    }
        //}
        //addComponent(grassField, grassRenderer);

        // Create sun light
        EntityID sun = createEntity();
        DirectionalLightComponent sunLight;
        Sunlight = DirLight{ glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.3f), 0.1f, 100.f };
        addComponent(sun, sunLight);

        // Create point lights
        float near = 1.0f;
        float far = 25.0f;
        for (int i = 0; i < 10; ++i) {
            EntityID light = createEntity();
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

            PointLightComponent pointLight(position, ambient, diffuse, specular, near, far);
            addComponent(light, pointLight);
        }
        // Create spot lights
        // Seed the random number generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        // Define common attenuation and cutoff values
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::vec2 cut = glm::vec2(glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)));
        glm::vec3 attenuation = glm::vec3(1.0f, 0.09f, 0.032f); // constant, linear, quadratic
        for (size_t i = 0; i < colors.size(); i++)
        {
            EntityID light = createEntity();
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
            SpotLightComponent pointLight(
                pos, dir,
                color * 0.1f, // Ambient
                color,        // Diffuse
                color * 0.5f, // Specular
                near_plane,
                far_plane,
                cut,
                attenuation
            );
            addComponent(light, pointLight);

        }
    }
};
*/
// ============================================================================
// USAGE EXAMPLE
// ============================================================================

//int main() {
//    // Create renderer directly
//    auto renderer = std::make_unique<DeferredRenderer>();
//
//    // Create scene
//    auto scene = std::make_unique<CarScene>(std::move(renderer));
//
//    // Initialize
//    scene->initialize(1920, 1080);
//
//    // Main loop
//    Camera camera;
//    while (true) {
//        // Update camera, input, etc.
//
//        // Render
//        scene->render(camera);
//
//        // Swap buffers, etc.
//    }
//
//    return 0;
//}
#endif // !SCENE_H