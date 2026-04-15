#pragma once

#include "../modules/Q-Function.hpp"
#include "../modules/Q-Scope.hpp"
#include "../modules/Q-Precedence.hpp"

namespace GDSL {
    struct Starter_DSL_Frontend : public virtual Scope_Unit, public virtual Function_Unit, public virtual Precedence_Unit  {
        Starter_DSL_Frontend() { init(); }

        Stage& n_handlers = reg_stage("naming");

        void n_take_right(Context& ctx, int amt) {
            for(int i = 0; i<amt; i++) {
                if(ctx.index + 1 < ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index + 1);
                }
            }
        }

        void init() override {
            Scope_Unit::init();
            Function_Unit::init();
            Precedence_Unit::init();

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

            t_handlers[end_id] = [this](Context& ctx){
                ctx.result->removeAt(ctx.index);
                ctx.index--;
            };

            t_handlers[comma_id] = [this](Context& ctx){
                ctx.result->removeAt(ctx.index);
                ctx.index--;
            };

            n_handlers.default_function = [this](Context& ctx){
                standard_sub_process(ctx);
            };
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
        }

        void print_root(g_ptr<Node> root){
            print(node_to_string(root));
            for(auto child_scope : root->scopes) {
                print_root(child_scope);
            }
        };

        g_ptr<Node> process(const std::string& code) override { 
            g_ptr<Node> root = make<Node>();
            root->children = tokenize(code);

            start_stage(a_handlers);
            standard_direct_pass(root);

            a_pass_resolve_keywords(root->children);

            start_stage(n_handlers);
            standard_direct_pass(root);

            start_stage(s_handlers);
            parse_scope(root);

            print("==PROCCESSING FINISHED==");
            print_root(root);
            
            return root;
        };
        
        void run(g_ptr<Node> root) override { 
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(d_handlers);
            discover_symbols(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);
    
            start_stage(e_handlers);
            standard_backwards_pass(root);
            #if PRINT_ALL
                span->print_all();
                print("==FINAL FORM==");
                print_root(root);
            #endif

            start_stage(x_handlers);
            standard_travel_pass(root);
        };
    };
}