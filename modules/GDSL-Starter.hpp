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
                ctx.index+=(i+1);
                if(ctx.index < ctx.result->length()) {
                    process_node(ctx,ctx.result->get(ctx.index));
                }
                ctx.index = ctx.result->find(ctx.node); 
            }

            for(int i = 0; i<amt; i++) {
                if(ctx.index + 1 < ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index + 1);
                } 
            }
        }

        void n_take_left(Context& ctx, int amt) {

            //0:a - 1:b - 2:c - 3:d
            //c takes i-1, as idx 2
            //0:a - 1:c - 2:d
            //idx++ lands on null
            //idx-- lands on c

            for(int i = 0; i<amt; i++) {
                // log("Current before result from ",ctx.node->name," taking left");
                // for(int c=0;c<ctx.result->length();c++) {
                //     log((c==ctx.index?"->":"  "),c,":\n",node_to_string(ctx.result->get(c),2));
                // }
                if(ctx.index - 1 >= 0) {
                    ctx.node->children.insert(ctx.result->take(ctx.index - 1),0);
                    ctx.index--;
                }
                //log("Current post result from ",ctx.node->name," taking left");
                // for(int c=0;c<ctx.result->length();c++) {
                //     log((c==ctx.index?"->":"  "),c,":\n",node_to_string(ctx.result->get(c),2));
                // }
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
                if(ctx.index>0) {
                    ctx.index--;
                    ctx.result->get(ctx.index)->quals << copy_as_token(ctx.result->take(ctx.index+1));
                } else if(ctx.index<ctx.result->length()) {
                    ctx.index++;
                    ctx.result->get(ctx.index)->quals << copy_as_token(ctx.result->take(ctx.index-1));
                }
            };
            t_handlers[comma_id] = t_handlers[end_id];

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

            // print("==PROCCESSING FINISHED==");
            // print_root(root);
            //span->print_all();
            
            return root;
        };

        void stamp_onto_page(g_ptr<Node> node, list<std::string>& lines) {
            if(node->x>=0.0f&&node->y>=0.0f) {
                int x = (int)node->x;
                int y = (int)node->y;
                while(y>=lines.length()) {lines << "";}
                while((x+node->name.length())>=lines[y].length()) lines[y]+=" ";
                for(char c : node->name) lines[y][x++] = c;
            }
            for(auto c : node->children) stamp_onto_page(c,lines);
            for(auto q : node->quals) stamp_onto_page(q,lines);
            for(auto s : node->scopes) stamp_onto_page(s,lines);
            if(node->value) {
                for(auto q : node->value->quals) stamp_onto_page(q,lines);
            }
        }

        std::string nodenet_to_string(g_ptr<Node> root) {
            list<std::string> lines;
            stamp_onto_page(root,lines);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        
        void run(g_ptr<Node> root) override { 
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(d_handlers);
            discover_symbols(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);
    
            start_stage(e_handlers);
            standard_backwards_pass(root);

            start_stage(i_handlers);
            standard_travel_pass(root);

            #if PRINT_ALL
                //span->print_all();
                print("==FINAL FORM==");
                print_root(root);
            #endif

            start_stage(x_handlers);
            standard_travel_pass(root);

            // print("AS STRING");
            // print(nodenet_to_string(root));


        };
    };
}