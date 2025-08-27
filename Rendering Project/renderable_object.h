#pragma once

#ifndef RENDERABLE_OBJECT_STRUCT_H
#define RENDERABLE_OBJECT_STRUCT_H

#include <glad/gl.h>       // Includi prima di qualsiasi altro file OpenGL
#define GLFW_INCLUDE_NONE    // Evita che GLFW includa automaticamente gl.h
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  
#include <algorithm>
#include <numeric>
#include <vector>      // Per std::vector
#include <memory>      // Per std::shared_ptr, std::unique_ptr
#include <random>      // Per std::random_device, std::mt19937, std::uniform_real_distribution, std::uniform_int_distribution
#include <iterator>    // Per std::is_sorted (C++11)
#include <functional>  // Per std::less<> (C++14)
#include <cmath>       // Per fmod (usato nella funzione updateTime)

// Nota: alcune funzioni potrebbero richiedere anche:
#include <iostream>    // Se usi std::cout per debug
#include <string>

#include "Shader.h"
#include "Mesh.h"

// the fucking jump go to WIP to try to understand whats going on there 

inline int segcounter = 0;
inline int segnum = 0;

class AbstractAnimation
{
protected:
    std::vector<float> timeStamp;
    std::vector<glm::vec3> controlPoint;
    std::vector<glm::quat> rotation;
    std::vector<glm::vec3> scale;
    float time;
public:
    // pure virtual
    virtual void updateTime(float time) = 0;
    virtual glm::vec3 getPosition() = 0;
    virtual glm::quat getRotation() = 0;
    virtual glm::vec3 getScale() = 0;

    // Virtual destructor for proper cleanup
    virtual ~AbstractAnimation() = default;

};

// Structure for B-spline transformation sequence
class BSplineAnimation : public AbstractAnimation {
public:
    BSplineAnimation(const std::vector<glm::vec3>& points, const std::vector<float>& time, bool isclosed) :
        isClosed{ isclosed }
    {
        // For closed B-spline, we need to extend the control points array
        // Remove duplicate last point if it exists (same as first point)
        std::vector<glm::vec3> uniquePoints = points;
        if (glm::length(points.front() - points.back()) < 0.0001f) {
            uniquePoints.pop_back(); // Remove duplicate last point
        }

        // adjust time to the standar format (this is the standard since its more efficent)
        // standard eg. {0.0f, 1.0f, 2.0f, 3.0f, 4.0f}
        // not standard eg.{1.0f, 1.0f, 1.0f, 1.0f, 1.0f}  
        std::vector<float> standardTime{ time };
        if (!std::is_sorted(time.begin(), time.end(), std::less<>())) // this check if is not strictly increasing
            std::partial_sum(time.begin(), time.end(), standardTime.begin());
        unsigned int n = static_cast<unsigned int> (uniquePoints.size());
        if (n < 4) throw std::invalid_argument("Need at least 3 points for closed spline");


        if (isclosed) {
            // Normally a B-spline with n control points has n-3 segments
            // But for closed curves, you add extra points to wrap around
            // This changes the timestamp requirements to k
            if (standardTime.size() < n) throw std::invalid_argument("Not enough time stamp");
            // Add all original points
            for (const auto& point : uniquePoints) {
                controlPoint.push_back(point);
            }

            // Add first 3 points to the end
            controlPoint.push_back(uniquePoints[0]);
            controlPoint.push_back(uniquePoints[1]);
            controlPoint.push_back(uniquePoints[2]);

            for (unsigned int i = 0; i <= n; ++i) {
                if (i < standardTime.size()) {
                    timeStamp.push_back(standardTime[i]);
                }
            }
        }
        else {
            // For open B-spline, use points as it is
            controlPoint = uniquePoints;
            if (standardTime.size() < n - 3) throw std::invalid_argument("Not enough time stamp");
            timeStamp = standardTime;
        }
    }

    void updateTime(float t) override
    {
        time = t;

        // Clamp time to valid range
        if (timeStamp.empty()) return;

        float minTime = timeStamp.front();
        float maxTime = timeStamp.back();

        if (isClosed) {
            // For closed curves, wrap time around
            float duration = maxTime - minTime;
            if (duration > 0) {
                // Normalize time to [0, 1] range within the duration
                float normalizedTime = fmod(t - minTime, duration);
                if (normalizedTime < 0) normalizedTime += duration;
                time = minTime + normalizedTime;
            }
        }
        else {
            // For open curves, clamp to range
            time = glm::clamp(t, minTime, maxTime);
        }

        // Find the segment and compute coefficients
        computeCoefficients();
    }

