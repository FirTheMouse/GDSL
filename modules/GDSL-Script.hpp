#pragma once

#include "../modules/GDSL-Starter.hpp"

namespace GDSL {
    struct Q_Script_Unit : public virtual Starter_DSL_Frontend {
        Q_Script_Unit() { init(); }

        struct stage_and_handler {
            stage_and_handler() {}
            stage_and_handler(map<uint32_t,Handler>* _map, Handler* _def) : 
                                default_handler(_def), handlers(_map)  {}
            stage_and_handler(map<uint32_t,Handler>& _map, Handler& _def) : 
                                default_handler(&_def), handlers(&_map)  {}
            map<uint32_t,Handler>* handlers;
            Handler* default_handler;
        };

        map<uint32_t,stage_and_handler> stages;

        size_t node_block_id = reg_id("NODE_BLOCK");
        size_t handler_block_id = reg_id("HANDLER_BLOCK");

        size_t add_stage_lookup(const std::string& f, map<uint32_t,Handler>* handlers, Handler* default_handler) {
            size_t id = make_tokenized_keyword(f);
            stages.put(id,stage_and_handler(handlers,default_handler));
            scope_link_handlers[id] = [this,id](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                standard_scope_link_handler(new_scope,current_scope,owner_node);
                owner_node->type = handler_block_id;
                owner_node->sub_type = id;
                new_scope->name = owner_node->name;
            };
            t_handlers[id] = [this](Context& ctx){
                n_take_right(ctx,1);
            };
            x_handlers[id] = [this,handlers,default_handler](Context& ctx){
                if(ctx.node->left()) {
                    start_stage(handlers,*default_handler);
                    Context sub_ctx;
                    sub_ctx.root = ctx.node;
                    sub_ctx.node = ctx.node->left();
                    x_handlers.getOrDefault(ctx.node->left()->type,x_default_function)(sub_ctx);
                    start_stage(&x_handlers,x_default_function);
                }
            };
            return id;
        }

        size_t assemble_stage = add_stage_lookup("assemble",&a_handlers,&a_default_function);
        size_t scope_stage = add_stage_lookup("scoping",&s_handlers,&s_default_function);
        size_t type_stage = add_stage_lookup("type",&t_handlers,&t_default_function);
        size_t resolve = add_stage_lookup("resolve",&r_handlers,&r_default_function);
        size_t execute_stage = add_stage_lookup("execute",&x_handlers,&x_default_function);

        size_t travel_pass_id = make_tokenized_keyword("travel_pass");
        size_t direct_pass_id = make_tokenized_keyword("direct_pass");

        size_t push_id = register_binary_operator(reg_id("push"));
        size_t scan_id = make_tokenized_keyword("scan");
        size_t on_id = make_tokenized_keyword("on");

        size_t test_id = make_tokenized_keyword("test");
        size_t ctx_node_id = make_tokenized_keyword("node");
        size_t ctx_root_id = make_tokenized_keyword("root");
        size_t ctx_name_id = make_tokenized_keyword("name");
        size_t ctx_source_id = make_tokenized_keyword("source");
        size_t ctx_scope_id = make_tokenized_keyword("scope");
        size_t ctx_children_id = make_tokenized_keyword("children");

        size_t read_file_id = make_tokenized_keyword("read_file");

        size_t type_scope_id = reg_id("TYPE_SCOPE");
        size_t print_id = add_function("print");
        size_t while_id = add_scoped_keyword("while", 2);
        size_t if_id = add_scoped_keyword("if", 2);
        size_t else_id = add_scoped_keyword("else", 1);

        size_t list_id = reg_id("list");


        

        void init() override {
            Starter_DSL_Frontend::init();

            tokenizer_functions['<'] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c=='<') {
                    ctx.node = make<Node>(push_id);
                    ctx.node->name = "<<";
                    ctx.result->push(ctx.node);
                    ctx.index++;
                } else {
                    ctx.node = make<Node>(langle_id,c);
                    ctx.result->push(ctx.node);
                }
            };
            set_binding_powers(push_id,4,6);

            x_handlers[travel_pass_id] = [this](Context& ctx){
                standard_travel_pass(ctx.node->scope());
            };
            x_handlers[direct_pass_id] = [this](Context& ctx){
                standard_direct_pass(ctx.node->scope());
            };

