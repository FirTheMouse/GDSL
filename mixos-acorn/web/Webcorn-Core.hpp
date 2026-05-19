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

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core() {init();}

        uint32_t property_id = reg_id("property");
        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined");
        uint32_t invisible_id = reg_id("invisible");
        uint32_t component_id = reg_id("component");


        _lookup is_structural{{
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload",
            "role","lang","colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node.mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            for(int q=0;q<ctx.node.quals().length();q++) {
                Node qual = ctx.node.quals()[q];
                for(int i=0;i<qual.children().length();i++) {
                    Node c = qual.children()[i];
                    if(c.type()==property_id) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(ctx,c.children()[0]);
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(ctx,c.children()[1]);
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
            ctx.sub->sub->source.push(s);
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        void standard_gather_from_scope(Context& ctx) {
            Node node = ctx.node;
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
                                //instantiate_template(c,ref->owner);
                            }
                        } else {
                            print("ACTUAL FUNC CALL");
                            //Is an actual func call, handle as such
                        }
                    } else if(c.type() == func_decl_id) {

                    } else if(c.children().empty()) {
                        if(c.value().type()==inlined_id) {
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

        void init() override {

            set_binding_powers(colon_id,4,6);

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                ctx.node = ctx.sub->node;
                standard_gather_from_scope(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_inline_html")] = [this](Context& ctx){
                if(ctx.sub->node.scopes().empty()) return;
                ctx.node = ctx.sub->node.scopes()[0];
                emit_inline_html(ctx);
            };

            x_handlers[make_tokenized_keyword("webcorn")] = [this](Context& ctx){
                ctx.node.value(make_value(int_id,4));
                int server_fd = 6;
                ctx.node.value().set((void*)&server_fd);
            };

            auto make_int_node = [this](Context& ctx){
                ctx.node.value(make_value(int_id,4));
            };
        
            uint32_t socket_id = make_tokenized_keyword("socket");
            r_handlers[socket_id] = make_int_node;
            x_handlers[socket_id] = [this](Context& ctx){
                int fd = socket(AF_INET, SOCK_STREAM, 0);
                ctx.node.value().set((void*)&fd);
            };
        
            uint32_t bind_id = make_tokenized_keyword("bind");
            r_handlers[bind_id] = make_int_node;
            x_handlers[bind_id] = [this](Context& ctx){
                //Retrive fd and port from children
                int fd = *(int*)ctx.node.children()[0].value().get();
                int port = *(int*)ctx.node.children()[1].value().get();
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                addr.sin_addr.s_addr = INADDR_ANY;
                int result = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
                ctx.node.value().set((void*)&result);
            };
        
            uint32_t listen_id = make_tokenized_keyword("listen");
            r_handlers[listen_id] = make_int_node;
            x_handlers[listen_id] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                int result = listen(fd, 10);
                ctx.node.value().set((void*)&result);
            };
        
            uint32_t accept_id = make_tokenized_keyword("accept");
            r_handlers[accept_id] = make_int_node;
            x_handlers[accept_id] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
                ctx.node.value().set((void*)&client_fd);
            };
        
            uint32_t read_id = make_tokenized_keyword("read");
            //Read returns a string, not an int
            r_handlers[read_id] = [this](Context& ctx){
                ctx.node.value(make_value(string_id,sizeof(Ptr)));
            };
            x_handlers[read_id] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                char buffer[4096];
                std::string request;
                while(true) {
                    int bytes = read(fd, buffer, sizeof(buffer)-1);
                    if(bytes <= 0) break;
                    buffer[bytes] = 0;
                    request += buffer;
                    if(bytes < (int)sizeof(buffer)-1) break;
                }
                Ptr ticket(name_store_id, types[name_store_id].note_value("request",sizeof(char),char_id), 0);
                for(auto c : request) types[name_store_id][ticket.idx].push((void*)&c);
                ctx.node.value().set((void*)&ticket);
            };
        
            uint32_t write_id = make_tokenized_keyword("write");
            r_handlers[write_id] = make_int_node;
            x_handlers[write_id] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                //Second child is the string to write
                Ptr strptr = *(Ptr*)ctx.node.children()[1].value().get();
                Col& col = types[strptr.pool][strptr.idx];
                int result = ::write(fd, col.storage, col.size);
                ctx.node.value().set((void*)&result);
            };
        
            uint32_t close_id = make_tokenized_keyword("close");
            r_handlers[close_id] = make_int_node;
            x_handlers[close_id] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                ::close(fd);
            };

            x_handlers[make_tokenized_keyword("respond")] = [this](Context& ctx){
                int fd = *(int*)ctx.node.children()[0].value().get();
                string str = *(Ptr*)ctx.node.children()[1].value().get();
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
        }
    };
}