    glm::vec3 getPosition() override
    {
        if (controlPoint.size() < 4 || timeStamp.empty()) {
            return glm::vec3(0.0f);
        }

        // Get the current segment index using binary search
        int segment = findSegment();
        if (segment < 0) return controlPoint.front();

        // Get the four control points for this segment
        std::vector<glm::vec3> segmentPoints = getSegmentControlPoints(segment);

        // Compute B-spline position using the coefficient vector
        glm::vec3 position =
            coef[0] * segmentPoints[0] +
            coef[1] * segmentPoints[1] +
            coef[2] * segmentPoints[2] +
            coef[3] * segmentPoints[3];

        return position;
    }

    glm::quat getRotation() override
    {
        glm::vec3 currentTangent = getTangent();

        // Smoothly rotate from previous direction to current tangent
        glm::quat deltaRotation = rotationBetweenVectors(previousTangent, currentTangent);
        currentRotation = deltaRotation * currentRotation;

        previousTangent = currentTangent;
        return currentRotation;
    }

    glm::vec3 getScale() override {
        return glm::vec3(1.0f);
    }

private:
    bool isClosed;
    std::vector<float> coef{ 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 previousTangent{ 0.0f,0.0f ,1.0f };
    glm::quat currentRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    int findSegment()
    {
        if (timeStamp.size() < 2) return 0;

        // Binary search for open curves
        int left = 0; int right = static_cast<int>(timeStamp.size()) - 1;

        while (left <= right) {
            int mid = left + ((right - left) / 2);

            if (time >= timeStamp[mid] && time <= timeStamp[mid + 1]) {
                return mid;
            }
            else if (time < timeStamp[mid]) {
                right = mid - 1;
            }
            else {
                left = mid + 1;
            }
        }

        // Fallback: return the last valid segment
        return std::max(0, static_cast<int>(timeStamp.size()) - 2);

    }

    void computeCoefficients()
    {
        int segment = findSegment();


        // Normalize time parameter t within the segment [0, 1]
        float ts1 = timeStamp[segment];
        float ts2 = timeStamp[segment + 1];
        float t = (time - ts1) / (ts2 - ts1);

        // Compute cubic B-spline basis functions
        float t2 = t * t;
        float t3 = t2 * t;

        // B-spline basis functions (uniform cubic B-spline)
        coef[0] = (-t3 + 3.0f * t2 - 3.0f * t + 1.0f) / 6.0f;
        coef[1] = (3.0f * t3 - 6.0f * t2 + 4.0f) / 6.0f;
        coef[2] = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) / 6.0f;
        coef[3] = t3 / 6.0f;
    }

    std::vector<glm::vec3> getSegmentControlPoints(int segment)
    {
        std::vector<glm::vec3> segmentPoints(4);
        int n = static_cast<int>(controlPoint.size());

        if (n < 4)
        {
            // Not enough control points, pad with available points
            for (int i = 0; i < 4; ++i)
            {
                segmentPoints[i] = controlPoint[std::min(i, n - 1)];
            }
            return segmentPoints;
        }

        for (int i = 0; i < 4; ++i)
        {
            int idx = segment + i; // B-spline uses 4 control points: P[i], P[i+1], P[i+2], P[i+3]
            idx = glm::clamp(idx, 0, n - 1);
            segmentPoints[i] = controlPoint[idx];
        }

        return segmentPoints;
    }

    glm::vec3 getTangent()
    {
        int segment = findSegment();
        if (segment < 0) return glm::vec3(0.0f);

        std::vector<glm::vec3> segmentPoints = getSegmentControlPoints(segment);

        // Compute derivative coefficients (derivative of B-spline basis functions)
        float ts1 = timeStamp[segment];
        float ts2 = timeStamp[segment + 1];
        float t = (time - ts1) / (ts2 - ts1);

        // Derivative of cubic B-spline basis functions
        float dt_coef[4];
        dt_coef[0] = (-3.0f * t * t + 6.0f * t - 3.0f) / 6.0f;
        dt_coef[1] = (9.0f * t * t - 12.0f * t) / 6.0f;
        dt_coef[2] = (-9.0f * t * t + 6.0f * t + 3.0f) / 6.0f;
        dt_coef[3] = (3.0f * t * t) / 6.0f;

        // wip i dont get it
        // Scale by time step for proper derivative
        float timeStep = ts2 - ts1;

        glm::vec3 tangent = glm::vec3(0.0f);
        for (int i = 0; i < 4; ++i) {
            tangent += (dt_coef[i] / timeStep) * segmentPoints[i];
        }

        return tangent;
    }

    glm::quat rotationBetweenVectors(glm::vec3 from, glm::vec3 to)
    {
        from = glm::normalize(from);
        to = glm::normalize(to);

        float dot = glm::dot(from, to);

        // Vectors are the same
        if (dot >= 0.999999f) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // Vectors are opposite
        if (dot <= -0.999999f) {
            glm::vec3 axis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), from);
            if (glm::length(axis) < 0.0001f) {
                axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), from);
            }
            axis = glm::normalize(axis);
            return glm::angleAxis(glm::pi<float>(), axis);
        }

        glm::vec3 axis = glm::cross(from, to);
        float w = 1.0f + dot;

        glm::quat q = glm::quat(w, axis.x, axis.y, axis.z);
        return glm::normalize(q);
    }

};

