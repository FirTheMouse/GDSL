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

        uint32_t labels_id = make_tokenized_keyword("labels");

        uint32_t node_block_id = reg_id("node_block");
        uint32_t invoke_stage_id = make_keyword("invoke_stage");
        uint32_t in_id = make_keyword("in");
        uint32_t precompile_id = make_tokenized_keyword("precompile");
        uint32_t end_precompile_id = make_tokenized_keyword("end_precompile");

        uint32_t ctx_node_id = make_tokenized_keyword("node");
        uint32_t ctx_result_id = make_tokenized_keyword("result");
        uint32_t ctx_source_id = make_tokenized_keyword("source");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        node_list daycare;

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

            a_handlers[make_tokenized_keyword("register")] = [this](Context& ctx){
                make_keyword(ctx.result.take(ctx.index+1).name().to_std());
            };
            a_handlers[make_tokenized_keyword("newstage")] = [this](Context& ctx){
                reg_stage(ctx.result.take(ctx.index+1).name().to_std());
            };

            a_handlers[precompile_id] = [this](Context& ctx){
                ctx.result.removeAt(ctx.index); //remove this node
                for(int i=ctx.index;i<ctx.result.length();i++) {
                    if(ctx.result[i].name().to_std()=="end_precompile") {
                        ctx.result.removeAt(i); //and remove the end
                        daycare = ctx.node.children();
                        while(ctx.root.children().length()>i) daycare << ctx.root.children().take(i);
                        break;
                    }
                }
            };

            x_handlers[make_tokenized_keyword("as_data")] = [this](Context& ctx){
                Value rv = ctx.node.children()[0].value();
                ctx.node.value(make_value(ptr_id,sizeof(Ptr)));
                ctx.node.value().set((void*)&rv.data_ptr());
            };

            r_handlers[var_decl_id] = [this](Context& ctx){
                ctx.node.value().init_data();
                fire_quals(ctx,ctx.node.value());
            };
            r_handlers[prefix_node_id] = [this](Context& ctx){
                if(ctx.value.idx==1) {
                    Node n = make_node();
                    ctx.value.set((void*)&n);
                    ctx.value.type_scope(Ptr(node_type_id,1,0)); //The node template
                }
            };
            r_handlers[prefix_value_id] = [this](Context& ctx){
                if(ctx.value.idx==1) {
                    Value v = make_value();
                    ctx.value.set((void*)&v);
                    ctx.value.type_scope(Ptr(node_type_id,1,1)); //The value template
                }
            };

            r_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];

                if(left.value().type()==col_id) { //Lists and such get handled on their own
                    if(right.name().to_std()=="get") { //If we're getting then right will be of the type we're getting
                        if(left.value().sub_type()==node_id) {
                            right.value(make_value(node_id,sizeof(Ptr)));
                            right.value().init_data();
                            right.value().type_scope(Ptr(node_type_id,1,0)); //The node template
                        } else if(left.value().sub_type()==value_id) {
                            right.value(make_value(value_id,sizeof(Ptr)));
                            right.value().init_data();
                            right.value().type_scope(Ptr(node_type_id,1,1)); //The value template
                        }
                    }
                    ctx.node.value(right.value()); //Stealing it from right
                    return;
                }

                std::string prop = right.name().to_std();
                right.value(make_value());
                value_table look;
                if(left.value().type_scope().idx==1) {
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
                Ptr ptr = left;
                if(left.value().idx==1&&(left.value().type()==node_id||left.value().type()==value_id||left.value().type()==col_id)) {
                    ptr = *(Ptr*)left.value().get();
                }

                int addr = ctx.node.value().address();

                if(left.value().type()==col_id) {
                    std::string opp = right.name().to_std();
                    if(opp=="push") {
                        Node rleft = right.children()[0];
                        types[ptr.pool][ptr.idx].push(rleft.value().get());
                    } else if(opp=="get") {
                        Node rleft = right.children()[0];
                        if(rleft.value().type()==int_id) {
                            addr = *(int*)rleft.value().get();
                        }
                        Ptr targetptr(ptr.pool,ptr.idx,addr);
                        if(ctx.root.type()==equals_id) {
                            ctx.node.value().data_col().set(ctx.node.value().sidx,(void*)&targetptr);
                        } else {
                            ctx.node.value().set(types[targetptr.pool][targetptr.idx][targetptr.sidx]);
                        }
                    } else if(opp=="length") {
                        ctx.node.value().type(int_id);
                        ctx.node.value().data_col().clear();
                        ctx.node.value().data_col().element_size = 4;
                        int len = types[ptr.pool][ptr.idx].length();
                        ctx.node.value().data_col().push((void*)&len);
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

            r_handlers[ctx_node_id] = [this](Context& ctx){
                ctx.node.value(make_value(node_id,sizeof(Ptr)));
                ctx.node.value().init_data();
                ctx.node.value().type_scope(Ptr(node_type_id,1,0)); //The node template
            };
            x_handlers[ctx_node_id] = [this](Context& ctx){
                ctx.node.value().set((void*)&ctx.sub->node);
            };


            r_handlers[ctx_result_id] = [this](Context& ctx){
                ctx.node.value(make_value(col_id,sizeof(Ptr),0,node_id));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_result_id] = [this](Context& ctx){
                ctx.node.value().set((void*)&ctx.sub->result);
            };


            r_handlers[ctx_source_id] = [this](Context& ctx){
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_source_id] = [this](Context& ctx){
                if(ctx.root.type()==equals_id) {
                    ctx.sub->source = ctx.root.children()[1].name().to_std();
                } else {
                    ctx.node.value().set((void*)&ctx.sub->source);
                }
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
                if(!ctx.node.children().empty()&&ctx.node.in_scope().idx==1&&ctx.node.in_scope().owner().idx==1) {
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

            x_handlers[invoke_stage_id] = [this](Context& ctx){
                std::string stage_name = ctx.node.name().to_std();
                if(!stages.hasKey(stage_name)) {
                    if(!stages.hasKey(stage_name+"ing")) {
                        print(red("invoke_stage_id:x_handler unknown stage "+stage_name));
                        return;
                    } else {
                        stage_name+="ing";
                    }
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                Node left = ctx.node.children()[0];
                ctx.node = left;
                stage->run(left.type())(ctx);
            };

            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node.children()[0]);
                if(*(bool*)ctx.node.children()[0].value().get()) {
                    ctx.flag = standard_travel_pass(ctx.node.scopes()[0],ctx.sub);
                }
                else if(ctx.node.scopes().length()>1) {
                    ctx.flag = standard_travel_pass(ctx.node.scopes()[1],ctx.sub);
                }
            };
            t_handlers[else_id] = [this](Context& ctx) {
                if(ctx.index>0) {
                    if(ctx.left.type()==if_id) {
                        ctx.node.scopes()[0].owner(ctx.left);
                        ctx.left.scopes() << ctx.node.scopes()[0];
                        ctx.result.removeAt(ctx.index);
                        ctx.index--;
                    }
                }
            };
            x_handlers[while_id] = [this](Context& ctx) {
                while(true) {
                    process_node(ctx, ctx.node.children()[0]);
                    if(!(*(bool*)ctx.node.children()[0].value().get()))break;
                    if(standard_travel_pass(ctx.node.scopes()[0],ctx.sub)) {
                        ctx.flag = true;
                        break;
                    } 
                }
            };         
            x_handlers[for_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node.children()[0]);
                while(true) {
                    process_node(ctx, ctx.node.children()[1]);
                    if(!(*(bool*)ctx.node.children()[1].value().get()))break;
                    if(standard_travel_pass(ctx.node.scopes()[0],ctx.sub)) {
                        ctx.flag = true;
                        break;
                    } 
                    process_node(ctx, ctx.node.children()[2]);
                }
            };  
        }

        virtual Node process(std::string path) override {
            Node root = tokenize(path);
            unit_root = root;

            return root;
        }

        void lemmatize_stages() {
            for(auto e : stages.entrySet()) {
                std::string label = e.key;
                if(e.key.length()>3 && e.key.substr(e.key.length()-3)=="ing") {
                    label = e.key.substr(0,e.key.length()-3);
                }
                if(!keywords.hasKey(label)) { //to stop double registraion
                    add_alias(label,invoke_stage_id);
                }
            }
        }

        void compile(Node root) {
            start_stage(a_handlers);
            standard_direct_pass(root);

            lemmatize_stages();
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
        }
    
        virtual void run(Node root) override {
            compile(root);
            if(!daycare.empty()) {
                print(node_to_string(root,0,0,true));
                start_stage(x_handlers);
                standard_travel_pass(root);

                root.children(daycare);
                compile(root);
            }

            // span->print_all();
            print(node_to_string(root,0,0,true));

            start_stage(x_handlers);
            standard_travel_pass(root);

            //print(node_to_string(root,0,0,true));

            dump_unit(true);

        }


    };
}