
#include<core/type.hpp>
#include<util/strings.hpp>
#include<util/logger.hpp>

#include<core/GDSL.hpp>

namespace GDSL {
    map<std::string,g_ptr<Value>> keywords;
    map<std::string,uint32_t> tokenized_keywords;

    void a_pass_resolve_keywords(list<g_ptr<Node>>& nodes) {
        for(g_ptr<Node>& node : nodes) {
            a_pass_resolve_keywords(node->children);
            g_ptr<Value> value = keywords.getOrDefault(node->name,fallback_value);
            if(value!=fallback_value) {
                if(!node->value)
                    node->value = make<Value>(0);
                
                node->value->copy(value);
            }
        }
    };

    map<char,bool> char_is_split;

    list<size_t> discard_types;

    g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
        return make<Value>(sub_type,size,reg_id(name));
    }

    size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
        g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
        keywords.put(name,val);
        return val->sub_type;
    }

    size_t make_tokenized_keyword(const std::string& f) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t id = reg_id(uppercase_name);
        tokenized_keywords.put(f,id);
        return id;
    }

    // T() {} <- As with function, but ctx.node->frame contains the bracketed code for further execution.
    static size_t add_scoped_keyword(const std::string& name, int scope_prec,void(*exec_fn)(Context&))
    {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        size_t id = make_tokenized_keyword(name);
        s_handlers[id] = [](Context& ctx) {
            g_ptr<Node> scope = ctx.node->in_scope->spawn_sub_scope();
            for(auto c : ctx.node->children) {
                c->place_in_scope(scope.getPtr());
            }
            scope->owner = ctx.node.getPtr();
            ctx.node->scopes << scope.getPtr();
            scope->name = ctx.node->name;
        };
        x_handlers[id] = exec_fn;
        return id;
    }

    size_t add_token(char c, const std::string& f) {
        size_t id = reg_id(f);
        tokenizer_functions.put(c,[id,c](Context& ctx) {
            ctx.node = make<Node>(id,c);
            ctx.result->push(ctx.node);
        });
        char_is_split.put(c, true);
        return id;
    }

    size_t var_decl_id = reg_id("VAR_DECL");
    size_t to_decl_id(size_t id) {return id+1;}
    size_t to_unary_id(size_t id) {return id+2;}

    size_t add_binary_operator(char c, const std::string& f,int left_bp, int right_bp) {
        size_t id = add_token(c,f);
        labels[to_decl_id(id)] = f+"_DECL";
        labels[to_unary_id(id)] = f+"_UNARY";   
        t_handlers[id] = [](Context& ctx) {
            standard_sub_process(ctx);
            if(!ctx.node->value) 
                ctx.node->value = make<Value>();
            ctx.node->value->copy(ctx.node->left()->value);
        }; 
        return id;
    }


    g_ptr<Value> make_qual_value(const std::string& f, size_t size = 0) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        size_t id = reg_id(uppercase_name);
        g_ptr<Value> val = make<Value>(id,size,id);
        return val;
    }

    g_ptr<Value> make_type(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_qual_value(f,size);

        t_handlers[val->type+1] = [](Context& ctx){
            ctx.value->sub_type = ctx.qual.sub_type;
            ctx.value->type = ctx.qual.type;
            ctx.value->size = ctx.qual.value->size;
            if(ctx.qual.value->type_scope)
                ctx.value->type_scope = ctx.qual.value->type_scope;
        };

        return  val;
    }

    size_t add_qualifier(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_qual_value(f,size);
        keywords.put(f,val);
        return val->type;
    }
        
    size_t add_type(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_type(f,size);
        keywords.put(f,val);
        return val->type;
    }

    static size_t add_function(const std::string& f, void(*op)(Context&), uint32_t return_type = 0) {
        std::string uppercase_name = f;
        std::transform(uppercase_name.begin(),uppercase_name.end(), uppercase_name.begin(), ::toupper);
        g_ptr<Value> val = make_value(uppercase_name,0,return_type);
        keywords.put(f,val);
        size_t call_id = val->sub_type;

        x_handlers[call_id] = op;
        return call_id;
    }


    //Qual handlers which act on the value
    size_t to_prefix_id(size_t id) {return id+1;}
    //Qual handlers which act on the node
    size_t to_suffix_id(size_t id) {return id+2;}



    //Need to define all these globally so that the function pointers can access them
    size_t identifier_id = reg_id("IDENTIFIER");
    size_t object_id = reg_id("OBJECT");
    size_t literal_id = reg_id("LITERAL");
    size_t end_id = add_token(';',"END");
    size_t func_decl_id = reg_id("FUNC_DECL");
    size_t lparen_id = add_token('(',"LPAREN");
    size_t rparen_id = add_token(')',"RPAREN");
    size_t comma_id = add_token(',',"COMMA");
    size_t float_id = add_type("float",4);
    size_t int_id = add_type("int",4);
    size_t bool_id = add_type("bool",1);
    size_t string_id = add_type("string",24);
    size_t in_string_id = reg_id("IN_STRING_KEY");
    size_t plus_id = add_binary_operator('+',"PLUS", 4, 5);
    size_t dash_id = add_binary_operator('-',"DASH", 4, 5);
    size_t rangle_id = add_binary_operator('>',"RANGLE", 2, 3);
    size_t langle_id = add_binary_operator('<',"LANGLE", 2, 3);
    size_t equals_id = add_binary_operator('=', "ASSIGNMENT", 1, 0);
    size_t func_call_id = reg_id("FUNC_CALL");
    size_t star_id = add_binary_operator('*',"STAR", 6, 7);
    size_t in_alpha_id = reg_id("IN_ALPHA");
    size_t in_digit_id = reg_id("IN_DIGIT");
    size_t if_id = 0;
    size_t else_id = 0;
    size_t while_id = 0;
    size_t print_id = 0;
    size_t for_id = 0;

    size_t defun_id = 0;

    size_t wubless_qual = add_qualifier("wubless");
    size_t wubfull_qual = add_qualifier("wubfull");
    size_t const_qual = add_qualifier("const");
    size_t static_qual = add_qualifier("static");
    size_t inline_qual = add_qualifier("inline");
    size_t struct_qual = add_qualifier("struct");
    size_t type_qual = add_qualifier("type");

    size_t live_qual = reg_id("LIVE");
    size_t ptr_qual = reg_id("POINTER");
    size_t paren_qual = reg_id("PAREN");

    g_ptr<Node> lisp_expression_parse(Context& ctx) {
        if(ctx.index >= ctx.nodes.length()) return nullptr;
        g_ptr<Node> token = ctx.nodes[ctx.index];
        ctx.index++;
        
        if(token->type == lparen_id) {
            g_ptr<Node> op = ctx.nodes[ctx.index];
            ctx.index++;
            op->children.clear();
            while(ctx.index < ctx.nodes.length() &&
                  ctx.nodes[ctx.index]->type != rparen_id) {
                g_ptr<Node> arg = lisp_expression_parse(ctx);
                if(arg) op->children << arg;
            }
            ctx.index++; // consume ')'
            return op;
        }
        
        return token; // atom
    }


    void test_module(const std::string& path) {
        span = make<Log::Span>();

        init_handlers(a_handlers,[](Context& ctx){
            g_ptr<Node> expr = lisp_expression_parse(ctx);
            if(expr && !discard_types.has(expr->getType())) {
                ctx.result->push(expr);
            }
        });

        discard_types.push(undefined_id);
        discard_types.push(end_id);
        discard_types.push(lparen_id);
        discard_types.push(rparen_id);
        discard_types.push(comma_id);

        x_handlers[literal_id] = [](Context& ctx){};
      
        value_to_string.put(float_id,[](void* data) {
            return std::to_string(*(float*)data);
        });
        negate_value.put(float_id,[](void* data) {
            *(float*)data = -(*(float*)data);
        });
        t_handlers[float_id] = [](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(float_id,4);
            value->set<float>(std::stoi(ctx.node->name));
            ctx.node->value = value;
        }; 

        value_to_string.put(int_id,[](void* data) {
            return std::to_string(*(int*)data);
        });
        negate_value.put(int_id,[](void* data) {
            *(int*)data = -(*(int*)data);
        });
        t_handlers[int_id] = [](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(int_id,4);
            value->set<int>(std::stoi(ctx.node->name));
            ctx.node->value = value;
        }; 

        value_to_string.put(bool_id,[](void* data){
            return (*(bool*)data) ? "TRUE" : "FALSE";
        });
        t_handlers[bool_id] = [](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(bool_id,1);
            value->set<bool>(ctx.node->name=="true" ? true : false); 
            ctx.node->value = value;
        }; 

        tokenizer_state_functions.put(in_string_id,[](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(c=='"') {
                ctx.state=0;
            }
            else {
                ctx.node->name += c;
            }
        });
        tokenizer_functions.put('"',[](Context& ctx) {
            ctx.state = in_string_id;
            ctx.node = make<Node>();
            ctx.node->type = string_id;
            ctx.node->name = "";
            ctx.result->push(ctx.node);
        });
        value_to_string.put(string_id,[](void* data){
            return *(std::string*)data;
        });
        t_handlers[string_id] = [](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(string_id,24);
            value->set<std::string>(ctx.node->name);
            ctx.node->value = value;
        }; 
           
        x_handlers[plus_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                +
                *(int*)ctx.node->right()->value->data
            );
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };

        x_handlers[dash_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                -
                *(int*)ctx.node->right()->value->data
            );
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };

        x_handlers[rangle_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                >
                *(int*)ctx.node->right()->value->data
            );
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };

        x_handlers[langle_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                <
                *(int*)ctx.node->right()->value->data
            );
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };

        x_handlers[star_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                *
                *(int*)ctx.node->right()->value->data
            );
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };
        
        e_handlers[equals_id] = [](Context& ctx){ 
            if(ctx.node->left()&&ctx.node->right()) {
                int at_id = ctx.node->left()->value->quals.find_if([](const Qual& q){return q.type==live_qual;});
                if(at_id!=-1) {
                    ctx.node->right()->value->quals << ctx.node->left()->value->quals[at_id];
                } else {
                    ctx.node = nullptr;
                }
            }
        };
        x_handlers[equals_id] = [](Context& ctx) {
            standard_sub_process(ctx);
            if(!ctx.node->left()->value->data) {
                ctx.node->left()->value->size = ctx.node->right()->value->size;
                ctx.node->left()->value->type = ctx.node->right()->value->type;
                ctx.node->left()->value->data = malloc(ctx.node->right()->value->size);
            }
            memcpy(ctx.node->left()->value->data, ctx.node->right()->value->data, ctx.node->right()->value->size);
        };

        r_handlers[func_call_id] = [](Context& ctx) {
            if(ctx.node->scope()) {
                for(int i = 0; i < ctx.node->children.size(); i++) {
                    g_ptr<Node> arg = resolve_symbol(ctx.node->children[i], ctx.root);
                    g_ptr<Node> param = ctx.node->scope()->owner->children[i];
                    g_ptr<Node> assignment = make<Node>();
                    assignment->type = equals_id;
                    assignment->children << param;
                    assignment->children << arg;
                    ctx.node->children[i] = assignment;
                }
            }
        };
        x_handlers[func_call_id] = [](Context& ctx) {
            standard_sub_process(ctx);
            process_node(ctx.node->scope()->owner->children.last());
        };

        defun_id = add_scoped_keyword("defun",2,[](Context& ctx){});
        t_handlers[defun_id] = [](Context& ctx) {
            g_ptr<Node> node = ctx.node;

            node->value = make<Value>();
            node->name = node->children.take(0)->name;

            g_ptr<Node> param_block = node->children[0];
            auto distribute_param = [](g_ptr<Node> param) {
                param->type = var_decl_id;
                param->value = make<Value>();
                param->value = param->in_scope->distribute_value(param->name, param->value);
            };
            for(int i=param_block->children.length()-1;i>=0;i--) {
                g_ptr<Node> c = param_block->children.take(i);
                node->children.insert(c,1);
                distribute_param(c);
            }
            distribute_param(param_block);


            g_ptr<Node> body = node->children.last();
            process_node(body);


            node->type = func_decl_id;
            node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
            body->value->type_scope = node->scope().getPtr();
            node->value = node->in_scope->distribute_value(node->name,body->value);
        };


        if_id = add_scoped_keyword("if", 0, [](Context& ctx) {
            process_node(ctx.node->children[0]); // condition
            if(ctx.node->children[0]->value->is_true()) {
                process_node(ctx.node->children[1]); // true branch
                if(ctx.node->children[1]->value)
                    ctx.node->value->copy(ctx.node->children[1]->value);
            } else if(ctx.node->children.length() > 2) {
                process_node(ctx.node->children[2]); // false branch
                if(ctx.node->children[2]->value)
                    ctx.node->value->copy(ctx.node->children[2]->value);
            }
        });

        t_handlers[identifier_id] = [](Context& ctx) {
            g_ptr<Node> node = ctx.node;
            g_ptr<Value> decl_value = make<Value>();
            
            bool had_a_value = (bool)(node->value);
            bool had_a_scope = (bool)(node->scope());
            bool found_a_value = node->find_value_in_scope();
            bool is_qualifier = node->value_is_valid();

            standard_sub_process(ctx);

            if(keywords.hasKey(node->name)) {
                node->type = node->value->sub_type;
                return;
            }

            node->value = decl_value;
            fire_quals(ctx, decl_value);

            bool has_scope = node->scope() != nullptr;
            bool has_type_scope = node->value->type_scope != nullptr;
            bool has_sub_type = node->value->sub_type != 0;
            
            if(has_scope) {
                node->scope()->owner = node.getPtr();
                node->scope()->name = node->name;
                node->type = func_decl_id;
                node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
                node->value->type_scope = node->scope().getPtr();
                node->value = node->in_scope->distribute_value(node->name,node->value);
                node->value->sub_type = 0;
            } else {
                has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
                if(has_scope) {
                    node->type = func_call_id;
                    node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(node->value->type_scope)
                        node->scopes[0] = node->value->type_scope; //Swap to the type scope
                } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
                    node->find_value_in_scope();
                } else {                                         

                }
            }
        };

        char_is_split.put(' ',true);
        tokenizer_state_functions.put(in_alpha_id,[](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.index;
                ctx.node->type = tokenized_keywords.getOrDefault(ctx.node->name,ctx.node->type);
                return;
            } else {
                ctx.node->name += c;
            }
        });

        tokenizer_state_functions.put(in_digit_id,[](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(char_is_split.getOrDefault(c,false)) {
                ctx.state = 0; 
                --ctx.index;
                return;
            } else if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
            } else if(c=='.') {
                ctx.node->type = float_id;
            }
            ctx.node->name += c;
        });

        tokenizer_default_function = [](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
                ctx.node = make<Node>(identifier_id,c);
                ctx.result->push(ctx.node);
            }
            else if(std::isdigit(c)) {
                ctx.state = in_digit_id;
                ctx.node = make<Node>(int_id,c);
                ctx.result->push(ctx.node);
            } else if(c==' '||c=='\t'||c=='\n') {
                //just skip
            }
            else {
                print("tokenize::default_function missing handling for char: ",c);
            }
        };

        init_handlers(s_handlers,[](Context& ctx) {
            
        });


        init_handlers(t_handlers,[](Context& ctx) {
            standard_sub_process(ctx);
        });

        init_handlers(r_handlers,[](Context& ctx) {
            resolve_sub_nodes(ctx);
        });

        init_handlers(d_handlers,[](Context& ctx){
            for(auto c : ctx.node->children) {
                discover_symbol(c,ctx.root);
            }
        });

        init_handlers(e_handlers,[](Context& ctx){
            //print("Missing e_handler for ",labels[ctx.node->type]);
        });

        init_handlers(x_handlers,[](Context& ctx){
            //print("Defualt x handler for: ",ctx.node->info());
        });

        print_id = add_function("print",[](Context& ctx) {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                process_node(r);
                toPrint.append(r->value->to_string());
            }
            print(toPrint);
        });
        e_handlers[print_id] = [](Context& ctx){
            for(auto c : ctx.node->children) {
                int found_at = c->value->find_qual(live_qual);
                if(found_at!=-1) {
                    ctx.node->value->sub_values.push_if_absent(c->value->quals[found_at].value);
                } else {
                    g_ptr<Value> token = make<Value>();
                    ctx.node->value->sub_values << token;
                    Qual live(live_qual,token);
                    c->value->quals << live;
                }
            }
        };

        std::function<void(g_ptr<Node>)> print_scopes = [&print_scopes](g_ptr<Node> root) {
            for(auto t : root->scopes) {
                print(t->to_string());
            }
            for(auto child_scope : root->scopes) {
                print_scopes(child_scope);
            }
        };


        span->print_on_line_end = false; //While things aren't crashing
        //span->log_everything = true; //While things are crashing


        std::string code = readFile(path);
        Log::Line timer; timer.start();
        print("TOKENIZE");
        list<g_ptr<Node>> tokens = tokenize(code);
        print("A STAGE");
        start_stage(a_handlers);
        list<g_ptr<Node>> nodes = parse_tokens(tokens);
        a_pass_resolve_keywords(nodes);
        // print("==Keyword resolved nodes==");
        // for(auto a : nodes) {
        //     print(a->to_string(1));
        // }

        print("S STAGE");
        start_stage(s_handlers);
        g_ptr<Node> root = make<Node>();
        root->name = "GLOBAL";
        for(auto n : nodes) {
            n->place_in_scope(root.getPtr());
        }
        root->children = nodes;
        standard_travel_pass(root);

        // //print(root->to_string(0,0,true));

        print("T STAGE");
        start_stage(t_handlers);
        standard_travel_pass(root);


        newline("Discovering symbols");
        print("D STAGE");
        start_stage(d_handlers);
        discover_symbols(root);
        endline();
        print("R STAGE");
        start_stage(r_handlers);
        resolve_symbols(root);

        // print("E STAGE");
        // start_e_stage(root);

        

        std::string final_time = ftime(timer.end());

        // print("==LOG==");
        span->print_all();
        print(root->to_string());
        print_scopes(root);

        timer.start();

        print("X STAGE");
        start_stage(x_handlers);
        standard_travel_pass(root);

        print("Exec time: ",ftime(timer.end()));
        print("Final time: ",final_time);

        //span->print_all();

        // print(root->to_string());
        // print_scopes(root);

    }

}
