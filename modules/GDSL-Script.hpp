#pragma once

#include "../modules/GDSL-Starter.hpp"

namespace GDSL {
    struct Q_Script_Unit : public virtual Starter_DSL_Frontend {
        Q_Script_Unit() { init(); }

        size_t node_block_id = reg_id("NODE_BLOCK");

        size_t travel_pass_id = make_keyword("travel_pass");
        size_t direct_pass_id = make_keyword("direct_pass");

        size_t push_id = register_binary_operator(reg_id("push"));
        size_t is_id = register_binary_operator(reg_id("is"));
        size_t is_not_id = register_binary_operator(reg_id("is_not"));
        size_t scan_id = make_tokenized_keyword("scan");
        size_t on_id = make_tokenized_keyword("on");
        size_t in_id = make_contextual_keyword("in");

        size_t node_id = add_type("Node",8);
        size_t value_id = add_type("Value",8);
        size_t store_id = add_type("Store",8);

        size_t node_list_id = add_type("Node_list",8);
        size_t value_list_id = add_type("Value_list",8);
        size_t node_map_id = add_type("Node_map",8);
        size_t value_map_id = add_type("Value_map",8);

        size_t test_id = make_keyword("test");
        size_t ctx_node_id = add_function("node",node_id);
        size_t ctx_root_id = make_keyword("root",8,"",node_id);
        size_t ctx_source_id = make_keyword("source",sizeof(std::string),"",string_id);

        size_t general_type_id = make_keyword("type",4,"",int_id);
        size_t general_length_id = make_keyword("length",4,"",int_id);

        size_t node_scope_id = make_keyword("scope",8,"",node_id);
        size_t node_name_id = make_keyword("name",sizeof(std::string),"",string_id);
        size_t node_mute_id = make_keyword("mute",4,"",bool_id);
        size_t node_in_scope_id = make_keyword("in_scope",8,"",node_id);
        size_t node_owner_id = make_keyword("owner",8,"",node_id);
        size_t node_children_id = make_keyword("children");
        size_t node_value_id = make_keyword("value",8,"",value_id);
        size_t node_node_table_id = make_keyword("node_table");
        size_t node_value_table_id = make_keyword("value_table");

        size_t value_store_id = make_keyword("store",8,"",store_id);

        size_t xor_encrypy_id = make_tokenized_keyword("xor_encrypt");

        size_t read_file_id = make_tokenized_keyword("read_file");
        size_t find_in_labels_id = add_function("find_in_labels");

        size_t type_scope_id = reg_id("TYPE_SCOPE");
        size_t print_id = add_function("print");
        size_t while_id = add_scoped_keyword("while", 2);
        size_t if_id = add_scoped_keyword("if", 2);
        size_t else_id = add_scoped_keyword("else", 1);

        size_t test_test_id = make_tokenized_keyword("test_test");


        template<typename T>
        void list_op_on_node(Context& ctx, list<T>& accessor, uint32_t stores_type, uint32_t container_type) {
            if(ctx.node->left()) {
                int idx = ctx.node->left()->value->get<int>();

                if(ctx.node->value->sub_values.empty()) {
                    g_ptr<Value> sub = make<Value>();
                    sub->data = &accessor;
    
                    sub->sub_values << ctx.node->left()->value;
                    ctx.node->value->sub_values << sub;
                } 
        
                ctx.node->value->type = stores_type;
                ctx.node->value->size = 8;
                if(idx < accessor.length()) {
                    ctx.node->value->set<T>(accessor[idx]);
                } else {
                    print(red("list_op_on_node: index out of bounds"));
                }
            } else {
                if(!ctx.node->value->sub_values.empty()) {
                    if(!ctx.node->value->sub_values[0]->sub_values.empty()) {
                        ctx.node->value->sub_values[0]->sub_values.clear();
                    }
                } else {
                    if(ctx.node->value->sub_values.empty()) {
                        g_ptr<Value> sub = make<Value>();
                        sub->type = container_type;
                        sub->size = 8;
                        sub->data = &accessor;
                        ctx.node->value->sub_values << sub;
                    }
                }
                ctx.node->value->type = container_type;
                ctx.node->value->size = 8;
                ctx.node->value->data = &accessor;
            }
        }

