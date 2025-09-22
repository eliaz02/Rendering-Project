#pragma once

#ifndef LIGHTSTRUCT_H
#define LIGHTSTRUCT_H

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <atomic>
#include "Component.h"

struct PointLight: public Component 
{
    PointLight() :
        Pos{ glm::vec3(0.f) },
        Ambient{ glm::vec3(0.f) },
        Diffuse{ glm::vec3(0.f) },
        Specular{ glm::vec3(0.f) },
        near_plane{ 0.f },
        far_plane{ 0.f },
        Projection{ glm::mat4(0.f) }
    {
        ;
    }
    PointLight(glm::vec3 pos,
        glm::vec3 ambient,
        glm::vec3 diffuse,
        glm::vec3 specular,
        float near,
        float far
    ) :
        Pos{ pos },
        Ambient{ ambient },
        Diffuse{ diffuse },
        Specular{ specular },
        near_plane{ near },
        far_plane{ far }
    {
        Projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

    }
    glm::vec3 Pos;
    glm::vec3 Ambient;
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    float near_plane;
    float far_plane;
    glm::mat4 Projection;
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };

private:

};

// struct contain all the for the implemnentation of a directional light and its shadow (i'll use it for the sun light)
// the logic is quite standard use lookAt function to compute the view matrix and an othogonal for the projection matrix 
// the view matrix always look at the position (that is suppose to be at the height of the terrein), 
// the actual camera position will be computed from the position, direction and height 
// in such a way that the projection will be parallel to the direction of the light 
struct DirLight : public Component 
{
    DirLight() :
        Direction{ glm::vec3(0.f) },
        Position{ glm::vec3(0.f) },
        Ambient{ glm::vec3(0.f) },
        Diffuse{ glm::vec3(0.f) },
        Specular{ glm::vec3(0.f) },
        near_plane{ 0.f },
        far_plane{ 0.f },
        Projection{ glm::mat4(0.f) },
        View{ glm::mat4(0.f) }
    {
        ;
    }

    DirLight(
        glm::vec3 dir,
        glm::vec3 pos,
        glm::vec3 ambient,
        glm::vec3 diffuse,
        glm::vec3 specular,
        float near,
        float far
    ) :
        Direction{ glm::normalize(dir) },
        Position{ pos.x,0.f,pos.z },
        Ambient{ ambient },
        Diffuse{ diffuse },
        Specular{ specular },
        near_plane{ near },
        far_plane{ far }
    {
        Projection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
        updateView();
    }

    glm::vec3 Direction;
    // position is needed since the light do not cover all the space (the shadow map would be too large)
    // and its refered to the point the camere is looking at (not the position of the camera) 
    // the camera position will be computed with the use of direction position and height 
    glm::vec3 Position;
    glm::vec3 Ambient;
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    float near_plane; // just to keap track of the value used in the lightProjection
    float far_plane;
    glm::mat4 Projection;
    glm::mat4 View;
    float height{ 10.f }; // height of the camera from the terrein (the terrein is supposed be 0.f)

    void setPos(glm::vec3 newPos)
    {
        Position = glm::vec3{ newPos.x,0,newPos.z };
        updateView();
    }
    void setDir(glm::vec3 newDir)
    {
        Direction = glm::normalize(newDir);
        updateView();
    }
private:
    // this function update the view matrix after the position has changed 
    // the view always point to the position with height (y component)
    void updateView()
    {
        // camera position the computation come out from simple linear algebra 
        // such that the frustum is parallel to the direction.
        glm::vec3 cameraPosition = Position - (Direction * height);

        // non standard up coorinate since the camera is supposed to be use as sun light so the camera has to look down
        // and this may be casuse that the direction is parallel to the up vector.
        glm::vec3 upVector = glm::vec3(1.0f, 0.0f, 0.0f);

        // We still check this with the dot product for avoid any issues.
        if (glm::abs(glm::dot(Direction, upVector)) > 0.999f) {
            // use Z-axis instead of X-axis as "up" to stabilize the matrix.
            upVector = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        //  Create the view matrix. It looks FROM the cameraPosition , AT the Position.
        View = glm::lookAt(cameraPosition, Position, upVector);
    }
};

// this struct contain all that is needed for the costruction of a spotlight and its shadow map 
// (with the exption of the Frame Buffer Object ) that ha its own calls and is shared with the dirLght struct.
struct SpotLight : public Component 
{
    SpotLight(glm::vec3 pos, glm::vec3 dir, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float near, float far, glm::vec2 cut, glm::vec3 lightDecreasingConstant) :
        Pos{ pos },
        Dir{ glm::normalize(dir) },
        Ambient{ ambient },
        Diffuse{ diffuse },
        Specular{ specular },
        near_plane{ near },
        far_plane{ far },
        cutOff{ cut.x },
        outerCutOff{ cut.y },
        constant{ lightDecreasingConstant.x },
        linear{ lightDecreasingConstant.y },
        quadratic{ lightDecreasingConstant.z }
    {
        // i'm not sure this magic numer are right i'll think about it later
        Projection = glm::perspective(glm::radians(45.0f), 1.0f, near_plane, far_plane);

        // standard up vector
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        // avoid that up vector and direction are linear dipendent 
        if (glm::abs(glm::dot(Dir, upVector)) > 0.999f) upVector = glm::vec3(0.0f, 0.0f, 1.0f);

        View = glm::lookAt(pos, pos + dir, upVector);
    }
    // base light data.
    glm::vec3 Pos;
    glm::vec3 Dir;
    glm::vec3 Ambient;
    glm::vec3 Diffuse;
    glm::vec3 Specular;

    // data that will be used for the frustum.
    float near_plane;
    float far_plane;

    // pretty handle of the edge of the light
    float cutOff;
    float outerCutOff;

    // used for determinate the decreasing power of the ligth  
    float constant;
    float linear;
    float quadratic;

    // matrix for cumpute the lightSpaceMatrix
    glm::mat4 Projection;
    glm::mat4 View;
};

// Helper colors for visual diversity
inline  std::vector<glm::vec3> colors = {
    glm::vec3{1.0f, 0.0f, 0.0f},   // Red
    glm::vec3{0.0f, 1.0f, 0.0f},   // Green
    glm::vec3{0.0f, 0.0f, 1.0f},   // Blue
    glm::vec3{1.0f, 1.0f, 0.0f},   // Yellow
    glm::vec3{0.0f, 1.0f, 1.0f},   // Cyan
    glm::vec3{1.0f, 0.0f, 1.0f},   // Magenta
    glm::vec3{1.0f, 0.5f, 0.0f},   // Orange
    glm::vec3{0.5f, 0.0f, 1.0f},   // Purple
    glm::vec3{0.0f, 0.5f, 1.0f},   // Sky Blue
    glm::vec3{0.5f, 1.0f, 0.0f}    // Lime
};
#endif // !LIGHTSTRUCT_H
