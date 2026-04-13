#pragma once

#include "../core/GDSL.hpp"

namespace GDSL {
    struct TEMPLATE_Unit : public virtual Unit {
        TEMPLATE_Unit() { init(); }

        void init() override {

        }

        g_ptr<Node> process(const std::string& code) override {
            return nullptr;
        }

        void run(g_ptr<Node> root) override {
    
        }
    };
}