        template<typename T>
        void map_op_on_node(Context& ctx, map<std::string,T>& accessor, uint32_t stores_type, uint32_t container_type) {
            if(ctx.node->left()) {
                ctx.node->value->type = stores_type;

                if(ctx.node->value->sub_values.empty()) {
                    g_ptr<Value> sub = make<Value>();
                    sub->data = &accessor;
    
                    sub->sub_values << ctx.node->left()->value;
                    ctx.node->value->sub_values << sub;
                } 

                ctx.node->value->set<T>(accessor[ctx.node->left()->value->get<std::string>()]);
            } else {
                if(!ctx.node->value->sub_values.empty()) {
                    if(!ctx.node->value->sub_values[0]->sub_values.empty()) {
                        ctx.node->value->sub_values[0]->sub_values.clear();
                    }
                } else {
                    if(ctx.node->value->sub_values.empty()) {
                        g_ptr<Value> sub = make<Value>();
                        sub->type = container_type;
                        sub->size = 8;
                        sub->data = &accessor;
                        ctx.node->value->sub_values << sub;
                    }
                }
                ctx.node->value->type = container_type;
                ctx.node->value->size = 8;
                ctx.node->value->data = &accessor;
            }
        }

        template<typename T>
        void standard_accessor(Context& ctx, T& accessor) {
            ctx.node->value->set<T>(accessor);
            ctx.node->value->size = sizeof(T);
            if(ctx.node->value->sub_values.empty()) {
                g_ptr<Value> sub = make<Value>();
                sub->data = &accessor;
                sub->type = object_id;
                sub->size = 8;
                ctx.node->value->sub_values << sub;
            }
        }


        g_ptr<Value> resolve_value(g_ptr<Value> val, void*& dst, uint32_t& type, uint32_t& size) {            
            if(val->sub_values.empty() || !val->sub_values[0]->isActive()) {
                dst = val->data;
                return nullptr;
            }
            
            g_ptr<Value> sub = val->sub_values[0];
            
            if(sub->sub_values.empty() || !sub->sub_values[0]->isActive()) {
                dst = sub->data;
                return sub;
            }
            
            g_ptr<Value> sub_sub = sub->sub_values[0];
            
            if(sub->store) {
                _note item_data;
                if(sub_sub->type == int_id) {
                    if(sub_sub->address == -1) {
                        item_data = sub->store->get_note(sub_sub->get<int>());
                    } else {
                        item_data.index = sub_sub->get<int>();
                        item_data.sub_index = sub_sub->address;
                    }
                } else if(sub_sub->type == string_id) {
                    item_data = sub->store->get_note(sub_sub->get<std::string>());
                }
                _column& col = sub->store->columns[item_data.index];
                dst = col.get(item_data.sub_index);
                size = col.element_size;
                type = item_data.tag;
            } else if(sub_sub->type == int_id) {
                dst = &(*(list<g_ptr<q_object>>*)sub->data)[sub_sub->get<int>()];
                type = object_id;
                size = 8;
            } else if(sub_sub->type == string_id) {
                std::string& key = sub_sub->get<std::string>();
                dst = &(*(map<std::string,g_ptr<q_object>>*)sub->data)[key];
                type = object_id;
                size = 8;
            }
            return sub;
        }

        void init() override {
            Starter_DSL_Frontend::init();
            //span->log_everything = true;

            // set_binding_powers(with_id,2,4);
            // set_binding_powers(size_id,6,6);

            tokenizer_functions['<'] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c=='<') {
                    ctx.node = make<Node>(push_id);
                    ctx.node->x = at_x;
                    ctx.node->y = at_y;
                    ctx.node->name = "<<";
                    ctx.result->push(ctx.node);
                    at_x+=1.0f;
                    ctx.index++;
                } else {
                    ctx.node = make<Node>(langle_id,c);
                    ctx.node->x = at_x;
                    ctx.node->y = at_y;
                    ctx.result->push(ctx.node);
                }
            };
            set_binding_powers(push_id,4,6);

