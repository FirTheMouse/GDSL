#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"
#include "../ext/g_lib/core/thread.hpp"


#define LOBOTOMIZE_M_STAGE 1

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

        uint32_t ctx_id = make_tokenized_keyword("ctx");
        uint32_t lctx_id = make_tokenized_keyword("lctx");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        uint32_t read_file_id = make_tokenized_keyword("read_file");
        uint32_t write_file_id = make_tokenized_keyword("write_file");
        uint32_t compile_id = make_tokenized_keyword("compile");

        uint32_t live_qual = add_qualifer("live");
        uint32_t gatekeeper_qual = add_qualifer("gatekeeper");
        uint32_t assigned_qual = add_qualifer("assigned");
        uint32_t constant_qual = add_qualifer("constant");

        uint32_t to_string_id = make_tokenized_keyword("to_string");
        uint32_t to_type_id = make_tokenized_keyword("to_type");
        uint32_t DEBUG_ROOT_id = make_tokenized_keyword("DEBUG_ROOT");

        uint32_t ptr_get_id = reg_id("PTR_GET");
        uint32_t ptr_take_id = reg_id("PTR_TAKE");
        uint32_t ptr_push_id = reg_id("PTR_PUSH");
        uint32_t ptr_length_id = reg_id("PTR_LENGTH");
        uint32_t ptr_clear_id = reg_id("PTR_CLEAR");
        // uint32_t string_append_id = reg_id("STRING_APPEND");
        uint32_t string_substr_id = reg_id("STRING_SUBSTR");
        uint32_t string_slice_id = reg_id("STRING_SLICE");
        uint32_t string_find_id = reg_id("STRING_FIND");
        uint32_t string_find_from_id = reg_id("STRING_FIND_FROM");

        uint32_t check_equality_int = overload_type(int_id,"==int","CHECK_EQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()==*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_equality_string = overload_type(string_id,"==string","CHECK_EQUALITY_STRING",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            bool result = false;
            if(l.length()!=r.length()) {ctx.node().value().set((void*)&result); return;}
            for(int i=0;i<l.length();i++) {
                if(l.at(i)!=r.at(i)) {
                    ctx.node().value().set((void*)&result);
                    return;
                }
            }
            result = true;
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_lessthan_or_equalsto_int = overload_type(int_id,"<=int","CHECK_LEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()<=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_or_equalsto_int = overload_type(int_id,">=int","CHECK_GEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            x_handlers.run(check_lessthan_or_equalsto_int)(ctx);
            bool result = !*(bool*)ctx.node().value().get();
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_lessthan_int = overload_type(int_id,"<int","CHECK_LT_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = (*(int*)ctx.node().children()[0].value().get()<*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_int = overload_type(int_id,">int","CHECK_GT_INT",make_value(bool_id,1),[this](Context& ctx){
            x_handlers.run(check_lessthan_int)(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = !*(bool*)ctx.node().value().get();
            ctx.node().value().set((void*)&result);
        });

        uint32_t increment_int = overload_type(int_id,"++int","INCREMENT_INT",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node().value(ctx.node().children()[0].value());
            int inced = *(int*)ctx.node().value().get()+1;
            ctx.node().value().set((void*)&inced);
        });

        uint32_t valueGetStr_id = overload_type(value_id,".\"getStr\"","VALUE_GETSTR",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });



        uint32_t string_equals_id = overload_type(string_id,"=string","STRING_EQUALS",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            if(!is_live(l)) {Ptr ticket = get_ticket(name_store_id,1,char_id); l = ticket; ctx.node().children()[0].value().set((void*)&ticket);}
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            l.col().clear();
            for(int i=0;i<r.length();i++) {
                l.push(r.at(i));
            }
        });

        uint32_t string_append_id = overload_type(string_id,"+string","STRING_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                Ptr ticket = get_ticket(name_store_id,1,char_id); ctx.node().value().set((void*)&ticket);
            }
            string o(*(Ptr*)ctx.node().value().get());
            o.col().clear();
            o.push(l.to_std()); o.push(r.to_std());
        });

        uint32_t string_func_append_id = overload_type(string_id,".\"append\"","STRING_FUNC_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].children()[0].value().get());
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });        

        uint32_t string_append_eq_id = overload_type(string_id,"+=string","STRING_APPEND_EQ",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });

        void e_stage_assignment_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage assignment handler because one of the values is missing"));
                return;
            }
            ctx.node().quals().push((make_node(live_qual))); //This liveness thing is kludgy but I'm tired and just trying to get ctx.source = "wub" to work
            process_node(ctx,left);
            if(left.has_qual(live_qual)) { //Transfer liveness
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
            } else {
                ctx.node().quals().pop(); //Kill it
            }

            int const_at = left.value().find_qual(constant_qual);
            if(const_at==-1) { //A value is constant if it's only asigned once
                if(!left.has_qual(assigned_qual)) {
                    left.value().quals().push(make_node(constant_qual));
                }
            } else {
                left.value().quals().removeAt(const_at);
            }

            if(!left.has_qual(assigned_qual)) {
                left.value().quals().push(make_node(assigned_qual));
            }
        };
        void e_stage_binary_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage binary handler because one of the values is missing"));
                return;
            }
            if(ctx.node().has_qual(live_qual)) {
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
                if(!left.has_qual(live_qual)) {
                    left.value().quals().push(make_node(live_qual));
                }
            } else {
                if(left.has_qual(live_qual)) { //Transfer liveness
                    ctx.node().quals().push((make_node(live_qual)));
                    if(!right.has_qual(live_qual)) {
                        right.value().quals().push(make_node(live_qual));
                    }
                } else if(right.has_qual(live_qual)) {
                    ctx.node().quals().push((make_node(live_qual)));
                    left.value().quals().push(make_node(live_qual));
                }
            }
        };
        void e_scoped_handler(Context& ctx) {
            if(!ctx.node().scopes().empty()) {
                Node scope = ctx.node().scopes()[0];
                for(int i=0;i<scope.children().length();i++) {
                    if(scope.children()[i].has_qual(live_qual)) {
                        ctx.node().quals().push(make_node(live_qual));
                        break;
                    }
                }
            }
            if(ctx.node().has_qual(live_qual)) {
                for(int i=0;i<ctx.node().children().length();i++) {
                    Node c = ctx.node().children().get(i);
                    if(is_live(c.value())&&!c.has_qual(live_qual)) {
                        c.value().quals().push(make_node(live_qual));
                    } else if(!is_live(c.value())) {
                        c.quals().push(make_node(live_qual));
                    }
                }
            }
            standard_sub_process(ctx);
        }

        void e_pass_prune_dead(node_col nodes) {
            for(int i=nodes.length()-1;i>=0;i--) {
                Node node = nodes[i];
                log("Pruning: ",node_info(node));
                e_pass_prune_dead(node.children());
                for(int s = 0;s<node.scopes().length();s++) {
                    if(node.scopes()[s].owner()==node)
                        e_pass_prune_dead(node.scopes()[s].children());
                }
                if(!node.has_qual(live_qual)) {
                    log(yellow("Recycled: "+Ptr_as_string(nodes.get(i))));
                    recycle_node(nodes.get(i));
                    nodes.removeAt(i);
                }
            }
        };

        void m_stage_assignment_handler(Context& ctx) {
            standard_sub_process(ctx); //Not sure what to do with this yet
        };
        bool should_allocate_data(Value v) {
            return (is_live(v)&&!is_live(v.data_ptr())&&v.type()!=0);
        }
        bool should_recycle_data(Value v) {
            return (is_live(v)&&is_live(v.data_ptr())&&v.type()!=0);
        }
        void allocate_data(Value v) {
            v.data_ptr(get_ticket(data_store_id,v.size(),v.type()));
            if(v.data_col().empty()) {
                v.data_col().push_default();
            }
        }

        uint32_t precompile_brace = add_token_combo("precompile_brace",'#','#');
        uint32_t comment_brace = add_token_combo("comment_brace",'/','/');

        void init() override {
            overload_type(ptr_id,".\"get\"",ptr_get_id,make_value(0)); //The value with no type means to take the subtype and subsize from left
            overload_type(ptr_id,".\"take\"",ptr_take_id,make_value(0));
            overload_type(ptr_id,".\"push\"",ptr_push_id);
            overload_type(ptr_id,"<<any",ptr_push_id);
            overload_type(ptr_id,".\"length\"",ptr_length_id,make_value(int_id,4));

            //overload_type(string_id,"+string",string_append_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"length\"",ptr_length_id,make_value(int_id,4));
            overload_type(string_id,".\"clear\"",ptr_clear_id);
            overload_type(string_id,".\"substr\"",string_substr_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"slice\"",string_slice_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"find\"",string_find_id,make_value(int_id,4));

            overload_type(string_id,"|*^+int",reg_id("THRONGLIZE"),make_value(ptr_id,sizeof(Ptr),0,int_id,4));

            x_handlers[ptr_get_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value cv = right.value();
                void* lv = left.value().get();
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr get")); return;});
                Ptr ptr = *(Ptr*)lv;
                Col& col = resolve_to_col(ptr);
                if(!right.children().empty()) {
                    cv = right.children()[0].value();
                }
                if(cv.type()==int_id) {
                    int index = *(int*)cv.get();
                    if(index<col.length()) {
                        Value value = ctx.node().value();
                        if(ctx.root().type()==equals_id&&is_live(ctx.left())) {
                            ptr.sidx = index;
                            types[value.pool][value.idx].qset(value_data_offset,(void*)&ptr,sizeof(Ptr)); //Setting the data_ptr itself
                        } else {
                            value.set(col.get(index)); //Setting what the data_ptr points to
                        }
                    } else {
                        print(red("ptr_get:x_handler index "+std::to_string(index)+" out of bounds on "+Ptr_as_string(ptr)));
                    }
                } else if(cv.type()==string_id) {
                    //Add suppourt for this and indexing by string later
                    ctx.node().value().set(col.get(string(*(Ptr*)cv.get()).to_std()));
                }
            };
            x_handlers[ptr_take_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value cv = right.value();
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                if(!right.children().empty()) {
                    cv = right.children()[0].value();
                }
                if(cv.type()==int_id) {
                    int index = *(int*)cv.get();
                    ctx.node().value().set(col.get(index));
                    col.removeAt(index);
                }
            };
            x_handlers[ptr_push_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!right.children().empty()) {
                    right = right.children()[0];
                }
                void* lv = left.value().get();
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("No left value in ptr push")); return;});
                Col& col = resolve_to_col(*(Ptr*)lv);
                col.push(right.value().get());
            };
            x_handlers[ptr_length_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                int len = col.length();
                ctx.node().value().set((void*)&len);
            };
            x_handlers[ptr_clear_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Col& col = resolve_to_col(*(Ptr*)left.value().get());
                col.clear();
            };
            x_handlers[string_substr_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                    Ptr ticket = get_ticket(data_store_id,1,char_id);
                    ctx.node().value().set((void*)&ticket);
                }
                string target(*(Ptr*)ctx.node().value().get());
                Ptr ptr = *(Ptr*)left.value().get();
                int from = *(int*)right.children()[0].value().get();
                int to = target.length()-from;
                if(right.children().length()>1) {
                    to = *(int*)right.children()[1].value().get();
                }
                target.col().clear();
                for(int i=from;i<from+to;i++) {
                    target.push(*(char*)types[ptr.pool][ptr.idx][i]);
                }
            };
            x_handlers[string_slice_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                    Ptr ticket = get_ticket(data_store_id,1,char_id);
                    ctx.node().value().set((void*)&ticket);
                }
                string target(*(Ptr*)ctx.node().value().get());
                Ptr ptr = *(Ptr*)left.value().get();
                int from = 0;
                int to = 0; //Add some deffensive checking here later
                if(right.children()[0].value().type()==string_id) {
                    string refstr(*(Ptr*)right.children()[0].value().get());
                    string fromstr(ptr);
                    from = fromstr.find(refstr,0,*(int*)right.children()[1].value().get())+1;
                    to = fromstr.find(refstr,0,*(int*)right.children()[2].value().get());
                } else {
                    from = *(int*)right.children()[0].value().get();
                    to = *(int*)right.children()[1].value().get();
                }
                target.col().clear();
                for(int i=from;i<to;i++) {
                    target.push(*(char*)types[ptr.pool][ptr.idx][i]);
                }
            };
            x_handlers[string_find_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Ptr ptr = *(Ptr*)left.value().get();
                Col& tcol = types[ptr.pool][ptr.idx];
                string refstr(*(Ptr*)right.children()[0].value().get());
                int start_at = 0;
                if(right.children().length()>1) {
                    start_at = *(int*)right.children()[1].value().get();
                }
                int nth_of = 1;
                if(right.children().length()>2) {
                    nth_of = *(int*)right.children()[2].value().get();
                }
                int found_id = string(ptr).find(refstr,start_at,nth_of);
                ctx.node().value().set((void*)&found_id);
            };

            Handler discard = [this](Context& ctx){
                if(ctx.index()>0) {
                    ctx.index()--;
                    ctx.result().get(ctx.index()).quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                } else if(is_live(ctx.root())) {
                    ctx.root().quals() << turn_into_token(ctx.result().take(ctx.index()));
                }
            };
            t_handlers[end_id] = discard;
            t_handlers[comma_id] = discard;
            t_handlers[comment_id] = discard;

            a_handlers[make_tokenized_keyword("register")] = [this](Context& ctx){
                ctx.node().quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                make_tokenized_keyword(ctx.node().quals().last().name().to_std());
            };
            a_handlers[make_tokenized_keyword("newstage")] = [this](Context& ctx){
                reg_stage(ctx.result().take(ctx.index()+1).name().to_std());
            };


            tokenizer_state_functions[comment_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(c == '/' && (ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='/')) {
                    ctx.node().type(comment_id);
                    ctx.state(0);
                    ctx.result().removeAt(ctx.index());
                    ctx.index()++;
                    ctx.node().name().push("//");
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                    ctx.node().type(comment_id);
                    ctx.state(0);
                } else {
                    ctx.node().name().push(c);
                }
            };


            tokenizer_state_functions[precompile_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(precompiling_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '#'&&(ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='#')) {
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;

                    ctx.state(0);
                    //ctx.result().removeAt(ctx.index());
                    ctx.index()++;
                    at_x+=1.0f;

                    std::string oldsrc = ctx.source().to_std(); //Remember to just fix the source in context (when I'm not trying to ship a prototype)

                    // list<Watcher> watcher_daycare; //We don't log things like precompiling stages (for now)
                    // watcher_daycare << watchers;
                    // watchers.clear();

                    std::string oldlabel = unit_label;
                    unit_label = oldlabel+"pc";

                    Node root = process(ctx.node().name().to_std());
                    ctx.node().name().col().clear(); //To avoid stinking up the nodenet and memory dump
                    compile(root,false);
                    start_logged_stage(x_handlers);
                    standard_travel_pass(root);
                    end_logged_stage();

                    unit_label = oldlabel;
                    //watchers << watcher_daycare; //Restore watchers

                    ctx.node().scopes() << root;

                    ctx.source(oldsrc);
                } else if(c=='\n') {
                    at_y += 1.0f; at_x = -1.0f;
                    ctx.node().name().push(c);
                }
                else {
                    ctx.node().name().push(c);
                }
            };


            x_handlers[make_tokenized_keyword("as_data")] = [this](Context& ctx){
                Value rv = ctx.node().children()[0].value();
                ctx.node().value(make_value(ptr_id,sizeof(Ptr)));
                ctx.node().value().set((void*)&rv.data_ptr());
            };

            register_type("list",ptr_id,sizeof(Ptr));

            r_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[prefix_ptr_id] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    ctx.node().value().init_data();
                    if(ctx.value().quals().length()>1) {
                        Node left = ctx.value().quals()[1];
                        Ptr ticket = get_ticket(data_store_id,left.value().size(),left.value().type());
                        ctx.value().set((void*)&ticket);
                        ctx.value().sub_type(left.value().type());
                        ctx.value().sub_size(left.value().size());
                    } else {
                        print(red("prefix_ptr_id::r_handler missing type for list"));
                    }
                }
            };

            // r_handlers[prefix_string_id] = [this](Context& ctx){
            //     if(is_live(ctx.value())&&ctx.value().quals().length()==1) {
            //         Ptr ticket = get_ticket(name_store_id,1,char_id);
            //         ctx.value().set((void*)&ticket);
            //     }
            // };
            r_handlers[prefix_node_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals().length()==1) {
                    Node n = make_node();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&n);
                }
            };
            r_handlers[prefix_value_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals().length()==1) {
                    Value v = make_value();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&v);
                }
            };
            r_handlers[prefix_context_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals().length()==1) {
                    Context c = make_context();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&c);
                }
            };

            r_handlers[ctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[ctx_id] = [this](Context& ctx){
                if(is_live(ctx.sub())) {
                    Context sctx = ctx.sub();
                    ctx.node().value().set((void*)&sctx);
                } else {
                    ctx.node().value().set((void*)&ctx);
                }
            };
            r_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value().set((void*)&ctx);
            };

            r_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr)));
                ctx.node().value().init_data();
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string str(*(Ptr*)ctx.node().value().get());
                str = value_as_string(ctx.node().children()[0].value());
            };

            r_handlers[labels_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[labels_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    string label(*(Ptr*)ctx.node().value().get());
                    uint32_t p = *(uint32_t*)ctx.node().children()[0].value().get();
                    label = labels[p]; 
                }
            };

            r_handlers[to_type_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                ctx.node().value().init_data();
                resolve_overload(ctx);
            };
            x_handlers[to_type_id] = [this](Context& ctx){
                //Add caching for this later
                standard_sub_process(ctx);
                std::string search_for = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                for(auto e : labels.entrySet()) {
                    if(e.value == search_for) {
                        ctx.node().value().set((void*)&e.key);
                        return;
                    }
                }
            };

            x_handlers[make_tokenized_keyword("make_value")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int type = *(int*)ctx.node().children()[0].value().get();
                int size = *(int*)ctx.node().children()[1].value().get();
                ctx.sub().node().value(make_value(type,size));
                ctx.sub().node().value().init_data();
            };


            s_handlers[string_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                    ctx.node().quals() << copy_as_token(ctx.node().children().last());
                    ctx.node().type(node_block_id);
                    ctx.node().children().last().name(ctx.node().name().to_std());
                    ctx.node().children().last().x(-1.0f); ctx.node().children().last().y(-1.0f);
                }
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                for(auto e : labels.entrySet()) {
                    if(e.value==ctx.node().name().to_std()) {
                        ctx.node().sub_type(e.key);
                        break;
                    }
                }
                if(ctx.node().sub_type()==0) {
                    print(red("node_block:t_handler unrecognized node type: "+ctx.node().name().to_std()));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.flag(standard_travel_pass(ctx.node().scopes()[0]));
            };

            x_handlers[make_tokenized_keyword("test")] = [this](Context& ctx){
                print("THIS SHOULD NOT PRINT");
            };

            r_handlers[in_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()&&is_live(ctx.node().in_scope())&&is_live(ctx.node().in_scope().owner())) {
                    ctx.node().quals() << copy_as_token(ctx.node());
                    ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                    ctx.node().name("in "+ctx.node().children()[0].name().to_std()+" "+labels[ctx.node().in_scope().owner().sub_type()]);
                    if(!ctx.node().scopes().empty()) {
                        ctx.node().scopes()[0].name(ctx.node().name().to_std());
                    }
                }
            };
            x_handlers[in_id] = [this](Context& ctx){
                Node this_node = ctx.node();
                uint32_t target_type = ctx.node().in_scope().owner().sub_type();
                std::string stage_name = ctx.node().children()[0].name().to_std();
                if(!stages.hasKey(stage_name)) {
                    print(red("in_id:x_handler unknown stage "+stage_name));
                    return;
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                (*stage)[target_type] = [this,this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;

                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0],ctx);
                    start_stage(old_stage);
                    
                };

                uint32_t stage_id = *(uint32_t*)types[handler_type_id][stages_id].get(stage_name);
                while(types[handler_type_id][target_type].length()<=stage_id) types[handler_type_id][target_type].push_default();
                Node target_scope = this_node.scopes()[0];
                types[handler_type_id][target_type].set(stage_id,(void*)&target_scope);
            };

            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                DEBUG_ONLY(if(ERROR_FLAG) {return;});
                if(*(bool*)ctx.node().children()[0].value().get()) {
                    ctx.flag(standard_travel_pass(ctx.node().scopes()[0],ctx.sub()));
                }
                else if(ctx.node().scopes().length()>1) {
                    ctx.flag(standard_travel_pass(ctx.node().scopes()[1],ctx.sub()));
                }
            };
            t_handlers[else_id] = [this](Context& ctx) {
                if(ctx.index()>0) {
                    if(ctx.left().type()==if_id) {
                        ctx.node().scopes()[0].owner(ctx.left());
                        ctx.left().scopes() << ctx.node().scopes()[0];
                        ctx.left().quals() << turn_into_token(ctx.node());
                        ctx.result().removeAt(ctx.index());
                        ctx.index()--;
                    }
                }
            };
            x_handlers[while_id] = [this](Context& ctx) {
                while(true) {
                    DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute while while another error was flagged")); return;})
                    process_node(ctx, ctx.node().children()[0]);
                    if(!(*(bool*)ctx.node().children()[0].value().get()))break;
                    if(standard_travel_pass(ctx.node().scopes()[0],ctx.sub())) {
                        ctx.flag(true);
                        break;
                    } 
                }
            };         
            x_handlers[for_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                while(true) {
                    process_node(ctx, ctx.node().children()[1]);
                    DEBUG_ONLY(if(ERROR_FLAG) {return;})
                    if(!(*(bool*)ctx.node().children()[1].value().get()))break;
                    if(standard_travel_pass(ctx.node().scopes()[0],ctx.sub())) {
                        ctx.flag(true);
                        break;
                    } 
                    process_node(ctx, ctx.node().children()[2]);
                }
            };  

            r_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(name_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = *(Ptr*)ctx.node().value().get();
                string output(ptr);
                Ptr cptr = *(Ptr*)ctx.node().children()[0].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                output = readFile(string(cptr).to_std());
            };

            r_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                Ptr ticket = make_node();
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node& node = (Node&)(*(Ptr*)ctx.node().value().get());
                std::string source = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                Node root = process(source);
                compile(root);
                start_logged_stage(x_handlers);
                node.copy(root);
                ctx.node().scopes() << root;
                root.owner(ctx.node());
                end_logged_stage();
            };

            x_handlers[precompiling_id] = [this](Context& ctx){ctx.node().scopes()[0].owner(ctx.node());}; //To restore visibility

            e_handlers.default_function = [this](Context& ctx){
                if(!ctx.node().scopes().empty()&&ctx.node().scopes()[0].owner()==ctx.node()) {
                    e_scoped_handler(ctx);
                    return;
                }

                if(!is_live(ctx.node().value())) {ctx.node().quals().push(make_node(live_qual));} //For debug things and such

                if(is_node_opperator(ctx.node())) {
                    if(ctx.node().children().length()==2) {
                        e_stage_binary_handler(ctx);
                    } else if(ctx.node().children().length()==1) {
                        ctx.node().value((ctx.node().children()[0].value())); 
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[var_decl_id] = [this](Context& ctx){}; //Doing nothing
            e_handlers[equals_id] = [this](Context& ctx){
                e_stage_assignment_handler(ctx);
                standard_sub_process(ctx);
            };
            e_handlers[func_call_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[return_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[print_id] = [this](Context& ctx){
                ctx.node().value().quals().push(make_node(gatekeeper_qual));
                ctx.node().value().quals().push(make_node(live_qual));
                value_col subvals = ctx.node().value().sub_values();
                gather_all_values_in_scope(subvals,ctx.node());
                fire_quals(ctx,ctx.node().value());
                standard_sub_process(ctx);
            };
            e_handlers[to_prefix_id(gatekeeper_qual)] = [this](Context& ctx){
                for(int i=0;i<ctx.value().sub_values().length();i++) {
                    Value v = ctx.value().sub_values().get(i);
                    if(!v.has_qual(live_qual)) {v.quals().push(make_node(live_qual));}
                }
            };

            m_handlers.default_function = [this](Context& ctx){
                if(is_live(ctx.node().value())) {
                    if(should_allocate_data(ctx.node().value())) {
                        allocate_data(ctx.node().value());
                    }
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(m_handlers.has(child.type())) {
                            process_node(ctx,child);
                        } else {
                            if(is_live(child.value())) {
                                if(should_allocate_data(child.value())) {
                                    allocate_data(child.value());
                                }
                            }
                            process_node(ctx,child);
                        }
                    }
                    #if !LOBOTOMIZE_M_STAGE
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(!m_handlers.has(child.type())) {
                            if(should_recycle_data(child.value())) {
                                recycle_column(child.value().data_ptr());
                            }
                        }
                    }
                    #endif
                } else {
                    backwards_sub_process(ctx);
                }
            };
            m_handlers[var_decl_id] = [this](Context& ctx){
                #if !LOBOTOMIZE_M_STAGE
                if(should_recycle_data(ctx.node().value())) {
                    recycle_column(ctx.node().value().data_ptr());
                }
                #endif
            };
            m_handlers[identifier_id] = [this](Context& ctx){
                if(should_allocate_data(ctx.node().value())) {
                    allocate_data(ctx.node().value());
                }
            };

            x_handlers[literal_id] = [this](Context& ctx){
                std::string name = ctx.node().name().to_std();
                uint32_t vtype = ctx.node().value().type();
                if(vtype==int_id) {
                    int i = std::stoi(name); ctx.node().value().set((void*)&i);
                } else if(vtype==float_id) {
                    float f = std::stof(name); ctx.node().value().set((void*)&f);
                } else if(vtype==string_id) {
                    Ptr p = get_ticket(name_store_id,1,char_id); string s(p); s = name; ctx.node().value().set((void*)&p);
                } else if(vtype==bool_id) {
                    bool b = (name=="true"||name=="1");
                    ctx.node().value().set((void*)&b);
                } else if(vtype==char_id) {
                    char c = name.empty() ? '\0' : name[0];
                    ctx.node().value().set((void*)&c);
                } else if(vtype==null_id) { //For future use
                    Ptr dead = deadptr;
                    ctx.node().value().set((void*)&dead);
                } else {
                    throw_error("No way to handle value type ",labels[vtype],"!");
                }
            };


            a_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==A STAGE==");
                print(node_to_string(ctx.root()));
            };
            t_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==T STAGE==");
                print(node_to_string(ctx.root()));
            };
            r_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==R STAGE==");
                print(node_to_string(ctx.root()));
            };
            e_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==E STAGE==");
                print(node_to_string(ctx.root()));
            };
            m_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==M STAGE==");
                print(node_to_string(ctx.root()));
            };
            x_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==X STAGE==");
                print(node_to_string(ctx.root()));
            };

            r_handlers[make_tokenized_keyword("MISTAKE")] = [this](Context& ctx){
                print(ctx.node().value().reg());
            };

            e_handlers[make_tokenized_keyword("LBF_E")] = [this](Context& ctx){print("Launching blackfeather in e stage"); launch_blackfeather(unit_root);};
            x_handlers[make_tokenized_keyword("LBF_X")] = [this](Context& ctx){print("Launching blackfeather in x stage"); launch_blackfeather(unit_root);};


            x_handlers[invoke_stage_id] = [this](Context& ctx){
                std::string stage_name = ctx.node().name().to_std();
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
                Node left = ctx.node().children()[0];
                if(left.value().type()==node_id) {
                    left = Node(*(Ptr*)left.value().get());
                }
                ctx.node(left);
                stage->run(left.type())(ctx);
            };
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

        virtual Node process(std::string path) override {
            Node root = tokenize(path);
            unit_root = root;
            return root;
        }

        bool post_mortem_printed = false;
        void post_mortem(Node root) {
            if(post_mortem_printed) return; 
            else post_mortem_printed = true;

            print(red("FINISHED WITH ERROR: "),ERROR_MSG);
            ERROR_FLAG = false;
            launch_blackfeather(root);
            ERROR_FLAG = true;
        }

        void deresolve_nodes(Node root) {
            walk_nodenet(root,[](Node n){n.resolved(false);});
        }

        void print_stage_header(const std::string& label) {print_and_pause(0.7f,"\n\n\n\n\n\n\n\n=="+label+" STAGE==\n\n\n\n\n\n\n\n");} 

        void compile(Node root, bool prune = true) {
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(a_handlers);
            standard_direct_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            lemmatize_stages();
            newline("Resolving keywords");
                a_pass_resolve_keywords(root.children());
            endline();
            for(int i=0;i<root.children().length();i++) {
                place_node_in_scope(root.children()[i],root);
            }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(n_handlers);
            standard_direct_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(s_handlers);
            standard_direct_pass(root);
            end_logged_stage();
            
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(t_handlers);
            standard_resolving_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(r_handlers);
            standard_resolving_pass(root);
            end_logged_stage();

            //No E stage for now while testing
            // DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            // start_logged_stage(e_handlers);
            // memory_backwards_pass(root);
            // endline();

            // if(prune) {
            //     newline("Pruning dead nodes");
            //         e_pass_prune_dead(root.children());
            //     endline();
            // }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(m_handlers);
            memory_backwards_pass(root);
            end_logged_stage();
        }
    


        virtual void run(Node root) override {
            compile(root);

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //print(node_to_string(root,0,0,true));

            start_logged_stage(x_handlers);
            standard_travel_pass(root);
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //dump_unit(true);

            launch_blackfeather(root);
        }


    };
}