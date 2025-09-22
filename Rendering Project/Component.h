#pragma once
// the main  reason of this file is for avoiding circular dependencies 
#ifndef COMPONENT_H
#define COMPONENT_H

#include <concepts>
#include <type_traits>

// The base class for all components.
struct Component
{
    virtual ~Component() = default;
};

// The concept that checks if a type T derives from Component.
template<typename T>
concept IsComponent = std::derived_from<T, Component>;

#endif // COMPONENT_H