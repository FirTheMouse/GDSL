#pragma once

#include "../modules/GDSL-Core.hpp"
#include "../modules/Q-Arm64.hpp"
#include "../modules/Q-Scope.hpp"
#include "../modules/Q-Function.hpp"
#include "../modules/Q-Precedence.hpp"

//A subset of C, the ctest.gld file shows of most of what this can do.
//I'm activly expanding this, generics are next on the roadmap and one day I hope to bootstrap GDSL itself with this.

namespace GDSL {

struct C_Compiler : virtual public Scope_Unit, virtual public Function_Unit, virtual public Precedence_Unit, virtual public ARM64_Unit {
    //To prove a point about MLIR's tutourial example
    std::function<void(Context&)> make_involution(size_t involute_id){
        return [involute_id](Context& ctx){
            if(ctx.node->children[0]->type == involute_id) {
                ctx.node->copy(ctx.node->children[0]->children[0]);
            }
        };
    };

    size_t method_scope_id = reg_id("METHOD_SCOPE");
    size_t type_scope_id = reg_id("TYPE_SCOPE");

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
    std::string output_string = "";



    void init() override {
        span = make<Log::Span>();
        Function_Unit::init();
        Precedence_Unit::init();
        Scope_Unit::init();

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


       
        t_handlers[to_prefix_id(typename_id)] = [](Context& ctx){
            ctx.value->sub_type = ctx.qual->type;
            ctx.value->type = ctx.qual->type;
            ctx.value->size = ctx.qual->value->size;
            if(ctx.qual->value->type_scope)
                ctx.value->type_scope = ctx.qual->value->type_scope;
        };
           
        


        // a_handlers[langle_id] = [this](Context& ctx) {
        //     int i = ctx.index;
        //     while(i<ctx.nodes.length()) {
        //         if(ctx.nodes[i]->type==end_id||ctx.nodes[i]->type==lbrace_id) {
        //             break;
        //         } 
        //         else if(ctx.nodes[i]->type==rangle_id) {
        //             ctx.nodes[ctx.index-1]->type = lparen_id;
        //             ctx.nodes[i]->type = rparen_id;
        //             ctx.index--;
        //             return;
        //         }
        //         i++;
        //     }
        //     ctx.node = nullptr;
        //     ctx.flag = true; //A stage uses it's flag for this
        // };

        //Add this one day so we can have C++ style templates if we want
        // t_handlers[template_id] = [](Context& ctx) {

        // };

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

        t_handlers[end_id] = [this](Context& ctx){
            ctx.result->removeAt(ctx.index);
            ctx.index--;
        };

        t_handlers[comma_id] = [this](Context& ctx){
            ctx.result->removeAt(ctx.index);
            ctx.index--;
        };
        


        left_binding_power.put(lbracket_id, 10);
        // a_handlers[lbracket_id] = [this](Context& ctx) {
        //     g_ptr<Node> result_node = make<Node>();
        //     result_node->type = lbracket_id;
            
        //     if(ctx.left && ctx.left->type != 0) {
        //         result_node->children << ctx.left;
        //     }
            
        //     while(ctx.index < ctx.nodes.length() && ctx.nodes[ctx.index]->getType() != rbracket_id) {
        //         g_ptr<Node> expr = a_parse_expression(ctx, 0);
        //         if(expr) result_node->children << expr;
        //     }
        //     ctx.index++;
            
        //     ctx.node = result_node;
        // };
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

        left_binding_power.put(colon_id, 4);
        right_binding_power.put(colon_id, 9);

        t_handlers.default_function = [this](Context& ctx){
            standard_sub_process(ctx);
        };
        r_handlers.default_function = [this](Context& ctx){
            standard_sub_process(ctx);
        };
        d_handlers.default_function = [this](Context& ctx){
            for(auto c : ctx.node->children) {
                discover_symbol(c,ctx.root);
            }
        };

        print_id = add_function("print");
        x_handlers[print_id] = [this](Context& ctx) {
            std::string toPrint = "";
            for(auto r : ctx.node->children) {
                process_node(ctx, r);
                toPrint.append(value_as_string(r->value));
            }
            print(toPrint);
            output_string.append(toPrint+"\n");
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
        m_handlers.default_function = [this](Context& ctx){
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

        
        span2 = make<Log::Span>(); 

        // auto now = std::chrono::system_clock::now();
        // auto t = std::chrono::system_clock::to_time_t(now);
        // printnl("At: ",std::ctime(&t));
        //span->print_all();

    }

    std::string code_store;
    void print_scopes(g_ptr<Node> root){
        for(auto t : root->scopes) {
            print(node_to_string(t));
        }
        for(auto child_scope : root->scopes) {
            print_scopes(child_scope);
        }
    };

    g_ptr<Node> process(const std::string& code) override { 
        code_store = code;
        timer.start();

        span2->add_line("TOKENIZE STAGE");
        g_ptr<Node> root = make<Node>();
        root->children = tokenize(code);
        span2->end_line();

        span2->add_line("A STAGE");
        start_stage(a_handlers);
        standard_direct_pass(root);
        span2->end_line();

        span2->add_line("A CLEAN");
        a_pass_resolve_keywords(root->children);
        span2->end_line();
     
        span2->add_line("S STAGE");
        start_stage(s_handlers);
        parse_scope(root);
        span2->end_line();
        
        // print(node_to_string(root));
        return root;
    };

    bool emit_mode = false;

    void run(g_ptr<Node> root) override { 
        // print("T STAGE");
        span2->end_line();
        span2->add_line("T STAGE");
        start_stage(t_handlers);
        standard_resolving_pass(root);

        //print("D STAGE");
        span2->end_line();
        span2->add_line("D STAGE");
        start_stage(d_handlers);
        discover_symbols(root);

        //print("R STAGE");
        span2->end_line();
        span2->add_line("R STAGE");
        start_stage(r_handlers);
        standard_resolving_pass(root);


        if(emit_mode) {
            span2->end_line();
            span2->add_line("E STAGE");
            start_stage(e_handlers);
            standard_backwards_pass(root);

            span2->end_line();
            span2->add_line("M STAGE");
            start_stage(m_handlers);
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
            start_stage(i_handlers);
            int jump_placeholder = emit_buffer.length();
            emit_buffer << B(0); 
            standard_travel_pass(root);
            emit_buffer[jump_placeholder] = B(main_func->owner->value->loc - jump_placeholder);
        }
        span2->end_line();

        std::string final_time = ftime(timer.end());

        #if PRINT_ALL
            print("==LOG==");
            span->print_all();
            print(node_to_string(root));
            print_scopes(root);

            print("Ran:\n",code_store);
            if(emit_mode) {
                print_emit_buffer();
            }
        #endif
        span2->end_line();
        span2->add_line("X STAGE");
        start_stage(x_handlers);

        std::string exec_time = "";

        if(emit_mode) {
            //Make the buffer

            #ifdef __APPLE__

            #else
                #define MAP_JIT 0x0800
            #endif

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
            // struct sigaction sa;
            // sa.sa_sigaction = sigill_handler;
            // sa.sa_flags = SA_SIGINFO;
            // sigemptyset(&sa.sa_mask);
            // sigaction(SIGILL, &sa, nullptr);

            // struct sigaction sa2;
            // sa2.sa_sigaction = sigsegv_handler;
            // sa2.sa_flags = SA_SIGINFO;
            // sigemptyset(&sa2.sa_mask);
            // sigaction(SIGSEGV, &sa2, nullptr);
            // sigaction(SIGBUS, &sa2, nullptr);

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
            #if PRINT_ALL
                print("==EXECUTING==");
            #endif

            standard_travel_pass(main_func?main_func:root);

            exec_time = ftime(timer.end());
            span2->end_line();
        }
        
        // #if PRINT_ALL
            span2->print_all();

            print("Final time: ",final_time);
            print("Exec time: ",exec_time);

            print("==DONE==");
        //#endif
    };

};

g_ptr<Unit> return_unit() {
    return make<C_Compiler>();
}

}


//Potential future Q-OOP Unit, for now just grouped down here for convinent drop-in

// size_t method_scope_id = reg_id("METHOD_SCOPE");
// size_t type_scope_id = reg_id("TYPE_SCOPE");
// size_t template_id = make_tokenized_keyword("template");
// size_t typename_id = add_type("typename");
// void inject_this_param(Context& ctx) {
//     g_ptr<Node> node = ctx.node;
//     g_ptr<Node> star = make<Node>();
//     star->type = star_id;
//     star->place_in_scope(node->scope().getPtr());
//     star->name = "made_by_this";

//     g_ptr<Node> type_term = make<Node>();
//     type_term->type = identifier_id;
//     type_term->name = node->in_scope->owner->name;
//     type_term->place_in_scope(node->scope().getPtr());
//     star->children << type_term;

//     g_ptr<Node> id_term = make<Node>();
//     id_term->type = identifier_id;
//     id_term->name = "this";
//     id_term->place_in_scope(node->scope().getPtr());
//     star->children << id_term;

//     node->children.insert(star, 0);
// };
// void inject_member_access(Context& ctx) {
//     g_ptr<Node> node = ctx.node;
//     g_ptr<Node> prop = make<Node>();
//     prop->type = dot_id;
//     prop->place_in_scope(node->in_scope);
    
//     g_ptr<Node> star = make<Node>();
//     star->type = star_id;
//     star->place_in_scope(node->in_scope);
    
//     g_ptr<Node> this_id = make<Node>();
//     this_id->type = identifier_id;
//     this_id->name = "this";
//     this_id->place_in_scope(node->in_scope);
//     star->children << this_id;
    
//     g_ptr<Node> member_id = make<Node>();
//     member_id->type = identifier_id;
//     member_id->name = node->name;
//     member_id->place_in_scope(node->in_scope);
    
//     prop->children << star;
//     prop->children << member_id;
//     //We parse this node because it won't resolve again like something in a type scope would
//     process_node(ctx, prop);
//     node->copy(prop);
// };

// t_handlers[identifier_id] = [this](Context& ctx) {
//     // log("Parsing an idenitifer:\n",ctx.node->to_string(1));
//     g_ptr<Node> node = ctx.node;
//     g_ptr<Value> decl_value = make<Value>();
    
//     bool had_a_value = (bool)(node->value);
//     bool had_a_scope = (bool)(node->scope());
//     bool found_a_value = node->find_value_in_scope();
//     bool is_qualifier = node->value_is_valid();

//     int root_idx = -1;
//     if(node->value_is_valid() && node->value->sub_type != 0) {
//         //log("I am a qualifier");
//         decl_value->quals << value_to_qual(node->value);
//         for(int i = 0; i < node->children.length(); i++) {
//             g_ptr<Node> c = node->children[i];
//             c->find_value_in_scope();
//             if(c->value_is_valid()) {
//                 decl_value->quals << value_to_qual(c->value);
//             } else {
//                 root_idx = i;
//                 break;
//             }
//         }
//         if(root_idx!=-1) {
//             g_ptr<Node> root = node->children[root_idx];
//             node->name = root->name;
//             for(int i = root_idx+1; i < node->children.length(); i++) {
//                 g_ptr<Node> c = node->children[i];
//                 c->find_value_in_scope();
//                 if(c->value_is_valid()) {
//                     node->quals << value_to_qual(c->value);
//                 } 
//             }
//             node->children = node->children.take(root_idx)->children;
//         }
//     } else {
//         //log("no valid value, I am the root");
//     }
    
//     if(!node->scope()) { //Defer, the r_stage will do this later for scoped nodes
//         standard_sub_process(ctx);
//     }

//     if(keywords.hasKey(node->name)) {
//         node->type = node->value->sub_type;
//         return;
//     }

//     node->value = decl_value;
//     fire_quals(ctx, decl_value);

//     bool has_scope = node->scope() != nullptr;
//     bool has_type_scope = node->value->type_scope != nullptr;
//     bool has_sub_type = node->value->sub_type != 0;
    
//     if(has_scope) {
//         node->scope()->owner = node.getPtr();
//         node->scope()->name = node->name;
//         if(has_sub_type) {
//             node->type = func_decl_id;
//             node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
//             node->value->type_scope = node->scope().getPtr();
//             node->value = node->in_scope->distribute_value(node->name,node->value);
//             if(node->value->sub_type!=typename_id)
//                 node->value->sub_type = 0;

//             if(node->in_scope->type==type_scope_id) {
//                 node->scope()->type = method_scope_id;
//             }

//             //The 'this' field passed to a func_decl with members
//             if(node->in_scope->value_table.hasKey(node->in_scope->name)) {
//                 inject_this_param(ctx);
//             }
//         } else {
//             node->type = type_decl_id;
//             node->value->copy(make_type(node->name,0));
//             node->value->type_scope = node->scope().getPtr();
//             node->value = node->in_scope->distribute_value(node->name,node->value);

//             node->scope()->type = type_scope_id;

//             for(auto c : node->children) { //Parse inline quals for templates
//                 if(c->value->type==0) {
//                     g_ptr<Node> qual = make<Node>();
//                     qual->type = typename_id;
//                     qual->sub_type = typename_id;
//                     qual->value = c->value;
//                     c->value->quals << qual;
//                     c->value->type = typename_id;
//                     c->value->sub_type = typename_id;
//                     c->in_scope = node->scope().getPtr();
//                 } else if(c->value->type!=typename_id) {
//                     node->quals << value_to_qual(c->value);
//                     node->children.erase(c);
//                 }
//             }
//         }
//     } else {
//         has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
//         if(has_sub_type) {
//             if(node->value->sub_type == typename_id && node->in_scope->value_table.hasKey(node->name)) {
//                 //Reference to existing typename-typed variable, not a new declaration
//                 node->find_value_in_scope();
//                 node->type = identifier_id;
//             } else {
//                 node->type = var_decl_id;
//                 if(node->in_scope->type==type_scope_id) {
//                     node->in_scope->value_table.put(node->name, decl_value);
//                 } else {
//                     node->value = node->in_scope->distribute_value(node->name, decl_value);
//                 }
//             }
//             if(node->value->sub_type!=typename_id)
//                 node->value->sub_type = 0;
//         } else if(has_scope) {
//             node->type = func_call_id;
//             node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
//             if(node->value->type_scope)
//                 node->scopes[0] = node->value->type_scope; //Swap to the type scope

//             node->name.append("(");
//             for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
//         } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
//             node->find_value_in_scope();
//         } else {                                         
//             if(node->in_scope->value_table.hasKey("this")) { //The has check is so we don't inject this on the names of declared variables at the top
//                 if(node->in_scope->owner->type==func_decl_id&&!node->in_scope->children.has(node)) {

//                 } else {
//                     inject_member_access(ctx);
//                 }
//             } else {
//                 //HM Tracing goes here.
//                 //Attatch a qual with handlers for it
//                 //Borrow checker too, maybe not here though.
//             }
//         }
//     }

//     //log("Returning:\n",node->to_string(1));
// };