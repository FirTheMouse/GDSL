#pragma once

#include "../modules/Q-Tokenizer.hpp"

namespace GDSL {
    struct Precedence_Unit : public virtual Tokenizer_Unit {
        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power.put(id,lbp);
            right_binding_power.put(id,rbp);
        }

        void init() override {
            Tokenizer_Unit::init();

            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(comma_id);

            a_handlers.default_function = [this](Context& ctx) {
                int left_bp = left_binding_power.getOrDefault(ctx.node->type, -1);
                int right_bp = right_binding_power.getOrDefault(ctx.node->type, -1);
                
                if(left_bp == -1 && right_bp == -1) return;
                
                if(ctx.left && left_bp > 0 && !discard_types.has(ctx.left->type)) {
                    int left_left_bp = left_binding_power.getOrDefault(ctx.left->type, -1);
                    int left_right_bp = right_binding_power.getOrDefault(ctx.left->type, -1);
    
                    bool right_associative = right_bp < left_bp; //lbp > rbp means right assoc
                    bool should_steal = left_bp > (right_associative ? left_right_bp : left_left_bp);
                    
                    if(!ctx.left->children.empty()) {
                        if(ctx.left->children.length()==1) {
                            should_steal = true;
                        }
                        else if(discard_types.has(ctx.left->children.last()->type)) {
                            goto otter;
                        }
                    }
    
                    if(left_right_bp!=-1 && should_steal) {
                        if(ctx.left->children.length()>1) {
                            ctx.node->children << ctx.left->children.pop();
                        }
                        ctx.left->children << ctx.result->take(ctx.index);
                    } else {
                        ctx.node->children << ctx.left;
                        ctx.result->removeAt(ctx.index - 1);
                    }
                } else {
                    otter:
                    if(!discard_types.has(ctx.node->type))
                        ctx.node->type = to_unary_id(ctx.node->type);
                    ctx.index++;
                }
                
                if(right_bp != -1 && ctx.index < ctx.result->length()) {
                    g_ptr<Node> next = ctx.result->get(ctx.index);
                    int next_lbp = left_binding_power.getOrDefault(next->type, -1);
                    if(next_lbp == -1 && !discard_types.has(next->type)) { //It's an atom so we grab it
                        ctx.node->children << ctx.result->take(ctx.index);
                    } 
                }
                ctx.index--;
            };
    
            left_binding_power.put(lparen_id,10);
    
            a_handlers[rparen_id] = [this](Context& ctx) {
                ctx.result->removeAt(ctx.index);
                int i = ctx.index-1;
                list<g_ptr<Node>> gathered;
                while(i>=0) {
                    g_ptr<Node> on = ctx.result->get(i);
                    while(!on->children.empty()&&on->type!=lparen_id) {
                        on = on->children.last();
                    }
                    if(on->type==lparen_id) {
                        gathered.reverse();
                        bool was_given_children = false;
                        if(on->children.empty()) {
                            on->children << gathered;
                            was_given_children = true;
                        }
                        on->copy(on->children.take(0));
                        if(!was_given_children) {
                            if(on->children.empty()) {
                                on->children << gathered;
                            } else { //This case if for things like int main(int a), where we want the gathered to go under main, not int
                                on->children.last()->children << gathered;
                            }
                        }
                        ctx.index = i;
                        break;
                    } else {
                        gathered << ctx.result->take(i);
                        i--;
                    }
                }
            };
    
            a_handlers[identifier_id] = [this](Context& ctx){
                if(ctx.left && ctx.left->type == identifier_id) {
                    // log("Identifier handler, left looks like ",node_info(ctx.left)," my node looks like ",node_info(ctx.node),", the index is ",ctx.index,"/",ctx.result->length());
                    while(ctx.index < ctx.result->length() && ctx.result->get(ctx.index)->type == identifier_id) {
                        //log("Taking a new child: ",node_info(ctx.result->get(ctx.index)));
                        ctx.left->children << ctx.result->take(ctx.index);
                    }
                    ctx.index--;
                } 
            };
        }
    };
}