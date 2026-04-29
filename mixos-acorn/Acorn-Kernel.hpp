#pragma once

#include "../mixos-acorn/Acorn-Script.hpp"

namespace Acorn {
    struct Acorn_Kernel : public virtual Unit {
        Acorn_Kernel() {init();}
        
        void init() override {}

        virtual Node process(std::string code) override {
            Acorn_Script p1;
            Acorn_Script p2;

            p1.process(readFile("mixos-acorn/tests/p1.gld"));
            print("P1: ");
                p1.run(p1.unit_root);
                
            print("NODENETS");
            print("P1: ");
                print(p1.node_to_string(p1.unit_root,2,0,true));

            return make_node(this);
        }
    
        virtual void run(Node root) override {
            
        }
    };
}