#pragma once

#include "../mixos-acorn/Acorn-Script.hpp"

//This is for testing ideas out without the linter loosing it's mind like it seems to when I work in acorn_script

namespace Acorn {

    // uint32_t thing_id = global_register_type_ids("thing");
    // uint32_t thing_one_offset = 0;
    // uint32_t thing_two_offset = 0;
    // uint32_t thing_total_size = 0;

    // bool init_thing_type() {
    //     _layout ttemp(global_add_template(thing_id)); //Thing template
    //     thing_one_offset = ttemp.add_prop(int_id, 4, "one");
    //     thing_two_offset = ttemp.add_prop(int_id, 4, "two");
    //     thing_total_size = ttemp.total_size;
    //     return true;
    // }
    // bool thing_type_ready = init_thing_type();

    // struct Thing : Ptr {
    //     Thing() {}
    //     Thing(Ptr p) : Ptr(p) {}

    //     inline int& one() {return *(int*)resolve_to_col(*this).qget(sidx+thing_one_offset);}
    //     inline void one(int t)   {resolve_to_col(*this).qset(sidx+thing_one_offset, (void*)&t, 4); }

    //     inline int& two() {return *(int*)resolve_to_col(*this).qget(sidx+thing_two_offset);}
    //     inline void two(int t)   {resolve_to_col(*this).qset(sidx+thing_two_offset, (void*)&t, 4); }
    // };
    
    struct Workshop_Unit : public virtual Acorn_Script {
        Workshop_Unit(uint16_t _uid) : Unit(_uid) {init();}
        Workshop_Unit() {init();}

     
       
        void init() override {
          
        }
    };
}
