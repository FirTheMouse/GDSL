#pragma once

#include "../core/GDSL.hpp"
#include "../modules/Q-Function.hpp"

//GDSL-LISP
//Quick implmentation of LISP, I've never actually used the language, so I had to learn the syntax as I went
//It just took 5 hours to get this working, and it was a very nice break from GDSL-C.
//The a_stage in particular, just being able to have it all be parens, no Pratt to wrangle with.


namespace GDSL {

    struct LISP_Unit : public Function_Unit {
        size_t add_scoped_keyword(const std::string& name, int scope_prec)
        {
            size_t id = make_tokenized_keyword(name);
            s_handlers[id] = [this](Context& ctx) {
                g_ptr<Node> scope = ctx.node->in_scope->spawn_sub_scope();
                for(auto c : ctx.node->children) {
                    c->place_in_scope(scope.getPtr());
                }
                scope->owner = ctx.node.getPtr();
                ctx.node->scopes << scope.getPtr();
                scope->name = ctx.node->name;
            };
            return id;
        }


        //Need to define all these globally so that the function pointers can access them
        size_t if_id = 0;
        size_t for_id = 0;
        size_t else_id = 0;
        size_t while_id = 0;
        size_t print_id = 0;
        size_t defun_id = 0;
        size_t wubless_qual = add_qualifier("wubless");
        size_t wubfull_qual = add_qualifier("wubfull");

        Log::Line timer; 
        std::string output_string = "";

        void init() override {
            span = make<Log::Span>(); //<- This is the logging primitve, what all the log, newline, endline, work with
            Function_Unit::init();

            for(int i=0;i<binary_op_catalog.length();i++) {
                t_handlers[binary_op_catalog[i]] = [this](Context& ctx) {
                    standard_sub_process(ctx);
                    if(!ctx.node->value) 
                        ctx.node->value = make<Value>();
                    ctx.node->value->copy(ctx.node->left()->value);
                }; 
            }
            

            x_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                process_node(ctx, ctx.node->scope()->owner->children.last());
            };
            

            defun_id = add_scoped_keyword("defun",2);
            t_handlers[defun_id] = [this](Context& ctx) {
                g_ptr<Node> node = ctx.node;

                node->value = make<Value>();
                node->name = node->children.take(0)->name;

                g_ptr<Node> param_block = node->children[0];
                auto distribute_param = [this](g_ptr<Node> param) {
                    param->type = var_decl_id;
                    param->value = make<Value>();
                    param->value = param->in_scope->distribute_value(param->name, param->value);
                };
                for(int i=param_block->children.length()-1;i>=0;i--) {
                    g_ptr<Node> c = param_block->children.take(i);
                    node->children.insert(c,1);
                    distribute_param(c);
                }
                distribute_param(param_block);


                g_ptr<Node> body = node->children.last();
                process_node(ctx, body);


                node->type = func_decl_id;
                node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
                body->value->type_scope = node->scope().getPtr();
                node->value = node->in_scope->distribute_value(node->name,body->value);
            };


            if_id = add_scoped_keyword("if", 0);
            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node->children[0]); //Condition
                if(ctx.node->children[0]->value->is_true()) {
                    process_node(ctx, ctx.node->children[1]); //True
                    if(ctx.node->children[1]->value)
                        ctx.node->value->copy(ctx.node->children[1]->value);
                } else if(ctx.node->children.length() > 2) {
                    process_node(ctx, ctx.node->children[2]); //False
                    if(ctx.node->children[2]->value)
                        ctx.node->value->copy(ctx.node->children[2]->value);
                }
            };

            t_handlers[identifier_id] = [this](Context& ctx) {
                g_ptr<Node> node = ctx.node;
                g_ptr<Value> decl_value = make<Value>();
                
                bool had_a_value = (bool)(node->value);
                bool had_a_scope = (bool)(node->scope());
                bool found_a_value = node->find_value_in_scope();
                bool is_qualifier = node->value_is_valid();

                standard_sub_process(ctx);

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
                    node->type = func_decl_id;
                    node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
                    node->value->type_scope = node->scope().getPtr();
                    node->value = node->in_scope->distribute_value(node->name,node->value);
                    node->value->sub_type = 0;
                } else {
                    has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
                    if(has_scope) {
                        node->type = func_call_id;
                        node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                        if(node->value->type_scope)
                            node->scopes[0] = node->value->type_scope; //Swap to the type scope
                    } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
                        node->find_value_in_scope();
                    } else {                                         

                    }
                }
            };

            a_handlers[rparen_id] = [this](Context& ctx) {
                ctx.result->removeAt(ctx.index);
                int i = ctx.index - 1;
                list<g_ptr<Node>> gathered;
                
                while(i >= 0 && ctx.result->get(i)->type != lparen_id) {
                    gathered << ctx.result->take(i);
                    i--;
                }
                
                ctx.result->removeAt(i); 
                gathered.reverse();
                
                g_ptr<Node> op = gathered.take(0);
                op->children << gathered;
                
                ctx.result->insert(op, i);
                ctx.index = i;
            };

            a_default_function = [this](Context& ctx){

            };

            s_default_function = [](Context& ctx){};
            t_default_function = [this](Context& ctx){
                standard_sub_process(ctx);
            };
            r_default_function = [this](Context& ctx){
                standard_sub_process(ctx);
            };
            d_default_function = [this](Context& ctx){
                for(auto c : ctx.node->children) {
                    discover_symbol(c,ctx.root);
                }
            };
            x_default_function = [](Context& ctx){};

            print_id = add_function("print");
            x_handlers[print_id] = [this](Context& ctx) {
                std::string toPrint = "";
                for(auto r : ctx.node->children) {
                    process_node(ctx, r);
                    toPrint.append(value_as_string(r->value));
                }
                print(toPrint);
                output_string.append(toPrint);
            };
        }

        std::string code_store = "";

        g_ptr<Node> process(const std::string& code) override {
            code_store = code;
            timer.start();
            print("TOKENIZE");
            g_ptr<Node> root = make<Node>();
            root->name = "GLOBAL";
            root->is_scope = true;
            root->children << tokenize(code);
            print("A STAGE");
            start_stage(&a_handlers,a_default_function);
            standard_direct_pass(root);

            a_pass_resolve_keywords(root->children);

            print("S STAGE");
            start_stage(&s_handlers,s_default_function);
            for(auto n : root->children) {
                n->place_in_scope(root.getPtr());
                print(node_to_string(n));
            }
            standard_travel_pass(root);
            return root;
        };


        void run(g_ptr<Node> root) override {
            std::function<void(g_ptr<Node>)> print_scopes = [&print_scopes,this](g_ptr<Node> root) {
                for(auto t : root->scopes) {
                    print(node_to_string(t));
                }
                for(auto child_scope : root->scopes) {
                    print_scopes(child_scope);
                }
            };

            print("T STAGE");
            start_stage(&t_handlers,t_default_function);
            standard_travel_pass(root);

            newline("Discovering symbols");
            print("D STAGE");
            start_stage(&d_handlers,d_default_function);
            discover_symbols(root);
            endline();
            print("R STAGE");
            start_stage(&r_handlers,r_default_function);
            standard_resolving_pass(root);

            std::string final_time = ftime(timer.end());

            print("==LOG==");
            span->print_all();
            print(node_to_string(root));
            print_scopes(root);

            timer.start();

            print("X STAGE");
            start_stage(&x_handlers,x_default_function);
            standard_travel_pass(root);

            print("==DONE==");
            print("Ran:\n",code_store);

            print("Exec time: ",ftime(timer.end()));
            print("Final time: ",final_time);
        };

    };
}
