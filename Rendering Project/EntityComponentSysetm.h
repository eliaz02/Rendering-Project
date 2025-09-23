#pragma once
/**
    * @file Scene.h
    * @brief Defines a scene management system using an Entity-Component-System (ECS)
    * architecture and a complete deferred renderer.
    *
    * This file contains the core ECS framework for managing game objects and their data,
    * alongside a deferred rendering pipeline that supports dynamic lighting with shadows
    * for directional, spot, and point lights, and includes post-processing effects.
    */

#ifndef SCENE_H
#define SCENE_H 

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <typeindex>
#include <any>
#include <functional>   
#include <numeric>
#include <concepts>

#include "Utilities.h"
#include "Shader.h"
#include "Mesh.h"
#include "frameBufferObject.h"
#include "LightStruct.h"
#include "Camera.h"
#include "Skybox.h"
#include "Animation.h"
#include "Component.h"
#include "PathConfig.h"



class WindowContext;  //avoid circular declaratio #include "WindowContext.h"

// Entity Component System forms the foundation of the entire architecture.
// Rather than using traditional object - oriented inheritance hierarchies,
// this approach treats entities as simple numeric identifiers that serve as keys to access various components.

using EntityID = uint32_t;

// A non-templated base interface to allow storing different ComponentArray types in one map.
// This is a form of type erasure done manually for performance.
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entityDestroyed(EntityID entity) = 0;
};

template<IsComponent T>
class ComponentArray : public IComponentArray { 
private:
    // The tightly packed array of actual component data. This is the key to performance.
    std::vector<T> m_components; 

    // Maps from an EntityID to an index in the m_components vector.
    std::unordered_map<EntityID, size_t> m_entityToIndexMap; 

    // Maps from an index in the m_components vector back to an EntityID.
    std::unordered_map<size_t, EntityID> m_indexToEntityMap; 

public:
    /**
        * @brief add a Componet with an associated EntityID to the componentArray
    **/
    void addComponent(EntityID entity, T component) { 
        assert(m_entityToIndexMap.find(entity) == m_entityToIndexMap.end() && "Component added to same entity more than once."); 
        size_t newIndex = m_components.size();
        m_entityToIndexMap[entity] = newIndex;
        m_indexToEntityMap[newIndex] = entity;
        m_components.push_back(std::move(component));
    }
    /**
        * @brief Removes the component associated with a given entity.
        *
        * @details This function uses the "swap-and-pop" method to keep the component array
        *          tightly packed for cache efficiency. It moves the last element into the
        *          spot of the removed element, ensuring O(1) complexity.
        *
        * @param entity The ID of the entity whose component is to be removed.
        *
        * @pre The entity must have a component of this type stored in the array. This is
        *      enforced by an assertion in debug builds.
     **/
    void removeComponent(EntityID entity) {
        assert(m_entityToIndexMap.find(entity) != m_entityToIndexMap.end() && "Removing non-existent component.");

        size_t indexOfRemoved = m_entityToIndexMap[entity];
        size_t indexOfLast = m_components.size() - 1;

        m_components[indexOfRemoved] = std::move(m_components[indexOfLast]);


        EntityID entityOfLastElement = m_indexToEntityMap[indexOfLast];
        m_entityToIndexMap[entityOfLastElement] = indexOfRemoved;
        m_indexToEntityMap[indexOfRemoved] = entityOfLastElement;

        m_components.pop_back();
        m_entityToIndexMap.erase(entity);
        m_indexToEntityMap.erase(indexOfLast);
    }
/**
    * @brief gives the component associated with a given entity.
    *
    * @param entity The ID of the entity whose component is to be returned.
    *
    * @return return a pointer to the Componet with the EntityID, 
    * if the EntityID has no matching return a null pointer 
 **/
    T* getComponent(EntityID entity) {
        auto it = m_entityToIndexMap.find(entity);
        if (it == m_entityToIndexMap.end()) {
            return nullptr;
        }
        return &m_components[it->second];
    }

    //  function for fast iteration
    std::vector<T>& getComponentVector() {
        return m_components;
    }
    /**
        * @brief Provides read-only access to the map that links an EntityID to its
        * component's index in the tightly packed vector.
        * @return A constant reference to the entity-to-index map.
    */
    const std::unordered_map<EntityID, size_t>& getEntityToIndexMap() const {
        return m_entityToIndexMap;
    }

    void entityDestroyed(EntityID entity) override {
        if (m_entityToIndexMap.count(entity)) {
            removeComponent(entity);
        }
    }
    /**
    * @brief check if the data stuct is empty
    * @return return true if the data sttruct is empty, false otherwise 
*/
    bool empty() 
    {
        return (m_components.size() == 0);
    }
};


// ComponentStorage class acts as the central repository, 
// using a nested mapping system where the outer map is keyed by component type and the inner map associates entity IDs
class ComponentStorage {
private:
    // A map from component type to the storage object for that type.
    std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> m_componentArrays;
public:
    template<IsComponent T>
    void addComponent(EntityID entity, T component) {
        getComponentArray<T>()->addComponent(entity, std::move(component));
    }

