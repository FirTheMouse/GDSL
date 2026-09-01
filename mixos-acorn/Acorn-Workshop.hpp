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
       
        ColCol& resolve_pool_refrence(Node n) {
            if(n.value().type()==int_id) {
                return types[n.getInt()];
            } else if(is_ptr_alias(n.value().type())) {
                return resolve_to_pool(n.getPtr());
            }
            return col2_ref;
        }



        void init() override {

            add_function("print_hash",[this](Context& ctx){
                standard_sub_process(ctx);
                Value keyv = ctx.node().c0().value();
                if(is_ptr_alias(keyv.type())) {
                    Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                    print(hashBytes(keycol.storage,keycol.size));
                } else {
                    print(hashBytes(keyv.get(),keyv.size()));
                }
            });

            add_function("test_overload",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t type = ctx.node().getInt(0);
                std::string instr = ctx.node().getString(1).to_std();
                uint32_t overload_to = ctx.node().getInt(2);
                Value value = deadptr;
                if(ctx.node().children().length()>3) {
                    value = ctx.node().getValue(3);
                }
                overload_type(type,instr,overload_to,value);
            });
            add_function("derive_signature",[this](Context& ctx){
                standard_sub_process(ctx);
                list<std::string> sigs = derive_signatures(ctx.node().c0());
                for(int i=0;i<sigs.length();i++) {
                    print(i,": ",sigs[i]);
                }
            });

            add_function("overload_signature",[this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx.node().getContext(0));
            });

            add_function("cleanup",[this](Context& ctx){
                standard_sub_process(ctx);
                if(ctx.node().children().length()>0) {
                    if(ctx.node().children()[0].value().type()==node_id) {
                        Stage& previous_stage = *active_stage;
                        Node outer_node = ctx.node();
                        walk_handlers.default_function = [this,&outer_node](Context& ctx) {
                            standard_sub_process(ctx);
                            if(is_live(ctx.node().value())) {
                                Node ts = ctx.node().value().type_scope();
                                if(is_live(ts)) {
                                    print(node_info(ts)," ",ts.z()," VS ",outer_node.z());
                                }
                                if(is_live(ts) && ts.z()!=outer_node.z()) {
                                    ctx.node().value(deadptr);
                                }
                            }
                        };
                        start_stage(walk_handlers);
                        standard_backwards_pass(ctx.node().getNode(0));
                        start_stage(previous_stage);
                        recycle_node(ctx.node().getNode(0));
                    }
                }
            });
        }
    };
}
