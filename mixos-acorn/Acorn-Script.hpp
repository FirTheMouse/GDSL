#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"
#include "../ext/g_lib/core/thread.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script() {init();}

        uint32_t test_id = reg_id("TEST"); 
        
        void dump_unit(bool clear_dump) {
            if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");

            for(int t=0;t<types.length();t++) {
                std::string to_print = "";
                to_print += "TYPE "+std::to_string(t)+" "+types[t].type_name+":\n";
                to_print += type_to_string(types[t]);
                to_print += "\n\n\n";

                editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                    source+=to_print;
                });
            }
        }

        Stage& sys_handlers = reg_stage("system");

        uint32_t load_id = make_tokenized_keyword("load");
        uint32_t stop_id = make_tokenized_keyword("STOP");
        uint32_t labels_id = make_tokenized_keyword("labels");

        uint32_t node_block_id = reg_id("node_block");
        uint32_t in_id = make_keyword("in");

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
            set_binding_powers(pipe_id, 9,8);

            Handler discard = [this](Context& ctx){
                ctx.result.removeAt(ctx.index);
                ctx.index--;
            };
            t_handlers[end_id] = discard;
            t_handlers[comma_id] = discard;

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

            x_handlers[load_id] = [this](Context& ctx){
                if(unit_root.live) {
                    ctx.node.value(make_value(string_id,sizeof(Ptr))); //Allocate a space to recive a value
                    unit_root = ctx.node;
                    unit_root.live = false;
                    while(!unit_root.live) {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                    }
                    print("I LIVE!");
                }


                // if(!ctx.node.children().empty()) {
                //     process_node(ctx,ctx.node.children()[0]);
                //     ctx.node.value(make_value(this,string_id,sizeof(Ptr)));
                //     std::string path = string(*(Ptr*)ctx.node.children()[0].value().get(),this).to_std();
                //     uint32_t store_to = types[data_store_id].note_value(path,sizeof(char),char_id);
                //     for(auto c : readFile(path))
                //         types[data_store_id][store_to].push((void*)&c);
                //     Ptr ticket(data_store_id,store_to,0);
                //     ctx.node.value().set((void*)&ticket);
                // }
            };

            sys_handlers.default_function = [this](Context& ctx){
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            };  

            // sys_handlers[load_id] = [this](Context& ctx){
            //     if(!ctx.unit) return;
            //     Unit& acorn = *ctx.unit;
            //     if(!acorn.unit_root.live) {
            //         std::string path = string(*(Ptr*)acorn.unit_root.children()[0].value().get(),&acorn).to_std();
            //         uint32_t store_to = acorn.types[acorn.data_store_id].note_value(path,sizeof(char),char_id);
            //         for(auto c : readFile(path))
            //             acorn.types[acorn.data_store_id][store_to].push((void*)&c);
            //         Ptr ticket(acorn.data_store_id,store_to,0);
            //         acorn.unit_root.value().set((void*)&ticket);
            //         ctx.source = "sys:115 loaded file "+path+" for a process";
            //         acorn.unit_root.live = true;
            //     }
            // };
            // x_handlers[stop_id] = [this](Context& ctx){
            //     unit_root = ctx.node;
            // };
            // sys_handlers[stop_id] = [this](Context& ctx){
            //     ctx.source = "sys:122 stopping";
            //     ctx.flag = true;
            // };

            // x_handlers[make_tokenized_keyword("run")] = [this](Context& ctx){
            //     if(!ctx.node.children().empty()) {
            //         process_node(ctx,ctx.node.children()[0]);
            //         std::string torun = string(*(Ptr*)ctx.node.children()[0].value().get(),this).to_std();
            //         Acorn_Script acorn;
            //         acorn.process(torun);
            //         acorn.unit_root.children() << make_node(&acorn,stop_id);
            //         g_ptr<Thread> t = make<Thread>();
            //         t->run_blocking([&acorn]() mutable {
            //             acorn.run(acorn.unit_root);
            //         });
            //         Context actx;
            //         actx.unit = &acorn;
            //         while(true) {
            //             actx.node = acorn.unit_root;
            //             sys_handlers.run(acorn.unit_root.type())(actx);
            //             if(!actx.source.empty()) {
            //                 print(actx.source);
            //                 actx.source = "";
            //             }
            //             if(actx.flag) {
            //                 t->stop();
            //                 break;
            //             }
            //         }
            //     }
            // };

            r_handlers[var_decl_id] = [this](Context& ctx){
                ctx.node.value().init_data();
                fire_quals(ctx,ctx.node.value());
            };
            r_handlers[prefix_node_id] = [this](Context& ctx){
                if(ctx.value.live) {
                    Node n = make_node();
                    ctx.value.set((void*)&n);
                    ctx.value.type_scope(opt_ptr(node_type_id,0,true)); //The node template
                }
            };
            r_handlers[prefix_value_id] = [this](Context& ctx){
                if(ctx.value.live) {
                    Value v = make_value();
                    ctx.value.set((void*)&v);
                    ctx.value.type_scope(opt_ptr(node_type_id,1,true)); //The value template
                }
            };

            r_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];

                std::string prop = right.name().to_std();
                right.value(make_value());
                value_table look;
                if(left.value().type_scope().live) {
                    look = left.value().type_scope().value_table();
                } else {
                    if(left.value().type()==node_id) {
                        look = value_table(Ptr(value_table_store_id,0,0));
                    } else if(left.value().type()==value_id) {
                        look = value_table(Ptr(value_table_store_id,1,0));
                    } else {
                        print(red("dot:r_handler no clue what value table to use for "+labels[left.value().type()]));
                        return;
                    }
                }
                right.value().copy(look.get(prop));
                ctx.node.value(right.value()); //Stealing it from right
            };

            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                opt_ptr ptr = left;
                if(left.value().live&&(left.value().type()==node_id||left.value().type()==value_id)) {
                    ptr = *(opt_ptr*)left.value().get();
                }

                int addr = ctx.node.value().address();

                if(!right.children().empty()) {
                    Ptr p = *(Ptr*)types[ptr.pool][addr][ptr.sidx];
                    Node rleft = right.children()[0];
                    if(rleft.value().type()==int_id) {
                        addr = *(int*)rleft.value().get();
                    }

                    Ptr targetptr(p.pool,p.idx,addr);
                    types[ctx.node.value().pool][value_data_col].set(ctx.node.value().sidx,(void*)&targetptr);
                    if(ctx.root.type()!=equals_id) {
                        ctx.node.value().set(types[targetptr.pool][targetptr.idx][targetptr.sidx]);
                    }
                    return;
                }
                
                if(ctx.root.type()==equals_id) {
                    Ptr targetptr(ptr.pool,addr,ptr.sidx);
                    types[ctx.node.value().pool][value_data_col].set(ctx.node.value().sidx,(void*)&targetptr);
                } else {
                    ctx.node.value().set(types[ptr.pool][addr][ptr.sidx]);
                }
            };

            x_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Ptr p = left.value().data_ptr();
                Ptr ptr = *(Ptr*)types[p.pool][p.idx][p.sidx];
                types[ptr.pool][ptr.idx].push(right.value().get());
            };

            r_handlers[labels_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value());
                ctx.node.value().setup(string_id,sizeof(Ptr));
                Ptr ticket(data_store_id,types[data_store_id].note_value("labelstorage",sizeof(char),char_id),0);
                ctx.node.value().set((void*)&ticket);
            };
            x_handlers[labels_id] = [this](Context& ctx){
                if(!ctx.node.children().empty()) {
                    standard_sub_process(ctx);
                    string label(*(Ptr*)ctx.node.value().get());
                    uint32_t p = *(uint32_t*)ctx.node.children()[0].value().get();
                    label.push(labels[p]); 
                }
            };




            s_handlers[string_id] = [this](Context& ctx){
                if(ctx.index+1>=ctx.result.length()) return;

                Node right = ctx.result[ctx.index+1];
                if(right.type()==lbrace_id) {
                    ctx.node.children() << ctx.result.take(ctx.index+1);
                    ctx.node.type(node_block_id);
                    ctx.node.children().last().name(ctx.node.name().to_std());
                }
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                for(auto e : labels.entrySet()) {
                    if(e.value==ctx.node.name().to_std()) {
                        ctx.node.sub_type(e.key);
                        break;
                    }
                }
                if(ctx.node.sub_type()==0) {
                    print(red("node_block:t_handler unrecognized node type: "+ctx.node.name().to_std()));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.flag = standard_travel_pass(ctx.node.scopes()[0]);
            };

            x_handlers[make_tokenized_keyword("test")] = [this](Context& ctx){
                print("THIS SHOULD NOT PRINT");
            };

            r_handlers[in_id] = [this](Context& ctx){
                if(!ctx.node.children().empty()&&ctx.node.in_scope().live&&ctx.node.in_scope().owner().live) {
                    ctx.node.name("in "+ctx.node.children()[0].name().to_std()+" "+labels[ctx.node.in_scope().owner().sub_type()]);
                    if(!ctx.node.scopes().empty()) {
                        ctx.node.scopes()[0].name(ctx.node.name().to_std());
                    }
                }
            };
            x_handlers[in_id] = [this](Context& ctx){
                Node this_node = ctx.node;
                uint32_t target_type = ctx.node.in_scope().owner().sub_type();
                std::string stage_name = ctx.node.children()[0].name().to_std();
                if(!stages.hasKey(stage_name)) {
                    print(red("in_id:x_handler unknown stage "+stage_name));
                    return;
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                (*stage)[target_type] = [this,this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;

                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0],&ctx);
                    start_stage(old_stage);
                    
                };

                uint32_t stage_id = types[handler_type_id][stages_id].cells.get(stage_name);
                while(types[handler_type_id][target_type].length()<=stage_id) types[handler_type_id].add_row(target_type);
                Node target_scope = this_node.scopes()[0];
                types[handler_type_id][target_type].set(stage_id,(void*)&target_scope);
            };


            
        }

        virtual Node process(std::string path) override {
            Node root = tokenize(path);
            unit_root = root;

            return root;
        }
    
        virtual void run(Node root) override {
            start_stage(a_handlers);
            standard_direct_pass(root);

            a_pass_resolve_keywords(root.children());

            for(int i=0;i<root.children().length();i++) {
                place_node_in_scope(root.children()[i],root);
            }

            //print(node_to_string(root,0,0,true));

            start_stage(s_handlers);
            standard_direct_pass(root);
            
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);

            // span->print_all();
            print(node_to_string(root,0,0,true));

            start_stage(x_handlers);
            standard_travel_pass(root);

            //print(node_to_string(root,0,0,true));

            dump_unit(true);
        }


    };
}