#pragma once

#include "../modules/GDSL-Script.hpp"
#include "../modules/Q-Web.hpp"
#include "../modules/Q-HTML.hpp"

namespace GDSL {
    struct TwigSnap_DSL_Frontend : public virtual HTML_Unit,  public virtual Q_Script_Unit, public virtual Web_Unit {
        TwigSnap_DSL_Frontend() { init(); }

        size_t route_id = reg_id("route");
        size_t inlined_id = add_type("inlined");
        size_t component_id = add_type("component");
        size_t div_id = make_keyword("div",0,"",component_id);
        size_t find_id = make_tokenized_keyword("find");
        size_t serve_id = make_tokenized_keyword("serve");


        g_ptr<Node> make_property(const std::string& type, const std::string& value) {
            g_ptr<Node> prop_node = make<Node>(property_id);
            prop_node->name = type;
            prop_node->opt_str = value;
            return prop_node;
        }

        void standard_gather_from_scope(Context& ctx) {
            g_ptr<Node> node = ctx.node;
            if(node->scope()) {
                g_ptr<Node> properties = make<Node>(properties_id);
                properties->mute = true;
                node->scope()->quals << properties;
                for(int i = 0;i<node->scope()->children.length();i++) {
                    g_ptr<Node> c = node->scope()->children[i];
                    if(c->type==func_call_id) {
                        g_ptr<Node> ref = c->value->type_scope;
                        g_ptr<Value> ref_v = ref->owner->value;
                        if(ref_v->type==inlined_id) {
                            node->scope()->quals << ref->quals;
                            node->scope()->children.removeAt(i);
                            i--;
                        } else if(ref_v->type==component_id) {
                            node->scope()->children.removeAt(i);
                            node->scope()->children.insert(ref->owner,i);
                        } else {
                            //Is an actual func call, handle as such
                        }
                    } else if(c->children.empty()) {
                        if(c->value->type==inlined_id) {
                            node->scope()->quals << c->scope()->quals;
                            node->scope()->children.removeAt(i);
                            i--;
                        } else if(c->value->type==component_id) {
                            //Do nothing
                        }
                    } else if(c->children.length()==2&&c->type==colon_id||c->type==equals_id) {
                        properties->children << make_property(c->left()->name,c->right()->name);
                        node->scope()->children.removeAt(i);
                        i--;
                    } else if(c->children.length()==1&&c->value->type==component_id) {
                        properties->children << c;
                        node->scope()->children.removeAt(i);
                        i--;
                    } 
                }
            }
        }

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
                standard_gather_from_scope(ctx);
            };
        }

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
                ctx.source = html_encode_node(ctx.node);
            };
            html_handlers.default_function = [this](Context& ctx){
                //Emit nothing
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

                    body += html_encode_node(body_node);

                    page += body;
                    page += "</html>";
                    ctx.source = page;
                };
            };
            r_handlers[route_id] = [this](Context& ctx){
                standard_gather_from_scope(ctx);
            };
            html_handlers[route_id] = [this](Context& ctx){
                standard_emit_as_html(ctx);
            };

            r_handlers[func_decl_id] = [this](Context& ctx) {
                standard_gather_from_scope(ctx);
                ctx.node->scope()->type = div_id;
            };
            html_handlers[body_id] = [this](Context& ctx){
                standard_emit_as_html(ctx);
            };
            html_handlers[func_decl_id] = [this](Context& ctx){
                if(ctx.node->scope()) {
                    ctx.source = html_encode_node(ctx.node->scope());
                }   
            };
            html_handlers[div_id] = [this](Context& ctx){
                standard_emit_as_html(ctx);
            };
            html_handlers[var_decl_id] = [this](Context& ctx){
                if(ctx.node->scope()) {
                    ctx.source = html_encode_node(ctx.node->scope());
                }   
            };

            tokenized_keywords.put("script",script_id);
            n_handlers[script_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
                    ctx.node->name = "";
                }
            };
            html_handlers[script_id] =[this](Context& ctx){
                std::string s = "<script>\n";
                if(ctx.node->left()) {
                    process_node(ctx,ctx.node->left());
                    s+=ctx.node->left()->value->get<std::string>();
                    s+="\n";
                }
                s+="</script>\n";
                ctx.source = s;
            };
            set_binding_powers(colon_id,4,6);

            add_input_component("button",button_id);
            add_input_component("input",input_id);
            add_input_component("textarea");
            add_text_component("paragraph",paragraph_subtype);
            r_handlers[text_id] = [this](Context& ctx) {
                standard_gather_from_scope(ctx);
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
                        body += html_encode_node(retrived);

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
                print("==FINAL FORM==");
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
            //process_node(server);
            standard_travel_pass(root);
            // for(auto c : root->children) {
            //     if(c->name=="/") {
            //         std::string res = indent_html_text(html_encode_node(c->scope()));
            //         print(res);
            //         print("DONE EMITTING");
            //     }
            // }
        }

    };
}