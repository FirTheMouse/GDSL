#pragma once

#include "../core/GDSL.hpp"
#include "../modules/Q-AST.hpp"
#include "../modules/Q-Arm64.hpp"
#include "../modules/Q-Tokenizer.hpp"

//A subset of C, the ctest.gld file shows of most of what this can do.
//I'm activly expanding this, generics are next on the roadmap and one day I hope to bootstrap GDSL itself with this.

namespace GDSL {

struct C_Compiler : public AST_Unit, public Tokenizer_Unit, public ARM64_Unit {
    map<uint32_t,int> left_binding_power;
    map<uint32_t,int> right_binding_power;

    map<uint32_t, std::function<void(g_ptr<Node>, g_ptr<Node>, g_ptr<Node>)>> scope_link_handlers;
    map<uint32_t, int> scope_precedence;

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

    size_t scope_id = reg_id("SCOPE");

    g_ptr<Node> parse_scope(list<g_ptr<Node>> nodes) {
        g_ptr<Node> root_scope = make<Node>();
        root_scope->name = "GLOBAL";
        root_scope->type = scope_id;
        root_scope->is_scope = true;
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
                current_scope->type = scope_id;
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

    size_t add_scoped_keyword(const std::string& name, int scope_prec)
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
        return id;
    }



    size_t var_decl_id = reg_id("VAR_DECL");
    size_t to_decl_id(size_t id) {return id+1;}
    size_t to_unary_id(size_t id) {return id+2;}

    size_t add_binary_operator(char c, const std::string& f,int left_bp, int right_bp) {
        size_t id = add_token(c,f);
        left_binding_power.put(id, left_bp);
        right_binding_power.put(id, right_bp);

        size_t decl_id = reg_id(f+"_decl");
        size_t unary_id = reg_id(f+"_unary");

        t_handlers[id] = [this,decl_id,unary_id,c](Context& ctx){
            auto& children = ctx.node->children;
            standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
            if(children.length() == 2) {
                g_ptr<Node> type_term = children[0];
                g_ptr<Node> id_term = children[1];

                ctx.node->name = type_term->name+c+id_term->name;
                
                if(type_term->type==var_decl_id) {
                    ctx.node->type = decl_id;
                    ctx.node->value = type_term->value;
                    ctx.node->name = id_term->name;
                    ctx.node->value->sub_type = 0;
                    ctx.node->value = ctx.node->in_scope->distribute_value(ctx.node->name, ctx.node->value);
                    ctx.node->children.clear();

                    g_ptr<Node> marker = make<Node>(); //Make a muted qual marker
                    marker->type = decl_id;
                    marker->mute = true;
                    ctx.node->value->quals << marker;

                } else {
                    ctx.node->value->copy(type_term->value);
                }
            } else if(children.length() == 1) {
                g_ptr<Node> type_term = children[0];
                ctx.node->name = c+type_term->name;
                ctx.node->type = unary_id;
                ctx.node->value->copy(type_term->value);
            } 
        };
        
        return id;
    }

    //To prove a point about MLIR's tutourial example
    std::function<void(Context&)> make_involution(size_t involute_id){
        return [involute_id](Context& ctx){
            if(ctx.node->children[0]->type == involute_id) {
                ctx.node->copy(ctx.node->children[0]->children[0]);
            }
        };
    };

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
        bool has_func = a_handlers.hasKey(type);

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

        bool fallthrough = false;

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
            if(!ctx.node) {
                fallthrough = true;
            } else {
                left_node = ctx.node; //Read result back
            }
        }
        
        if(fallthrough || (!has_func && right_bp != -1)) { //Prefixual unary: recurse with right
            g_ptr<Node> right_node = a_parse_expression(
                ctx,
                right_bp,
                nullptr
            );
            left_node->type = type;
            if(right_node)
                left_node->children << right_node;
        }
        else if(!has_func) { //Else atom, like a literal or idenitifer
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

            fallthrough = false;
            bool op_has_func = a_handlers.hasKey(op->getType());

            if(op_has_func) {
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
                    if(ctx.flag) {
                        ctx.flag = false;
                        fallthrough = true;
                    } else {
                        ctx.index--;
                        break;
                    }
                }
            } 
            
