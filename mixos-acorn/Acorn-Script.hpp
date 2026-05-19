#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"
#include "../ext/g_lib/core/thread.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script() {init();}

        uint32_t test_id = reg_id("TEST");
        Stage& n_handlers = reg_stage("naming"); 
        
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
        uint32_t precompiling_id = reg_id("PRECOMPILING");

        uint32_t ctx_node_id = make_tokenized_keyword("node");
        uint32_t ctx_result_id = make_tokenized_keyword("result");
        uint32_t ctx_source_id = make_tokenized_keyword("source");
        uint32_t ctx_index_id = make_tokenized_keyword("index");
        uint32_t ctx_nosub_source_id = make_tokenized_keyword("ctxsource");
        uint32_t ctx_subsub_source_id = make_tokenized_keyword("subsource");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        uint32_t to_string_id = make_tokenized_keyword("to_string");
        uint32_t to_type_id = make_tokenized_keyword("to_type");

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
            t_handlers[comment_id] = discard;

            a_handlers[make_tokenized_keyword("register")] = [this](Context& ctx){
                make_tokenized_keyword(ctx.result.take(ctx.index+1).name().to_std());
            };
            a_handlers[make_tokenized_keyword("newstage")] = [this](Context& ctx){
                reg_stage(ctx.result.take(ctx.index+1).name().to_std());
            };

            add_double_string_token('#', '#', hash_id, hash_id, precompiling_id, precompiling_id);
            tokenizer_state_functions[precompiling_id] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(c == '#'&&ctx.source.at(ctx.index+1)=='#') {
                    ctx.state=0;
                    ctx.result.removeAt(ctx.index);
                    ctx.index++;
                    Node root = process(ctx.node.name().to_std());
                    ctx.node.name().col().clear(); //To avoid stinking up the nodenet and memory dump
                    compile(root);
                    print(node_to_string(root,0,0,true));
                    start_stage(x_handlers);
                    standard_travel_pass(root);
                    for(int i=0;i<root.children().length();i++) {
                        ctx.root.children().push(root.children()[i]);
                    }
                } else if(c=='\n') {
                    at_y += 1.0f; at_x = -1.0f;
                    ctx.node.name().push(c);
                }
                else {
                    ctx.node.name().push(c);
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

            //Target for a future burn, treat all things as Ptrs, use sub_size and sub_type to carry what gets will return
            r_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];

                if(left.value().type()==col_id) { //Lists and such get handled on their own
                    if(right.name().to_std()=="get"||right.name().to_std()=="take") { //If we're getting then right will be of the type we're getting
                        if(left.value().sub_type()==node_id) {
                            right.value(make_value(node_id,sizeof(Ptr)));
                            right.value().init_data();
                            right.value().type_scope(Ptr(node_type_id,1,0)); //The node template
                        } else if(left.value().sub_type()==value_id) {
                            right.value(make_value(value_id,sizeof(Ptr)));
                            right.value().init_data();
                            right.value().type_scope(Ptr(node_type_id,1,1)); //The value template
                        } else {
                            right.value(make_value(left.value().sub_type(),left.value().sub_size()));
                            right.value().init_data();
                        }
                    } else if(right.name().to_std()=="length") {
                        right.value(make_value(int_id,4));
                        right.value().init_data();
                    }
                    ctx.node.value(right.value()); //Stealing it from right
                } else if(left.value().type()==string_id) {
                    if(right.name().to_std()=="length") {
                        right.value(make_value(int_id,4));
                        right.value().init_data();
                    } else if(right.name().to_std()=="at") {
                        right.value(make_value(char_id,1));
                        right.value().init_data();
                    } else if(right.name().to_std()=="substr") {
                        right.value(make_value(string_id,sizeof(Ptr)));
                        right.value().init_data();
                        Ptr ticket(data_store_id,types[data_store_id].note_value("substrstorage",sizeof(char),char_id),0);
                        right.value().set((void*)&ticket);
                    } else if(right.name().to_std()=="find") {
                        right.value(make_value(int_id,4));
                        right.value().init_data();
                    } else if(right.name().to_std()=="split") {
                        right.value(make_value(col_id,sizeof(Ptr),0,string_id,sizeof(Ptr)));
                        right.value().init_data();
                        Ptr ticket(data_store_id,types[data_store_id].note_value("splitstorage",sizeof(Ptr),string_id),0);
                        right.value().set((void*)&ticket);
                    }
                    ctx.node.value(right.value()); //Stealing it from right
                } else {
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
                }
            };

            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Ptr ptr = left;
                if(left.value().idx==1&&(left.value().type()==node_id||left.value().type()==value_id||left.value().type()==col_id||left.value().type()==string_id)) {
                    ptr = *(Ptr*)left.value().get();
                }

                int addr = ctx.node.value().address();

                if(left.value().type()==col_id) {
                    std::string opp = right.name().to_std();
                    if(opp=="push") {
                        Node rleft = right.children()[0];
                        types[ptr.pool][ptr.idx].push(rleft.value().get());
                    } else if(opp=="get") {
                        addr = *(int*)right.children()[0].value().get();
                        Ptr targetptr(ptr.pool,ptr.idx,addr);
                        if(ctx.root.type()==equals_id&&ctx.left.idx==1) { //Checking livness of left, because we only expose if we're the left term of the assignment
                            //Except this doesn't actually work correctly! Correct it later
                            ctx.node.value().data_col().set(ctx.node.value().sidx,(void*)&targetptr);
                        } else {
                            ctx.node.value().set(types[targetptr.pool][targetptr.idx][targetptr.sidx]);
                        }
                    } else if(opp=="take") {
                        addr = *(int*)right.children()[0].value().get();
                        Ptr targetptr(ptr.pool,ptr.idx,addr);
                        ctx.node.value().set(types[targetptr.pool][targetptr.idx][targetptr.sidx]);
                        types[targetptr.pool][targetptr.idx].removeAt(targetptr.sidx);
                    } else if(opp=="length") {
                        int len = types[ptr.pool][ptr.idx].length();
                        ctx.node.value().data_col().set(0,(void*)&len);
                    }
                } else if(left.value().type()==string_id) {
                    std::string opp = right.name().to_std();
                    if(opp=="length") {
                        int len = types[ptr.pool][ptr.idx].length();
                        ctx.node.value().data_col().set(0,(void*)&len);
                    } else if(opp=="at") {
                        Ptr targetptr(ptr.pool,ptr.idx,*(uint32_t*)right.children()[0].value().get());
                        if(ctx.root.type()==equals_id&&ctx.left.idx==1) {
                            ctx.node.value().data_col().set(ctx.node.value().sidx,(void*)&targetptr);
                        } else {
                            ctx.node.value().set(types[targetptr.pool][targetptr.idx][targetptr.sidx]);
                        }
                    } else if(opp=="substr") {
                        int from = *(int*)right.children()[0].value().get();
                        int to = *(int*)right.children()[1].value().get();
                        string target(*(Ptr*)right.value().get());
                        target.col().clear();
                        for(int i=from;i<from+to;i++) {
                            target.push(*(char*)types[ptr.pool][ptr.idx][i]);
                        }
                    } else if(opp=="append") {
                        string to_add(*(Ptr*)right.children()[0].value().get());
                        for(int i=0;i<to_add.length();i++) {
                            types[ptr.pool][ptr.idx].push((void*)&to_add[i]);
                        }
                    } else if(opp=="clear") {
                        types[ptr.pool][ptr.idx].clear();
                    } else if(opp=="find") {
                        Col& tcol = types[ptr.pool][ptr.idx];
                        string refstr(*(Ptr*)right.children()[0].value().get());
                        int start_at = 0;
                        if(right.children().length()>1) {
                            start_at = *(int*)right.children()[1].value().get();
                        }
                        int found_id = -1;
                        for(int i=start_at;i<tcol.length();i++) {
                            bool match = true;
                            for(int s=0;s<refstr.length();s++) {
                                if(*(char*)tcol[i+s]!=refstr[s]) {
                                    match = false;
                                    break;
                                }
                            }
                            if(match) {
                                found_id = i; break;
                            }
                        }
                        ctx.node.value().data_col().set(0,(void*)&found_id);
                    } else if(opp=="split") { //DO NOT USE FOR NOW (this was an experiment)
                        string refstr(*(Ptr*)right.children()[0].value().get());
                        char delimt = refstr.at(0);
                        string str(ptr);
                        list<std::string> splt = split_str(str.to_std(),delimt);
                        Ptr tptr = *(Ptr*)right.value().get();
                        Col& tcol = types[tptr.pool][tptr.idx];
                        tcol.clear();
                        for(auto s : splt) {
                            Ptr ticket(data_store_id,types[data_store_id].note_value("spltstorage",sizeof(char),char_id),0);
                            string newstr(ticket);
                            newstr.push(s);
                        }
                    }
                } else {
                    if(!right.children().empty()) { //This is a bit jank, a burn is coming
                        //types[ptr.pool][addr].set(ptr.sidx,right.children()[0].value().get());
                        types[ptr.pool][addr].set(ptr.sidx,(void*)&right.children()[0].value().data_ptr());
                    } else {
                        if(ctx.root.type()==equals_id&&ctx.left.idx==1) {
                            Ptr targetptr(ptr.pool,addr,ptr.sidx);
                            types[ctx.node.value().pool][value_data_col].set(ctx.node.value().sidx,(void*)&targetptr);
                        } else {
                            ctx.node.value().set(types[ptr.pool][addr][ptr.sidx]);
                        }
                    }
                }
            };



            r_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                uint32_t ltype = left.value().type();
                if(layouts.hasKey(ltype)) {
                    _layout& layout = layouts.get(ltype);
                    std::string prop = right.name().to_std();
                    if(layout.label_to_index.hasKey(prop)) {
                        uint32_t index = layout.label_to_index.get(prop);
                        if(layout.tags[index]==func_call_id) {
                            if(layout.subtags[index]==0&&left.value().sub_type()!=0) { //For access to a subtype like when we have node.children and need to know that children.get returns a node
                                ctx.node.value(make_value(left.value().sub_type(), left.value().sub_size()));
                                //right.scopes().push(left.value().type_scope()); There could be a use case for this in the future
                            } else { //When we already know what we are, like x.length where length is always an int
                                ctx.node.value(make_value(layout.subtags[index], layout.subsizes[index]));
                                if(is_live(layout.ptrs[index])) {
                                    right.scopes().push(layout.ptrs[index]);
                                    right.type(func_call_id);
                                } else {
                                    right.type(temp_get_id+index);
                                }
                            }
                        } else {
                            ctx.node.value(make_value(layout.tags[index], layout.sizes[index], layout.offsets[index], layout.subtags[index], layout.subsizes[index], layout.ptrs[index]));
                        }
                    }
                    right.value(ctx.node.value());
                }
            };

            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Value value = ctx.node.value();
                if(value.address()!=0) {
                    Ptr ptr = *(Ptr*)left.value().get();
                    ptr.sidx = value.address();
                    if(ctx.root.type()==equals_id&&is_live(ctx.left)) {
                        types[value.pool][value.idx].set(value_data_offset,(void*)&ptr); //Setting the data_ptr itself
                    } else {
                        value.set(types[ptr.pool][ptr.idx][ptr.sidx]); //Setting what the data_ptr points to
                    }
                }
            };
            x_handlers[temp_get_id] = [this](Context& ctx){
                Ptr ptr = *(Ptr*)ctx.left.value().get();
                Value lval = ctx.node.children()[0].value();
                Value value = ctx.node.value();
                if(lval.type()==int_id) {
                    ptr.sidx = *(int*)lval.get();
                } else if(lval.type()==string_id) {
                    //Implment later
                }
                if(ctx.root.type()==equals_id&&is_live(ctx.left)) {
                    types[value.pool][value.idx].set(value_data_offset,(void*)&ptr); //Setting the data_ptr itself
                } else {
                    value.set(types[ptr.pool][ptr.idx][ptr.sidx]); //Setting what the data_ptr points to
                }
            };
            x_handlers[temp_length_id] = [this](Context& ctx){
                Ptr ptr = *(Ptr*)ctx.left.value().get();
                uint32_t len = types[ptr.pool][ptr.idx].length();
                ctx.node.value().set((void*)&len); //Setting what the data_ptr points to
            };
            x_handlers[temp_push_id] = [this](Context& ctx){
                Ptr ptr = *(Ptr*)ctx.left.value().get();
                types[ptr.pool][ptr.idx].push(ctx.node.children()[0].value().get());
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
                standard_sub_process(ctx);
                ctx.node.value(make_value(col_id,sizeof(Ptr),0,node_id));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_result_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value().set((void*)&ctx.sub->result);
            };

            r_handlers[ctx_index_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(int_id,4));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_index_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node.children().empty()) {
                    ctx.sub->index = *(int*)ctx.node.children()[0].value().get();
                } else {
                    ctx.node.value().set((void*)&ctx.sub->index);
                }
            };

            r_handlers[ctx_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node.children().empty()) {
                    ctx.sub->source.col_ptr = *(Ptr*)ctx.node.children()[0].value().get();
                } else {
                    ctx.node.value().set((void*)&ctx.sub->source.col_ptr);
                }
            };
            r_handlers[ctx_nosub_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_nosub_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node.children().empty()) {
                    ctx.source.col_ptr = *(Ptr*)ctx.node.children()[0].value().get();
                } else {
                    ctx.node.value().set((void*)&ctx.source.col_ptr);
                }
            };
            r_handlers[ctx_subsub_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
                ctx.node.value().init_data();
            };
            x_handlers[ctx_subsub_source_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node.children().empty()) {
                    ctx.sub->sub->source.col_ptr = *(Ptr*)ctx.node.children()[0].value().get();
                } else {
                    ctx.node.value().set((void*)&ctx.sub->sub->source.col_ptr);
                }
            };

            r_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
                ctx.node.value().init_data();
                Ptr ticket(data_store_id,types[data_store_id].note_value("tostringstorage",sizeof(char),char_id),0);
                ctx.node.value().set((void*)&ticket);
            };
            x_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string str(*(Ptr*)ctx.node.value().get());
                str = value_as_string(ctx.node.children()[0].value());
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

            r_handlers[to_type_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node.value(make_value(int_id,4));
                ctx.node.value().init_data();
            };
            x_handlers[to_type_id] = [this](Context& ctx){
                //Add caching for this later
                standard_sub_process(ctx);
                std::string search_for = string(*(Ptr*)ctx.node.children()[0].value().get()).to_std();
                for(auto e : labels.entrySet()) {
                    if(e.value == search_for) {
                        ctx.node.value().set((void*)&e.key);
                        return;
                    }
                }
            };

            x_handlers[make_tokenized_keyword("make_value")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int type = *(int*)ctx.node.children()[0].value().get();
                int size = *(int*)ctx.node.children()[1].value().get();
                ctx.sub->node.value(make_value(type,size));
                ctx.sub->node.value().init_data();
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
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                if(left.value().type()==node_id) {
                    left = Node(*(Ptr*)left.value().get());
                }
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

            x_handlers[make_tokenized_keyword("rectify")] = [this](Context& ctx){
                ctx.sub->source.col_ptr = ctx.sub->sub->source.col_ptr;
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

            start_stage(n_handlers);
            standard_direct_pass(root);
            // print(node_to_string(root,0,0,true));

            start_stage(s_handlers);
            standard_direct_pass(root);
            
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);
        }
    
        virtual void run(Node root) override {
            compile(root);

            // span->print_all();
            print(node_to_string(root,0,0,true));

            start_stage(x_handlers);
            standard_travel_pass(root);

            //print(node_to_string(root,0,0,true));
            
            dump_unit(true);
            // span->print_all();

        }


    };
}