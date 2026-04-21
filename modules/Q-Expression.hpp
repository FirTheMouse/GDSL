#pragma once

#include "../modules/Q-Tokenizer.hpp"

namespace GDSL {
    struct Expression_Unit : public virtual Tokenizer_Unit {


        list<size_t> binary_op_catalog;

        size_t add_binary_operator(char c, const std::string& f, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            binary_op_catalog << id;
        
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");
    
            Handler handler = [this,decl_id,unary_id,c](Context& ctx){
                auto& children = ctx.node->children;
                standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
                if(children.length() == 2) {
                    g_ptr<Node> type_term = children[0];
                    g_ptr<Node> id_term = children[1];
    
                    ctx.node->quals << copy_as_token(ctx.node);
                    ctx.node->x = -1.0f; ctx.node->y = -1.0f;

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
            t_handlers[id] = handler;
            t_handlers[unary_id] = handler;
            
            return id;
        }

        size_t register_binary_operator(int use_id) {
            return add_binary_operator(' ',labels[use_id],use_id);
        }

        size_t plus_id = add_binary_operator('+',"PLUS");
        size_t dash_id = add_binary_operator('-',"DASH");
        size_t slash_id = add_binary_operator('/',"SLASH");
        size_t rangle_id = add_binary_operator('>',"RANGLE");
        size_t langle_id = add_binary_operator('<',"LANGLE");
        size_t equals_id = add_binary_operator('=', "ASSIGNMENT");
        size_t star_id = add_binary_operator('*',"STAR");
        size_t caret_id = add_binary_operator('^',"CARET");
        size_t amp_id = add_binary_operator('&',"AMPERSAND");
        size_t dot_id = add_binary_operator('.', "PROP_ACCESS");

        void assign_value(g_ptr<Value> to, g_ptr<Value> from) {
            void* to_dst = nullptr;
            void* from_src = nullptr;
            uint32_t from_type = 0;
            size_t from_size = 0;

            g_ptr<Value> to_sub = nullptr;
            g_ptr<Value> to_sub_sub = nullptr;
            if(to->sub_values.empty()) {
                to_dst = to->data;
            } else {
                to_sub = to->sub_values[0];
                if(to_sub->sub_values.empty()) {
                    to_dst = *(void**)to_sub->data;
                } else {
                    to_sub_sub = to_sub->sub_values[0];
                    if(to_sub->store) {
                        _note item_data;
                        if(to_sub_sub->type == int_id) {
                            if(to_sub_sub->address == -1) {
                                item_data = to_sub->store->get_note(to_sub_sub->get<int>());
                            } else {
                                item_data.index = to_sub_sub->get<int>();
                                item_data.sub_index = to_sub_sub->address;
                            }
                        } else if(to_sub_sub->type == string_id) {
                            item_data = to_sub->store->get_note(to_sub_sub->get<std::string>());
                        }
                        _column& col = to_sub->store->columns[item_data.index];
                        to_dst = col.get(item_data.sub_index);
                    } else if(to_sub_sub->type == int_id) {
                        int idx = to_sub_sub->get<int>();
                        to_dst = &(*(list<g_ptr<q_object>>*)to_sub->data)[idx];
                    } else if(to_sub_sub->type == string_id) {
                        std::string& key = to_sub_sub->get<std::string>();
                        to_dst = &(*(map<std::string,g_ptr<q_object>>*)to_sub->data)[key];
                    }
                }
            }

            g_ptr<Value> from_sub = nullptr;
            g_ptr<Value> from_sub_sub = nullptr;
            from_type = from->type;
            from_size = from->size;
            if(from->sub_values.empty()) {
                from_src = from->data;
            } else {
                from_sub = from->sub_values[0];
                if(from_sub->sub_values.empty()) {
                    from_src = *(void**)from_sub->data;
                } else {
                    from_sub_sub = from_sub->sub_values[0];
                    if(from_sub->store) {
                        _note item_data;
                        if(from_sub_sub->type==int_id) {
                            if(from_sub_sub->address==-1) {
                                item_data = from_sub->store->get_note(from_sub_sub->get<int>());
                            } else {
                                item_data.index = from_sub_sub->get<int>();
                                item_data.sub_index = from_sub_sub->address;
                                item_data.tag = 0; //Undefined, we can't know
                            }
                        } else if(from_sub_sub->type==string_id) {
                            item_data = from_sub->store->get_note(from_sub_sub->get<std::string>());
                        }
                        _column& col = from_sub->store->columns[item_data.index];
                        from_src = col.get(item_data.sub_index);
                        from_size = col.element_size;
                        from_type = item_data.tag;
                    }
                    else if(from_sub_sub->type==int_id) {
                        int idx = from_sub_sub->get<int>();
                        from_src = &(*(list<g_ptr<q_object>>*)from_sub->data)[idx];
                    } else if(from_sub_sub->type==string_id) {
                        std::string& key = from_sub_sub->get<std::string>();
                        from_src = &(*(map<std::string,g_ptr<q_object>>*)from_sub->data)[key];
                    }
                }
            }


            if(from_type == string_id) {
                *(std::string*)to_dst = *(std::string*)from_src;
            } else if(from_type == object_id) {
                *(g_ptr<q_object>*)to_dst = *(g_ptr<q_object>*)from_src;
            } else {
                memcpy(to_dst, from_src, from_size);
            }
        }

        void init() override {
            Tokenizer_Unit::init();

            x_handlers[plus_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->value->set<int>(
                    *(int*)ctx.node->left()->value->data
                    +
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };
    
            x_handlers[dash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->value->set<int>(
                    *(int*)ctx.node->left()->value->data
                    -
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };

            x_handlers[slash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                int divisor = *(int*)ctx.node->right()->value->data;
                if(divisor == 0) {
                    attach_error(ctx.node, major_error, "slash:x_handler division by zero");
                    return;
                }
                ctx.node->value->set<int>(
                    *(int*)ctx.node->left()->value->data / divisor
                );
            };

            x_handlers[star_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->value->set<int>(
                    *(int*)ctx.node->left()->value->data
                    *
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };
    
            x_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->value->set<bool>(
                    *(int*)ctx.node->left()->value->data
                    >
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = bool_id;
                ctx.node->value->size = 1;
            };
    
            x_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->value->set<bool>(
                    *(int*)ctx.node->left()->value->data
                    <
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = bool_id;
                ctx.node->value->size = 1;
            };

            t_handlers[equals_id] = [this](Context& ctx){ //So we don't turn things into declerations
                standard_sub_process(ctx);
                if(!ctx.node->left()||!ctx.node->right()) return;
                ctx.node->name = ctx.node->left()->name+"="+ctx.node->right()->name;
            };
            x_handlers[equals_id] = [this](Context& ctx) {
                backwards_sub_process(ctx);

                if(!ctx.node->left()||!ctx.node->right()) return;

                if(!ctx.node->left()->value->data) {
                    ctx.node->left()->value->size = ctx.node->right()->value->size;
                    ctx.node->left()->value->type = ctx.node->right()->value->type;
                    ctx.node->left()->value->quals = ctx.node->right()->value->quals;
                    ctx.node->left()->value->data = malloc(ctx.node->right()->value->size);
                }  
                
                if(ctx.node->left()->value->type != ctx.node->right()->value->type) {
                    ctx.node->left()->value->size = ctx.node->right()->value->size;
                    ctx.node->left()->value->type = ctx.node->right()->value->type;
                    ctx.node->left()->value->quals = ctx.node->right()->value->quals;
                } 
                if(ctx.node->right()->value->data&&ctx.node->left()->value->data)
                    memcpy(ctx.node->left()->value->data, ctx.node->right()->value->data, ctx.node->right()->value->size);
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
        }
    };
}