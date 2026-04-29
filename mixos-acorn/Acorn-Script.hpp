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

            t_handlers[end_id] = [this](Context& ctx){
                ctx.result.removeAt(ctx.index);
                ctx.index--;
            };
            t_handlers[comma_id] = t_handlers[end_id];

            // Node n = make_node(this);
            // n.name("joe");
            // n.type(test_id);
            // Node v = make_node(this);
            // v.name("Vee");
            // n.children() << v;
            // Node j = make_node(this);
            // j.name("Jay");
            // n.scopes() << j;


            // a_handlers[test_id] = [this](Context& ctx){
            //     print(ctx.node.name());
            //     print(ctx.node.children()[0].name());
            //     print(ctx.node.scopes()[0].name());
            // };

            // Context ctx;
            // ctx.node = n;

            // start_stage(a_handlers);
            // standard_process(ctx);

            // print("Hello world");
        }

        void place_node_in_scope(Node node, Node insc) {
            node.in_scope(insc);
            for(int i=0;i<node.children().length();i++) {
                place_node_in_scope(node.children()[i],insc);
            }
        }

        uint32_t print_id = add_function("print");

        virtual Node process(std::string path) override {
            print("RECIVED ",path);
            Node root = tokenize(path);

            start_stage(a_handlers);
            standard_direct_pass(root);

            a_pass_resolve_keywords(root.children());

            for(int i=0;i<root.children().length();i++) {
                place_node_in_scope(root.children()[i],root);
            }
            
            start_stage(t_handlers);
            standard_resolving_pass(root);

            x_handlers[print_id] = [this](Context& ctx){
                std::string to_print = "";
                for(int i=0;i<ctx.node.children().length();i++) {
                    Node c = ctx.node.children()[i];
                    to_print += value_as_string(c.value());
                }
                print(to_print);
            };

            span->print_all();
            print(node_to_string(root));

            start_stage(x_handlers);
            standard_travel_pass(root);

            writeFile("mixos-acorn/tests/printout.txt","");
            for(int t=0;t<types.length();t++) {
                std::string to_print = "";
                to_print += "TYPE "+std::to_string(t)+" "+types[t].type_name+":\n";
                to_print += type_to_string(types[t]);
                to_print += "\n\n\n";

                editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                    source+=to_print;
                });
            }

            return root;
        }
    
        virtual void run(Node root) override {
            
        }


    };
}