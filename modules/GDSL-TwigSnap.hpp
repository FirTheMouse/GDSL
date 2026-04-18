#pragma once

#include "../modules/Q-TwigSnap.hpp"
#include "../modules/GDSL-Script.hpp"
#include "../modules/Q-Web.hpp"

namespace GDSL {
    struct TwigSnap_DSL_Frontend : public virtual TwigSnap_Unit, public virtual Q_Script_Unit, public virtual Web_Unit {
        TwigSnap_DSL_Frontend() { init(); }

        void add_text_component(const std::string& f, uint32_t type) {
            tokenized_keywords.put(f,type);
            n_handlers[type] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
                    ctx.node->name = ctx.node->left()->name;
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
            n_handlers[type] = [this](Context& ctx) {
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
        size_t serve_id = make_tokenized_keyword("serve");

        void init() override {

            tokenized_keywords.put(labels[server_id],server_id);
            tokenized_keywords.put(labels[port_id],port_id);
            tokenized_keywords.put(labels[listen_id],listen_id);
            tokenized_keywords.put(labels[accept_id],accept_id);
            tokenized_keywords.put(labels[request_id],request_id);
            tokenized_keywords.put(labels[response_id],response_id);
            tokenized_keywords.put(labels[close_id],close_id);

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

            fragment_handlers.default_function = [this](Context& ctx){
                int depth = 0;
                ctx.source = html_encode_node(ctx.node, depth);
            };

            add_text_component("paragraph",paragraph_subtype);
            t_handlers[text_id] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    g_ptr<Node> properties = make<Node>(properties_id);
                    ctx.node->quals << properties;
                    properties->mute = true; //So nobody fires it as a qual on accident
                    for(auto c : ctx.node->scope()->children) {
                        if(c->children.length()==2) {
                            properties->children << make_property(c->left()->name, c->right()->name);
                            if(c->left()->name == "id") {
                                ctx.node->name = c->right()->name;
                                ctx.node->in_scope->node_table.put(ctx.node->name,ctx.node);
                            }
                        } else if(ctx.root->node_table.hasKey(c->name)) {
                            ctx.node->quals << ctx.root->node_table.get(c->name);
                        }
                    }
                }
            };

            t_handlers[route_id] = [this](Context& ctx) {          
                size_t this_route = routes.getOrDefault(ctx.node->name,make_route(ctx.node->name));
                ctx.node->sub_type = this_route;
                g_ptr<Node> body_node = ctx.node->scope();
                body_node->type = body_id;
                route_nodes[this_route] = ctx.node;

                x_handlers[this_route] = [this,body_node](Context& ctx){
                    std::string page = "<!DOCTYPE html>\n";
                    page += "<html>\n";
                    std::string body = "";

                    int depth = 0;
                    body += html_encode_node(body_node, depth);

                    page += body;
                    page += "</html>";
                    ctx.source = page;
                };
            };

            r_handlers[route_id] = [this](Context& ctx){
                if(ctx.node->scope()) {
                    g_ptr<Node> properties = make<Node>(properties_id);
                    ctx.node->scope()->quals << properties;
                    properties->mute = true; //So nobody fires it as a qual on accident
                    for(auto c : ctx.node->scope()->children) {
                        if(c->type==func_call_id) {
                            ctx.node->scope()->quals << ctx.root->node_table[c->name];
                        } else if(c->type==var_decl_id) {
                            if(c->children.empty()) {
                                c->type = div_id;
                                c->children = c->value->type_scope->children;
                                for(auto idk : c->value->type_scope->children) {
                                    if(idk->type==func_call_id) {
                                        c->quals << ctx.root->node_table[idk->name];
                                    }
                                }
                            }
                        } else if(c->children.length()==2) {
                            properties->children << make_property(c->left()->name, c->right()->name);
                        }
                    }
                }
            };

            x_handlers[serve_id] = [this](Context& ctx){
                std::string page = "<!DOCTYPE html>\n";
                page += "<html>\n";
                std::string body = "";
                if(ctx.sub) {
                    if(ctx.node->left()) {
                        process_node(ctx,ctx.node->left());
                        g_ptr<Node> retrived = ctx.node->left()->value->get<g_ptr<Node>>();
                        if(retrived->type!=body_id) body+= "<body>\n";

                        int depth = 0;
                        body += html_encode_node(retrived, depth);

                        if(retrived->type!=body_id) body+= "</body>\n";
                    } else {
                        print("No left");
                    }
                } else {
                    body = "serve:x_handler context has no sub";
                }
                page += body;
                page += "</html>";
                ctx.node->value->set<std::string>(page);
            };

            x_handlers[route_id] = x_handlers[node_block_id];
            html_handlers[in_id] = [this](Context& ctx) {
                //Don't emit this
            };
            html_handlers[on_id] = [this](Context& ctx) {
                //Don't emit this
            };


            r_handlers[type_decl_id] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    g_ptr<Node> properties = make<Node>(properties_id);
                    ctx.node->quals << properties;
                    properties->mute = true; //So nobody fires it as a qual on accident
                    for(auto c : ctx.node->scope()->children) {
                        if(c->type==func_call_id) {
                            ctx.node->quals << ctx.root->node_table[c->name];
                        } else if(c->children.length()==2) {
                            properties->children << make_property(c->left()->name, c->right()->name);
                        }
                    }
                }
            };
            html_handlers[func_call_id] = [this](Context& ctx) {
                //Don't emit
            };
            html_handlers[identifier_id] = [this](Context& ctx) {
                //Don't emit
            };
            html_handlers[colon_id] = [this](Context& ctx) {
                //Don't emit
            };
            html_handlers[literal_id] = [this](Context& ctx) {
                //Don't emit
            };

            html_handlers[type_decl_id] = [this](Context& ctx) {
                if(ctx.node->scope()) {
                    if(ctx.node->scope()->type!=div_id) {
                        ctx.node->scope()->type = div_id;
                        ctx.node->scope()->name = "id=\"" + ctx.node->name + "\"";
                    }
                    ctx.node->scope()->quals = ctx.node->quals;
                    ctx.source = html_encode_node(ctx.node->scope(), ctx.index);
                }
            };

        

            tokenized_keywords.put("script",script_id);
            n_handlers[script_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
                    ctx.node->name = "";
                }
            };
            html_handlers[read_file_id] = [this](Context& ctx) {
                //Don't emit
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
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(d_handlers);
            discover_symbols(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);

            start_stage(e_handlers);
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

            start_stage(x_handlers);
            // process_node(server);
            standard_travel_pass(root);
        }

    };
}