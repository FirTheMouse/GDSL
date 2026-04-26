#pragma once

#include <iostream>
#include <atomic>
#include "ext/g_lib/util/util.hpp"

namespace Acorn {
    class TypePool;

    class Object {    
        public:
            uint32_t sidx = 0;
            bool recycled = false;
            TypePool* pool = nullptr;

            Object() {}
            ~Object() {}
    };
}


