#pragma once

#include "../modules/Q-Tokenizer.hpp"

namespace GDSL {
    struct Expression_Unit : public virtual Tokenizer_Unit {


        list<size_t> binary_op_catalog;

        size_t add_binary_operator(char c, const std::string& f) {
            size_t id = add_token(c,f);
            binary_op_catalog << id;
        
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");
    
            Handler handler = [this,decl_id,unary_id,c](Context& ctx){
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
            t_handlers[id] = handler;
            t_handlers[unary_id] = handler;
            
            return id;
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

        void init() override {
            Tokenizer_Unit::init();

            x_handlers[plus_id] = [this](Context& ctx){
                standard_sub_process(ctx);
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
                ctx.node->value->set<int>(
                    *(int*)ctx.node->left()->value->data
                    /
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };

            x_handlers[star_id] = [this](Context& ctx){
                standard_sub_process(ctx);
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
                ctx.node->value->set<bool>(
                    *(int*)ctx.node->left()->value->data
                    >
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };
    
            x_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node->value->set<bool>(
                    *(int*)ctx.node->left()->value->data
                    <
                    *(int*)ctx.node->right()->value->data
                );
                ctx.node->value->type = ctx.node->left()->value->type;
                ctx.node->value->size = ctx.node->left()->value->size;
            };

            t_handlers[equals_id] = [this](Context& ctx){ //So we don't turn things into declerations
                standard_sub_process(ctx);
                ctx.node->name = ctx.node->left()->name+"="+ctx.node->right()->name;
            };
            x_handlers[equals_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                if(!ctx.node->left()->value->data) {
                    ctx.node->left()->value->size = ctx.node->right()->value->size;
                    ctx.node->left()->value->type = ctx.node->right()->value->type;
                    ctx.node->left()->value->data = malloc(ctx.node->right()->value->size);
                }
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