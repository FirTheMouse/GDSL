#pragma once

#include "../mixos-acorn/Acorn-Script.hpp"

//This is for testing ideas out without the linter loosing it's mind like it seems to when I work in acorn_script

namespace Acorn {
    struct Workshop_Unit : public virtual Acorn_Script {
        Workshop_Unit(uint16_t _uid) : Unit(_uid) {init();}
        Workshop_Unit() {init();}

     
       
        void init() override {

           
        }
    };
}