            if(!op_has_func || fallthrough) {
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
    size_t colon_id = add_token(':',"COLON");
    size_t end_id = add_token(';',"END");
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
    size_t func_call_id = reg_id("FUNC_CALL");
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
    size_t randi_id = 0;

    size_t wubless_qual = add_qualifier("wubless");
    size_t wubfull_qual = add_qualifier("wubfull");
    size_t const_qual = add_qualifier("const");
    size_t static_qual = add_qualifier("static");
    size_t inline_qual = add_qualifier("inline");
    size_t struct_qual = add_qualifier("struct");
    size_t type_qual = add_qualifier("type");

    size_t template_id = make_tokenized_keyword("template");
    size_t typename_id = add_type("typename");

    size_t live_qual = reg_id("LIVE");
    size_t ptr_qual = reg_id("POINTER");
    size_t paren_qual = reg_id("PAREN");


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

    Log::Line timer;
    g_ptr<Log::Span> span2;
    IdPool reg_pool;

    void init() override {
        span = make<Log::Span>();
        
        discard_types.push(undefined_id);
        discard_types.push(end_id);
        discard_types.push(lparen_id);
        discard_types.push(rparen_id);
        discard_types.push(comma_id);

        value_to_string.put(object_id, [](void* data) {
            return std::string("[object @") + std::to_string((size_t)data) + "]";
        });
        x_handlers[literal_id] = [](Context& ctx){};

        a_handlers[identifier_id] = [this](Context& ctx){
            if(ctx.index>=ctx.nodes.length()) {
                log("Hey! Index overun by ",node_info(ctx.left));
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


        t_handlers[to_prefix_id(typename_id)] = [](Context& ctx){
            ctx.value->sub_type = ctx.qual->type;
            ctx.value->type = ctx.qual->type;
            ctx.value->size = ctx.qual->value->size;
            if(ctx.qual->value->type_scope)
                ctx.value->type_scope = ctx.qual->value->type_scope;
        };



        left_binding_power.put(lparen_id, 10);
        right_binding_power.put(lparen_id, 0);
        a_handlers[lparen_id] = [this](Context& ctx) {
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
        t_handlers[float_id] = [this](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(float_id,4);
            value->set<float>(std::stof(ctx.node->name));
            ctx.node->value = value;
        }; 

        value_to_string.put(int_id,[](void* data) {
            return std::to_string(*(int*)data);
        });
        negate_value.put(int_id,[](void* data) {
            *(int*)data = -(*(int*)data);
        });
        t_handlers[int_id] = [this](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(int_id,4);
            value->set<int>(std::stoi(ctx.node->name));
            ctx.node->value = value;
        }; 

        value_to_string.put(bool_id,[](void* data){
            return (*(bool*)data) ? "TRUE" : "FALSE";
        });
        t_handlers[bool_id] = [this](Context& ctx) {
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
        tokenizer_functions.put('"',[this](Context& ctx) {
            ctx.state = in_string_id;
            ctx.node = make<Node>();
            ctx.node->type = string_id;
            ctx.node->name = "";
            ctx.result->push(ctx.node);
        });
        value_to_string.put(string_id,[this](void* data){
            return *(std::string*)data;
        });
        t_handlers[string_id] = [this](Context& ctx) {
            ctx.node->type = literal_id;

            g_ptr<Value> value = make<Value>(string_id,24);
            value->set<std::string>(ctx.node->name);
            ctx.node->value = value;
        }; 
           
        x_handlers[plus_id] = [this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                +
                *(int*)ctx.node->right()->value->data
            );
        };

        x_handlers[dash_id] = [this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<int>(
                *(int*)ctx.node->left()->value->data
                -
                *(int*)ctx.node->right()->value->data
            );
        };

        x_handlers[rangle_id] = [this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                >
                *(int*)ctx.node->right()->value->data
            );
        };

        x_handlers[langle_id] = [this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node->value->set<bool>(
                *(int*)ctx.node->left()->value->data
                <
                *(int*)ctx.node->right()->value->data
            );
        };


        a_handlers[langle_id] = [this](Context& ctx) {
            int i = ctx.index;
            while(i<ctx.nodes.length()) {
                if(ctx.nodes[i]->type==end_id||ctx.nodes[i]->type==lbrace_id) {
                    break;
                } 
                else if(ctx.nodes[i]->type==rangle_id) {
                    ctx.nodes[ctx.index-1]->type = lparen_id;
                    ctx.nodes[i]->type = rparen_id;
                    ctx.index--;
                    return;
                }
                i++;
            }
            ctx.node = nullptr;
            ctx.flag = true; //A stage uses it's flag for this
        };

        //Add this one day so we can have C++ style templates if we want
        // t_handlers[template_id] = [](Context& ctx) {

        // };
        
        t_handlers[equals_id] = [this](Context& ctx){ //So we don't turn things into declerations
            standard_sub_process(ctx);
            ctx.node->name = ctx.node->left()->name+"="+ctx.node->right()->name;
        };
        e_handlers[equals_id] = [this](Context& ctx){ 
            if(ctx.node->left()&&ctx.node->right()) {
                int at_id = ctx.node->left()->value->find_qual(live_qual);
                if(at_id!=-1) {
                    ctx.node->right()->distribute_qual_to_values(ctx.node->left()->value->quals[at_id],live_qual);
                } else {
                    ctx.node = nullptr;
                }
            }
        };
        x_handlers[equals_id] = [this](Context& ctx) {
            standard_sub_process(ctx);
            //print("Assinging from:\n",ctx.node->right()->to_string(1),"\nto\n",ctx.node->left()->to_string(1));
            memcpy(ctx.node->left()->value->data, ctx.node->right()->value->data, ctx.node->right()->value->size);
            //print("Assignment finished, value is: ",ctx.node->left()->value->info());
        };


        r_handlers[func_call_id] = [this](Context& ctx) {
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
        x_handlers[func_call_id] = [this](Context& ctx) {
            standard_sub_process(ctx);
            standard_travel_pass(ctx.node->scope());
        };


        x_handlers[star_id] = [this](Context& ctx){
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
        
        x_handlers[to_unary_id(star_id)] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            ctx.node->value->data = *(void**)ctx.node->left()->value->data;
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = ctx.node->left()->value->size;
        };
        
        x_handlers[to_unary_id(amp_id)] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            ctx.node->value->type = ctx.node->left()->value->type;
            ctx.node->value->size = 8;
            ctx.node->value->set<void*>(ctx.node->left()->value->data);
        };


        d_handlers[to_unary_id(star_id)] = [this](Context& ctx) {
            discover_symbol(ctx.node->left(),ctx.root);
            ctx.node->value->copy(ctx.node->left()->value);
        };

        t_handlers[dot_id] = [this](Context& ctx) {
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

        x_handlers[dot_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            process_node(ctx, ctx.node->right());
            ctx.node->value->type = ctx.node->right()->value->type;
            ctx.node->value->size = ctx.node->right()->value->size;

            if(ctx.node->right()->type == func_call_id) {
                ctx.node->value->data = ctx.node->right()->value->data;
            } else if(ctx.node->left()->type!=to_unary_id(star_id)&&ctx.node->left()->has_qual(to_decl_id(star_id))) {
                ctx.node->value->data = (char*)(*(void**)ctx.node->left()->value->data) + ctx.node->right()->value->address;
            } else {
                ctx.node->value->data = (char*)ctx.node->left()->value->data + ctx.node->right()->value->address;
            }
        };


        
        a_handlers[return_id] = [this](Context& ctx) {
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
        x_handlers[return_id] = [this](Context& ctx) {
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


        left_binding_power.put(lbracket_id, 10);
        a_handlers[lbracket_id] = [this](Context& ctx) {
            g_ptr<Node> result_node = make<Node>();
            result_node->type = lbracket_id;
            
            if(ctx.left && ctx.left->type != 0) {
                result_node->children << ctx.left;
            }
            
            while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != rbracket_id) {
                g_ptr<Node> expr = a_parse_expression(ctx, 0);
                if(expr) result_node->children << expr;
            }
            ctx.index++;
            
            ctx.node = result_node;
        };
        t_handlers[lbracket_id] = [this](Context& ctx) {
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
        x_handlers[lbracket_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left()); //Base
            process_node(ctx, ctx.node->right()); //Index
            
            int i = ctx.node->right()->value->get<int>();
            size_t element_size = ctx.node->children[0]->value->size;
            
            ctx.node->value->type = ctx.node->children[0]->value->type;
            ctx.node->value->size = element_size;
            ctx.node->value->type_scope = ctx.node->children[0]->value->type_scope;
            ctx.node->value->data = (char*)ctx.node->children[0]->value->data + i * element_size;
        };

        t_handlers[var_decl_id] = [](Context& ctx) {
            //Do nothing
        };
        x_handlers[var_decl_id] = [this](Context& ctx) {
            if(!ctx.node->value->data) {
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

        d_handlers[type_decl_id] = [this](Context& ctx){
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
        t_handlers[func_decl_id] = [this](Context& ctx) {
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

        t_handlers[identifier_id] = [this](Context& ctx) {
            // log("Parsing an idenitifer:\n",ctx.node->to_string(1));
            g_ptr<Node> node = ctx.node;
            g_ptr<Value> decl_value = make<Value>();
            
            bool had_a_value = (bool)(node->value);
            bool had_a_scope = (bool)(node->scope());
            bool found_a_value = node->find_value_in_scope();
            bool is_qualifier = node->value_is_valid();

            int root_idx = -1;
            if(node->value_is_valid() && node->value->sub_type != 0) {
                //log("I am a qualifier");
                decl_value->quals << value_to_qual(node->value);
                for(int i = 0; i < node->children.length(); i++) {
                    g_ptr<Node> c = node->children[i];
                    c->find_value_in_scope();
                    if(c->value_is_valid()) {
                        decl_value->quals << value_to_qual(c->value);
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
                            node->quals << value_to_qual(c->value);
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
                    if(node->value->sub_type!=typename_id)
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

                    for(auto c : node->children) { //Parse inline quals for templates
                        if(c->value->type==0) {
                            g_ptr<Node> qual = make<Node>();
                            qual->type = typename_id;
                            qual->sub_type = typename_id;
                            qual->value = c->value;
                            c->value->quals << qual;
                            c->value->type = typename_id;
                            c->value->sub_type = typename_id;
                            c->in_scope = node->scope().getPtr();
                        } else if(c->value->type!=typename_id) {
                            node->quals << value_to_qual(c->value);
                            node->children.erase(c);
                        }
                    }
                }
            } else {
                has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    if(node->value->sub_type == typename_id && node->in_scope->value_table.hasKey(node->name)) {
                        //Reference to existing typename-typed variable, not a new declaration
                        node->find_value_in_scope();
                        node->type = identifier_id;
                    } else {
                        node->type = var_decl_id;
                        if(node->in_scope->type==type_scope_id) {
                            node->in_scope->value_table.put(node->name, decl_value);
                        } else {
                            node->value = node->in_scope->distribute_value(node->name, decl_value);
                        }
                    }
                    if(node->value->sub_type!=typename_id)
                        node->value->sub_type = 0;
                } else if(has_scope) {
                    node->type = func_call_id;
                    node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(node->value->type_scope)
                        node->scopes[0] = node->value->type_scope; //Swap to the type scope

                    node->name.append("(");
                    for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
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

            //log("Returning:\n",node->to_string(1));
        };

        x_handlers[identifier_id] = [](Context& ctx){};

        scope_precedence.put(lbrace_id, 10);
        scope_precedence.put(rbrace_id, -10);

        // scope_precedence.put(lbracket_id, 10);
        // scope_precedence.put(rbracket_id, -10);

        char_is_split.put(' ',true);
        left_binding_power.put(colon_id, 4);
        right_binding_power.put(colon_id, 9);
        tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
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

        tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            if(char_is_split.getOrDefault(c,false)) {
                if(c=='.') {
                    ctx.node->type = float_id;
                } else {
                    ctx.state = 0; 
                    --ctx.index;
                    return;
                }
            } else if(std::isalpha(c)) {
                ctx.state = in_alpha_id;
            }
            ctx.node->name += c;
        });

        tokenizer_default_function = [this](Context& ctx) {
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


        a_parse_function = [this](Context& ctx){
            g_ptr<Node> expr = a_parse_expression(ctx, 0);
            if(expr && !discard_types.has(expr->getType())) {
                ctx.result->push(expr);
            }
        };
        s_default_function = [](Context& ctx){};
        t_default_function = [this](Context& ctx){
            standard_sub_process(ctx);
        };
        r_default_function = [this](Context& ctx){
            standard_sub_process(ctx);
        };
        d_default_function = [this](Context& ctx){
            for(auto c : ctx.node->children) {
                discover_symbol(c,ctx.root);
            }
        };

        e_default_function=  [](Context& ctx){
            //Doing nothing for now
        };
        

        x_default_function = [](Context& ctx){};

        print_id = add_function("print");
        x_handlers[print_id] = [this](Context& ctx) {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                process_node(ctx, r);
                toPrint.append(value_as_string(r->value));
            }
            print(toPrint);
        };

        e_handlers[print_id] = [this](Context& ctx){
            for(auto c : ctx.node->children) {
                int found_at = c->value->find_qual(live_qual);
                if(found_at!=-1) {
                    ctx.node->value->sub_values.push_if_absent(c->value->quals[found_at]->value);
                } else {
                    g_ptr<Value> token = make<Value>();
                    ctx.node->value->sub_values << token;
                    g_ptr<Node> live = make<Node>();
                    live->type = live_qual;
                    live->value = token;
                    c->value->quals << live;
                }
            }
        };

        randi_id = add_function("randi");
        x_handlers[randi_id] = [this](Context& ctx){
            standard_sub_process(ctx);
            int min = ctx.node->left()->value->get<int>();
            int max = ctx.node->right()->value->get<int>();
            ctx.node->value->type = int_id;
            ctx.node->value->size = 4;
            ctx.node->value->set<int>(randi(min,max));
        };


        if_id = add_scoped_keyword("if", 2);
        x_handlers[if_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            if(ctx.node->left()->value->is_true()) {
                ctx.flag = standard_travel_pass(ctx.node->scope());
            }
            else if(ctx.node->right()) {
                ctx.flag = standard_travel_pass(ctx.node->right()->scope());
            }
        };
        e_handlers[if_id] = [](Context& ctx) {
            //Decisions can be  made here to either replace the scope with the right scope (only else is valid)
            //or to not add it (only main if is valid) depending on the provablility of branch perdiction in the future.
        };


        else_id = add_scoped_keyword("else", 1);
        t_handlers[else_id] = [this](Context& ctx) {
            int my_id = ctx.root->children.find(ctx.node);
            if(my_id>0) {
                g_ptr<Node> left = ctx.root->children[my_id-1];
                if(left->type==if_id) {
                    ctx.node->scope()->owner = left.getPtr();
                    left->scopes << ctx.node->scopes;
                    ctx.root->children.removeAt(my_id);
                }
            }
            ctx.node = nullptr;
        };

        while_id = add_scoped_keyword("while", 2);
        x_handlers[while_id] =  [this](Context& ctx) {
            while(true) {
                process_node(ctx, ctx.node->left());
                if(!ctx.node->left()->value->is_true()) break;
                if(standard_travel_pass(ctx.node->scope())) {
                    ctx.flag = true;
                    break;
                } 
            }
        };
        e_handlers[while_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            bool has_dead = false;
            for(auto c : ctx.node->left()->children) {
                if(c->type == identifier_id && !c->has_qual(live_qual)) {
                    has_dead = true;
                    break;
                }
            }
            if(has_dead) {
                ctx.root->scopes.erase(ctx.node->scope());
                ctx.node = nullptr;
            }
        };

        for_id = add_scoped_keyword("for", 2);
        x_handlers[for_id] = [this](Context& ctx) {
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
        };

        reg_pool.init({15, 14, 13, 12, 11, 10, 9}); //Because I'm on a Mac
        m_default_function = [this](Context& ctx){
            backwards_sub_process(ctx);
            // if(ctx.node->value->reg == -1) {
            //     int r = reg_pool.alloc();
            //     if(r != -1) {
            //         ctx.node->value->reg = r;
            //     } else { //Walking up and assigning a stack slot as address, the walk up is because of nesting
            //         g_ptr<Node> func = find_scope(ctx.node->in_scope, [](g_ptr<Node> n) {
            //             return n->owner && n->owner->type == func_decl_id;
            //         });
            //         if(func && func->owner) {
            //             ctx.node->value->address = func->owner->value->address;
            //             func->owner->value->address += ctx.node->value->size;
            //         }
            //     }
            // }
            if(ctx.node->value->reg == -1) {
                ctx.node->value->reg = reg_pool.alloc();
            }
            if(ctx.node->value->reg==-1 && !ctx.node->value->data) {
                ctx.node->value->data = malloc(ctx.node->value->size);
            }
        };
        m_handlers[var_decl_id] = [this](Context& ctx){
            backwards_sub_process(ctx);
            if(!ctx.flag && ctx.node->value->reg != -1) {
                reg_pool.free(ctx.node->value->reg);
            } else if(ctx.node->value->reg == -1) {
                ctx.node->value->data = malloc(ctx.node->value->size);
            }
        };
        m_handlers[equals_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            if(ctx.node->right()) {
                ctx.node->right()->value->reg = ctx.node->left()->value->reg;
                ctx.flag = true; //Blocks variable decelreations from freeing themselves
                process_node(ctx, ctx.node->right());
                ctx.flag = false;
            }
        };
        m_handlers[func_call_id] = [this](Context& ctx) {
            ctx.flag = true; //Blocks variable decelreations from freeing themselves
            backwards_sub_process(ctx);
            ctx.node->value->reg = REG_RETURN_VALUE;
            ctx.flag = false;
        };
        m_handlers[func_decl_id] = [this](Context& ctx) {
            ctx.node->value->address = 0;
            reg_pool.init({15, 14, 13, 12, 11, 10, 9});
        };


        i_default_function = [](Context& ctx){
            //Do nothing
        };

        i_handlers[literal_id] = [this](Context& ctx) {
            emit_load_literal(ctx.node->value);
        };
        i_handlers[plus_id] = [this](Context& ctx) {
            standard_sub_process(ctx);
            log(emit_buffer.length(),": add ",ctx.node->left()->value->reg," and ",ctx.node->right()->value->reg," store at ",ctx.node->value->reg);
            emit_add(ctx.node->value, ctx.node->left()->value, ctx.node->right()->value);
        };
        i_handlers[rangle_id] = [this](Context& ctx) {
            standard_sub_process(ctx);
            emit_compare(ctx.node->value, ctx.node->left()->value, ctx.node->right()->value, COND_GT);
        };
        i_handlers[langle_id] = [this](Context& ctx) {
            standard_sub_process(ctx);
            emit_compare(ctx.node->value, ctx.node->left()->value, ctx.node->right()->value, COND_LT);
        };
        i_handlers[equals_id] = [this](Context& ctx) {
            // standard_sub_process(ctx);
            // if(ctx.node->left()->value->reg == -1) {
            //     int result_reg = get_reg(ctx.node->right()->value, LEFT_REG);
            //     emit_save_to_ptr(result_reg, (uint64_t)ctx.node->left()->value->data, ctx.node->left()->value->size);
            // }

            standard_sub_process(ctx);
            int left_reg = ctx.node->left()->value->reg;
            int right_reg = ctx.node->right()->value->reg;
            if(left_reg != -1 && right_reg != -1 && left_reg != right_reg) {
                emit_copy(left_reg, right_reg);
            } else if(left_reg == -1) {
                int result_reg = get_reg(ctx.node->right()->value, LEFT_REG);
                emit_save_to_ptr(result_reg, (uint64_t)ctx.node->left()->value->data, ctx.node->left()->value->size);
            }
        };

        i_handlers[if_id] = [this](Context& ctx) {
            //The condition
            process_node(ctx, ctx.node->left());
            int cond_reg = get_reg(ctx.node->left()->value, LEFT_REG);
            emit_compare_value(cond_reg, 0);

            //Pushing the buffer so we know the offset to jump to if false
            push_buffer();
            standard_travel_pass(ctx.node->scope());
            list<uint32_t> if_body = pop_buffer();
            
            if(ctx.node->scopes.length()>1) {
                push_buffer();
                standard_travel_pass(ctx.node->scopes[1]);
                list<uint32_t> else_body = pop_buffer();
                
                emit_jump_if(COND_EQ, if_body.length() + 2); //+1 for jump past else, +1 to clear branch
                emit_buffer << if_body;
                emit_jump(else_body.length() + 1);
                emit_buffer << else_body;
            } else {
                //No else, just skip over the if body
                emit_jump_if(COND_EQ, if_body.length() + 1);
                emit_buffer << if_body;
            }
        };

        i_handlers[while_id] = [this](Context& ctx) {
            push_buffer(); //Mesuring the size of the loop body
            process_node(ctx, ctx.node->left());
            int cond_reg = get_reg(ctx.node->left()->value, LEFT_REG);
            emit_compare_value(cond_reg, 0);
            list<uint32_t> condition = pop_buffer();
        
            push_buffer();
            standard_travel_pass(ctx.node->scope());
            list<uint32_t> body = pop_buffer();
        
            
            //Exit branch skips body + back jump
            int exit_offset = body.length() + 2; // +1 for back jump, +1 to clear branch
            //Back jump goes to start of condition (negative)
            int back_offset = -(int)(condition.length() + body.length() + 1);
            
            emit_buffer << condition;
            emit_jump_if(COND_EQ, exit_offset);
            emit_buffer << body;
            emit_jump(back_offset);
        };

        i_handlers[func_call_id] = [this](Context& ctx) {
            for(auto [key, val] : ctx.root->value_table.entrySet()) {
                if(val->reg > 0) {
                    int ot = (val->reg - 9);
                    log(emit_buffer.length(),": emitting save to offset ",ot);
                    emit_buffer << STR(val->reg, REG_SP, ot);
                }
            }

            standard_sub_process(ctx);

            g_ptr<Node> decl = ctx.node->scope()->owner;
            if(decl->value->loc == -1) process_node(ctx, decl);
            int offset = decl->value->loc - (int)emit_buffer.length();

            log(emit_buffer.length(),": emitting call to ",decl->value->loc," (offset ",offset,")");
            emit_call(offset);

            for(auto [key, val] : ctx.root->value_table.entrySet()) {
                if(val->reg > 0) {
                    int ot = (val->reg - 9);
                    log(emit_buffer.length(),": emitting load from offset ",ot);
                    emit_buffer << LDR(val->reg, REG_SP, ot);
                }
            }

            ctx.flag = false; //Because the return may have set a flag to return from the travel pass
        };

        i_handlers[func_decl_id] = [this](Context& ctx) {
            if(ctx.node->value->loc != -1) return;
            ctx.node->value->loc = emit_buffer.length();

            log(emit_buffer.length(),": emitting prolouge STP");
            emit_buffer << STP(REG_FRAME_POINTER, REG_LINK, REG_SP, -2); //Offset is passed in units of 8
            log(emit_buffer.length(),": emitting prolouge MOV 29 to SP");
            emit_buffer << 0x910003FD; //MOV x29, sp

            log(emit_buffer.length(),": emitting prolouge reserve 8 bytes");
            emit_buffer << SUB_sp(64);

            standard_travel_pass(ctx.node->scope());

            log(emit_buffer.length(),": emitting epilouge MOV SP to 29");
            emit_buffer << 0x910003BF; // MOV sp, x29  (ADD sp, x29, #0)
            log(emit_buffer.length(),": emitting epilouge LDP");
            emit_buffer << LDP(REG_FRAME_POINTER, REG_LINK, REG_SP, 2);
            emit_return();
        };

        i_handlers[return_id] = [this](Context& ctx) {
            process_node(ctx, ctx.node->left());
            int result_reg = get_reg(ctx.node->left()->value, LEFT_REG);
            emit_copy(REG_RETURN_VALUE, result_reg);

            log(emit_buffer.length(),": emitting epilouge MOV SP to 29");
            emit_buffer << 0x910003BF; // MOV sp, x29  (ADD sp, x29, #0)
            log(emit_buffer.length(),": emitting epilouge LDP");
            emit_buffer << LDP(REG_FRAME_POINTER, REG_LINK, REG_SP, 2);
            emit_return();

            ctx.flag = true;
        };

        i_handlers[print_id] = [this](Context& ctx) {
            for(auto c : ctx.node->children) {
                if(c->value->reg != -1) {
                    for(auto [key, val] : ctx.root->value_table.entrySet()) {
                        if(val->reg > 0) {
                            int ot = (val->reg - 9);
                            log(emit_buffer.length(),": emitting save to offset ",ot);
                            emit_buffer << STR(val->reg, REG_SP, ot);
                        }
                    }
                    emit_buffer << MOV_reg(0, c->value->reg, 0);
                    emit_load_64(LEFT_REG, (uint64_t)&jit_print_int);
                    emit_call_register(LEFT_REG);
                    for(auto [key, val] : ctx.root->value_table.entrySet()) {
                        if(val->reg > 0) {
                            int ot = (val->reg - 9);
                            log(emit_buffer.length(),": emitting load from offset ",ot);
                            emit_buffer << LDR(val->reg, REG_SP, ot);
                        }
                    }
                }
            }
        };
        
        size_t jint_id = add_function("jint");
        i_handlers[jint_id] = [this](Context& ctx) {
                for(auto [key, val] : ctx.root->value_table.entrySet()) {
                    if(val->reg > 0) {
                        int ot = (val->reg - 9);
                        log(emit_buffer.length(),": emitting save to offset ",ot);
                        emit_buffer << STR(val->reg, REG_SP, ot);
                    }
                }
                emit_buffer << MOV_reg(0, REG_FRAME_POINTER, 1);
                emit_load_64(LEFT_REG, (uint64_t)&jit_print_int);
                emit_call_register(LEFT_REG);

                emit_buffer << MOV_from_sp(0);
                emit_load_64(LEFT_REG, (uint64_t)&jit_print_int);
                emit_call_register(LEFT_REG);

                for(auto [key, val] : ctx.root->value_table.entrySet()) {
                    if(val->reg > 0) {
                        int ot = (val->reg - 9);
                        log(emit_buffer.length(),": emitting load from offset ",ot);
                        emit_buffer << LDR(val->reg, REG_SP, ot);
                    }
                }
        };

        size_t jont_id = add_function("jont");
        i_handlers[jont_id] = [this](Context& ctx){
            emit_buffer << STR(REG_LINK, REG_FRAME_POINTER, 0);
            emit_load_64(LEFT_REG, (uint64_t)&jont);
            emit_call_register(LEFT_REG);
            emit_buffer << LDR(REG_LINK, REG_FRAME_POINTER, 0);  
        };

        span->print_on_line_end = false; //While things aren't crashing
        //span->log_everything = true; //While things are crashing

        
        span2 = make<Log::Span>(); 

        // auto now = std::chrono::system_clock::now();
        // auto t = std::chrono::system_clock::to_time_t(now);
        // printnl("At: ",std::ctime(&t));
        //span->print_all();

    }

    std::string code_store;

    g_ptr<Node> process(const std::string& path) override { 
        std::string code = readFile(path);
        code_store = code;
        timer.start();
        span2->add_line("TOKENIZE STAGE");
        span2->log_everything = true;

        list<g_ptr<Node>> tokens = tokenize(code);
        span2->end_line();
        span2->add_line("A STAGE");
        // print("A STAGE");
        start_stage(&a_handlers,a_parse_function);
        list<g_ptr<Node>> nodes = parse_tokens(tokens);
        
        a_pass_resolve_keywords(nodes);

        // for(auto n : nodes) {
        //     print(n->to_string());
        // }

        span2->end_line();
        span2->add_line("S STAGE");
        start_stage(&s_handlers,s_default_function);
        g_ptr<Node> root = parse_scope(nodes);
        return root;
    };

    bool emit_mode = false;

    void run(g_ptr<Node> root) override { 
        // print("T STAGE");
        span2->end_line();
        span2->add_line("T STAGE");
        start_stage(&t_handlers,t_default_function);
        standard_resolving_pass(root);

        //print("D STAGE");
        span2->end_line();
        span2->add_line("D STAGE");
        start_stage(&d_handlers,d_default_function);
        discover_symbols(root);

        //print("R STAGE");
        span2->end_line();
        span2->add_line("R STAGE");
        start_stage(&r_handlers,r_default_function);
        standard_resolving_pass(root);


        if(emit_mode) {
            span2->end_line();
            span2->add_line("E STAGE");
            start_stage(&e_handlers,e_default_function);
            standard_backwards_pass(root);

            span2->end_line();
            span2->add_line("M STAGE");
            start_stage(&m_handlers,m_default_function);
            memory_backwards_pass(root);
        }

        g_ptr<Node> main_func = nullptr;
        for(auto c : root->scopes) {
            if(c->name == "main") {
                main_func = c;
                break;
            }
        }

        if(emit_mode) {
            span2->end_line();
            span2->add_line("I STAGE");
            start_stage(&i_handlers,i_default_function);
            int jump_placeholder = emit_buffer.length();
            emit_buffer << B(0); 
            standard_travel_pass(root);
            emit_buffer[jump_placeholder] = B(main_func->owner->value->loc - jump_placeholder);
        }
        span2->end_line();

        std::string final_time = ftime(timer.end());

        // print("==LOG==");
        // span->print_all();
        // print(node_to_string(root));

        std::function<void(g_ptr<Node>)> print_scopes = [&print_scopes,this](g_ptr<Node> root) {
            for(auto t : root->scopes) {
                print(node_to_string(t));
            }
            for(auto child_scope : root->scopes) {
                print_scopes(child_scope);
            }
        };
        print_scopes(root);

        print("Ran:\n",code_store);
        if(emit_mode) {
            print_emit_buffer();
        }
        span2->end_line();
        span2->add_line("X STAGE");
        start_stage(&x_handlers,x_default_function);

        std::string exec_time = "";

        if(emit_mode) {
            //Make the buffer
            size_t byte_size = emit_buffer.length() * sizeof(uint32_t);
            void* buf = mmap(nullptr, byte_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
                -1, 0);

            if(buf == MAP_FAILED) {
                print("mmap failed: ", strerror(errno));
                return;
            }

            //Copy the instructions
            uint32_t* ptr = (uint32_t*)buf;
            for(int i=0;i<emit_buffer.length();i++) {
                ptr[i] = emit_buffer[i];
            }

            //Make executable
            mprotect(buf, byte_size, PROT_READ | PROT_EXEC);
            struct sigaction sa;
            sa.sa_sigaction = sigill_handler;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGILL, &sa, nullptr);

            struct sigaction sa2;
            sa2.sa_sigaction = sigsegv_handler;
            sa2.sa_flags = SA_SIGINFO;
            sigemptyset(&sa2.sa_mask);
            sigaction(SIGSEGV, &sa2, nullptr);
            sigaction(SIGBUS, &sa2, nullptr);

            jit_buf_start = buf;
            jit_buf_size = byte_size;

            timer.start();
            print("==EXECUTING==");

            typedef int (*JitFunc)();
            JitFunc func = (JitFunc)buf;
            int result = func();

            exec_time =  ftime(timer.end());
            span2->end_line();

            print("Native result: ", result);
            munmap(buf, byte_size); //Cleanup
        } else {
            timer.start();
            print("==EXECUTING==");

            standard_travel_pass(main_func?main_func:root);

            exec_time = ftime(timer.end());
            span2->end_line();
        }
        
        span2->print_all();

        print("Final time: ",final_time);
        print("Exec time: ",exec_time);

        print("==DONE==");
    };

};

g_ptr<Unit> return_unit() {
    return make<C_Compiler>();
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