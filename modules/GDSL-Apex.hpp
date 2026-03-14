
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
            for(auto scope : node->scopes) {
                a_pass_resolve_keywords(scope->children);
            }
        }
    };

    map<char,bool> char_is_split;

    map<uint32_t,int> left_binding_power;
    map<uint32_t,int> right_binding_power;

    map<uint32_t, std::function<void(g_ptr<Node>, g_ptr<Node>, g_ptr<Node>)>> scope_link_handlers;
    map<uint32_t, int> scope_precedence;

    list<size_t> discard_types;



    void print_scope(g_ptr<Node> scope, int depth = 0) {
        std::string indent(depth * 2, ' ');
        
        if (depth > 0) {
            log(indent, "{");
        }
        
        for (auto& subnode : scope->children) {
            log(indent, "  ", labels[subnode->type]);
        
            
            if(!subnode->scopes.empty()) {
                for(auto subscope : subnode->scopes) {
                    print_scope(subscope, depth + 1);
                }
            }
        }
        
        if (depth > 0) {
            log(indent, "}");
        }
    }

    struct g_value {
    public:
        g_value() {}
        g_value(g_ptr<Node> _owner) : owner(_owner) {}
        bool explc = false;
        g_ptr<Node> owner = nullptr;
        bool deferred = false;
    };

    static g_ptr<Node> parse_scope(list<g_ptr<Node>> nodes) {
        g_ptr<Node> root_scope = make<Node>();
        root_scope->name = "GLOBAL";
        g_ptr<Node> current_scope = root_scope;
        list<g_value> stack{g_value()};

        #if PRINT_ALL
        newline("Parse scope pass");
        #endif

        for (int i = 0; i < nodes.size(); ++i) {
            g_ptr<Node> node = nodes[i];
            g_ptr<Node> owner_node = (i>0) ? nodes[i - 1] : nullptr;

            int p = scope_precedence.getOrDefault(node->type,0);
            bool on_stack = stack.last().owner ? true : false;
            if(p<=0) {
                if(p<0) {
                    if (current_scope->parent) {
                        current_scope = current_scope->parent;
                    }
                }
                else {
                    current_scope->children << node; 
                    node->place_in_scope(current_scope.getPtr());
                    if(on_stack && !stack.last().deferred && !stack.last().explc) {
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                }
            }
            else {
                if(p<10) {
                    current_scope->children << node;
                    node->place_in_scope(current_scope.getPtr());
                    owner_node = node;
                }

                if(on_stack) {
                    int stack_precedence = scope_precedence.getOrDefault(stack.last().owner->type,0);
                    if(p >= stack_precedence) {
                        stack.last().deferred = true;
                    }
                }

                if(p == 10) {
                    if(on_stack) {
                        stack.last().explc = true;
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                }
                else {
                    stack << g_value(owner_node);
                }

                g_ptr<Node> parent_scope = current_scope;
                current_scope = current_scope->spawn_sub_scope();
                if (owner_node) {
                    //Deffensive check here
                    try {
                        auto func = scope_link_handlers.get(owner_node->type);
                        func(current_scope,parent_scope,owner_node);
                    }
                    catch(std::exception e) {
                        print("parse_scope::809 missing scope link handler for type: ",labels[owner_node->type]);
                    }
                
                } else {
                    current_scope->type = 0; //Suppoused to be GET_TYPE(BLOCK), doesn't matter, don't care
                }
            }
        }

        #if PRINT_ALL
        //print_scope(root_scope);
        endline();
        #endif
        return root_scope;
    }


    g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
        return make<Value>(sub_type,size,reg_id(name));
    }

    size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
        g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
        keywords.put(name,val);
        return val->sub_type;
    }

    size_t make_tokenized_keyword(const std::string& f) {
        size_t id = reg_id(f);
        tokenized_keywords.put(f,id);
        return id;
    }

    // T() {} <- As with function, but ctx.node->frame contains the bracketed code for further execution.
    static size_t add_scoped_keyword(const std::string& name, int scope_prec,void(*exec_fn)(Context&))
    {
        size_t id = make_tokenized_keyword(name);
        scope_precedence.put(id, scope_prec);
        scope_link_handlers.put(id, [](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
            if(owner_node->scope()) { //To handle implicit scoping
                owner_node->in_scope->scopes.erase(owner_node->scopes.take(0));
            } 
            new_scope->owner = owner_node.getPtr();
            owner_node->scopes << new_scope.getPtr();
            for(auto c : owner_node->children) {
                c->place_in_scope(new_scope.getPtr());
            }
            new_scope->name = owner_node->name;
        });
        t_handlers[id] = [](Context& ctx){};
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

    //Qual handlers which act on the value
    size_t to_prefix_id(size_t id) {return id+1;}
    //Qual handlers which act on the node
    size_t to_suffix_id(size_t id) {return id+2;}

    // xTx <- Binary operator, ctx.node->right() and ctx.node->left() are what's on your right and left
    size_t add_binary_operator(char c, const std::string& f,int left_bp, int right_bp) {
        size_t id = add_token(c,f);
        left_binding_power.put(id, left_bp);
        right_binding_power.put(id, right_bp);

        labels[to_decl_id(id)] = f+"_DECL";
        labels[to_unary_id(id)] = f+"_UNARY";

        t_handlers[id] = [](Context& ctx){
            size_t decl_id = to_decl_id(ctx.node->type);
            size_t unary_id = to_unary_id(ctx.node->type);
            auto& children = ctx.node->children;
            standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
            if(children.length() == 2) {
                g_ptr<Node> type_term = children[0];
                g_ptr<Node> id_term = children[1];
                
                if(type_term->type==var_decl_id) {
                    ctx.node->type = decl_id;
                    ctx.node->value = type_term->value;
                    ctx.node->name = id_term->name;
                    ctx.node->value->sub_type = 0;
                    ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                    ctx.node->children.clear();

                    Qual marker(decl_id,true); //Make a muted qual marker
                    ctx.node->value->quals << marker;

                } else {
                    ctx.node->value->copy(type_term->value);
                }
            } else if(children.length() == 1) {
                g_ptr<Node> type_term = children[0];

                ctx.node->type = unary_id;
                ctx.node->value->copy(type_term->value);
            } 
        };
        
        return id;
    }


    g_ptr<Value> make_qual_value(const std::string& f, size_t size = 0) {
        size_t id = reg_id(f);
        g_ptr<Value> val = make<Value>(id,size,id);
        return val;
    }

    g_ptr<Value> make_type(const std::string& f, size_t size = 0) {
        g_ptr<Value> val = make_qual_value(f,size);

        t_handlers[to_prefix_id(val->type)] = [](Context& ctx){
            if(ctx.value->sub_type==0) {
                ctx.value->sub_type = ctx.qual.sub_type;
                ctx.value->type = ctx.qual.type;
                ctx.value->size = ctx.qual.value->size;
                if(ctx.qual.value->type_scope)
                    ctx.value->type_scope = ctx.qual.value->type_scope;
            }
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

    size_t func_call_id = reg_id("FUNC_CALL");
    size_t builtin_func_qual = reg_id("BUILTIN_FUNC");
    size_t sobj_qual = reg_id("SOBJ_QUAL");

    static size_t add_function(const std::string& f, void(*op)(Context&), uint32_t return_type = 0) {
        g_ptr<Value> val = make_value(f,0,return_type);
        keywords.put(f,val);
        size_t call_id = val->sub_type;
        val->quals << Qual(builtin_func_qual);

        x_handlers[call_id] = op;
        return call_id;
    }


    #define LOG_A_PARSE 0

    g_ptr<Node> a_parse_expression(Context& ctx, int min_bp, g_ptr<Node> left_node = nullptr) {
        //Prefixual pass
        if(ctx.index>=ctx.nodes.length()) return nullptr;
        g_ptr<Node> token = ctx.nodes[ctx.index];
        #if LOG_A_PARSE
            newline("a_parse_expression_pass: "+token->info());
        #endif

        uint32_t type = token->getType();
        int left_bp = left_binding_power.getOrDefault(type,-1);
        int right_bp = right_binding_power.getOrDefault(type,-1);
        bool has_func = a_handlers[type] != a_parse_function;

        #if LOG_A_PARSE
            log("Starting at index ",ctx.index,", token: ",token->info()," (left_bp: ",left_bp,", right_bp: ",right_bp,", has_func: ",has_func?"yes":"no",")");
            if(left_node)
                log("Left_node: ",left_node->info());
            if(ctx.left) 
                log("ctx.left: ",ctx.left->info());
            if(ctx.node) 
                log("ctx.node: ",ctx.node->info());
        #endif

        ctx.index++;
        #if LOG_A_PARSE
            if(ctx.index<ctx.nodes.length())
                log("Looking at index ",ctx.index,", token: ",ctx.nodes[ctx.index]->info());
        #endif

        if(!left_node)
            left_node = make<Node>();

        if(has_func) { //Has a function but no left: direct node build
            ctx.left = token;
            ctx.node = make<Node>();  //Fresh output target
            #if LOG_A_PARSE
                newline("Running a_function for "+token->info());
            #endif
            a_handlers[type](ctx);
            #if LOG_A_PARSE
                if(ctx.node) 
                    log("Function returned:\n",ctx.node->to_string(1));
                endline();
            #endif
            left_node = ctx.node; //Read result back
        }
        else if(right_bp!=-1) { //Prefixual unary: recurse with right
            g_ptr<Node> right_node = a_parse_expression(
                ctx,
                right_bp,
                nullptr
            );
            left_node->type = type;
            if(right_node)
                left_node->children << right_node;
        }
        else { //Else atom, like a literal or idenitifer
            left_node = token;
            ctx.node = left_node;
        }

        //Infixual pass
        #if LOG_A_PARSE
            newline("Running infix");
        #endif
        while(ctx.index < ctx.nodes.length()) {
            g_ptr<Node> op = ctx.nodes[ctx.index];
            int op_left_bp = left_binding_power.getOrDefault(op->getType(), -1);

            #if LOG_A_PARSE
                log("In infix at index ",ctx.index,", op: ",op->info()," (left_bp: ",op_left_bp,")");
            #endif

            if(op_left_bp < min_bp || op_left_bp==-1) {
                break;
            }
            
            ctx.index++;

            #if LOG_A_PARSE
                if(ctx.index<ctx.nodes.length()) log("Looking at index ",ctx.index,", token: ",ctx.nodes[ctx.index]->info());
            #endif

            if(a_handlers[op->getType()]!=a_parse_function) {
                if(left_node->type!=0)
                    ctx.left = left_node;
                ctx.node = make<Node>();
                #if LOG_A_PARSE
                    newline("Running function in infix from: "+op->info());
                #endif
                a_handlers[op->getType()](ctx);
                #if LOG_A_PARSE
                    if(ctx.node) 
                        log("Infix function returned:\n",ctx.node->to_string(1));
                    endline();
                #endif
                if(ctx.node) {
                    left_node = ctx.node;
                } else {
                    ctx.index--;
                    break;
                }
            } else {
                g_ptr<Node> right_node = a_parse_expression(
                    ctx,
                    right_binding_power.getOrDefault(op->getType(), op_left_bp + 1),
                    nullptr
                );
                
                g_ptr<Node> node = make<Node>();
                node->type = op->getType();
                if(left_node)
                    node->children << left_node;
                if(right_node) {
                    if(!discard_types.has(right_node->type))
                        node->children << right_node;
                }
                left_node = node;
                
            }
        }
        #if LOG_A_PARSE
            log("Ended infix loop at index ",ctx.index,ctx.index<ctx.nodes.length()?", token: "+ctx.nodes[ctx.index]->info():" OVERSHOT INDEX!!");
            endline();
        #endif
        if(left_node) {
            ctx.node = left_node;
            #if LOG_A_PARSE
                log("Returning left node:\n",left_node->to_string(4));
            #endif
        }
        #if LOG_A_PARSE
            endline();
        #endif
        return left_node;
    }





    //Need to define all these globally so that the function pointers can access them
    size_t identifier_id = reg_id("IDENTIFIER");
    size_t object_id = reg_id("OBJECT");
    size_t literal_id = reg_id("LITERAL");
    size_t end_id = add_token(';',"END");
    size_t colon_id = add_token(':',"COLON");
    size_t function_id = reg_id("FUNCTION");
    size_t method_id = reg_id("METHOD");
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
    size_t star_id = add_binary_operator('*',"STAR", 6, 7);
    size_t amp_id = add_binary_operator('&',"AMPERSAND",-1,8);
    size_t dot_id = add_binary_operator('.', "PROP_ACCESS", 8, 9);
    size_t return_id = make_tokenized_keyword("return");
    size_t break_id = make_tokenized_keyword("break");
    size_t lbracket_id = add_token('[', "LBRACKET");
    size_t rbracket_id = add_token(']', "RBRACKET");
    size_t type_decl_id = reg_id("TYPE_DECL");
    size_t method_scope_id = reg_id("METHOD_SCOPE");
    size_t type_scope_id = reg_id("TYPE_SCOPE");
    size_t lbrace_id = add_token('{', "LBRACE");
    size_t rbrace_id = add_token('}', "RBRACE");
    size_t in_alpha_id = reg_id("IN_ALPHA");
    size_t in_digit_id = reg_id("IN_DIGIT");
    size_t if_id = 0;
    size_t else_id = 0;
    size_t while_id = 0;
    size_t print_id = 0;
    size_t for_id = 0;

    size_t wubless_qual = add_qualifier("wubless");
    size_t wubfull_qual = add_qualifier("wubfull");
    size_t const_qual = add_qualifier("const");
    size_t static_qual = add_qualifier("static");
    size_t inline_qual = add_qualifier("inline");
    size_t struct_qual = add_qualifier("struct");
    size_t type_qual = add_qualifier("type");

    size_t template_id = add_qualifier("template");
    size_t typename_id = add_type("typename");

    size_t live_qual = reg_id("LIVE");
    size_t ptr_qual = reg_id("POINTER");
    size_t paren_qual = reg_id("PAREN");


    size_t map_id = add_type("map",0);
    size_t list_id = add_type("list",0);
    size_t set_id = add_type("set",0);

    size_t get_id = 0;
    size_t put_id = 0;
    size_t push_id = 0;

    size_t add_soql_keyword(const std::string& f) {
        size_t id = make_tokenized_keyword(f);
        t_handlers[id] = [](Context& ctx) {
            if(ctx.result && ctx.index < ctx.result->length()-1) {
                ctx.node->children << ctx.result->take(ctx.index+1);
                while(ctx.index < ctx.result->length()-1 && ctx.result->get(ctx.index+1)->type == comma_id) {
                    ctx.result->removeAt(ctx.index+1);
                    ctx.node->children << ctx.result->take(ctx.index+1);
                }
            }
        };
        return id;
    }
    
    size_t select_id = add_soql_keyword("SELECT");
    size_t from_id = add_soql_keyword("FROM");
    size_t where_id = add_soql_keyword("WHERE");
    size_t and_id = add_soql_keyword("AND");
    size_t or_id = add_soql_keyword("OR");
    size_t limit_id = add_soql_keyword("LIMIT");
    size_t order_id = add_soql_keyword("ORDER");
    size_t by_id = add_soql_keyword("BY");
    size_t like_id = add_soql_keyword("LIKE");
    size_t in_id = add_soql_keyword("IN");
    size_t insert_id = make_tokenized_keyword("insert");
    size_t update_id = make_tokenized_keyword("update");
    size_t delete_id = make_tokenized_keyword("delete");
    size_t upsert_id = make_tokenized_keyword("upsert");



    void inject_this_param(Context& ctx) {
        g_ptr<Node> node = ctx.node;
        g_ptr<Node> star = make<Node>();
        star->type = star_id;
        star->place_in_scope(node->scope().getPtr());
        star->name = "made_by_this";

        g_ptr<Node> type_term = make<Node>();
        type_term->type = identifier_id;
        type_term->name = node->in_scope->owner->name;
        type_term->place_in_scope(node->scope().getPtr());
        star->children << type_term;

        g_ptr<Node> id_term = make<Node>();
        id_term->type = identifier_id;
        id_term->name = "this";
        id_term->place_in_scope(node->scope().getPtr());
        star->children << id_term;

        node->children.insert(star, 0);
    };
    void inject_member_access(Context& ctx) {
        g_ptr<Node> node = ctx.node;
        g_ptr<Node> prop = make<Node>();
        prop->type = dot_id;
        prop->place_in_scope(node->in_scope);
        
        g_ptr<Node> star = make<Node>();
        star->type = star_id;
        star->place_in_scope(node->in_scope);
        
        g_ptr<Node> this_id = make<Node>();
        this_id->type = identifier_id;
        this_id->name = "this";
        this_id->place_in_scope(node->in_scope);
        star->children << this_id;
        
        g_ptr<Node> member_id = make<Node>();
        member_id->type = identifier_id;
        member_id->name = node->name;
        member_id->place_in_scope(node->in_scope);
        
        prop->children << star;
        prop->children << member_id;
        //We parse this node because it won't resolve again like something in a type scope would
        process_node(ctx, prop);
        node->copy(prop);
    };

    static void point_fields_at_row(g_ptr<Node> node, g_ptr<Type> store, int tid) {
        if(node->type == identifier_id) {
            _note& note = store->get_note(node->name);
            if(note.index!=-1) {
                node->value->data = store->get(note.index, tid, note.size);
                node->value->size = note.size;
            }
        }
        for(auto c : node->children) {
            point_fields_at_row(c, store, tid);
        }
    }

    void test_module(const std::string& path) {
        span = make<Log::Span>();

        init_handlers(a_handlers,[](Context& ctx){
            g_ptr<Node> expr = a_parse_expression(ctx, 0);
            if(expr && !discard_types.has(expr->getType())) {
                ctx.result->push(expr);
            }
        });

        x_handlers[lbracket_id] = [](Context& ctx) {
            //If array acess
            // process_node(ctx.node->left()); // base
            // process_node(ctx.node->right()); // index
            
            // int i = ctx.node->right()->value->get<int>();
            // size_t element_size = ctx.node->children[0]->value->size;
            
            // ctx.node->value->type = ctx.node->children[0]->value->type;
            // ctx.node->value->size = element_size;
            // ctx.node->value->type_scope = ctx.node->children[0]->value->type_scope;
            // ctx.node->value->data = (char*)ctx.node->children[0]->value->data + i * element_size;

            // 1. Find FROM and WHERE children
            g_ptr<Node> from_node = nullptr;
            g_ptr<Node> where_node = nullptr;
            g_ptr<Node> select_node = nullptr;
            
            for(auto c : ctx.node->children) {
                if(c->type == from_id) from_node = c;
                else if(c->type == where_id) where_node = c;
                else if(c->type == select_id) select_node = c;
            }

            if(!from_node) return;
            
            g_ptr<Type> store = from_node->children[0]->value->store;
            
            g_ptr<Type> results = make<Type>();
            int row_count = store->row_length(0, 24); //Because Id always exists (or should)
            
            for(int tid = 0; tid < row_count; tid++) {
                if(where_node) {
                    point_fields_at_row(where_node->children[0], store, tid);
                    process_node(ctx, where_node->children[0]);
                    if(!where_node->children[0]->value->is_true()) continue;
                }
                g_ptr<Value> handle = make<Value>();
                handle->type = from_node->children[0]->type;
                handle->store = store;
                handle->address = tid;
                handle->quals << Qual(sobj_qual);
                ctx.node->value->sub_values << handle;
                //print("PUSHED: ",handle->info());
            }
        };

        
        discard_types.push(undefined_id);
        discard_types.push(end_id);
        discard_types.push(lparen_id);
        discard_types.push(rparen_id);
        discard_types.push(comma_id);

        value_to_string.put(object_id, [](void* data) {
            return std::string("[object @") + std::to_string((size_t)data) + "]";
        });
        x_handlers[literal_id] = [](Context& ctx){};

        a_handlers[identifier_id] = [](Context& ctx){
            if(ctx.index>=ctx.nodes.length()) {
                log("Hey! Index overun by ",ctx.left->info());
                return;
            }

            if(ctx.left) {
                //log("Left looks like: ",ctx.left->info());
                if(ctx.nodes[ctx.index]->getType()==identifier_id) {
                    ctx.node = ctx.left;
                    while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() == identifier_id) {
                        ctx.node->children << ctx.nodes[ctx.index];
                        ctx.index++;
                    }
                    //log("Folded into the left, forming:\n",ctx.left->to_string(1));
                    //a_parse_expression(ctx,0,nullptr);
                } else {
                    //log("Not an identifier, so no folding");
                    ctx.node = ctx.left;
                }
                //log("Ending on: ",ctx.nodes[ctx.index]->info());
            } else {
                //log("Nothing to my left");
            }
        };


        left_binding_power.put(lparen_id, 10);
        right_binding_power.put(lparen_id, 0);
        a_handlers[lparen_id] = [](Context& ctx) {
            size_t open_id = lparen_id;
            size_t close_id = rparen_id;

            if(ctx.left && ctx.left->type == lbrace_id || ctx.left->type == lbracket_id) {
                ctx.node = nullptr;
                return;
            }

            list<g_ptr<Node>>* main_result = ctx.result;
            g_ptr<Node> result_node = nullptr;
            if(ctx.left && ctx.left->type != 0 && ctx.left->type != open_id) {
                result_node = ctx.left;
                if(!ctx.left->children.empty())
                    ctx.left = ctx.left->children.last();
                ctx.result = &ctx.left->children;
            } else {
                result_node = make<Node>();
                ctx.result = &result_node->children;
            }
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != close_id) {
                g_ptr<Node> inner = a_parse_expression(ctx, 0);
                if(inner && !discard_types.has(inner->getType()))
                    ctx.result->push(inner);
            }
            ctx.result = main_result;
            if(result_node && discard_types.has(result_node->getType()) && !result_node->children.empty()) {
                g_ptr<Node> to_promote = result_node->children.take(0);
                to_promote->children << result_node->children;
                result_node = to_promote;
            }
            ctx.node = result_node;
            ctx.index++;

            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() == identifier_id) {
                ctx.node->children << ctx.nodes[ctx.index];
                ctx.index++;
            }
        };
      

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
            if(c=='"'||c=='\'') {
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
        tokenizer_functions.put('\'',tokenizer_functions.get('"'));
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
        };

        x_handlers[dash_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                -
                *(int*)ctx.node->right()->value->data
            );
        };

        x_handlers[rangle_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                >
                *(int*)ctx.node->right()->value->data
            );
        };

        x_handlers[langle_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                <
                *(int*)ctx.node->right()->value->data
            );
        };
            
        t_handlers[rangle_id] = [](Context& ctx){

            // if(ctx.node->children.length() == 2) {
            //     g_ptr<Node> left = ctx.node->children[0];
            //     g_ptr<Node> right = ctx.node->children[1];
            //     if(left->value_is_valid() && right->value_is_valid()) {
            //         // Normal binary operator path
            //         parse_sub_nodes(ctx);
            //         ctx.node->value->copy(left->value);
            //         return;
            //     }
            // }

            g_ptr<Node> tail = ctx.node->children.last();
            while(!tail->children.empty()) {
                tail = tail->children.last();
            }
            if(tail->name==","||tail->name==";") { //If we have a comma or other delimiter as our last
                ctx.node->type=langle_id;
                process_node(ctx, ctx.node);
                return;
            }

            list<g_ptr<Node>> stream;
            std::function<void(g_ptr<Node>)> flatten = [&](g_ptr<Node> node) {
                if(node->type != langle_id && node->type != rangle_id) {
                    stream << node;
                }
                for(auto c : node->children) {
                    flatten(c);
                }
            };
            flatten(ctx.node);
            //The nodes get flattened into a stream so we can process by airity

            print("INIT STREAM");
            for(auto n : stream) {
                print("   ",n->to_string(2));
            }

            bool is_func_decl = false;

            for(auto n : stream) {
                if(n->value->type==template_id) is_func_decl = true;
                process_node(ctx, n);
                if(n->value&&n->value->type==0) {
                    continue;
                }
                if(is_func_decl||n->value->quals.empty()) {
                    ctx.node->value->quals << n->value->to_qual();
                } else {
                    ctx.node->value->quals << n->value->quals;
                }
            }

            g_ptr<Node> namenode = stream.last();
            if(stream.first()->type==func_call_id) {
                namenode = stream.first();
            }
            ctx.node->name = namenode->name;
            ctx.node->children.clear();
            print("STREAM OF ",ctx.node->name,": ");
            for(auto n : stream) {
                print("   ",n->to_string(2));
            }

            if(is_func_decl) {
                ctx.node->type = identifier_id;
                list<Qual> qual_daycare = ctx.node->value->quals;
                standard_process(ctx);
                ctx.node->value->quals << qual_daycare;

                ctx.node->in_scope->scopes.erase(ctx.node->scope());
            } else {
                fire_quals(ctx,ctx.node->value);
                ctx.node->type = var_decl_id;
                if(ctx.node->in_scope->type==type_scope_id) {
                    ctx.node->in_scope->value_table.put(ctx.node->name, ctx.node->value);
                } else {
                    ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                }
                ctx.node->value->sub_type = 0;
                



                print("FORM:\n",ctx.node->to_string(1));
            }
        };



        t_handlers[langle_id] = [](Context& ctx){
            list<g_ptr<Node>> stream;
            std::function<void(g_ptr<Node>)> flatten = [&](g_ptr<Node> node) {
                if(node->type != langle_id && node->type != rangle_id) {
                    stream << node;
                }
                for(auto c : node->children) {
                    flatten(c);
                }

            };
            flatten(ctx.node);
            
            //This is throwing the langle to the reciving rangle in cases where we're a map split by commas that needs to be rejoined
            int found_at = ctx.node->in_scope->children.find(ctx.node);
            list<g_ptr<Node>>* loc = &ctx.node->in_scope->children;
            if(found_at==-1) { //Search both the node tree and owner, because we may be args
                loc =  &ctx.node->in_scope->owner->children;
                if(loc) {
                    found_at = loc->find(ctx.node);
                }
            }

            if(!loc) {
                if(found_at!=-1) {
                    //If we can be found, throw ourselves right until we hit a rangle with a valid tail-name
                    while(loc->length()>found_at+1) {
                        g_ptr<Node> on = loc->get(found_at+1);
                        // print("LOOKING AT:\n",on->to_string(1));
                        if(on->type==rangle_id) {  //Need to check if the tail has a valid name or just picked up a comma
                            g_ptr<Node> tail = on->children.last();
                            while(!tail->children.empty()) {
                                tail = tail->children.last();
                            }
                            if(tail->type==identifier_id) { //We've found our catcher!
                                for(auto n : stream) {
                                    process_node(ctx, n);
                                    if(n->value->type==0) continue;
                                    on->value->quals << n->value->to_qual();

                                }
                                loc->erase(ctx.node);
                                process_node(ctx, on);
                                break;
                            }
                        } else {
                            flatten(on); //Just add it to our stream and keep moving
                            loc->erase(on);
                        }
                    }
                }
            }

            //Remember to restore normal functionaliy!
        };


       
        
        t_handlers[equals_id] = [](Context& ctx){standard_sub_process(ctx);}; //So we don't turn things into declerations
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
            //print("Assinging from:\n",ctx.node->right()->to_string(1),"\nto\n",ctx.node->left()->to_string(1));
            memcpy(ctx.node->left()->value->data, ctx.node->right()->value->data, ctx.node->right()->value->size);
            if(!ctx.node->right()->value->sub_values.empty()) {
                ctx.node->left()->value->sub_values = ctx.node->right()->value->sub_values;
            }
            //print("Assignment finished, value is: ",ctx.node->left()->value->info());
        };


        r_handlers[func_call_id] = [](Context& ctx) {
            if(ctx.node->scope()) {
                for(int i = 0; i < ctx.node->children.size(); i++) {
                    g_ptr<Node> arg = process_node(ctx, ctx.node->children[i]);
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
            standard_travel_pass(ctx.node->scope());
        };


        x_handlers[star_id] = [](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                *
                *(int*)ctx.node->right()->value->data
            );
        };

        d_handlers[to_decl_id(star_id)] = [](Context& ctx) {
            ctx.node->value->size = 8;
        };

        x_handlers[to_decl_id(star_id)] = [](Context& ctx) {
            if(!ctx.node->value->data) {
                ctx.node->value->data = malloc(8);
            }
        };
        
        x_handlers[to_unary_id(star_id)] = [](Context& ctx) {
            process_node(ctx, ctx.node->left());
            ctx.node->value->data = *(void**)ctx.node->left()->value->data;
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };
        
        x_handlers[to_unary_id(amp_id)] = [](Context& ctx) {
            process_node(ctx, ctx.node->left());
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = 8;
            ctx.node->value->set<void*>(ctx.node->left()->value->data);
        };


        d_handlers[to_unary_id(star_id)] = [](Context& ctx) {
            discover_symbol(ctx.node->left(),ctx.root);
            ctx.node->value->copy(ctx.node->left()->value);
        };

        t_handlers[dot_id] = [](Context& ctx) {
            g_ptr<Node> left = ctx.node->children[0];
            g_ptr<Node> right = ctx.node->children[1];
            process_node(ctx, left);
            // log("Giving at:\n",left->to_string(1),"\nto\n",right->to_string(1));

            if(left->value->type_scope) {
                right->in_scope = left->value->type_scope;
            } else  {
                //log(red("NO SCOPE SEEN!"));
            }

            if(right) {
                process_node(ctx, right);

                //The &this that gets passed to object calls.
                //we inject it fully formed here because right won't parse it's children again, it already has (has no scope)
                if(right->type == func_call_id) {
                    g_ptr<Node> amp = make<Node>();
                    amp->type = to_unary_id(amp_id);
                    amp->value->copy(left->value);
                    amp->children << left;
                    amp->children[0] = left;
                    right->children.insert(amp, 0);
                }

                ctx.node->value->copy(right->value);
            }
        };

        x_handlers[dot_id] = [](Context& ctx) {
            process_node(ctx, ctx.node->left());
            process_node(ctx, ctx.node->right(),ctx.node->left());
            // ctx.node->value->type = ctx.node->right()->value->type;
            // ctx.node->value->size = ctx.node->right()->value->size;
            ctx.node->value->copy(ctx.node->right()->value);

            if(ctx.node->left()->has_qual(sobj_qual)) {
                g_ptr<Type> store = ctx.node->left()->value->store;
                int tid = ctx.node->left()->value->address;
                std::string field = ctx.node->right()->name;
                _note& note = store->get_note(field);
                ctx.node->value->data = store->get(note.index,tid,note.size);
                ctx.node->value->size = note.size;
                if(note.size==4) ctx.node->value->type = int_id;
                if(note.size==24) ctx.node->value->type = string_id;
            } else if(ctx.node->right()->type == func_call_id || ctx.node->right()->has_qual(builtin_func_qual)) {
                ctx.node->value->data = ctx.node->right()->value->data;
            } else if(ctx.node->left()->type!=to_unary_id(star_id)&&ctx.node->left()->has_qual(to_decl_id(star_id))) {
                ctx.node->value->data = (char*)(*(void**)ctx.node->left()->value->data) + ctx.node->right()->value->address;
            } else {
                ctx.node->value->data = (char*)ctx.node->left()->value->data + ctx.node->right()->value->address;
            }
        };


        
        a_handlers[return_id] = [](Context& ctx) {
            size_t open_id = return_id;
            size_t close_id = end_id;
            list<g_ptr<Node>>* main_result = ctx.result;
            g_ptr<Node> result_node = nullptr;
            if(ctx.left && ctx.left->type != 0) {
                result_node = ctx.left;
                if(!ctx.left->children.empty())
                    ctx.left = ctx.left->children.last();
                ctx.result = &ctx.left->children;
            } else {
                result_node = make<Node>();
                ctx.result = &result_node->children;
            }
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != close_id) {
                g_ptr<Node> inner = a_parse_expression(ctx, 0);
                if(inner && !discard_types.has(inner->getType()))
                    ctx.result->push(inner);
            }
            ctx.result = main_result;
            ctx.node = result_node;
            ctx.index++;
        };
        x_handlers[return_id] = [](Context& ctx) {
            process_node(ctx, ctx.node->left());
            g_ptr<Node> on_scope = ctx.node->in_scope;
            while(on_scope->owner->type!=func_decl_id) {
                if(on_scope->owner) {
                    on_scope = on_scope->owner->in_scope;
                } else {
                    on_scope = ctx.node->in_scope;
                    break;
                }
            }
            on_scope->owner->value->copy(ctx.node->left()->value);
            ctx.flag = true;
        };

        x_handlers[break_id] = [](Context& ctx) {
            ctx.flag = true;
        };

        right_binding_power.put(colon_id, 9);
        //left_binding_power.put(lbracket_id, 10);
        right_binding_power.put(lbracket_id, 10);
        left_binding_power.put(lbracket_id, -1);
        a_handlers[lbracket_id] = [](Context& ctx) {
            g_ptr<Node> result_node = make<Node>();
            result_node->type = lbracket_id;
            
            if(ctx.left && ctx.left->type != 0) {
                result_node = ctx.left;
            }
            
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != rbracket_id) {
                g_ptr<Node> expr = a_parse_expression(ctx, 0);
                if(expr) result_node->children << expr;
            }
            ctx.index++; // consume ']'
            
            ctx.node = result_node;
        };
        t_handlers[lbracket_id] = [](Context& ctx) {
            standard_sub_process(ctx);
            auto& children = ctx.node->children;
            
            if(children[0]->type == var_decl_id) {
                //Declaration: int a[3]
                ctx.node->type = var_decl_id;
                ctx.node->name = children[0]->name;
                ctx.node->value = children[0]->value;
                ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                ctx.node->value->sub_type = 0;
                g_ptr<Node> size_expr = children[1];
                ctx.node->children.clear();
                ctx.node->children << size_expr;
            } else {
                //Just an access case
            }
        };

        t_handlers[var_decl_id] = [](Context& ctx) {
            //Do nothing
        };

        r_handlers[var_decl_id] = [](Context& ctx) {
            fire_quals(ctx,ctx.node->value);
            standard_sub_process(ctx);
        };

        x_handlers[var_decl_id] = [](Context& ctx) {
            if(ctx.node->has_qual(sobj_qual)) {
                g_ptr<Object> sobj = ctx.node->value->store->create();
                ctx.node->value->address = sobj->TID;
            }
            else if(!ctx.node->value->data) {
                size_t alloc_size = ctx.node->value->size;
                if(alloc_size == 0 && ctx.node->value->type_scope) {
                    alloc_size = ctx.node->value->type_scope->owner->value->size;
                    ctx.node->value->size = alloc_size;
                }
                if(!ctx.node->children.empty()) { //Arrays
                    process_node(ctx, ctx.node->children[0]);
                    int count = ctx.node->children[0]->value->get<int>();
                    alloc_size *= count;
                }
                ctx.node->value->data = malloc(alloc_size);
            }
        };

        d_handlers[type_decl_id] = [](Context& ctx){
            g_ptr<Node> node  = ctx.node;
            for(auto child : node->scope()->children) {
                child->value->address = node->value->size;
                if(child->type == var_decl_id) {
                    if(child->value->type_scope) {
                        node->value->size+=child->value->type_scope->owner->value->size; //Note to self: too indirect, should make this path more direct at some point
                    } else {
                        node->value->size+=child->value->size;
                    }
                } else if(child->type == to_decl_id(star_id)) {
                    node->value->size+=8;
                }
            }
        };
        x_handlers[type_decl_id] = [](Context& ctx){};
        t_handlers[func_decl_id] = [](Context& ctx) {
            g_ptr<Node> saved_root = ctx.root;
            ctx.root = ctx.node->scope();
            for(auto c : ctx.node->children) {
                g_ptr<Node> child = process_node(ctx, c);
            }
            ctx.root = saved_root;
        };
        x_handlers[func_decl_id] = [](Context& ctx){};
        
        scope_link_handlers.put(identifier_id,[](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
            new_scope->owner = owner_node.getPtr();
            owner_node->scopes << new_scope.getPtr();
            for(auto c : owner_node->children) {
                c->place_in_scope(new_scope.getPtr());
            }
            new_scope->name = owner_node->name;
        });
        scope_link_handlers.put(rangle_id,scope_link_handlers.get(identifier_id));

        t_handlers[identifier_id] = [](Context& ctx) {
            log("Parsing an idenitifer:\n",ctx.node->to_string(1));
            g_ptr<Node> node = ctx.node;
            g_ptr<Value> decl_value = make<Value>();
            
            bool had_a_value = (bool)(node->value);
            bool had_a_scope = (bool)(node->scope());
            bool found_a_value = node->find_value_in_scope();
            bool is_qualifier = node->value_is_valid();

            int root_idx = -1;
            if(node->value_is_valid() && node->value->sub_type != 0) {
                //log("I am a qualifier");
                decl_value->quals << node->value->to_qual();
                for(auto& q : node->value->quals) {
                    if(!decl_value->has_qual(q.type)) {
                        decl_value->quals << q;
                    }
                }
                if(node->value->store) {
                    decl_value->store = node->value->store;
                }
                for(int i = 0; i < node->children.length(); i++) {
                    g_ptr<Node> c = node->children[i];
                    c->find_value_in_scope();
                    if(c->value_is_valid()) {
                        decl_value->quals << c->value->to_qual();
                        for(auto& q : c->value->quals) {
                            if(!decl_value->has_qual(q.type)) {
                                decl_value->quals << q;
                            }
                        }
                        if(c->value->store) {
                            decl_value->store = c->value->store;
                        }
                    } else {
                        root_idx = i;
                        break;
                    }
                }
                if(root_idx!=-1) {
                    g_ptr<Node> root = node->children[root_idx];
                    node->name = root->name;
                    for(int i = root_idx+1; i < node->children.length(); i++) {
                        g_ptr<Node> c = node->children[i];
                        c->find_value_in_scope();
                        if(c->value_is_valid()) {
                            node->quals << c->value->to_qual();
                        } 
                    }
                    node->children = node->children.take(root_idx)->children;
                }
            } else {
                //log("no valid value, I am the root");
            }

            if(!node->scope()) { //Defer, the r_stage will do this later for scoped nodes
                standard_sub_process(ctx);
            }

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
                if(has_sub_type) {
                    node->type = func_decl_id;
                    node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
                    node->value->type_scope = node->scope().getPtr();
                    node->value = node->in_scope->distribute_value(node->name,node->value);
                    node->value->sub_type = 0;

                    if(node->in_scope->type==type_scope_id) {
                        node->scope()->type = method_scope_id;
                    }

                    //The 'this' field passed to a func_decl with members
                    if(node->in_scope->value_table.hasKey(node->in_scope->name)) {
                        inject_this_param(ctx);
                    }
                } else {
                    node->type = type_decl_id;
                    node->value->copy(make_type(node->name,0));
                    node->value->type_scope = node->scope().getPtr();
                    node->value = node->in_scope->distribute_value(node->name,node->value);

                    node->scope()->type = type_scope_id;
                }
            } else {
                has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node->type = var_decl_id;
                    if(node->in_scope->type==type_scope_id) {
                        node->in_scope->value_table.put(node->name, decl_value);
                    } else {
                        node->value = node->in_scope->distribute_value(node->name, decl_value);
                    }
                    node->value->sub_type = 0;
                } else if(has_scope) {
                    node->type = func_call_id;
                    node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(node->value->type_scope)
                        node->scopes[0] = node->value->type_scope; //Swap to the type scope
                } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
                    node->find_value_in_scope();
                } else {                                         
                    if(node->in_scope->value_table.hasKey("this")) { //The has check is so we don't inject this on the names of declared variables at the top
                        if(node->in_scope->owner->type==func_decl_id&&!node->in_scope->children.has(node)) {

                        } else {
                            inject_member_access(ctx);
                        }
                    } else {
                        //HM Tracing goes here.
                        //Attatch a qual with handlers for it
                        //Borrow checker too, maybe not here though.
                    }
                }
            }

            log("Returning:\n",node->to_string(1));
        };

        x_handlers[identifier_id] = [](Context& ctx){};

        scope_precedence.put(lbrace_id, 10);
        scope_precedence.put(rbrace_id, -10);

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
                ctx.node->name+=c;
            }
        };


        // tokenizer_functions.put('[',[](Context& ctx){
        //     int l_ind = ctx.index;
        //     std::string opener = "";
        //     for(int i=l_ind;i<ctx.source.length();i++) {
        //         char c = ctx.source.at(l_ind);
        //         if(char_is_split.getOrDefault(c,false)) continue;
        //         opener+=c;
        //     }
        // });

        init_handlers(s_handlers,[](Context& ctx) {
            //Do nothing
        });

        init_handlers(t_handlers,[](Context& ctx) {
            standard_sub_process(ctx);
        });

        init_handlers(r_handlers,[](Context& ctx) {
            standard_sub_process(ctx);
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
                process_node(ctx, r);
                if(r->has_qual(sobj_qual)) {
                    toPrint.append(r->value->store->type_to_string(4));
                } else {
                    toPrint.append(r->value->to_string());
                }
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


        left_binding_power.put(and_id, 2);
        right_binding_power.put(and_id, 0);
        left_binding_power.put(or_id, 2);  
        right_binding_power.put(or_id, 0);


        map<std::string,g_ptr<Type>> soql_stores;
        
        g_ptr<Type> s1 = make<Type>();
        s1->note_value<std::string>("Id");
        s1->note_value<std::string>("Name");
        s1->note_value<std::string>("Type");
        s1->note_value<double>("AnnualRevenue");
        s1->note_value<int>("Val");
        s1->note_value<std::string>("Industry");
        s1->note_value<bool>("IsDeleted");
        soql_stores.put("Account", s1);
        g_ptr<Value> s1_val = make_type("Account",4); //Size 4 because we store tids
        s1_val->quals << Qual(sobj_qual);
        s1_val->store = s1;
        s1->type_name = "Account";
        keywords.put("Account",s1_val);




        r_handlers[to_prefix_id(list_id)] = [](Context& ctx) {
            int found_at = ctx.value->find_qual(list_id);
            if(found_at!=-1) {
                for(int i=found_at+1;i<ctx.value->quals.length();i++) {
                    Qual& q = ctx.value->quals[i];
                    if(q.value->size>0) {
                        if(!ctx.value->store) {
                            ctx.value->store = make<Type>();
                        }
                        ctx.value->store->add_column(q.value->size);
                    }
                }
            }
        };
        r_handlers[to_prefix_id(map_id)] = [](Context& ctx) {
            int found_at = ctx.value->find_qual(map_id);
            if(found_at!=-1) {
                for(int i=found_at+1;i<ctx.value->quals.length();i++) {
                    Qual& q = ctx.value->quals[i];
                    if(q.value->size>0) {
                        if(!ctx.value->store) {
                            ctx.value->store = make<Type>();
                        }
                        ctx.value->store->add_column(q.value->size);
                    }
                }
            }
        };
        r_handlers[to_prefix_id(set_id)] = [](Context& ctx) {

        };

        push_id = add_function("push",[](Context& ctx) {
            list<g_ptr<Node>>& children = ctx.node->children;
            standard_sub_process(ctx);
            if(ctx.left) {
                g_ptr<Type> store = ctx.left->value->store;
                if(ctx.left->value->type == list_id) {
                    store->push(children[0]->value->data, children[0]->value->size);
                } else if (ctx.left->value->type == map_id) {

                } else if (ctx.left->value->type == set_id) {
                    
                }
            }
        });
        get_id = add_function("get",[](Context& ctx) {
            list<g_ptr<Node>>& children = ctx.node->children;
            standard_sub_process(ctx);
            if(ctx.left) {
                g_ptr<Type> store = ctx.left->value->store;
                if(ctx.left->value->type == list_id) {
                    if(!ctx.left->value->sub_values.empty()) {
                        g_ptr<Value> result = ctx.left->value->sub_values.get(children[0]->value->get<int>());
                        ctx.node->value->copy(result);
                    } else {
                        ctx.node->value->data = store->get(children[0]->value->get<int>(), 0, store->array[0].size); //Need to find the real size
                        ctx.node->value->type = int_id;
                    }
                } else if (ctx.left->value->type == map_id) {
                    std::string key = ctx.node->children[0]->value->to_string();
                    ctx.node->value->data = std::any_cast<void*>(store->fallback_map.get(key));
                    ctx.node->value->type = int_id;
                } else if (ctx.left->value->type == set_id) {

                }
            }

        });
        put_id = add_function("put",[](Context& ctx) {
            if(ctx.left) {
                g_ptr<Type> store = ctx.left->value->store;
                if(ctx.left->value->type == list_id) {

                } else if (ctx.left->value->type == map_id) {
                    std::string key = ctx.node->children[0]->value->to_string();
                    store->fallback_map.put(key, std::any(ctx.node->children[1]->value->data));
                } else if (ctx.left->value->type == set_id) {
                    
                }
            }

        });








        if_id = add_scoped_keyword("if", 2, [](Context& ctx) {
            process_node(ctx, ctx.node->left());
            if(ctx.node->left()->value->is_true()) {
                ctx.flag = standard_travel_pass(ctx.node->scope());
            }
            else if(ctx.node->right()) {
                ctx.flag = standard_travel_pass(ctx.node->right()->scope());
            }
        });

        else_id = add_scoped_keyword("else", 1, [](Context& ctx){
            //This doesn't ever really execute
        });
        t_handlers[else_id] = [](Context& ctx) {
            int my_id = ctx.root->children.find(ctx.node);
            if(my_id>0) {
                g_ptr<Node> left = ctx.root->children[my_id-1];
                if(left->type==if_id) {
                    left->children << ctx.root->children.take(my_id);
                    left->children[1] = ctx.node;
                }
            }
            ctx.node = nullptr;
        };

        while_id = add_scoped_keyword("while", 2, [](Context& ctx) {
            while(true) {
                process_node(ctx, ctx.node->left());
                if(!ctx.node->left()->value->is_true()) break;
                if(standard_travel_pass(ctx.node->scope())) {
                    ctx.flag = true;
                    break;
                } 
            }
        });

        for_id = add_scoped_keyword("for", 2, [](Context& ctx) {
            process_node(ctx, ctx.node->children[0]); //Run var decl;
            while(true) {
                process_node(ctx, ctx.node->children[1]); //Condition check
                if(!ctx.node->children[1]->value->is_true()) break;
                if(standard_travel_pass(ctx.node->scope())) {
                    ctx.flag = true;
                    break;
                }
                process_node(ctx, ctx.node->children[2]); //Incrementer 
            }
        });
        


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
        for(auto n : nodes) {
            print(n->to_string(1));
        }
        
        print("S STAGE");
        start_stage(s_handlers);
        g_ptr<Node> root = parse_scope(nodes);

        //print(root->to_string(0,0,true));

        print("T STAGE");
        start_stage(t_handlers);
        standard_resolving_pass(root);

        // print("==LOG==");
        // span->print_all();

        //log(root->to_string(0,0,true));


        newline("Discovering symbols");
        print("D STAGE");
        start_stage(d_handlers);
        discover_symbols(root);
        endline();
        print("R STAGE");
        start_stage(r_handlers);
        standard_resolving_pass(root);

        // print("==LOG==");
        // span->print_all();
        // print(root->to_string());
        // print_scopes(root);

        // print("E STAGE");
        // start_e_stage(root);

        

        std::string final_time = ftime(timer.end());

        print("==LOG==");
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
        
        // print("==LOG==");
        // span->print_all();
        // print(root->to_string());
        // print_scopes(root);
        // print("==DONE==");
        // return;
        return;
    }

}

        //-> for sugar
        // t_handlers[dash_id] = [](Context& ctx) {
        //     if(ctx.node->right() && ctx.node->right()->type == to_unary_id(rangle_id)) {
        //         ctx.node->type = dot_id;
        //         g_ptr<Node> star = make<Node>();
        //         star->type = to_unary_id(star_id);
        //         star->children << ctx.node->left();
        //         star->children[0] = ctx.node->left();
        //         ctx.node->children[0] = star;
        //         ctx.node->children[1] = ctx.node->right()->left();
        //         print("Assembled node: ",ctx.node->to_string());
        //     } else {
        //         resolve_sub_nodes(ctx);
        //     }
        // };