    template<IsComponent T>
    T* getComponent(EntityID entity) {
        return getComponentArray<T>()->getComponent(entity);
    }

    template<IsComponent T>
    void removeComponent(EntityID entity) {
        getComponentArray<T>()->removeComponent(entity);
    }

    template<IsComponent T>
    bool hasComponent(EntityID entity) {
        return getComponent<T>(entity) != nullptr;
    }

    // Helper function to get the correctly typed ComponentArray.
    template<IsComponent T>
    ComponentArray<T>* getComponentArray() {
        std::type_index typeName = std::type_index(typeid(T));

        // Create the component array if it doesn't exist yet
        if (m_componentArrays.find(typeName) == m_componentArrays.end()) {
            m_componentArrays[typeName] = std::make_unique<ComponentArray<T>>();
        }

        return static_cast<ComponentArray<T>*>(m_componentArrays[typeName].get());
    }

    // When an entity is destroyed, we must notify each component array
    // so it can remove the entity's components.
    void entityDestroyed(EntityID entity) {
        for (auto const& [type, array] : m_componentArrays) {
            array->entityDestroyed(entity);
        }
    }
};



// Components themselves are pure data structures that describe different aspects of objects
struct Transform : public Component
{
    glm::quat rotation= glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity quaterion 
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.f);
};

struct Animation : public Component {
    std::unique_ptr<AbstractAnimation> animation;
    bool isPlaying = true;
    bool loop = true;
};

struct MeshRenderer : public Component {
    std::shared_ptr<BasicMesh> mesh;
    bool castShadows = true;
    bool receiveShadows = true;
};

struct InstancedMeshRenderer : public Component {
    std::shared_ptr<BasicMesh> mesh;
    std::vector<glm::mat4> instanceMatrices;
};


struct RenderCommand 
{
    glm::mat4 modelMatrix;
    std::shared_ptr<BasicMesh> mesh;
    bool castShadows{ true };
    bool receiveShadows{ true };
};

struct InstancedRenderCommand
{
    std::shared_ptr<BasicMesh> mesh;
    std::vector<glm::mat4> instances;
};

struct LightData
{
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    DirLight sunLight;

};