            scope_link_handlers[string_id] = [this](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                standard_scope_link_handler(new_scope,current_scope,owner_node);
                owner_node->type = node_block_id;
                new_scope->name = owner_node->name;
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                for(auto e : labels.entrySet()) {
                    if(e.value==ctx.node->name) {
                        ctx.node->sub_type = e.key;
                        break;
                    }
                }
                if(ctx.node->sub_type==0) {
                    print(red("WARNING: unrecognized node type: "+ctx.node->name));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.flag = standard_travel_pass(ctx.node->scope());
            };
            
            x_handlers[handler_block_id] = [this](Context& ctx){
                g_ptr<Node> this_node = ctx.node;
                size_t target_type = ctx.node->in_scope->owner->sub_type;
                stage_and_handler& stage = stages.get(ctx.node->sub_type);
                (*stage.handlers)[target_type] = [this,this_node](Context& ctx) {
                    map<uint32_t,Handler>* old_active = active_handlers;
                    Handler old_default = active_default_function;

                    start_stage(&x_handlers,x_default_function);
                    standard_travel_pass(this_node->scope(),&ctx);
                    start_stage(old_active,old_default);
                    
                };
            };

            x_handlers[ctx_node_id] = [this](Context& ctx){
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->node);
            };
            x_handlers[ctx_root_id] = [this](Context& ctx){
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->root);
            };
            x_handlers[ctx_source_id] = [this](Context& ctx){
                g_ptr<Value> v = ctx.node->value;
                v->type = string_id;
                v->size = sizeof(std::string);
                v->data = &ctx.sub->source;
            };
            x_handlers[ctx_name_id] = [this](Context& ctx){
                g_ptr<Value> v = ctx.node->value;
                v->type = string_id;
                v->size = sizeof(std::string);
                v->data = &ctx.left->value->get<g_ptr<Node>>()->name;
            };
            x_handlers[ctx_scope_id] = [this](Context& ctx){
                ctx.node->value->set<g_ptr<Node>>(ctx.left->value->get<g_ptr<Node>>()->scope());
            };
            x_handlers[ctx_children_id] = [this](Context& ctx){
                if(ctx.node->left()) {
                    ctx.node->value->type = object_id;
                    int idx = ctx.node->left()->value->get<int>();
                    list<g_ptr<Node>>& children = ctx.left->value->get<g_ptr<Node>>()->children;
                    if(idx<children.length()) {
                        ctx.node->value->set<g_ptr<Node>>(children[idx]);
                    } else {
                        print(red("ctx_children:x_handler index out of bounds for children"));
                    }
                } else {
                    ctx.node->value->type = list_id;
                    ctx.node->value->data = &ctx.left->value->get<g_ptr<Node>>()->children;
                }
            };

            x_handlers[read_file_id] = [this](Context& ctx){
                ctx.node->value->set<std::string>(readFile(ctx.node->left()->value->get<std::string>()));
            };

            x_handlers[test_id] = [this](Context& ctx){
                print("This should do nothing");
            };

            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node->sub_type = ctx.node->right()->type;
                ctx.node->value = ctx.node->right()->value;
            };

            n_handlers[scan_id] = [this](Context& ctx) {
                n_take_right(ctx,2);
            };
          
            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node->left());
                if(ctx.node->left()->value->is_true()) {
                    ctx.flag = standard_travel_pass(ctx.node->scope());
                }
                else if(ctx.node->scopes.length()>1) {
                    ctx.flag = standard_travel_pass(ctx.node->scopes[1]);
                }
            };
            t_handlers[else_id] = [this](Context& ctx) {
                if(ctx.index>0) {
                    if(ctx.left->type==if_id) {
                        ctx.node->scope()->owner = ctx.left.getPtr();
                        ctx.left->scopes << ctx.node->scopes;
                        ctx.result->removeAt(ctx.index);
                        ctx.index--;
                    }
                }
                ctx.node = nullptr;
            };


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

            x_handlers[print_id] = [this](Context& ctx) {
                std::string toPrint = "";
                for(auto r : ctx.node->children) {
                    process_node(ctx, r);
                    toPrint.append(value_as_string(r->value));
                }
                print(toPrint);
            };


            t_handlers[identifier_id] = [this](Context& ctx) {
                g_ptr<Node> node = ctx.node;
                g_ptr<Value> decl_value = make<Value>();
                
                bool had_a_value = (bool)(node->value);
                bool had_a_scope = (bool)(node->scope());
                bool found_a_value = node->find_value_in_scope();
                bool is_qualifier = node->value_is_valid();

                int root_idx = -1;
                if(node->value_is_valid() && node->value->sub_type != 0) {
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
                        if(!node->children.empty()) {
                            node->name.append("(");
                            for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
                        }
                    } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
                        node->find_value_in_scope();
                    } else {                                         
                        //We don't know what the thing is
                    }
                }
            };
        } 
    };
}