struct renderableObject {
    renderableObject(std::shared_ptr <BasicMesh> Mesh, Shader* Shader, glm::vec3 Pos, glm::mat4 Model, std::unique_ptr<AbstractAnimation> animation) :
        startModel{ glm::translate(Model,Pos) },
        shader{ Shader },
        mesh{ std::move(Mesh) },
        Animation{ std::move(animation) },
        isAnimated{ true }
    {
        ;
    }
    renderableObject(std::shared_ptr <BasicMesh> Mesh, Shader* Shader, glm::mat4 Model, std::unique_ptr<AbstractAnimation> animation) :
        startModel{ Model },
        shader{ Shader },
        mesh{ std::move(Mesh) },
        Animation{ std::move(animation) },
        isAnimated{ true }
    {
        ;
    }
    renderableObject(std::shared_ptr <BasicMesh> Mesh, Shader* Shader, bool IsInstaced, std::unique_ptr<AbstractAnimation> animation) :
        shader{ Shader },
        mesh{ std::move(Mesh) },
        isInstaced{ IsInstaced },
        Animation{ std::move(animation) },
        isAnimated{ true }
    {
        ;
    }
    renderableObject(std::shared_ptr <BasicMesh> mesh, Shader* shader, glm::vec3 pos, glm::mat4 model) :
        model{ glm::translate(model,pos) },
        shader{ shader },
        mesh{ std::move(mesh) },
        Animation{ nullptr }
    {
        ;
    }

    renderableObject(std::shared_ptr <BasicMesh> Mesh, Shader* Shader, glm::mat4 Model) :
        model{ Model },
        shader{ Shader },
        mesh{ std::move(Mesh) },
        Animation{ nullptr }
    {
        ;
    }
    renderableObject(std::shared_ptr <BasicMesh> Mesh, Shader* Shader, bool IsInstaced) :
        shader{ Shader },
        mesh{ std::move(Mesh) },
        isInstaced{ IsInstaced },
        Animation{ nullptr }
    {
        ;
    }

    std::shared_ptr <BasicMesh> mesh;
    Shader* shader;
    glm::mat4 model{ 1.f };
    glm::mat4 startModel{ 1.f }; // need for keep track othe statrting point of the animation
    bool isInstaced = false;

    // animation attribute 
    std::unique_ptr<AbstractAnimation> Animation;
    bool isAnimated = false;
};

extern std::vector<renderableObject> renderingObjectList;
extern Shader instanceShader;

inline void loadInstanceRenderingObjectList(std::shared_ptr <BasicMesh> Mesh)
{
    static std::random_device rd;  // Will be used to obtain a seed for the random number engine
    static std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    static std::uniform_real_distribution<double> distrib(-1.0, 1.0);
    static std::uniform_real_distribution<double> scale(1.0, 6.0);
    static std::uniform_int_distribution<> rot(0, 180);

    std::vector<glm::mat4> instanceMatrix(1);
    //for (unsigned int i = 0; i < instanceMatrix.size(); ++i)
    //{
    //    float size = 1;//scale(gen); 
    //    instanceMatrix[i] = glm::scale(instanceMatrix[i], glm::vec3(size, size, size)); 
    //    instanceMatrix[i] = glm::translate(glm::mat4(1.0f), glm::vec3(distrib(gen), distrib(gen), distrib(gen)));
    //    float angle = rot(gen); 
    //    instanceMatrix[i] = glm::rotate(instanceMatrix[i], angle, glm::vec3(0.0f, 1.0f, 0.0f));
    //}
    instanceMatrix[0] = glm::mat4(1);
    Mesh->SetupInstancedArrays(instanceMatrix);
    renderingObjectList.push_back(renderableObject{ Mesh, &instanceShader, true });
};



// keyframe interpolation 
inline  glm::mat4 getAnimatedTransform(renderableObject& obj, float currentTime)
{
    // Check if the object has an animation
    if (!obj.isAnimated || !obj.Animation)
        return obj.model;

    // Update state of the animation
    obj.Animation->updateTime(currentTime);

    // Get animation components
    glm::vec3 position = obj.Animation->getPosition();
    glm::quat rotation = obj.Animation->getRotation();
    glm::vec3 scale = obj.Animation->getScale();

    // Build transformation matrix: T * R * S
    glm::mat4 translationMatrix = glm::translate(obj.startModel, position);
    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    // Combine transformations and update model matrix
    obj.model = translationMatrix * rotationMatrix * scaleMatrix;

    return obj.model;
};

#endif // !RENDERABLE_OBJECT_STRUCT_H