class IRenderer
{
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
    virtual WindowContext& getContext() = 0;
protected:
    // Prevent creating an IRenderer directly. Only derived classes can.
    IRenderer() = default;
    // Prevent copying/moving the interface itself.
    IRenderer(const IRenderer&) = delete;
    IRenderer& operator=(const IRenderer&) = delete;
    
};
//
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
    
    enum GbufferBind
    {
        Position = 0,
        NormalShininess = 1,
        ColorSpec = 2,
        Depth = 3
    };
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

        shader->load(getShaderFullPath("shadowMap.vert").c_str(), getShaderFullPath("shadowMap.frag").c_str());

        shaderBox->load(getShaderFullPath("shadowMapPoint.vert").c_str(), getShaderFullPath("shadowMapPoint.frag").c_str() , getShaderFullPath("shadowMapPoint.geom").c_str() );

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

    // resize the frame buffer object to match the window size
    void resize() override 
    {

        gbuffer->Resize(m_context.getWidth(), m_context.getHeight()); 
        fxaa->resize(m_context.getWidth(), m_context.getHeight());
    }

    void setSkybox(const std::string& path, const std::vector<std::string>& faces) override 
    {
        skybox = std::make_unique<Skybox>();
        skybox->load(path.c_str(), faces);
    }
    
    WindowContext& getContext()
    {
        return m_context;
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

        gbuffer->shaderInstanced->use();
        gbuffer->shaderInstanced->setMat4("projection", this->projectionMatrix);
        gbuffer->shaderInstanced->setMat4("view", this->viewMatrix);
        // Render instanced objects
        for (const auto& insCmd : instancedCommands) { //// wip 
            // Set instance matrices
            insCmd.mesh->SetupInstancedArrays(insCmd.instances);
            insCmd.mesh->RenderInstanced(gbuffer->shaderInstanced, insCmd.instances.size());
        }
        gbuffer->UnBind();        
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
        shader->setInt("gPosition", GbufferBind::Position);
        shader->setInt("gNormalShininess", GbufferBind::NormalShininess);
        shader->setInt("gColorSpec", GbufferBind::ColorSpec);
        shader->setInt("gDepth", GbufferBind::Depth);

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

            shader->setFloat(idx + ".constant", light.constant);
            shader->setFloat(idx + ".linear", light.linear);
            shader->setFloat(idx + ".quadratic", light.quadratic);

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

   
    virtual void initialize() {
        renderer->initialize();
        loadScene(); 
    }
    // Scene lifecycle
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

    void collectRenderCommands() 
    {
        // Get the component arrays you will need.
        ComponentArray<MeshRenderer>* meshRendererArray = static_cast<ComponentArray<MeshRenderer>*>(components.getComponentArray<MeshRenderer>());
        ComponentArray<Transform>* transformArray = static_cast<ComponentArray<Transform>*>(components.getComponentArray<Transform>());
        ComponentArray<Animation>* animationArray = static_cast<ComponentArray<Animation>*>(components.getComponentArray<Animation>());
              
        // Iterate over all entities that have a MeshRenderer.
        for (const auto& [entityID, meshRendererIndex] : meshRendererArray->getEntityToIndexMap()) 
        {

            Transform* transform = transformArray->getComponent(entityID);


            // If the entity also has a Transform component, create the command.
            if (transform) 
            {
                const MeshRenderer& meshRenderer = meshRendererArray->getComponentVector()[meshRendererIndex]; 
                    RenderCommand cmd; 

                    // Base object transformation components
                glm::mat4 basePositionMatrix = glm::translate(glm::mat4(1.f), transform->position);

                glm::mat4 baseRotationMatrix = glm::mat4_cast(transform->rotation);
                glm::mat4 baseScaleMatrix = glm::scale(glm::mat4(1.0f), transform->scale);

                    
                glm::mat4 finalObjectTransform;
                   
                    
                cmd.mesh = meshRenderer.mesh;         // Data from MeshRenderer component 
                cmd.castShadows = meshRenderer.castShadows; 
                cmd.receiveShadows = meshRenderer.receiveShadows;

                // get the AnimationComponent using the EntityID
                Animation* animComponent = animationArray->getComponent(entityID);
                if (animComponent && animComponent->isPlaying && animComponent->animation)
                {
                    animComponent->animation->updateTime(renderer->getContext().getTotalTime());
                    glm::vec3 animPosition = animComponent->animation->getPosition(); 
                    glm::quat animRotation = animComponent->animation->getRotation(); 
                    glm::vec3 animScale = animComponent->animation->getScale(); 
                    glm::mat4 animatedPositionMatrix = glm::translate(glm::mat4(1.0f), transform->position + animPosition); 
                    glm::mat4 animatedRotationMatrix = glm::mat4_cast(animRotation); 
                    glm::mat4 animatedScaleMatrix = glm::scale(glm::mat4(1.0f), animScale); 

                    // Combine them. order (from left to right) 
                    // First: scale (base scaling, animation scaling)
                    // Second: rotation (base rotation, animate rotation )
                    // third: traslation (base traslation, animate rotation ) 
                    finalObjectTransform = animatedPositionMatrix * animatedRotationMatrix * baseRotationMatrix  * animatedScaleMatrix * baseScaleMatrix;
                }
                else
                {
                    // This will be modified if an animation is present
                    finalObjectTransform = basePositionMatrix * baseRotationMatrix * baseScaleMatrix;
                }


                cmd.modelMatrix = finalObjectTransform; 
                renderer->submitRenderCommand(cmd);
            }
        }

        ComponentArray<InstancedMeshRenderer>* instancedArray = static_cast<ComponentArray<InstancedMeshRenderer>*>( components.getComponentArray<InstancedMeshRenderer>());

        if (instancedArray) {
            // We can iterate directly over all instanced components
            for (auto& instancedRenderer : instancedArray->getComponentVector()) {
                if (instancedRenderer.mesh && !instancedRenderer.instanceMatrices.empty()) {
                    InstancedRenderCommand cmd;
                    cmd.mesh = instancedRenderer.mesh;
                    cmd.instances = instancedRenderer.instanceMatrices;
                    renderer->submitInstancedRenderCommand(cmd);
                }
            }
        }
    }

    // setup all the light in the scene 
    void collectLightData() {
        LightData lights;

        // --- Collect Point Lights ---
        auto pointLightEntities = static_cast<ComponentArray<PointLight>*>(components.getComponentArray<PointLight>());
        for (const auto& [entityID, meshRendererIndex] : pointLightEntities->getEntityToIndexMap()) {
            auto* light = components.getComponent<PointLight>(entityID);
            if (light) {
                lights.pointLights.push_back(*light);
            }
        }

        // --- Collect Spot Lights ---
        auto spotLightEntities = static_cast<ComponentArray<SpotLight>*>(components.getComponentArray<SpotLight>());
        for (const auto& [entityID, meshRendererIndex] : spotLightEntities->getEntityToIndexMap()) {
            auto* light = components.getComponent<SpotLight>(entityID);
            if (light) {
                lights.spotLights.push_back(*light);
            }
        }

        // --- Collect Directional Light (The Sun) ---
        // The renderer expects only one primary directional light.
        // We will find the first entity with a DirLight component and use that.
        auto dirLightArray = static_cast<ComponentArray<DirLight>*> (components.getComponentArray<DirLight>());
        if (dirLightArray && !dirLightArray->empty()) {
            auto& lightsVector = dirLightArray->getComponentVector();
            lights.sunLight = lightsVector[0]; // Get the first element

            if (lightsVector.size() > 1) {
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
        DirLight sunLight;
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

            PointLight pointLight(position, ambient, diffuse, specular, near, far);
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
            SpotLight pointLight(
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