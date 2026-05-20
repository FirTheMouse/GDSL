#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"
#include "../ext/g_lib/core/thread.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script() {init();}

        uint32_t test_id = reg_id("TEST");
        Stage& n_handlers = reg_stage("naming"); 
        
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

        uint32_t ptr_get_id = reg_id("PTR_GET");
        uint32_t ptr_push_id = reg_id("PTR_PUSH");
        uint32_t string_append_id = reg_id("STRING_APPEND");
        //uint32_t string_substr = reg_id("STRING_SUBSTR");

        void init() override {


            overload_type(ptr_id,".\"get\"",ptr_get_id,make_value(0)); //The value with no type means to take the subtype and subsize from left
            //overload_type(ptr_id,"any",ptr_get_id,make_value(0)); //This indicates with children
            overload_type(ptr_id,".\"push\"",ptr_push_id);
            overload_type(string_id,".\"append\"",string_append_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,"+string",string_append_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            //overload_type(string_id,".\"substr\"",string_substr,make_value(string_id,sizeof(Ptr),0,char_id,1));

            x_handlers[ptr_get_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Value cv = right.value();
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                if(!right.children().empty()) {
                    cv = right.children()[0].value();
                }
                if(cv.type()==int_id) {
                    ctx.node.value().set(col.get(*(int*)cv.get()));
                } else if(cv.type()==string_id) {
                    ctx.node.value().set(col.get(string(*(Ptr*)cv.get()).to_std()));
                }
            };
            x_handlers[ptr_push_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                if(!right.children().empty()) {
                    Col& col = resolve_to_col(*(Ptr*)left.value().get());
                    col.push(right.children()[0].value().get());
                }
            };
            x_handlers[string_append_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Value cv = right.value();
                if(!right.children().empty()) {
                    cv = right.children()[0].value();
                }
                string s(*(Ptr*)left.value().get());
                if(cv.type()==string_id) {
                    string rs(*(Ptr*)cv.get());
                    s.push(rs.to_std());
                } else if(cv.type()==char_id) {
                    s.push(*(char*)cv.get());
                }
                ctx.node.value(left.value());
            };


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
                if(is_live(ctx.value)) {
                    Node n = make_node();
                    ctx.value.set((void*)&n);
                }
            };
            r_handlers[prefix_value_id] = [this](Context& ctx){
                if(is_live(ctx.value)) {
                    Value v = make_value();
                    ctx.value.set((void*)&v);
                }
            };
  
            // x_handlers[temp_get_id] = [this](Context& ctx){
            //     Ptr ptr = *(Ptr*)ctx.left.value().get();
            //     Value value = ctx.node.value();
            //     if(value.address()!=0) {
            //         ptr = *(Ptr*)types[ptr.pool][ptr.idx].qget(value.address());
            //     }

            //     Value lval = ctx.node.children()[0].value();
            //     if(lval.type()==int_id) {
            //         ptr.sidx = *(int*)lval.get();
            //     } else if(lval.type()==string_id) {
            //         //Implment later
            //     }
            //     if(ctx.root.type()==equals_id&&is_live(ctx.left)) {
            //         types[value.pool][value.idx].qset(value_data_offset,(void*)&ptr,sizeof(Ptr)); //Setting the data_ptr itself
            //     } else {
            //         value.set(types[ptr.pool][ptr.idx][ptr.sidx]); //Setting what the data_ptr points to
            //     }
            // };
            // x_handlers[temp_length_id] = [this](Context& ctx){
            //     Ptr ptr = *(Ptr*)ctx.left.value().get();
            //     uint32_t len = types[ptr.pool][ptr.idx].length();
            //     ctx.node.value().set((void*)&len); //Setting what the data_ptr points to
            // };
            // x_handlers[temp_push_id] = [this](Context& ctx){
            //     Ptr ptr = *(Ptr*)ctx.left.value().get();
            //     if(!ctx.node.children().empty()) {
            //         types[ptr.pool][ptr.idx].push(ctx.node.children()[0].value().get());
            //     } else {
            //         Col& to = types[ptr.pool][ptr.idx];
            //         Ptr fromptr = *(Ptr*)ctx.node.value().get();
            //         Col& from = types[fromptr.pool][fromptr.idx];
            //         for(int i=0;i<from.length();i++) {
            //             to.push(from[i]);
            //         }
            //     }
            // };


            r_handlers[ctx_node_id] = [this](Context& ctx){
                ctx.node.value(make_value(node_id,sizeof(Ptr)));
                ctx.node.value().init_data();
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
                if(!ctx.node.children().empty()&&is_live(ctx.node.in_scope())&&is_live(ctx.node.in_scope().owner())) {
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

            start_stage(s_handlers);
            standard_direct_pass(root);
            
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);
        }
    
        virtual void run(Node root) override {
            compile(root);

            //span->print_all();
            print(node_to_string(root,0,0,true));

            start_stage(x_handlers);
            standard_travel_pass(root);

            // print("AFTER");
            // print(node_to_string(root,0,0,true));
            
            dump_unit(true);
            // span->print_all();
        }


    };
}