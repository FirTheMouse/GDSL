#pragma once

#include "../modules/Q-TwigSnap.hpp"
#include "../modules/GDSL-Script.hpp"
#include "../modules/Q-Web.hpp"

namespace GDSL {
    struct TwigSnap_DSL_Frontend : public virtual TwigSnap_Unit, public virtual Q_Script_Unit, public virtual Web_Unit {
        TwigSnap_DSL_Frontend() { init(); }


        size_t post_stage = add_stage_lookup("post",&p_handlers,&p_default_function);

        void add_text_component(const std::string& f, uint32_t type) {
            tokenized_keywords.put(f,type);
            a_handlers[type] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    g_ptr<Node> next = ctx.result->get(ctx.index+1);
                    if(next->type==string_id) {
                        ctx.node->name = next->name;
                        ctx.result->removeAt(ctx.index+1);
                    }
                }
                ctx.node->sub_type = ctx.node->type;
                ctx.node->type = text_id;
            };
        }

        void add_input_component(const std::string& f, uint32_t type = 0) {
            if(type==0) {
                type = make_tokenized_keyword(f);
            } else {
                tokenized_keywords.put(f,type);
            }
            a_handlers[type] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    g_ptr<Node> next = ctx.result->get(ctx.index+1);
                    if(next->type==identifier_id||next->type==string_id) {
                        ctx.node->name = next->name;
                        ctx.result->removeAt(ctx.index+1);
                    }
                }
            };
            t_handlers[type] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    g_ptr<Node> properties = make<Node>(properties_id);
                    ctx.node->quals << properties;
                    properties->mute = true; //So nobody fires it as a qual on accident
                    for(auto c : ctx.node->scope()->children) {
                        if(c->children.length()==2) {
                            properties->children << make_property(c->left()->name, c->right()->name);
                        } else if(ctx.root->node_table.hasKey(c->name)) {
                            ctx.node->quals << ctx.root->node_table.get(c->name);
                        }
                    }
                }
            };
        }

        size_t route_id = reg_id("route");
        size_t find_id = make_tokenized_keyword("find");

        void init() override {
            TwigSnap_Unit::init();
            Q_Script_Unit::init();
            Web_Unit::init();

            tokenized_keywords.put("server",server_id);
            scope_link_handlers[string_id] = [this](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                standard_scope_link_handler(new_scope,current_scope,owner_node);
                if(owner_node->name.at(0)=='/') {
                    owner_node->type = route_id;
                } else {
                    owner_node->type = node_block_id;
                }
                new_scope->name = owner_node->name;
            };

            discard_types.push_if_absent(rbrace_id);
            discard_types.push_if_absent(lbrace_id);

            add_text_component("paragraph",paragraph_subtype);

            t_handlers[route_id] = [this](Context& ctx) {          
                size_t this_route = routes.getOrDefault(ctx.node->name,make_route(ctx.node->name));
                ctx.node->sub_type = this_route;
                g_ptr<Node> html_root = make<Node>(html_id);
                g_ptr<Node> body = ctx.node->scope();
                body->type = body_id;
                html_root->children << body;    

                x_handlers[this_route] = [this,html_root](Context& ctx) {
                    std::string page = "<!DOCTYPE html>\n";
                    int depth = 0;
                    page += html_encode_node(html_root, depth);
                    ctx.source = page;
                };
                p_handlers[this_route] = [this,body,html_root](Context& ctx) {
                    // for(auto c : body->children) {
                    //     if(c->type == button_id && c->name == "btn") {
                    //         for(auto q : c->quals) {
                    //             if(q->type == style_id) {
                    //                 for(auto p : q->children) {
                    //                     if(p->name == "background-color") {
                    //                         p->opt_str = p->opt_str == "green" ? "red" : "green";
                    //                     }
                    //                 }
                    //             }
                    //         }
                    //     }
                    // }
                    // std::string page = "<!DOCTYPE html>\n";
                    // int depth = 0;
                    // page += html_encode_node(html_root, depth);
                    // ctx.source = page;
                };
            };
            x_handlers[route_id] = x_handlers[node_block_id];
            html_handlers[handler_block_id] = [this](Context& ctx) {
                //Don't emit this
            };

            html_handlers[type_decl_id] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    if(ctx.node->scope()->type!=div_id) {
                        ctx.node->scope()->type = div_id;
                        ctx.node->scope()->name = "id=\"" + ctx.node->name + "\"";
                    }
                    ctx.source = html_encode_node(ctx.node->scope(), ctx.index);
                }
            };

            tokenized_keywords.put("script",script_id);
            a_handlers[script_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    g_ptr<Node> next = ctx.result->get(ctx.index+1);
                    if(next->type==string_id) {
                        ctx.node->opt_str = next->name;
                        ctx.node->name = "";
                        ctx.result->removeAt(ctx.index+1);
                    }
                }
            };


            set_binding_powers(colon_id,4,6);

            tokenized_keywords.put("style",style_id);
            a_handlers[style_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    g_ptr<Node> next = ctx.result->get(ctx.index+1);
                    if(next->type==identifier_id||next->type==string_id) {
                        ctx.node->name = next->name;
                        ctx.result->removeAt(ctx.index+1);
                    }
                }
            };
            t_handlers[style_id] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    ctx.node->mute = true; //So nobody fires it as a qual on accident
                    ctx.root->distribute_node(ctx.node->name,ctx.node);
                    for(auto c : ctx.node->scope()->children) {
                        if(c->children.length()==2) {
                            ctx.node->children << make_property(c->left()->name, c->right()->name);
                        }
                    }
                }
            };

            add_input_component("button",button_id);
            add_input_component("input",input_id);
            add_input_component("textarea");
            

            t_handlers[find_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
                }
            };
            x_handlers[find_id] = [this](Context& ctx) {

            };
        }

        void run(g_ptr<Node> root) override {
            start_stage(&t_handlers,t_default_function);
            standard_resolving_pass(root);
    
            start_stage(&d_handlers,d_default_function);
            discover_symbols(root);

            start_stage(&r_handlers,r_default_function);
            standard_resolving_pass(root);
    
            start_stage(&e_handlers,e_default_function);
            standard_backwards_pass(root);

            #if PRINT_ALL
                print_root(root);
            #endif

            g_ptr<Node> server = nullptr;
            for(auto c : root->children) {
                if(c->type == server_id) {
                    if(server) {
                        print(red("WARNING: more than one server at root level!"));
                    } else {
                        server = c;
                    }
                }
            }

            start_stage(&x_handlers,x_default_function);
            // process_node(server);
            standard_travel_pass(root);
        }

    };
}