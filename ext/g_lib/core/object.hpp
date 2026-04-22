#pragma once

#include <iostream>
#include <atomic>
#include "../util/util.hpp"
#include "../core/q_object.hpp"

class Type;

class Object : virtual public q_object {    
    public:
        uint32_t sidx = 0;
        std::atomic<bool> recycled{false};
        Type* type_ = nullptr;

        Object() {

        }
        virtual ~Object() {}

        Object(Object&& other) noexcept 
        : q_object(std::move(other)) {}

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                q_object::operator=(std::move(other));
            }
            return *this;
        }
    };


