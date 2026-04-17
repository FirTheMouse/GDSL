#pragma once


#define _UUID_T
typedef unsigned char uuid_t[16];


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "../core/GDSL.hpp"

namespace GDSL {

struct Web_Unit : public virtual Unit {
    Web_Unit() { init(); }
    map<std::string,size_t> routes;
    map<size_t,g_ptr<Node>> route_nodes;

    size_t make_route(const std::string& path) {
        size_t id = reg_id(path);
        routes.put(path,id);
        return id;
    }

    size_t server_id = reg_id("server");
    size_t port_id = reg_id("port");
    size_t request_id = reg_id("request");
    size_t response_id = reg_id("response");
    size_t accept_id = reg_id("accept");
    size_t listen_id = reg_id("listen");
    size_t close_id = reg_id("close");

    size_t root_route = make_route("/");

    Stage& p_handlers = reg_stage("posting");
    Stage& fragment_handlers = reg_stage("fragmenting");

    void init() override {
        x_handlers[server_id] = [this](Context& ctx){
            print("Starting a server");
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd < 0) {
                print(red("server_id::x_handler socket() failed"));
                return;
            }
            ctx.node->value->set<int>(server_fd);

            if(ctx.node->scope()) {
                standard_travel_pass(ctx.node->scope(),&ctx);
            }
        };

        x_handlers[listen_id] = [this](Context& ctx){
            ctx.node->value->set<int>(ctx.root->owner->value->get<int>()); //Propagate the server_fd
            if(ctx.node->scope()) {
                while(true) {
                    standard_travel_pass(ctx.node->scope(),ctx.sub);
                }
            }
        };

        x_handlers[accept_id] = [this](Context& ctx){
            int server_fd = ctx.root->owner->value->get<int>();
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            ::socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if(client_fd < 0) { //The socket to accept inputs into, accept sets the port and such per client
                print(red("server_id::x_handler accept() failed"));
                return;
            }
            ctx.sub->state = client_fd;
        };

        x_handlers[request_id] = [this](Context& ctx){
            char buffer[4096];
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = read(ctx.sub->state, buffer, sizeof(buffer) - 1);
            print("Bytes read: ", bytes_read);
            if(bytes_read < 0) { //And just reading from the client buffer
                print(red("server_id::x_handler read() failed"));
                return;
            }
            std::string request(buffer);
            ctx.sub->source = request;
        };

        x_handlers[response_id] = [this](Context& ctx){
            std::string request = ctx.sub->source;
            std::string first_line = request.substr(0, request.find("\r\n"));
            std::string method = first_line.substr(0, first_line.find(" "));
            std::string path = first_line.substr(first_line.find(" ") + 1);
            path = path.substr(0, path.find(" ")); //strip the HTTP/1.1
            print("Method: ",method);
            print("Path: ",path);

            size_t route_id = routes.getOrDefault(path, root_route);
            ctx.sub->node = route_nodes[route_id]; 
            ctx.sub->root = ctx.sub->node->in_scope;

            if(method=="POST") {
                size_t body_start = request.find("\r\n\r\n");
                if(body_start != std::string::npos) {                        
                    ctx.sub->source = request.substr(body_start + 4);
                    p_handlers.run(route_id)(*ctx.sub);
                    std::string body = ctx.sub->source;
                    std::string response = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: " + std::to_string(body.length()) + "\r\n"
                        "\r\n" + body;
                    print("Response:\n",response);
                    if(::write(ctx.sub->state, response.c_str(), response.length()) < 0) {
                        print(red("server_id::x_handler write() failed"));
                    }
                }
            } else if(method=="GET") {
                x_handlers.run(route_id)(*ctx.sub);
                std::string body = ctx.sub->source;
                std::string response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n"
                    "\r\n" + body;
                print("Response:\n",response);
                if(::write(ctx.sub->state, response.c_str(), response.length()) < 0) {
                    print(red("server_id::x_handler write() failed"));
                }
            } else if(method=="FRAG") {
                size_t body_start = request.find("\r\n\r\n");
                if(body_start != std::string::npos) {    
                    std::string instruction = request.substr(body_start + 4);
                    std::string target = instruction.substr(0, instruction.find(" "));
                    ctx.sub->node = scan_for_node(target,ctx.sub->node->scope());
                    if(ctx.sub->node) {
                        ctx.sub->source = instruction;
                        fragment_handlers.run(route_id)(*ctx.sub);
                        std::string body = ctx.sub->source;
                        std::string response = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: " + std::to_string(body.length()) + "\r\n"
                            "\r\n" + body;
                        print("Response:\n",response);
                        if(::write(ctx.sub->state, response.c_str(), response.length()) < 0) {
                            print(red("server_id::x_handler write() failed"));
                        }
                    } else {
                        print(red("response:x_handler unable to find node "+target+" in route "+path));
                    }
                }
            }
        };

        fragment_handlers.default_function = [this](Context& ctx){
            print("Default fragment, served instruction: ",ctx.source);
            print("Node: ",node_info(ctx.node));
        };

        x_handlers[close_id] = [this](Context& ctx){
            close(ctx.sub->state);
        };

        x_handlers[port_id] = [this](Context& ctx) {
            int port = 8080;
            if(ctx.node->left()) {
                port = ctx.node->left()->value->get<int>();
            }
            int server_fd = ctx.root->owner->value->get<int>();

            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr)); //C, setting evrything to 0 because no default constructer
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
    
            if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { //Bind our socket to the port
                print(red("server_id::x_handler bind() failed"));
                return;
            }
    
            if(listen(server_fd, 10) < 0) { //Set the socket to be passive, 10 is the backlog number
                print(red("server_id::x_handler listen() failed"));
                return;
            }
        };
    }

    void run(g_ptr<Node> server) override {
        start_stage(x_handlers);
        process_node(server);
    }

};

}