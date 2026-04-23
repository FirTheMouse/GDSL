#pragma once


#define _UUID_T
typedef unsigned char uuid_t[16];


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "../ext/g_lib/core/thread.hpp"

#include "../modules/GDSL-Core.hpp"

namespace GDSL {

struct Web_Unit : public virtual Unit {
    Web_Unit() { init(); }
    map<std::string,size_t> routes;
    map<size_t,g_ptr<Node>> route_nodes;

    std::string generate_token() {
        unsigned char buf[32];
        int fd = open("/dev/urandom", O_RDONLY);
        read(fd, buf, 32);
        ::close(fd);
        std::string token = "";
        const char* hex = "0123456789abcdef";
        for(int i = 0; i < 32; i++) {
            token += hex[buf[i] >> 4];
            token += hex[buf[i] & 0xf];
        }
        return token;
    }

    size_t session_id = reg_id("SESSION");
    size_t ip_id = reg_id("IP");
    size_t role_id = reg_id("ROLE");
    size_t timestamp_id = reg_id("TIMESTAMP");
    map<std::string, g_ptr<Node>> sessions;
    map<size_t, list<std::string>> route_roles;

    std::string extract_cookie(const std::string& request, const std::string& name) {
        std::string cookie_header = "Cookie: ";
        size_t start = request.find(cookie_header);
        if(start == std::string::npos) return "";
        start += cookie_header.length();
        size_t end = request.find("\r\n", start);
        std::string cookies = request.substr(start, end - start);
        
        std::string search = name + "=";
        size_t pos = cookies.find(search);
        if(pos == std::string::npos) return "";
        pos += search.length();
        size_t pos_end = cookies.find(";", pos);
        return cookies.substr(pos, pos_end - pos);
    }

    size_t make_route(const std::string& path) {
        size_t id = reg_id(path);
        routes.put(path,id);
        return id;
    }

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
            print("Starting a server with ",servers.length()," others");
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd < 0) {
                print(red("server_id::x_handler socket() failed"));
                return;
            }
            ctx.node->value->set<int>(server_fd);

            g_ptr<Server> new_server = make<Server>();
            new_server->fd = server_fd;
            new_server->thread = make<Thread>();
            new_server->unit = this;
            servers << new_server;
            
            if(ctx.node->scope()) {
                g_ptr<Node> scope = ctx.node->scope();
                new_server->thread->run_blocking([this, scope, ctx]() mutable {
                    standard_travel_pass(scope, &ctx);
                });
            }
        
        };

        x_handlers[listen_id] = [this](Context& ctx){
            ctx.node->value->set<int>(ctx.root->owner->value->get<int>()); //Propagate the server_fd
            if(ctx.node->scope()) {
                while(true) {
                    if(standard_travel_pass(ctx.node->scope(),ctx.sub)) return;
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
                ctx.flag = true;
                return;
            }

            g_ptr<Node> ip_qual = make<Node>(ip_id);
            ip_qual->name = inet_ntoa(client_addr.sin_addr);
            ctx.sub->out = ip_qual;
            ctx.sub->state = client_fd;
        };

        x_handlers[request_id] = [this](Context& ctx){
            std::string request;
            char buffer[4096];
            while(true) {
                int bytes_read = read(ctx.sub->state, buffer, sizeof(buffer) - 1);
                if(bytes_read <= 0) break;
                buffer[bytes_read] = 0;
                request += buffer;
                if(bytes_read < (int)sizeof(buffer) - 1) break; // got everything
            }
            
            print("Total bytes read: ", request.length());
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
            
            if(route_roles.hasKey(route_id)) {
                list<std::string> valid_roles = route_roles.get(route_id);
                std::string token = extract_cookie(request, "session");
                g_ptr<Node> nosession = nullptr;
                g_ptr<Node> session = sessions.getOrDefault(token, nosession);
                if(session) {
                    std::string role = "";
                    for(auto q : session->quals) {
                        if(q->type == role_id) {
                            role = q->name;
                            break;
                        }
                    }
                    if(valid_roles.has(role)) {
                        goto Serve;
                    }
                }
                std::string response = 
                    "HTTP/1.1 302 Found\r\n"
                    "Location: /login\r\n"
                    "\r\n";
                ::write(ctx.sub->state, response.c_str(), response.length());
                print(red("response:x_hadnler missing required role"));
                return;
            }
            Serve:;

            if(method=="POST") {
                size_t body_start = request.find("\r\n\r\n");
                if(body_start != std::string::npos) {                        
                    ctx.sub->source = request.substr(body_start + 4);
                    p_handlers.run(route_id)(*ctx.sub);
                    std::string body = ctx.sub->source;
                    std::string response;
                    if(!body.empty() && body.substr(0,5) == "HTTP/") {
                        response = body; //Handler built the full response
                    } else {
                        response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + std::to_string(body.length()) + "\r\n"
                            "\r\n" + body;
                    }
                    //print("Response:\n",response);
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
                        //print("Response:\n",response);
                        if(::write(ctx.sub->state, response.c_str(), response.length()) < 0) {
                            print(red("server_id::x_handler write() failed"));
                        }
                    } else {
                        print(red("frag:x_handler unable to find node "+target+" in route "+path));
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