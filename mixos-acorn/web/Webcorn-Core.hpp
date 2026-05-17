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

        void init() override {
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