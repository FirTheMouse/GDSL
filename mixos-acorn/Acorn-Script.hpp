#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script() {init();}

        uint32_t test_id = reg_id("TEST"); 
        
        void init() override {

            set_binding_powers(plus_id, 4,6);
            set_binding_powers(dash_id, 4,5);
            set_binding_powers(slash_id, 4,5);
            set_binding_powers(rangle_id, 2,3);
            set_binding_powers(langle_id, 2,3);
            set_binding_powers(equals_id, 1,1);
            set_binding_powers(star_id, 5,7);
            set_binding_powers(caret_id, 8,4);
            set_binding_powers(amp_id, 4,8);
            set_binding_powers(dot_id, 8,9);
            
            Node n = make_node(this);
            n.name("joe");
            n.type(test_id);
            Node v = make_node(this);
            v.name("Vee");
            n.children() << v;
            Node j = make_node(this);
            j.name("Jay");
            n.scopes() << j;


            a_handlers[test_id] = [this](Context& ctx){
                print(ctx.node.name());
                print(ctx.node.children()[0].name());
                print(ctx.node.scopes()[0].name());
            };

            Context ctx;
            ctx.node = n;

            start_stage(a_handlers);
            standard_process(ctx);

            print("Hello world");
        }

        virtual Node process(std::string path) override {
            print("RECIVED ",path);
            Node root = tokenize(path);

            start_stage(a_handlers);
            standard_direct_pass(root);

            span->print_all();
            print(node_to_string(root));
            return root;
        }
    
        virtual void run(Node root) override {
            
        }


    };
}