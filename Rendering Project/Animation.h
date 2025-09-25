#pragma once

#ifndef ANIMATION_H
#define ANIMATION_H 


#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <numeric>
#include <stdexcept>

// this is interface for a static animation that in any given time compute the position and the rotation and scale of one or more object   
// the concrete impementation can compute the every thing on-fly or precompute all 
// there is many flaws in this structure 1) the animation is only determinate of the time, 2) 
class AbstractAnimation 
{
protected:
    std::vector<float> timeStamp;
    std::vector<glm::vec3> controlPoint;
    float time;
public:
    // pure virtual
    virtual void updateTime(float time) = 0; // this interface update the animation just thorght time since is static (it doesn't recive any other dynamic input)
    virtual glm::vec3 getPosition() = 0;
    virtual glm::quat getRotation() = 0;
    virtual glm::vec3 getScale() = 0;
    virtual bool isOver() = 0;
    // Virtual destructor for proper cleanup
    virtual ~AbstractAnimation() = default;

};
// Structure for B-spline transformation sequence
class BSplineAnimation : public AbstractAnimation {
public:
/**
    * @brief this functio  create a sequence of animation/movment that follow a Bspline curve
    * @param points: vector of control points in the 3D space that decribe the curve
    * @param time: vector of time stamp that descrive the time required to go from one control point to another. 
    *   If the Bsline is closed the time has to be atleast of the same size of points, 
    *   if the Bsline is open d the time has to be atleast of the same size of points - 3.
    *   time vector support 2 different format :
    *   this 2 are equivalent {0.0f, 1.0f, 2.0f, 3.0f, 4.0f} {1.0f, 1.0f, 1.0f, 1.0f, 1.0f} 
    * @param isclosed: if is true it crate a closed bsline (useful for repiting animation).
    *   there is no need to ajdust the control points for an open or closed version,  
    *   but you may have to adjust the time.
 */
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
        if (std::is_sorted(time.begin(), time.end(), std::less<>())) // this check if is not strictly increasing
        {
            standardTime.resize(time.size());
            std::partial_sum(time.begin(), time.end(), standardTime.begin());
        }
        else
        {
            standardTime = time;
        }
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

    bool isOver()
    {
        if (isClosed) return false; // assumption if the animation is a loop i want that the animation never stops
        if (time > timeStamp.back()) return true;
        return false;
    }
};


#endif