            tokenizer_functions['='] = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c=='=') {
                    ctx.node = make<Node>(is_id);
                    ctx.node->name = "==";
                    ctx.node->x = at_x;
                    ctx.node->y = at_y;
                    ctx.result->push(ctx.node);
                    at_x+=1.0f;
                    ctx.index++;
                } else {
                    ctx.node = make<Node>(equals_id,c);
                    ctx.node->x = at_x;
                    ctx.node->y = at_y;
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



            r_handlers[identifier_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node->value);
            };


            x_handlers[identifier_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node->value);
            };
            x_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node->value);
            };




            x_handlers[push_id] = [this](Context& ctx){
                backwards_sub_process(ctx);
                if(ctx.node->left()->value->type==store_id) {
                    if(ctx.node->right()->children.length()==1) {
                        print(node_to_string(ctx.node->right()));
                        ctx.node->right()->value->store_value(
                            ctx.node->left()->value->store, 
                            ctx.node->right()->left()->value->get<std::string>()
                        );
                    } else {
                        ctx.node->right()->value->store_value(ctx.node->left()->value->store);
                    }
                } else if(ctx.node->left()->value->type==node_list_id) {
                    if(ctx.node->right()->value->type==node_id) {
                        ctx.node->left()->value->get<list<g_ptr<Node>>>() << ctx.node->right()->value->get<g_ptr<Node>>();
                    } else {
                        ctx.node->left()->value->get<list<g_ptr<Node>>>() << ctx.node->right();
                    }
                } else if(ctx.node->left()->value->type == value_list_id) {
                    if(ctx.node->right()->value->type==value_id) {
                        ctx.node->left()->value->get<list<g_ptr<Value>>>() << ctx.node->right()->value->get<g_ptr<Value>>();
                    }
                } else if(ctx.node->left()->value->type == node_map_id) {
                    if(ctx.node->right()->value->type == node_id) {
                        ctx.node->left()->value->get<map<std::string,g_ptr<Node>>>().put(
                            ctx.node->right()->left()->value->get<std::string>(), 
                            ctx.node->right()->value->get<g_ptr<Node>>()
                        );
                    } else {
                        ctx.node->left()->value->get<map<std::string,g_ptr<Node>>>().put(
                            ctx.node->right()->left()->value->get<std::string>(), 
                            ctx.node->right()
                        );
                    }
                } else if(ctx.node->left()->value->type == value_map_id) {
                    ctx.node->left()->value->get<map<std::string,g_ptr<Value>>>().put(
                        ctx.node->right()->left()->value->get<std::string>(), 
                        ctx.node->right()->value
                    );
                }
            };

            x_handlers[equals_id] = [this](Context& ctx) {
                standard_sub_process(ctx);


                g_ptr<Value> from = ctx.node->right()->value;
                g_ptr<Value> to = ctx.node->left()->value;

                if(!to->data) {
                    to->size = from->size;
                    to->type = from->type;
                    to->quals = from->quals;
                    to->data = malloc(from->size);
                }  
                if(to->type != from->type) {
                    to->size = from->size;
                    to->type = from->type;
                    to->quals = from->quals;
                } 

                void* from_src = nullptr;
                void* to_dst = nullptr;
                uint32_t from_type = from->type;
                uint32_t from_size = from->size;
                uint32_t to_type = to->type;
                uint32_t to_size = to->size;
                
                g_ptr<Value> from_sub = resolve_value(from, from_src, from_type, from_size);
                g_ptr<Value> to_sub = resolve_value(to, to_dst, to_type, to_size);

                if(to_sub&&from_sub) {
                    if(to_sub->type == node_list_id || to_sub->type == value_list_id) {
                        *(list<g_ptr<q_object>>*)to_sub->data = *(list<g_ptr<q_object>>*)from_sub->data;
                        return;
                    } else if(to_sub->type == node_map_id || to_sub->type == value_map_id) {
                        *(map<std::string,g_ptr<q_object>>*)to_sub->data = *(map<std::string,g_ptr<q_object>>*)from_sub->data;
                        return;
                    }
                }

                if(from_type == string_id) {
                    *(std::string*)to_dst = *(std::string*)from_src;
                } else if(from_type == object_id || from_type == node_id || from_type == value_id) {
                    *(g_ptr<q_object>*)to_dst = *(g_ptr<q_object>*)from_src;
                } else {
                    memcpy(to_dst, from_src, from_size);
                }
            };


            r_handlers[to_prefix_id(store_id)] = [this](Context& ctx){
                if(!ctx.value->store) {
                    ctx.value->store = make<Type>();
                    ctx.value->type = store_id;
                    ctx.value->set<g_ptr<Type>>(ctx.value->store);
                }
            };

            x_handlers[to_prefix_id(store_id)] = [this](Context& ctx){
                if(ctx.node->left()) { //Like l(3)
                    process_node(ctx,ctx.node->left());
                    g_ptr<Value> idx = ctx.node->left()->value;

                    if(ctx.node->value->sub_values.empty()) { //Temporary: at the momment of creation, r value already set main value to be the refrence so we just demote it for the index access case
                        g_ptr<Value> new_sub = make<Value>();
                        new_sub->copy(ctx.value);
                        new_sub->sub_values << idx;
                        ctx.value->sub_values << new_sub;
                    }

                    if(idx->type==int_id) {
                        ctx.value->query_store(idx->get<int>(),ctx.value->sub_values[0]->store);
                    } else if(idx->type==string_id) {
                        ctx.value->query_store(idx->get<std::string>(),ctx.value->sub_values[0]->store);
                    } else if(idx->type==store_id) {
                        //Do nothing
                    } else {
                        print(red("list_prefix:x_handler unrecognized value for query type: "+labels[idx->type]));
                    }
                }
            };


       

            scope_link_handlers[string_id] = [this](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                standard_scope_link_handler(new_scope,current_scope,owner_node);
                owner_node->type = node_block_id;
                new_scope->name = owner_node->name;
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                a_pass_resolve_keywords(ctx.node->scope()->children,node_block_id);
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

            x_handlers[node_mute_id] = [this](Context& ctx){ //node->scope
                standard_accessor<bool>(ctx, 
                    ctx.left->value->type==node_id?
                        ctx.left->value->get<g_ptr<Node>>()->mute :
                        ctx.left->mute
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

            x_handlers[node_children_id] = [this](Context& ctx){
                list_op_on_node<g_ptr<Node>>(ctx,
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->children :
                    ctx.left->children,
                    node_id, node_list_id
                );
            };
            
            x_handlers[node_value_table_id] = [this](Context& ctx){
                map_op_on_node<g_ptr<Value>>(ctx,
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->value_table :
                    ctx.left->value_table,
                    value_id, value_map_id
                );
            };
            x_handlers[node_node_table_id] = [this](Context& ctx){
                map_op_on_node<g_ptr<Node>>(ctx,
                    ctx.left->value->type==node_id?
                    ctx.left->value->get<g_ptr<Node>>()->node_table :
                    ctx.left->node_table,
                    node_id, node_map_id
                );
            };

            x_handlers[read_file_id] = [this](Context& ctx){
                if(!ctx.node->left()) {
                    attach_error(ctx.node, major_error, "read_file:x_handler no filename provided");
                    return;
                } else if(!ctx.node->left()->value->data) {
                    attach_error(ctx.node, major_error, "read_file:x_handler left has no data to read filename from");
                }
                std::string contents = "";
                std::string filename = "";
                try {
                    filename = ctx.node->left()->value->get<std::string>();
                    contents = readFile(filename);
                } catch(std::exception e) {
                    attach_error(ctx.node, major_error, "read_file:x_handler could not find file: "+filename);
                }
                ctx.node->value->set<std::string>(contents);
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

                g_ptr<Node> n = make<Node>(); n->name = "n";
                g_ptr<Node> v = make<Node>(); v->name = "v";
                g_ptr<Node> k = make<Node>(); k->name = "k";
                g_ptr<Node> j = make<Node>(); j->name = "j";
                g_ptr<Node> m = make<Node>(); m->name = "m";
                n->children << v;
                n->children << k;
                j = v;
                j->name = "Joe";
                n->children[1] = j;
                print(node_to_string(n));
                m->children = n->children;
                print(node_to_string(m));
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
                
                bool found_a_value = node->find_value_in_scope();
                bool is_qualifier = node->value->type!=0;


                int root_idx = -1;
                if(is_qualifier && node->value->sub_type != 0) {
                    decl_value->quals << value_to_qual(node->value);
                    decl_value->quals.last()->name = node->name;
                    decl_value->quals.last()->x = node->x;
                    decl_value->quals.last()->y = node->y;
                    for(int i = 0; i < node->children.length(); i++) {
                        g_ptr<Node> c = node->children[i];
                        c->find_value_in_scope();
                        if(c->value->type!=0) {
                            decl_value->quals << value_to_qual(c->value);
                            decl_value->quals.last()->name = c->name;
                            decl_value->quals.last()->x = c->x;
                            decl_value->quals.last()->y = c->y;
                        } else {
                            root_idx = i;
                            break;
                        }
                    }
                    if(root_idx!=-1) {
                        g_ptr<Node> root = node->children[root_idx];
                        node->name = root->name;
                        node->x = root->x;
                        node->y = root->y;
                        for(int i = root_idx+1; i < node->children.length(); i++) {
                            g_ptr<Node> c = node->children[i];
                            c->find_value_in_scope();
                            if(c->value->type!=0) {
                                node->quals << value_to_qual(c->value);
                                node->quals.last()->name = c->name;
                                node->quals.last()->x = c->x;
                                node->quals.last()->y = c->y;
                            } 
                        }
                        node->children = node->children.take(root_idx)->children;
                    }
                }
                
                if(!node->scope()) { //Defer, the r_stage will do this later for scoped nodes
                    standard_sub_process(ctx);
                }

                if(keywords.hasKey(node->name)) {
                    if(node->value->sub_type!=0) {
                        node->type = node->value->sub_type;
                        return;
                    }
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