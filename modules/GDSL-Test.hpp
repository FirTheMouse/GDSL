#pragma once

#include "../modules/Q-Expression.hpp"

namespace GDSL {

    struct Test_Unit : virtual public Expression_Unit{
        void init() override {
        
        }

        g_ptr<Node> process(const std::string& code) override {
            return nullptr;
        }

        void run(g_ptr<Node> server) override {
            
        }
    };
}