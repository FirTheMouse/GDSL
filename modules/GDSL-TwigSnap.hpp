#pragma once

#include "../modules/GDSL-Script.hpp"
#include "../modules/Q-Web.hpp"
#include "../modules/Q-HTML.hpp"

namespace GDSL {
    struct TwigSnap_DSL_Frontend : public virtual HTML_Unit,  public virtual Q_Script_Unit, public virtual Web_Unit {
        TwigSnap_DSL_Frontend() { init(); }

        size_t route_id = reg_id("route");
        size_t inlined_id = add_type("inlined");
        size_t invisible_id = add_type("invisible");
        size_t component_id = add_type("component");
        size_t foldable_id = add_type("foldable");
        size_t iframe_id = add_type("iframe");
        size_t div_id = make_keyword("div",0,"",component_id);
        size_t find_id = make_tokenized_keyword("find");
        size_t serve_id = make_tokenized_keyword("serve");

        size_t fragment_highlight_id = make_tokenized_keyword("fragment_highlight");
        size_t pebble_post_id = make_tokenized_keyword("pebble_post");
        size_t emit_as_html_id = make_tokenized_keyword("emit_as_html");
        size_t quick_server_id = make_tokenized_keyword("quick_server");

        g_ptr<Node> make_property(g_ptr<Node> type, g_ptr<Node> value, g_ptr<Node> parent = nullptr) {
            g_ptr<Node> prop_node = make<Node>(property_id);
            prop_node->name = type->name;
            prop_node->opt_str = value->name;
            prop_node->quals << copy_as_token(type);
            prop_node->quals << copy_as_token(value);
            if(parent)
                prop_node->quals << copy_as_token(parent);
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
                    if(c->type==func_call_id) { //Anything defined with a type and identifer becomes a function call when refrenced elsewhere
                        g_ptr<Node> ref = c->value->type_scope;
                        if(!ref || !ref->owner) {
                            attach_error(c, major_error, "gather_from_scope:func_call type_scope or owner is null");
                            continue;
                        }
                        g_ptr<Value> ref_v = ref->owner->value;
                        if(ref_v->type==inlined_id) {
                            node->scope()->quals << ref->quals;
                            node->scope()->quals << copy_as_token(c);
                            node->scope()->children.removeAt(i);
                            i--;
                        } else if(ref_v->type==component_id) {
                            node->scope()->quals << copy_as_token(c);
                            node->scope()->children.removeAt(i);
                            node->scope()->children.insert(ref->owner,i);
                        } else {
                            //Is an actual func call, handle as such
                        }
                    } else if(c->type == func_decl_id) {



                    } else if(c->children.empty()) {
                        if(c->value->type==inlined_id) {
                            node->scope()->quals << copy_as_token(c);
                            node->scope()->quals << c->scope()->quals;
                            node->scope()->children.removeAt(i);
                            i--;
                        }
                    } else if(c->children.length()==2&&c->type==colon_id||c->type==equals_id) {
                        properties->children << make_property(c->left(),c->right(),c);
                        node->scope()->quals <= properties->children.last()->quals; //Stealing the tokens for ourselves
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
                    ctx.node->opt_str = ctx.node->left()->name;
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

                        ctx.node->quals << copy_as_token(ctx.node);
                        ctx.node->x = -1.0f; ctx.node->y = -1.0f;

                        ctx.node->name = next->name;
                        //ctx.result->removeAt(ctx.index+1);
                        ctx.node->children  << ctx.result->take(ctx.index+1);
                    }
                }
            };
            r_handlers[type] = [this,type](Context& ctx) {
                standard_gather_from_scope(ctx);
                if(ctx.node->scope()) {
                    ctx.node->scope()->type = type;
                }
            };
            html_handlers[type] = [this](Context& ctx) {
                // if(ctx.node->left()) {
                //     process_node(ctx.node->left());
                //     if(ctx.node->left()->value->type==string_id) {
                //         ctx.node->name = 
                //     } else {

                //     }
                // }

                if(ctx.node->scope()) {
                    standard_emit_as_html(ctx,ctx.node->scope());
                } else {
                    standard_emit_as_html(ctx);
                }
            };
        }

        void stamp_types_onto_page(g_ptr<Node> node, list<list<uint32_t>>& lines) {
            if(node->x>=0.0f&&node->y>=0.0f && !node->name.empty()) {
                int x = (int)node->x;
                int y = (int)node->y;
                while(y>=lines.length()) {lines << list<uint32_t>();}
                while((x+(int)node->name.length())>=lines[y].length()) lines[y] << (uint32_t)0;
                for(int i=0;i<node->name.length();i++) lines[y][x+i] = node->type;
            }
            for(auto c : node->children) stamp_types_onto_page(c,lines);
            for(auto q : node->quals) stamp_types_onto_page(q,lines);
            for(auto s : node->scopes) stamp_types_onto_page(s,lines);
            if(node->value) {
                for(auto q : node->value->quals) {
                    stamp_types_onto_page(q,lines);
                }
            }
        }
        
        std::string nodenet_to_highlighted(g_ptr<Node> root, const std::string& code) {
            list<list<uint32_t>> type_lines;
            stamp_types_onto_page(root, type_lines);
            
            std::string out = "";
            int x = 0, y = 0;
            uint32_t current_type = 0;
            
            for(char c : code) {
                if(c=='\n') {
                    if(current_type!=0) { out += "</span>"; current_type = 0; }
                    out += '\n';
                    x = 0; y++;
                    continue;
                }
                uint32_t t = (y<type_lines.length()&&x<type_lines[y].length()) ? type_lines[y][x] : 0;
                if(t != current_type) {
                    if(current_type!=0) out += "</span>";
                    if(t!=0) out += "<span class='" + labels[t] + "'>";
                    current_type = t;
                }
                out += c;
                x++;
            }
            if(current_type!=0) out += "</span>";
            return out;
        }

        list<g_ptr<TwigSnap_DSL_Frontend>> twig_daycare;

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
                if(!owner_node->name.empty() && owner_node->name.at(0)=='/') {
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


            x_handlers[emit_as_html_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node->left());
                ctx.node->value->set<std::string>(standard_emit_as_html(ctx, ctx.node->left()->value->get<g_ptr<Node>>()));
                ctx.node->value->type = string_id;
                ctx.node->value->size = 24;
            };

            x_handlers[fragment_highlight_id] = [this](Context& ctx) {
                std::string source = ctx.sub->source;
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                std::string out = "";
                g_ptr<TwigSnap_DSL_Frontend> twig = make<TwigSnap_DSL_Frontend>();
                if(instruction=="compile") {
                    Log::Line l; l.start();
                    g_ptr<Node> root = twig->process(content);
                    twig->resolve_and_evaluate(root);
                    double a_time = l.end(); l.start();
                    out += twig->nodenet_to_highlighted(root, content);
                    double b_time = l.end(); 
                    // l.start();
                    //print_root(root);
                    // double c_time = l.end();

                    print("A: ",ftime(a_time));
                    print("B: ",ftime(b_time));
                    //print("C: ",ftime(c_time));
                } else if(instruction=="end") {
                    print("REQUEST TO END: ",target," OF ",servers.length());
                    g_ptr<Server> to_end = get_server(target);
                    if(to_end) {
                        ::close(to_end->fd); 
                        to_end->fd = -1;
                        to_end->thread->end();
                        servers.erase(to_end);
                    } else {
                        print(red("Unable to find server "+target+" to end"));
                    }
                } else if(instruction=="preview") {
                    twig_daycare << twig;
                    g_ptr<Node> root = twig->process(content);
                    twig->run(root);
                    int port_num = 8082;
                    for(auto c : root->children) {
                        if(c->type==server_id) {
                            for(auto sc : c->scope()->children) {
                                if(sc->type==port_id) {
                                    port_num = sc->left()->value->get<int>();
                                }
                            }
                        }
                    }
                    servers << twig->servers;
                    servers.last()->label = target;
                    print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
                    out = std::to_string(port_num);
                } else if(instruction=="read") {
                    out = readFile(content);
                } else {
                    print(red("Unrecognized instruction for fragment: "+ctx.sub->source));
                }
                ctx.sub->source = out;
            };

            x_handlers[pebble_post_id] = [this](Context& ctx) {
                std::string source = ctx.sub->source;
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string instruction = source.substr(0, first);
                std::string file = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);
                
                if(instruction == "write") {
                    writeFile(file, content);
                    ctx.sub->source = "ok";
                }
            };

            x_handlers[quick_server_id] = [this](Context& ctx) {

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
                    ctx.source = indent_html_text(page);
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
                if(ctx.node->value->type==invisible_id) {
                    ctx.node->scope()->type = invisible_id;
                } else if(ctx.node->value->type==foldable_id) {
                    ctx.node->scope()->type = foldable_id;
                } else if(ctx.node->value->type==iframe_id) {
                    ctx.node->scope()->type = iframe_id;
                } else {
                    ctx.node->scope()->type = div_id;
                }
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

            html_handlers[inlined_id] = [this](Context& ctx){
                //Emit nothing
            };

            html_handlers[invisible_id] = [this](Context& ctx){
                //Emit nothing
            };

            html_handlers[foldable_id] = [this](Context& ctx){
                std::string out = "";
                out+="<details>\n";
                out+="<summary ";
                out+=emit_inline_html(ctx); 
                out+=">\n"+ctx.node->name+"\n</summary>\n";
                for(auto c : ctx.node->children) {
                    out+=html_encode_node(c);
                }
                out+="</details>\n";
                ctx.source = out;
            };

            html_handlers[iframe_id] = [this](Context& ctx){
                std::string out = "";
                out += "<iframe ";
                out += emit_inline_html(ctx);
                out += ">\n</iframe>\n";
                ctx.source = out;
            };

            tokenized_keywords.put("script",script_id);
            n_handlers[script_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
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

            tokenized_keywords.put("style",style_id);
            n_handlers[style_id] = [this](Context& ctx) {
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->children << ctx.result->take(ctx.index+1);
                }
            };
            html_handlers[style_id] =[this](Context& ctx){
                std::string s = "<style>\n";
                if(ctx.node->left()) {
                    process_node(ctx,ctx.node->left());
                    s+=ctx.node->left()->value->get<std::string>();
                    s+="\n";
                }
                s+="</style>\n";
                ctx.source = s;
            };
            set_binding_powers(colon_id,4,6);

            add_input_component("button",button_id);
            add_input_component("input",input_id);
            add_input_component("textarea");
            add_text_component("paragraph",paragraph_subtype);
            add_text_component("text",paragraph_subtype);
            r_handlers[text_id] = [this](Context& ctx) {
                standard_gather_from_scope(ctx);
            };
            x_handlers[text_id] = [this](Context& ctx) {
                if(ctx.node->left()) {
                    process_node(ctx, ctx.node->left());
                    if(ctx.node->left()->value->type == string_id) {
                        ctx.node->opt_str = ctx.node->left()->value->get<std::string>();
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

        void resolve_and_evaluate(g_ptr<Node> root) {
            start_stage(t_handlers);
            standard_resolving_pass(root);

            start_stage(d_handlers);
            discover_symbols(root);

            start_stage(r_handlers);
            standard_resolving_pass(root);

            start_stage(e_handlers);
            standard_backwards_pass(root);
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

            // print("AS STRING");
            // print(nodenet_to_string(root));

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