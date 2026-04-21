#pragma once

#include "../modules/Q-Expression.hpp"

namespace GDSL {
    struct Function_Unit : public virtual Expression_Unit {

        size_t return_id = make_tokenized_keyword("return");
        size_t break_id = make_tokenized_keyword("break");
        
        void init() override {
            Expression_Unit::init();
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

            a_handlers[return_id] = [this](Context& ctx) {
                ctx.index++;
                while(ctx.index<ctx.result->length()) {
                    g_ptr<Node> take = ctx.result->take(ctx.index);
                    if(take->type==end_id) {
                        standard_direct_pass(ctx.node);
                        break;
                    }
                    ctx.node->children << take;
                }
            };

            x_handlers[return_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node->left());
                g_ptr<Node> on_scope = ctx.node->in_scope;
                if(on_scope) {
                    while(on_scope->owner && on_scope->owner->type!=func_decl_id) {
                        if(on_scope->owner) {
                            on_scope = on_scope->owner->in_scope;
                        } else {
                            on_scope = ctx.node->in_scope;
                            break;
                        }
                    }
                    if(ctx.node->left()) {
                        on_scope->owner->value->copy(ctx.node->left()->value);
                    }
                    ctx.flag = true;
                } else {
                    attach_error(ctx.node,trivial_error,"return:x_handler return is not in any scope");
                }
            };

            x_handlers[break_id] = [](Context& ctx) {
                ctx.flag = true;
            };
        }
    };
}