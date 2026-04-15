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

        size_t node_block_id = reg_id("NODE_BLOCK");

        size_t travel_pass_id = make_keyword("travel_pass");
        size_t direct_pass_id = make_keyword("direct_pass");

        size_t push_id = register_binary_operator(reg_id("push"));
        size_t is_id = register_binary_operator(reg_id("is"));
        size_t is_not_id = register_binary_operator(reg_id("is_not"));
        size_t scan_id = make_tokenized_keyword("scan");
        size_t on_id = make_tokenized_keyword("on");
        size_t in_id = make_keyword("in");

        size_t node_id = add_type("Node",8);
        size_t value_id = add_type("Value",8);
        size_t store_id = add_type("Store",8);

        size_t test_id = make_keyword("test");
        size_t ctx_node_id = add_function("node",node_id);
        size_t ctx_root_id = make_keyword("root",8,"",node_id);
        size_t ctx_source_id = make_keyword("source",sizeof(std::string),"",string_id);

        size_t general_type_id = make_keyword("type",4,"",int_id);
        size_t general_length_id = make_keyword("length",4,"",int_id);

        size_t node_scope_id = make_keyword("scope",8,"",node_id);
        size_t node_name_id = make_keyword("name",sizeof(std::string),"",string_id);
        size_t node_in_scope_id = make_keyword("in_scope",8,"",node_id);
        size_t node_owner_id = make_keyword("owner",8,"",node_id);
        size_t node_children_id = make_keyword("children");
        size_t node_value_id = make_keyword("value",8,"",value_id);

        size_t value_store_id = make_keyword("store",8,"",store_id);

        size_t read_file_id = make_tokenized_keyword("read_file");
        size_t find_in_labels_id = add_function("find_in_labels");

        size_t type_scope_id = reg_id("TYPE_SCOPE");
        size_t print_id = add_function("print");
        size_t while_id = add_scoped_keyword("while", 2);
        size_t if_id = add_scoped_keyword("if", 2);
        size_t else_id = add_scoped_keyword("else", 1);


        size_t node_list_id = add_type("node_list",8);
        size_t value_list_id = add_type("value_list",8);
        size_t node_map_id = add_type("node_map",8);
        size_t value_map_id = add_type("value_map",8);

        size_t test_test_id = make_tokenized_keyword("test_test");



        template<typename T>
        void list_op_on_node(Context& ctx, list<T>& accessor) {
            if(ctx.node->left()) {
                ctx.node->value->type = node_id;
                int idx = ctx.node->left()->value->get<int>();
                if(idx<accessor.length()) {
                    ctx.node->value->set<T>(accessor[idx]);
                } else {
                    print(red("ctx_list_op_on_node:x_handler index out of bounds for op in form of list"));
                }
            } else {
                bool is_last_opperand = (ctx.root->left()->right()==ctx.node);
                if(is_last_opperand && ctx.root->type==push_id) {
                    if(ctx.root->right()->value->type==node_id) {
                        accessor << ctx.root->right()->value->get<T>();
                    } else {
                        accessor << ctx.root->right();
                    }
                } else if(is_last_opperand && ctx.root->type == equals_id) {
                    if(ctx.root->right()->value->type == node_id) {
                        accessor = ctx.root->right()->value->get<list<T>>();
                    } else {
                        print(red("This shouldn't happen"));
                    }
                } else {
                    ctx.node->value->type = node_list_id;
                    ctx.node->value->size = 8;
                    ctx.node->value->data = &accessor;
                }
            }
        }

        template<typename T>
        void standard_accessor(Context& ctx, T& accessor) {
            ctx.node->value->set<T>(accessor);
            if(binary_op_catalog.has(ctx.root->type)) {
                bool is_last_opperand = (ctx.root->left()->right()==ctx.node);
                if(is_last_opperand && ctx.root->type == equals_id) {
                    accessor = ctx.root->right()->value->get<T>();
                }
            }
        }

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

            tokenizer_functions['='] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c=='=') {
                    ctx.node = make<Node>(is_id);
                    ctx.node->name = "==";
                    ctx.result->push(ctx.node);
                    ctx.index++;
                } else {
                    ctx.node = make<Node>(equals_id,c);
                    ctx.result->push(ctx.node);
                }
            };
            set_binding_powers(is_id,4,6);

            tokenizer_functions['!'] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c=='=') {
                    ctx.node = make<Node>(is_not_id);
                    ctx.node->name = "!=";
                    ctx.result->push(ctx.node);
                    ctx.index++;
                } else {
                    //Implment exclamation point token later (yes, I'm that lazy, yes, it's one line)
                }
            };
            set_binding_powers(is_not_id,4,6);

            std::function<bool(Context&)> handler_for_is = [this](Context& ctx) -> bool {
                standard_sub_process(ctx);
                ctx.node->value->type = bool_id;
                
                g_ptr<Value> l = ctx.node->left()->value;
                g_ptr<Value> r = ctx.node->right()->value;
                
                bool result = false;
                if(l->type != r->type) {
                    result = false;
                } else if(l->type == string_id) {
                    result = l->get<std::string>() == r->get<std::string>();
                } else if(l->type == node_id) {
                    // result = l->data == r->data;
                    result = l->get<g_ptr<Node>>().getPtr() == r->get<g_ptr<Node>>().getPtr();
                } else if(l->data && r->data && l->size == r->size) {
                    result = memcmp(l->data, r->data, l->size) == 0;
                }
                return result;
            };

            x_handlers[is_id] = [this,handler_for_is](Context& ctx) {
                bool result = handler_for_is(ctx);
                ctx.node->value->set<bool>(result);
            };
            x_handlers[is_not_id] = [this,handler_for_is](Context& ctx) {
                bool result = handler_for_is(ctx);
                ctx.node->value->set<bool>(!result);
            };

            x_handlers[travel_pass_id] = [this](Context& ctx){
                standard_travel_pass(ctx.node->scope());
            };
            x_handlers[direct_pass_id] = [this](Context& ctx){
                standard_direct_pass(ctx.node->scope());
            };

            r_handlers[in_id] = [this](Context& ctx){
                ctx.node->name = "in "+ctx.node->left()->name+" "+labels[ctx.node->in_scope->owner->sub_type];
                ctx.node->scope()->name = ctx.node->name;
            };
            x_handlers[in_id] = [this](Context& ctx){
                g_ptr<Node> this_node = ctx.node;
                size_t target_type = ctx.node->in_scope->owner->sub_type;
                std::string stage_name = ctx.node->left()->name;
                if(!stages.hasKey(stage_name)) {
                    print(red("in_id:x_handler unknown stage "+stage_name));
                    return;
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                (*stage)[target_type] = [this,this_node](Context& ctx) {
                    g_ptr<Stage> old_stage = active_stage;

                    start_stage(x_handlers);
                    standard_travel_pass(this_node->scope(),&ctx);
                    start_stage(old_stage);
                    
                };
            };

            r_handlers[to_prefix_id(node_id)] = [this](Context& ctx){
                ctx.value->set<g_ptr<Node>>(ctx.node);
            };
            r_handlers[to_prefix_id(string_id)] = [this](Context& ctx){
                ctx.value->set<std::string>(ctx.node->name);
            };
            value_to_string[node_id] = [this](void* data) -> std::string {
                return ptr_to_string((*(g_ptr<Node>*)data)->value.getPtr());
            };

            x_handlers[push_id] = [this](Context& ctx){
                backwards_sub_process(ctx);
            };

            r_handlers[identifier_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node->value);
            };
            x_handlers[identifier_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node->value);
            };

            r_handlers[to_prefix_id(store_id)] = [this](Context& ctx){
                if(!ctx.value->store) {
                    ctx.value->store = make<Type>();
                }
            };

            x_handlers[to_prefix_id(node_list_id)] = [this](Context& ctx){

            };
            x_handlers[to_prefix_id(node_map_id)] = [this](Context& ctx){

            };

            x_handlers[to_prefix_id(store_id)] = [this](Context& ctx){
                if(ctx.node->left()) { //Like l(3)
                    process_node(ctx,ctx.node->left());
                    g_ptr<Value> idx = ctx.node->left()->value;

                    if(idx->type==int_id) {
                        ctx.value->query_store(idx->get<int>());
                    } else if(idx->type==string_id) {
                        ctx.value->query_store(idx->get<std::string>());
                    } else if(idx->type==store_id) {
                        //Do nothing
                    } else {
                        print(red("list_prefix:x_handler unrecognized value for query type: "+labels[idx->type]));
                    }
                } else if(binary_op_catalog.has(ctx.root->type)) {
                    bool is_last_opperand = true;
                    if(ctx.root->left()->children.length()>1) {
                        is_last_opperand = (ctx.root->left()->right()==ctx.node);
                    }
                    if(is_last_opperand && ctx.root->type==push_id) {
                        //WARNING: If we push a node that already has children in this will break, find a better way to distinqiush
                        if(ctx.root->right()->children.empty()) { //Like l<<1
                            ctx.root->right()->value->store_value(ctx.value->store);
                        } else { //Like l << 1("one")
                            ctx.root->right()->value->store_value(ctx.value->store,ctx.root->right()->left()->name);
                        }
                    }
                }
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
                    //ctx.node->sub_type = reg_id(ctx.node->name);
                    print(red("WARNING: unrecognized node type: "+ctx.node->name));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.flag = standard_travel_pass(ctx.node->scope());
            };
            
            x_handlers[ctx_node_id] = [this](Context& ctx){ //ctx.node
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->node);
            };
            x_handlers[ctx_root_id] = [this](Context& ctx){ //ctx.root
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->root);
            };
            x_handlers[ctx_source_id] = [this](Context& ctx){ //ctx.source
                g_ptr<Value> v = ctx.node->value;
                v->type = string_id;
                v->size = sizeof(std::string);
                v->data = &ctx.sub->source;
            };

            x_handlers[ctx_node_id] = [this](Context& ctx){ //ctx.node
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->node);
            };
            x_handlers[ctx_root_id] = [this](Context& ctx){ //ctx.root
                ctx.node->value->set<g_ptr<Node>>(ctx.sub->root);
            };
            x_handlers[ctx_source_id] = [this](Context& ctx){ //ctx.source
                g_ptr<Value> v = ctx.node->value;
                v->type = string_id;
                v->size = sizeof(std::string);
                v->data = &ctx.sub->source;
            };

            x_handlers[general_length_id] = [this](Context& ctx){
                if(ctx.left->value->type==node_list_id){
                    ctx.node->value->set<int>(ctx.left->value->get<list<g_ptr<Node>>>().length());
                }
            };

            x_handlers[general_type_id] = [this](Context& ctx){ //node->type & value->type
                standard_accessor<uint32_t>(ctx, 
                    ctx.left->value->type==value_id?
                    ctx.left->value->get<g_ptr<Value>>()->type :
                        ctx.left->value->type==node_id?
                        ctx.left->value->get<g_ptr<Node>>()->type :
                        ctx.left->type
                );
            };

            x_handlers[node_name_id] = [this](Context& ctx){ //node->name
                standard_accessor<std::string>(ctx, 
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->name :
                    ctx.left->name
                );
            };

            x_handlers[node_scope_id] = [this](Context& ctx){ //node->scope
                standard_accessor<g_ptr<Node>>(ctx, 
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->scopes[0] :
                    ctx.left->scopes[0]
                );
            };

            x_handlers[node_value_id] = [this](Context& ctx){ //node->value
                standard_accessor<g_ptr<Value>>(ctx, 
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->value :
                    ctx.left->value
                );
            };

            x_handlers[node_in_scope_id] = [this](Context& ctx){ //node->in_scope
                standard_accessor<Node*>(ctx, 
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->in_scope :
                    ctx.left->in_scope
                );
            };

            x_handlers[node_owner_id] = [this](Context& ctx){ //node->owner
                standard_accessor<Node*>(ctx, 
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->owner :
                    ctx.left->owner
                );
            };

            x_handlers[node_children_id] = [this](Context& ctx){ //node->children
                list_op_on_node<g_ptr<Node>>(ctx,
                    ctx.left->value->type==node_id? 
                    ctx.left->value->get<g_ptr<Node>>()->children :
                    ctx.left->children
                );
            };

            x_handlers[read_file_id] = [this](Context& ctx){
                ctx.node->value->set<std::string>(readFile(ctx.node->left()->value->get<std::string>()));
            };

            x_handlers[find_in_labels_id] = [this](Context& ctx){
                std::string label = ctx.node->left()->value->get<std::string>();
                ctx.node->value->type = int_id;
                ctx.node->value->set<uint32_t>(0);
                for(auto e : labels.entrySet()) {
                    if(e.value==label) {
                        ctx.node->value->set<uint32_t>(e.key);
                        break;
                    }
                }
            };

            x_handlers[test_id] = [this](Context& ctx){
                print("This should do nothing");
            };

            x_handlers[dot_id] = [this](Context& ctx){
                process_node(ctx,ctx.node->left()); //We use process_node here to preserve the root context as we chain down
                process_node(ctx,ctx.node->right(),ctx.node->left());
                // standard_sub_process(ctx);
                ctx.node->sub_type = ctx.node->right()->type;
                ctx.node->value = ctx.node->right()->value;
            };

            n_handlers[scan_id] = [this](Context& ctx) {
                n_take_right(ctx,2);
            };
          
            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node->left());
                if(ctx.node->left()->value->is_true()) {
                    ctx.flag = standard_travel_pass(ctx.node->scope(),ctx.sub);
                }
                else if(ctx.node->scopes.length()>1) {
                    ctx.flag = standard_travel_pass(ctx.node->scopes[1],ctx.sub);
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
                    if(standard_travel_pass(ctx.node->scope(),ctx.sub)) {
                        ctx.flag = true;
                        break;
                    } 
                }
            };

            x_handlers[print_id] = [this](Context& ctx) {
                std::string toPrint = "";
                for(auto r : ctx.node->children) {
                    process_node(ctx, r);
                    if(r->name=="ROOTS") {
                        print_root(ctx.root);
                        continue;
                    }

                    if(r->value->type==node_id) {
                        toPrint.append(node_to_string(r->value->get<g_ptr<Node>>()));
                    } else {
                        toPrint.append(value_as_string(r->value));
                    }
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