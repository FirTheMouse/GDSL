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

    map<std::string,size_t> routes;

    size_t make_route(const std::string& path) {
        size_t id = reg_id(path);
        routes.put(path,id);
        return id;
    }

    size_t server_id = reg_id("server");
    size_t port_id = reg_id("port");
    size_t request_id = reg_id("request");
    size_t response_id = reg_id("response");

    size_t root_route = make_route("/");

    map<uint32_t,Handler> p_handlers; Handler p_default_function;

    void init() override {


        x_handlers[port_id] = [this](Context& ctx) {
            //Does nothing because the value would have already been set earlier via literal in the t stage.
        };

        x_handlers[server_id] = [this](Context& ctx){
            print("Starting a server");
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd < 0) {
                print(red("server_id::x_handler socket() failed"));
                return;
            }
            int port = 8080;
            if(ctx.node->left()) {
                port = ctx.node->left()->value->get<int>();
            }

            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr)); //C, setting evrything to 0 because no default constructer
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port); //Using port 8080
            addr.sin_addr.s_addr = INADDR_ANY;
    
            if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { //Bind our socket to port 8080
                print(red("server_id::x_handler bind() failed"));
                return;
            }
    
            if(listen(server_fd, 10) < 0) { //Set the socket to be passive, 10 is the backlog number
                print(red("server_id::x_handler listen() failed"));
                return;
            }

            ctx.node->value->set<int>(server_fd);

            while(true) {
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                ::socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd < 0) { //The socket to accept inputs into, accept sets the port and such per client
                    print(red("server_id::x_handler accept() failed"));
                    return;
                }
                print("Accepted: ",client_fd);
    
                char buffer[4096];
                memset(buffer, 0, sizeof(buffer));
                int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                print("Bytes read: ", bytes_read);
                if(bytes_read < 0) { //And just reading from the client buffer
                    print(red("server_id::x_handler read() failed"));
                    return;
                }
    
                std::string request(buffer);
                std::string first_line = request.substr(0, request.find("\r\n"));
                std::string method = first_line.substr(0, first_line.find(" "));
                std::string path = first_line.substr(first_line.find(" ") + 1);
                path = path.substr(0, path.find(" ")); //strip the HTTP/1.1
                print("Method: ",method);
                print("Path: ",path);
    
                if(method=="POST") {
                    size_t body_start = request.find("\r\n\r\n");
                    if(body_start != std::string::npos) {                        
                        Context sub_ctx;
                        sub_ctx.source = request.substr(body_start + 4);
                        p_handlers.getOrDefault(routes.getOrDefault(path,root_route),p_default_function)(sub_ctx);
                        std::string body = sub_ctx.source;
                        std::string response = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + std::to_string(body.length()) + "\r\n"
                            "\r\n" + body;
                        print("Response:\n",response);
                        if(::write(client_fd, response.c_str(), response.length()) < 0) {
                            print(red("server_id::x_handler write() failed"));
                        }
                    }
                } else {
                    Context sub_ctx;
                    standard_process(sub_ctx,routes.getOrDefault(path,root_route));
                    std::string body = sub_ctx.source;
                    std::string response = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + std::to_string(body.length()) + "\r\n"
                        "\r\n" + body;
                    print("Response:\n",response);
                    if(::write(client_fd, response.c_str(), response.length()) < 0) {
                        print(red("server_id::x_handler write() failed"));
                    }
                }
    
                close(client_fd);
            }
        };

        x_default_function = [this](Context& ctx){

        };

        p_default_function = [this](Context& ctx){

        };
    }

    void run(g_ptr<Node> server) override {
        start_stage(&x_handlers,x_default_function);
        process_node(server);
    }

};

}