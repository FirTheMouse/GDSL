#pragma once

#include "../Acorn-Script.hpp"
#include "../../ext/g_lib/core/thread.hpp"

#define _UUID_T
typedef unsigned char uuid_t[16];

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <mach/mach.h>

size_t current_memory_usage() {
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    return info.resident_size; // current RSS in bytes
}

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}

        struct Server : q_object {
            int fd;
            std::string label;
            g_ptr<Thread> thread;
            Unit* unit;
        };
    
        list<g_ptr<Server>> servers;
    
        g_ptr<Server> get_server(int fd) {
            for(auto& s : servers) {
                if(s->fd == fd) return s;
            }
            return nullptr;
        }
    
        g_ptr<Server> get_server(const std::string& label) {
            for(auto& s : servers) {
                if(s->label == label) return s;
            }
            return nullptr;
        }

        uint32_t property_id = reg_id("property");
        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined"); uint32_t suffix_inlined_id = reg_id("suffix_inlined"); uint32_t prefix_inlined_id = reg_id("prefix_inlined");
        uint32_t invisible_id = reg_id("invisible"); uint32_t suffix_invisible_id = reg_id("suffix_invisible"); uint32_t prefix_invisible_id = reg_id("prefix_invisible");
        uint32_t component_id = reg_id("component"); uint32_t suffix_component_id = reg_id("suffix_component"); uint32_t prefix_component_id = reg_id("prefix_component");

        uint32_t find_node_id = make_tokenized_keyword("find_node");

        Stage& html_handlers = reg_stage("htmlemiting");

        _lookup is_structural{{
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload", "onmouseover",
            "role","lang","colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            for(int q=0;q<ctx.node().quals().length();q++) {
                Node qual = ctx.node().quals()[q];
                for(int i=0;i<qual.children().length();i++) {
                    Node c = qual.children()[i];
                    if(c.type()==property_id) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(ctx,c.children()[0]);
                            if(!is_live(c.children()[0].value().data_ptr())) { //For templates and such where we might use an identifer
                                continue;
                            }
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(ctx,c.children()[1]);
                            if(!is_live(c.children()[1].value().data_ptr())) {
                                continue;
                            }
                            val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                        } else {
                            val = c.children()[1].name().to_std();
                        }

                        list<std::string>* prop_labels; list<std::string>* prop_values;
                        if(is_prop_structural(prop)) {
                            prop_labels = &structural_prop_labels; 
                            prop_values = &structural_prop_values;
                        } else {
                            prop_labels = &style_prop_labels; 
                            prop_values = &style_prop_values;
                        }

                        if(q==0||!prop_labels->has(prop)) {
                            prop_labels->push(prop);
                            prop_values->push(val);
                        }
                    } else {

                    }
                }
            }


            for(int i=0;i<structural_prop_labels.length();i++) {
                s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            }   
            if(!style_prop_labels.empty()) {
                s += " style=\"";
                for(int i=0;i<style_prop_labels.length();i++) {
                    s += style_prop_labels[i]+":"+style_prop_values[i]+";";
                }  
                s += "\""; 
            }
            ctx.sub().source().push(s);
        }

        std::string emit_inline_html(Context& ctx, Node node) {
            Node old_node = ctx.node();
            std::string old_source = ctx.source().to_std();
            ctx.source().col().clear();
            ctx.node(node);
            emit_inline_html(ctx);
            std::string to_reutrn = ctx.source().to_std();
            ctx.node(old_node);
            ctx.source() = old_source;
            return to_reutrn;
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        void standard_gather_from_scope(Context& ctx) {
            Node node = ctx.node();
            if(!node.scopes().empty()) {
                Node scope = node.scopes()[0];
                Node properties = make_node(properties_id);
                properties.mute(true);
                scope.quals().push(properties);
                for(int i = 0;i<scope.children().length();i++) {
                    Node c = scope.children()[i];
                    if(c.type()==func_call_id) { //Anything defined with a type and identifer becomes a function call when refrenced elsewhere
                        Node ref = c.value().type_scope();
                        if(ref.idx==0 || ref.owner().idx==0) {
                            print(red("gather_from_scope:func_call type_scope or owner is null"));
                            continue;
                        }
                        Value ref_v = ref.owner().value();
                        if(ref_v.type()==inlined_id) {
                            for(int q=0;q<ref.quals().length();q++) {
                                scope.quals().push(ref.quals()[q]);
                            }
                            // scope.quals() << copy_as_token(c);
                            scope.children().removeAt(i);
                            i--;
                        } else if(ref_v.type()==component_id) {
                            if(ref.owner().children().empty()) {
                                // scope.quals() << copy_as_token(c);
                                Node owner = ref.owner();
                                scope.children_col().set(i,(void*)&owner);
                            } else {
                                instantiate_template(c,ref.owner(),ctx);
                            }
                        } else {
                            print("ACTUAL FUNC CALL");
                            //Is an actual func call, handle as such
                        }
                    } else if(c.type() == func_decl_id) {

                    } else if(c.children().empty()) {
                        if(is_live(c.value())&&c.value().type()==inlined_id) {
                            //scope.quals() << copy_as_token(c);
                            for(int q=0;q<c.scopes()[0].quals().length();q++) {
                                scope.quals().push(c.scopes()[0].quals()[q]);
                            }
                            scope.children().removeAt(i);
                            i--;
                        }
                    } else if(c.children().length()==2&&c.type()==colon_id) {
                        properties.children().push(make_property(c.children()[0],c.children()[1],c));
                        for(int q=0;q<properties.children().last().quals().length();q++) {
                            scope.quals().push(properties.children().last().quals()[q]); //Stealing the tokens for ourselves
                        }
                        properties.children().last().quals_col().clear();
                        scope.children().removeAt(i);
                        i--;
                    }
                }
            }
        }

        Node webcorn_node_scan(const std::string& label, Node from) {
            if(!from.scopes().empty()) {
                for(int q=0;q<from.scopes()[0].quals().length();q++) {
                    Node qual = from.scopes()[0].quals()[q];
                    for(int i=0;i<qual.children().length();i++) {
                        Node c = qual.children()[i];
                        if(c.type()==property_id) {
                            std::string prop = "";
                            std::string val = "";
    
                            if(c.children()[0].value().type()==string_id) {
                                if(!is_live(c.children()[0].value().data_ptr())) {continue;}
                                prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                            } else {
                                prop = c.children()[0].name().to_std();
                            }
    
                            if(c.children()[1].value().type()==string_id) {
                                if(!is_live(c.children()[1].value().data_ptr())) {continue;}
                                val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                            } else {
                                val = c.children()[1].name().to_std();
                            }

                            if(prop=="id"&&val==label) return from;
                        }
                    }
                }
            }
            for(int i=0;i<from.children().length();i++) {
                Node found = webcorn_node_scan(label,from.children()[i]);
                if(is_live(found)) {
                    return found;
                }
            }
            for(int i=0;i<from.scopes().length();i++) {
                if(from.scopes()[i].owner()==from) {
                    Node found = webcorn_node_scan(label,from.scopes()[i]);
                    if(is_live(found)) {
                        return found;
                    }
                }
            }
            return deadptr;
        }

        struct style_manager : public q_object {
            style_manager(Webcorn_Core* _unit) : unit(_unit) {}
            style_manager(Webcorn_Core* _unit, list<std::string> init) : unit(_unit) {
                for(auto s : init) add_prop(s);
            }
            Webcorn_Core* unit;
            list<Node> props;
            list<std::string> prop_names;

            void add_prop(const std::string& name, Node prop = deadptr) {
                props << prop;
                prop_names << name;
            }

            void match_prop(const std::string& name, Node prop) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name) {
                        props[i] = prop;
                        return;
                    }
                }
            }

            std::string resolve_prop(Context& ctx, const std::string& name) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name&&is_live(props[i])) {
                        return unit->emit_inline_html(ctx,props[i]);
                    }
                }
                return "";
            }
        };

        std::string TypeCol_to_html_table(Context& ctx, ColCol& t) {
            g_ptr<style_manager> styles = make<style_manager>(this);
            list<list<std::string>> lines = TypeCol_to_lines(t);

            std::string out = "";
            out += "<table id='" + ctx.sub().node().name().to_std() + "' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(auto& col : lines) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += col.empty() ? "" : col[0];
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(auto& col : lines) if(col.length() > max_rows) max_rows = col.length();
            
            for(int r = 1; r < max_rows; r++) {
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(auto& col : lines) {
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">";
                    out += r < col.length() ? col[r] : "";
                    out += "</td>";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        map<std::string,uint32_t> routes;
        map<uint32_t,Node> route_nodes;

        void init() override {
            set_binding_powers(colon_id,4,6);
            register_type("div",component_id,0);
            register_type("inlined",inlined_id,0);
            register_type("invisible",invisible_id,0);

            r_handlers[func_decl_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                if(ctx.node().type()==func_call_id) {
                    if(ctx.node().value().type()!=component_id) {
                        //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
                        sync_args(ctx);
                    }
                } else {
                    Node scope = ctx.node().scopes()[0];
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                }
                if(ctx.node().value().type()==component_id) {
                    standard_gather_from_scope(ctx);    
                    if(!ctx.node().scopes().empty()) {
                        if(ctx.node().type()==func_decl_id&&!ctx.node().children().empty()) {
                            ctx.node().value().type(invisible_id);
                        }
                        for(int i=0;i<ctx.node().scopes().length();i++) {
                            Node s = ctx.node().scopes()[i];
                            if(ctx.node().value().type()==invisible_id) {
                                s.type(invisible_id);
                            // } else if(ctx.node()->value->type==foldable_id) {
                            //     s->type = foldable_id;
                            // } else if(ctx.node()->value->type==iframe_id) {
                            //     s->type = iframe_id;
                            } else {
                                s.type(component_id);
                            }
                        }
                    }
                }
            };
            r_handlers[func_call_id] = r_handlers[func_decl_id];

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                ctx.node(ctx.sub().node());
                standard_gather_from_scope(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_inline_html")] = [this](Context& ctx){
                if(ctx.sub().node().scopes().empty()) return;
                ctx.node(ctx.sub().node().scopes()[0]);
                emit_inline_html(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_contents")] = [this](Context& ctx){
                Node node = ctx.sub().node();
                if(node.scopes().length()>0) {
                    Node scope = node.scopes().get(0);
                    for(int i=0;i<scope.children().length();i++) {
                        Node child = scope.children().get(i);
                        start_stage(html_handlers);
                        process_node(ctx,child);
                        start_stage(x_handlers);
                    }
                }
            };

            ColCol t;
            for(int i=0;i<5;i++) {
                Col c;
                c.element_size = 4;
                c.tag = int_id;
                for(int n=0;n<8;n++) {
                    c.push((void*)&n);
                }
                t.push(c);
            }
            types.push(t);

            x_handlers[make_tokenized_keyword("render_col")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int idx = *(int*)ctx.node().children()[0].value().get();
                ctx.sub().source().push(TypeCol_to_html_table(ctx,types[idx]));
                print(red("RENDER_COL NODE\n"),node_to_string(ctx.sub().node()));
            };

            uint32_t display_node_id = make_tokenized_keyword("display_node");
            r_handlers[display_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                string contents(ticket);
                


                ctx.node().value().set((void*)&ticket);
            };
            x_handlers[display_node_id] = [this](Context& ctx){
                string addr(*(Ptr*)ctx.node().children()[0].value().get());
                string output(*(Ptr*)ctx.node().value().get());

                

                output = ("<p>"+addr.to_std()+"</p>");
            };

            r_handlers[find_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                resolve_overload(ctx);
            };
            x_handlers[find_node_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                std::string target = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                Node& from = (Node&)(*(Ptr*)ctx.node().children()[1].value().get());
                Node result = webcorn_node_scan(target,from);
                ctx.node().value().set((void*)&result);
                // print("TARGET: ",target," FROM: ",node_info(from));
                // if(is_live(result)) {
                //     print("FOUND: ",node_to_string(result));
                // } else {
                //     print(red("COULD NOT FIND "+target));
                // }
            };

            x_handlers[make_tokenized_keyword("webcorn")] = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
                int server_fd = 6;
                ctx.node().value().set((void*)&server_fd);
            };

            auto make_int_node = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
            };
        
            uint32_t socket_id = make_tokenized_keyword("socket");
            r_handlers[socket_id] = make_int_node;
            x_handlers[socket_id] = [this](Context& ctx){
                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(server_fd < 0) {
                    print(red("server_id::x_handler socket() failed"));
                    return;
                }        
                ctx.node().value().set((void*)&server_fd);
            };

            x_handlers[make_tokenized_keyword("run_server")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int server_fd = *(int*)ctx.node().children()[0].value().get();
                g_ptr<Server> new_server = make<Server>();
                new_server->fd = server_fd;
                new_server->thread = make<Thread>();
                new_server->unit = this;
                servers << new_server;
                
                if(!ctx.node().scopes().empty()) {
                    Node scope = ctx.node().scopes()[0];
                    new_server->thread->run_blocking([this, scope, ctx]() mutable {
                        standard_travel_pass(scope, ctx);
                    });
                }
            };
        
            uint32_t bind_id = make_tokenized_keyword("bind");
            r_handlers[bind_id] = make_int_node;
            x_handlers[bind_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                //Retrive fd and port from children
                int fd = *(int*)ctx.node().children()[0].value().get();
                int port = *(int*)ctx.node().children()[1].value().get();
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                addr.sin_addr.s_addr = INADDR_ANY;
                int result = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
                ctx.node().value().set((void*)&result);
            };
        
            uint32_t listen_id = make_tokenized_keyword("listen");
            r_handlers[listen_id] = make_int_node;
            x_handlers[listen_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int result = listen(fd, 10);
                ctx.node().value().set((void*)&result);
            };
        
            uint32_t accept_id = make_tokenized_keyword("accept");
            r_handlers[accept_id] = make_int_node;
            x_handlers[accept_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd == -1) {
                    if(ERROR_FLAG) return;
                    throw_error("accept failed: ", strerror(errno));
                    return;
                }
                ctx.node().value().set((void*)&client_fd);
            };
        
            uint32_t read_id = make_tokenized_keyword("read");
            //Read returns a string, not an int
            r_handlers[read_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id,sizeof(Ptr)));
            };
            x_handlers[read_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                char buffer[4096];
                std::string request;
                while(true) {
                    int bytes = read(fd, buffer, sizeof(buffer)-1);
                    if(bytes <= 0) break;
                    buffer[bytes] = 0;
                    request += buffer;
                    if(bytes < (int)sizeof(buffer)-1) break;
                }
                Ptr ticket = get_ticket(name_store_id,1,char_id);
                for(auto c : request) types[name_store_id][ticket.idx].push((void*)&c);
                ctx.node().value().set((void*)&ticket);
            };
        
            uint32_t write_id = make_tokenized_keyword("write");
            r_handlers[write_id] = make_int_node;
            x_handlers[write_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                //Second child is the string to write
                Ptr strptr = *(Ptr*)ctx.node().children()[1].value().get();
                Col& col = types[strptr.pool][strptr.idx];
                int result = ::write(fd, col.storage, col.size);
                ctx.node().value().set((void*)&result);
            };
        
            uint32_t close_id = make_tokenized_keyword("close");
            r_handlers[close_id] = make_int_node;
            x_handlers[close_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                ::close(fd);
            };

            x_handlers[make_tokenized_keyword("respond")] = [this](Context& ctx){
                int fd = *(int*)ctx.node().children()[0].value().get();
                string str = *(Ptr*)ctx.node().children()[1].value().get();
                print("RESPONDING TO:\n",str.to_std());
                std::string body = "<html><body> <p> hello world </p>  <body></html>";
                std::string response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n"
                    "\r\n" + body;
                print("Response:\n",response);
                if(::write(fd, response.c_str(), response.length()) < 0) {
                    print(red("server_id::x_handler write() failed"));
                }
            };


            x_handlers[make_tokenized_keyword("mem_test")] = [this](Context& ctx){
                uint32_t host_before = 0;
                uint32_t host_after = 0;
                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_before += types[t][c].size;
                    }
                }

                int iterations = 200;
                //readFile("mixos-acorn/web/webtest.gld");
                std::string sample = 
                //"int i = 5; print(i);"; 
                "Ptr Ptr Ptr int double_nested;\n"
                "Ptr Ptr int nested;\n"
                "Ptr int nums;\n"
                "nums.push(3);\n"
                "nums.push(8);\n"
                "nested.push(nums);\n"
                "Ptr int tums;\n"
                "tums.push(12);\n"
                "tums.push(14);\n"
                "nested.push(tums);\n"
                "double_nested.push(nested);\n"
                "print(double_nested.get(0).get(0).get(0));\n"
                "print(double_nested.get(0).get(0).get(1));\n"
                "print(double_nested.get(0).get(1).get(0));\n"
                "print(double_nested.get(0).get(1).get(1));\n";
            
                list<size_t> snapshots;
                
                for(int i = 0; i < iterations; i++) {
                    size_t before = current_memory_usage();
                    
                    Log::Line total; total.start();
                    Log::Line l; l.start();
                    g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                    print("INIT TIME: ",ftime(l.end())); l.start();
                    Node root = twig->process(sample);
                    print("PROCESS TIME: ",ftime(l.end())); l.start();
                    twig->compile(root);
                    print("COMPILE TIME: ",ftime(l.end())); l.start();
                    twig->start_stage(x_handlers);
                    twig->standard_travel_pass(root);
                    print("EXECUTE TIME: ",ftime(l.end())); l.start();
                    print("TOTAL TIME: ",ftime(total.end()));

                    units.removeAt(twig->uid);
                    twig->release();

                    size_t after = current_memory_usage();
                    snapshots << after;
                    print("iter ",i,": ",before," -> ",after," (delta: ",((int64_t)after-(int64_t)before),")");
                }

                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_after += types[t][c].size;
                    }
                }
                print("Host pool growth: ", (int)host_after - (int)host_before);
                
                // Print overall trend
                if(snapshots.length() > 1) {
                    int64_t total_growth = (int64_t)snapshots.last() - (int64_t)snapshots[0];
                    print("Total growth over ",iterations," iterations: ",total_growth," bytes");
                    print("Average per iteration: ",total_growth/iterations," bytes");
                }
            };

            x_handlers[make_tokenized_keyword("fragment_highlight")] = [this](Context& ctx) {
                std::string source = ctx.sub().source().to_std();
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                print("MEMORY USED: ",current_memory_usage());

                std::string out = "";
                g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                if(instruction=="compile") {
                    Log::Line l; l.start();
                    Node root = twig->process(content);
                    twig->compile(root);
                    double a_time = l.end(); l.start();
                    out += fnodenet_to_string(root,Stamper{[this](Node n, list<int>& offsets){
                        std::string to_return = n.name().to_std();
                        if(n.type()!=0) {
                            std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
                            while((int)n.y()>=offsets.length()) {offsets<<0;}
                            n.x(n.x()+offsets[(int)n.y()]);
                            offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                            to_return = nreturn;
                        }
                        return to_return;
                    },[this](Node n){
                        list<Node> stamps;
                        map<uint64_t,bool> visited;
                        collect_stamps(n,stamps,visited);
                        return stamps;
                    }});
                    double b_time = l.end(); 
                    // l.start();
                    //print_root(root);
                    // double c_time = l.end();

                    print("A: ",ftime(a_time));
                    print("B: ",ftime(b_time));
                    //print("C: ",ftime(c_time));

                    // print(node_to_string(root));

                    // recycle_node(root); //Deal with memory managment later, like in the mem_test
                    // units.erase(twig);

                    print("POST TWIG: ",current_memory_usage());

                } else if(instruction=="end") {
                    // print("REQUEST TO END: ",target," OF ",servers.length());
                    // g_ptr<Server> to_end = get_server(target);
                    // if(to_end) {
                    //     ::close(to_end->fd); 
                    //     to_end->fd = -1;
                    //     to_end->thread->end();
                    //     servers.erase(to_end);
                    // } else {
                    //     print(red("Unable to find server "+target+" to end"));
                    // }
                } else if(instruction=="preview") {
                    Node root = twig->process(content);
                    twig->run(root);

                    int port_num = 8081;
                    // for(auto c : root->children) {
                    //     if(c->type==server_id) {
                    //         for(auto sc : c->scope()->children) {
                    //             if(sc->type==port_id) {
                    //                 port_num = sc->left()->value->get<int>();
                    //             }
                    //         }
                    //     }
                    // }
                    servers << twig->servers;
                    servers.last()->label = target;
                    print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
                    out = std::to_string(port_num);
                } else if(instruction=="read") {
                    out = readFile(content);
                } else {
                    print(red("Unrecognized instruction for fragment: "+ctx.sub().source().to_std()));
                }
                ctx.sub().source() = out;
            };
            


        }
    };
}