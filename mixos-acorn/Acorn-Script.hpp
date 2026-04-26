#pragma once

#include "../mixos-acorn/Acorn-Core.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Unit {
        Acorn_Script() {init();}

        uint32_t test_id = reg_id("TEST"); 
        
        void init() override {
            Node n = make_node(this);
            n.name("joe");
            n.type(test_id);


            a_handlers[test_id] = [this](Context& ctx){
                print(ctx.node.name());
            };

            Context ctx;
            ctx.node = n;

            start_stage(a_handlers);
            standard_process(ctx);

            print("Hello world");
        }
    };
}