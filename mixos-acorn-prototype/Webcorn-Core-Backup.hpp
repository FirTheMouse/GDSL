#pragma once

//#include "../GDSL/mixos-acorn/Acorn-Script.hpp"
#include "../GDSL/mixos-acorn/Acorn-Workshop.hpp"


#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(fd) closesocket(fd)
    #define READ_SOCKET(fd, buf, len) recv(fd, buf, len, 0)
    #define WRITE_SOCKET(fd, buf, len) send(fd, buf, len, 0)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>

    #include <netinet/in.h>
    #include <netdb.h>

    #define _UUID_T
    typedef unsigned char uuid_t[16];

    #define CLOSE_SOCKET(fd) ::close(fd)
    #define READ_SOCKET(fd, buf, len) ::read(fd, buf, len)
    #define WRITE_SOCKET(fd, buf, len) ::write(fd, buf, len)
#endif

#if USE_TLS 
    #include <mbedtls/x509_crt.h>
    #include <mbedtls/ssl.h>
    #include <mbedtls/net_sockets.h>
    #include <psa/crypto.h>
    #include <mbedtls/error.h>
    #include <mbedtls/pk.h>
    std::string tls_get(const std::string& host, const std::string& path) {
        int ret;
        mbedtls_net_context server_fd;
        mbedtls_ssl_context ssl;
        mbedtls_ssl_config conf;
        mbedtls_x509_crt cacert;

        mbedtls_net_init(&server_fd);
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        psa_crypto_init();
        mbedtls_x509_crt_init(&cacert);

        // Connect
        ret = mbedtls_net_connect(&server_fd, host.c_str(), "443", MBEDTLS_NET_PROTO_TCP);
        if(ret != 0) { print("Connect failed: ", ret); return ""; }

        // Setup
        mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE); // Skip cert verify for now
        mbedtls_ssl_setup(&ssl, &conf);
        mbedtls_ssl_set_hostname(&ssl, host.c_str());
        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

        ret = mbedtls_ssl_handshake(&ssl);
        if(ret != 0) { print("Handshake failed: ", ret); return ""; }

        // Send GET request
        std::string request = "GET " + path + " HTTP/1.1\r\n"
            "Host: " + host + "\r\n"
            "Connection: close\r\n\r\n";
        //print("REQUEST: ",request);
        ret = mbedtls_ssl_write(&ssl, (const unsigned char*)request.c_str(), request.length());
        if(ret < 0) { print("Write failed: ", ret); return ""; }
        //print("Wrote: ", ret, " bytes");


        //Give server time to respond
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string response;
        unsigned char buf[4096];
        int len;
        int retries = 0;
        while(true) {
            len = mbedtls_ssl_read(&ssl, buf, sizeof(buf)-1);
            if(len == MBEDTLS_ERR_SSL_WANT_READ) continue;
            if(len == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) continue;
            if(len == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) break;
            if(len < 0) {
                char error_buf[256];
                mbedtls_strerror(len, error_buf, sizeof(error_buf));
                print("Read error: ", error_buf);
                break;
            }
            buf[len] = 0;
            response += (char*)buf;
        }

        // Cleanup
        mbedtls_ssl_close_notify(&ssl);
        mbedtls_net_free(&server_fd);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);

        return response;
    }
    #define TLS_READ(ssl, buf, len) mbedtls_ssl_read(ssl, buf, len)
    #define TLS_WRITE(ssl, buf, len) mbedtls_ssl_write(ssl, buf, len)
    #define TLS_CLOSE(ssl) mbedtls_ssl_close_notify(ssl)

    struct tls_conn : q_object {
        mbedtls_ssl_context ssl;
        mbedtls_net_context net;
    };

    mbedtls_ssl_config tls_conf;
    mbedtls_x509_crt tls_cert;
    mbedtls_pk_context tls_key;

    void init_tls(const std::string& cert_path, const std::string& key_path) {
        psa_status_t psa_ret = psa_crypto_init();
        if(psa_ret != PSA_SUCCESS) {
            print("psa_crypto_init failed: ", (int)psa_ret);
        }
        mbedtls_ssl_config_init(&tls_conf);
        mbedtls_x509_crt_init(&tls_cert);
        mbedtls_pk_init(&tls_key);
    
        // Load cert
        std::string cert_data = readFile(cert_path);
        int ret = mbedtls_x509_crt_parse(&tls_cert, 
            (const unsigned char*)cert_data.c_str(), 
            cert_data.length() + 1); // +1 for null terminator
    
        // Load key
        std::string key_data = readFile(key_path);
        ret = mbedtls_pk_parse_key(&tls_key,
            (const unsigned char*)key_data.c_str(),
            key_data.length() + 1,
            nullptr, 0);
    
        // Configure
        mbedtls_ssl_config_defaults(&tls_conf, MBEDTLS_SSL_IS_SERVER,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_ca_chain(&tls_conf, tls_cert.next, nullptr);
        mbedtls_ssl_conf_own_cert(&tls_conf, &tls_cert, &tls_key);
    }  
#endif



namespace Acorn {
    //struct Webcorn_Core : public virtual Acorn_Script {
    struct Webcorn_Core : public virtual Workshop_Unit {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}


       uint32_t print_memory_id = add_function("print_memory",[this](Context& ctx){
            print(children_to_string(ctx),": ",current_rss()," | ",current_vsz());
       });

        struct qeue_request {
            qeue_request() {}
            qeue_request(std::string _session, int _fd, std::string _message, std::string _unitcode) : 
            session(_session), fd(_fd), message(_message), unitcode(_unitcode) {}
            std::string session = "";
            int fd = 0; 
            std::string message = "";
            std::string unitcode = "";
            #if USE_TLS
                g_ptr<tls_conn> tls = nullptr;
            #endif
        };
        struct Server : q_object {
            g_ptr<Thread> thread = nullptr;
            uint16_t unit = 0;
            list<qeue_request> requests;
            bool authourized = false;

            uint32_t getfd() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.index;}
            std::string getlabel() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.label.to_std();}
            uint32_t gethash() {std::lock_guard<std::mutex> lock(units_mutex); return units[unit]->types.hash;}
            void setfd(uint32_t fd) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.index = fd;}
            void setlabel(const std::string& label) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.label = label; }
            void sethash(const std::string& hashstr) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.hash = hashBytes(hashstr.data(), hashstr.length());}
            void sethash(uint32_t hash) {std::lock_guard<std::mutex> lock(units_mutex); units[unit]->types.hash = hash;}

            bool needshelp() {std::lock_guard<std::mutex> lock(units_mutex); return !units[unit]->types.live;}
        };

        struct Bot : q_object {
            g_ptr<Thread> thread = nullptr;
            g_ptr<Webcorn_Core> unit;  
            list<qeue_request> requests;     

            uint32_t taskID() {return unit->types.index;}
            std::string taskMessage() {return unit->types.label.to_std();}
            void task(uint32_t fd, const std::string& label) {unit->types.index = fd; unit->types.label = label;}
            bool needshelp() {return !unit->types.live;}
        };
    
        list<g_ptr<Server>> servers;
        std::mutex servers_mutex;

        list<g_ptr<Bot>> bots;
        std::mutex bot_mutex;


        std::string generate_token() {
            #ifdef _WIN32
                return "";
            #else
                unsigned char buf[32];
                int fd = open("/dev/urandom", O_RDONLY);
                ssize_t n = ::read(fd, buf, 32);
                if(n != 32) { ::close(fd); return ""; }
                ::close(fd);
                std::string token = "";
                const char* hex = "0123456789abcdef";
                for(int i = 0; i < 32; i++) {
                    token += hex[buf[i] >> 4];
                    token += hex[buf[i] & 0xf];
                }
                return token;
            #endif
        }
        uint32_t generate_token_id = add_function("generate_token",[this](Context& ctx){
            string output = resolve_string_ticket(ctx.node());
            output = generate_token();
        },sizeof(Ptr),string_id);


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
        uint32_t get_cookie_id = add_function("get_cookie",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string request = ctx.node().getString(0).to_std();
            std::string name = ctx.node().getString(1).to_std();
            string output = resolve_string_ticket(ctx.node());
            output = extract_cookie(request,name);
        },sizeof(Ptr),string_id);

        void cry(const std::string& message) {
            types.label = message;
            types.live = false;
            //print("Unit ",uid," cried for ",message);
            while(!types.live) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
           // rint("Unit ",uid," was answered ");
        }
        uint32_t cry_id = add_function("cry",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string message = ((string&)*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            cry(message);
        });



        map<std::string,bool> distributed_tokens;


        bool find_pool(uint32_t& to_return, ColColCol& col3, uint32_t tag, std::string label) {
            for(uint32_t p=0;p<col3.length();p++) {
                if(col3[p].tag==tag&&col3[p].label==label) {
                    to_return = p;
                    return true;
                }   
            }
            return false;
        }

        void move_file(const std::string& from, const std::string& to) {
            std::error_code ec;
            std::filesystem::path from_path(from);
            std::filesystem::path to_path(to);
        
            if(std::filesystem::is_directory(to_path)) {
                to_path /= from_path.filename();
            }
        
            std::filesystem::create_directories(to_path.parent_path(), ec);
            std::filesystem::rename(from_path, to_path, ec);
            if(ec) {
                print(red("webcorn:move_file failed to move "+from+" to "+to_path.string()+": "+ec.message()));
            }
        }
        uint32_t move_file_id = add_function("move_file",[this](Context& ctx){
            standard_sub_process(ctx);
            string from = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            string to = (string&)*(Ptr*)ctx.node().children()[1].value().get();
            move_file(from.to_std(),to.to_std());
        });

        std::string http_get(const std::string& host, const std::string& path, uint16_t port) {
            #ifdef _WIN32
                reutrn "IMPLMENT HTTP GET FOR WINDOWS";
            #else 
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if(sock < 0) { print("Socket failed"); return ""; }
            
                struct hostent* server = gethostbyname(host.c_str());
                if(!server) { print("Host not found"); close(sock); return ""; }
            
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
            
                if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    print("Connect failed"); close(sock); return "";
                }
            
                std::string request = "GET " + path + " HTTP/1.1\r\n"
                    "Host: " + host + "\r\n"
                    "Connection: close\r\n\r\n";
            
                send(sock, request.c_str(), request.length(), 0);
            
                std::string response;
                char buf[4096];
                int len;
                while((len = recv(sock, buf, sizeof(buf)-1, 0)) > 0) {
                    buf[len] = 0;
                    response += buf;
                }
            
                close(sock);
                return response;
            #endif
        }

        uint32_t http_get_id = add_function("http_get", [this](Context& ctx){
            standard_sub_process(ctx);
            std::string host = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            std::string path = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            int port = 8080;
            if(ctx.node().children().length()>2) {
                port = ctx.node().getInt(2);
            }
            //print("GETTING HTTP FROM ",host," AT ",path);
            string output = resolve_string_ticket(ctx.node());
            #if USE_TLS 
                output = tls_get(host, path);
            #else
                output = http_get(host,path,(uint16_t)port);
            #endif
        }, sizeof(Ptr), string_id);

        uint32_t init_tls_id = add_function("init_tls",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string cert = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            std::string key = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            #if USE_TLS
                init_tls(cert, key);
            #else
                //Do nothing because TLS is disabled
            #endif
        });

        #if USE_TLS
            g_ptr<tls_conn> current_tls;
        #endif

        int webcorn_socket() {
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd < 0) { print(red("socket() failed")); return -1; }
            return server_fd;
        }
        int webcorn_bind(int fd, int port, int opt) {
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt));
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
            int result = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
            return result;
        }
        int webcorn_listen(int fd) {
            int result = listen(fd, SOMAXCONN);
            return result;
        }
        int webcorn_accept(int fd) {
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
            if(client_fd == -1) { throw_error("accept failed"); return -1; }

            //Read timeout
            struct timeval tv;
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            #if USE_TLS
                g_ptr<tls_conn> conn = make<tls_conn>();
                mbedtls_ssl_init(&conn->ssl);
                mbedtls_net_init(&conn->net);
                conn->net.fd = client_fd;
                mbedtls_ssl_setup(&conn->ssl, &tls_conf);
                mbedtls_ssl_set_bio(&conn->ssl, &conn->net, mbedtls_net_send, mbedtls_net_recv, nullptr);
                int ret = mbedtls_ssl_handshake(&conn->ssl);
                if(ret != 0) {
                    if(ret != MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE) {
                        char err[256]; mbedtls_strerror(ret, err, sizeof(err));
                        //print(red("TLS handshake to "+std::to_string(client_fd)+" failed: "), err);
                    }
                    mbedtls_ssl_free(&conn->ssl);
                    ::close(client_fd);
                    return -1;
                }
                //print("Created a TLS connection to ",client_fd);
                current_tls = conn;
            #endif

            return client_fd;
        }
        std::string webcorn_read(int fd) {
            char buffer[4096];
            std::string request = "";

            if(fd==-1) return request;
            try {
                #if USE_TLS
                if(current_tls) {
                    mbedtls_ssl_context* ssl = &current_tls->ssl;
                    //print("Starting TLS read on fd ",fd);
                    while(true) {
                        int bytes = mbedtls_ssl_read(ssl, (unsigned char*)buffer, sizeof(buffer)-1);
                        //print("TLS read returned: ",bytes);
                        if(bytes == MBEDTLS_ERR_SSL_WANT_READ) {print("WANT_READ"); continue;}
                        if(bytes == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {print("SESSION_TICKET"); continue;}
                        if(bytes == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {print("PEER_CLOSE"); break;}
                        if(bytes == MBEDTLS_ERR_NET_CONN_RESET) {
                            break; //Browser dropped the connection, just return empty
                        }
                        if(bytes == 0) {
                            break; //Peer closed cleanly with no data
                        }
                        if(bytes <= 0) {
                            char err[256]; mbedtls_strerror(bytes, err, sizeof(err));
                            print("TLS read error: ",err);
                            break;
                        }
                        buffer[bytes] = 0;
                        //print("Read ",bytes," bytes: ",std::string(buffer,std::min(bytes,50)));
                        request += buffer;
                        size_t header_end = request.find("\r\n\r\n");
                        if(header_end != std::string::npos) {
                            size_t cl_pos = request.find("Content-Length: ");
                            if(cl_pos == std::string::npos) break; // No body, we're done
                            int content_length = std::stoi(request.substr(cl_pos+16));
                            std::string body = request.substr(header_end+4);
                            while((int)body.length() < content_length) {
                                int b = mbedtls_ssl_read(ssl, (unsigned char*)buffer, sizeof(buffer)-1);
                                if(b == MBEDTLS_ERR_SSL_WANT_READ) continue;
                                if(b <= 0) break;
                                buffer[b] = 0;
                                body += buffer;
                            }
                            request = request.substr(0, header_end+4) + body;
                            break;
                        }
                    }
                    //print("TLS read done, request length: ",request.length());
                    return request;
                }
                #endif

                while(true) {
                    int bytes = READ_SOCKET(fd, buffer, sizeof(buffer)-1);
                    if(bytes <= 0) break;
                    buffer[bytes] = 0;
                    request += buffer;
                    if(bytes < (int)sizeof(buffer)-1) break;
                }
                
                size_t header_end = request.find("\r\n\r\n");
                if(header_end != std::string::npos) {
                    size_t cl_pos = request.find("Content-Length: ");
                    if(cl_pos != std::string::npos) {
                        int content_length = std::stoi(request.substr(cl_pos+16));
                        std::string body = request.substr(header_end+4);
                        while((int)body.length() < content_length) {
                            int bytes = READ_SOCKET(fd, buffer, sizeof(buffer)-1);
                            if(bytes <= 0) break;
                            buffer[bytes] = 0;
                            body += buffer;
                        }
                        request = request.substr(0, header_end+4) + body;
                    }
                }
            } catch(std::exception& e) {
                print(red("webcorn:webcorn_read exception thrown: "),e.what());   
                request = "BADREQUEST";
            }
            return request;
        }
        void webcorn_write(int fd, const std::string& s) {
            #if USE_TLS
            if(current_tls) {
                mbedtls_ssl_context* ssl = &current_tls->ssl;
                size_t total = 0;
                while(total < s.size()) {
                    int ret = mbedtls_ssl_write(ssl,
                        (const unsigned char*)s.data() + total,
                        s.size() - total);
                    if(ret <= 0) {
                        char err[256]; mbedtls_strerror(ret, err, sizeof(err));
                        break;
                    }
                    total += ret;
                }
                return;
            } else {
                print(uid," has no current tls for write");
            }
            #endif
            ssize_t wres = WRITE_SOCKET(fd, (const char*)s.data(), s.size());
        }
        void webcorn_close(int fd) {
            #if USE_TLS
            if(current_tls) {
                mbedtls_ssl_close_notify(&current_tls->ssl);
                mbedtls_ssl_free(&current_tls->ssl);
                current_tls = nullptr;
            }
            #endif
            CLOSE_SOCKET(fd);
        }

        uint32_t socket_id = add_function("socket",[this](Context& ctx){
            #ifdef _WIN32
                WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
            #endif
            int server_fd = webcorn_socket();
            ctx.node().value().set((void*)&server_fd);
        },4,int_id);
    
        uint32_t bind_id = add_function("bind",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            int port = *(int*)ctx.node().children()[1].value().get();
            int opt = 1;
            int result = webcorn_bind(fd,port,opt);
            ctx.node().value().set((void*)&result);
        },4,int_id);
        
        uint32_t listen_id = add_function("listen",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            int result = webcorn_listen(fd);
            ctx.node().value().set((void*)&result);
        },4,int_id);
        
        uint32_t accept_id = add_function("accept",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            int client_fd = webcorn_accept(fd);
            ctx.node().value().set((void*)&client_fd);
        },4,int_id);

        uint32_t ip_of_id = add_function("ip_of",[this](Context& ctx){
            string output = resolve_string_ticket(ctx.node());
            #ifdef _WIN32
                output = "";
            #else
                standard_sub_process(ctx);
                int fd = ctx.node().getInt(0);
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);
                if(getpeername(fd, (struct sockaddr*)&addr, &len) == -1) {
                    output = "";
                    return;
                }
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
                output = std::string(ip);
            #endif
        },sizeof(Ptr),string_id);
        
        uint32_t read_id =  add_function("read",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            string output = resolve_string_ticket(ctx.node());
            output = webcorn_read(fd);
        },sizeof(Ptr),string_id);
        
        uint32_t write_id = add_function("write",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            string strptr = (string&)*(Ptr*)ctx.node().children()[1].value().get();
            webcorn_write(fd,strptr.to_std());
        });

        uint32_t close_id = add_function("close",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = *(int*)ctx.node().children()[0].value().get();
            webcorn_close(fd);
        });

        void dispatch_bot(uint32_t taskID, std::string taskLabel, std::string unitcode) {
            g_ptr<Bot> bot = nullptr;
            {
                std::lock_guard<std::mutex> lock(bot_mutex);
                for(int i=0;i<bots.length();i++) {
                    if(bots[i]->taskID()==0) {
                        bot = bots[i];
                        print("Found avaliable bot unit ",bot->unit->uid);
                        break;
                    }
                }
                if(!bot) {
                    g_ptr<Bot> new_bot = make<Bot>();
                    new_bot->thread = make<Thread>();
                    g_ptr<Webcorn_Core> webcorn = make_unit<Webcorn_Core>();
                    webcorn->uargs << uargs;
                    new_bot->unit = webcorn;
                    bots << new_bot;
                    bot = new_bot;
                    print("Dispatched a new bot unit ",bot->unit->uid);
                    new_bot->thread->run_blocking([webcorn, unitcode]() mutable {
                        webcorn->run(webcorn->process(unitcode));
                    });
                } 
            }
            bot->task(taskID,taskLabel);
        }

        uint32_t dispatch_bot_id = add_function("dispatch_bot",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t taskID = *(int*)ctx.node().children()[0].value().get();
            std::string taskLabel = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            std::string unitcode = string(*(Ptr*)ctx.node().children()[2].value().get()).to_std();
            dispatch_bot(taskID,taskLabel,unitcode);
        });

        void manage_bots(const std::string& unitcode) {
            while(true) {
                g_ptr<Webcorn_Core> unit = nullptr;
                qeue_request queued;
                bool has_queued = false;
                g_ptr<Bot> bot = nullptr;
                {
                    std::lock_guard<std::mutex> lock(bot_mutex);
                    for(int i=0;i<bots.length();i++) {
                        if(bots[i]->needshelp()) {
                            unit = bots[i]->unit;
                            bot = bots[i];
                            break;
                        } 
                        if(bots[i]->taskID()==0 && !bots[i]->requests.empty()) {
                            queued = bots[i]->requests.take(0);
                            has_queued = true;
                            bot = bots[i];
                            break;
                        }
                    }
                }
                if(unit) {
                    std::string fullreq = unit->types.label.to_std();
                    print("Unit ",unit->uid," has asked for ",fullreq);
                    list<std::string> req = split_str(fullreq,':');
                    std::string cmd = req[0];
                    std::string arg = req.length()>1?req[1]:"";
                    if(cmd=="REPORT") {
                        print("Bot reporting ",arg);
                    }
                    unit->types.live = true;
                }   
                else if(has_queued) {
                    print(green("Fuffiled a qeued request for a bot"));
                    bot->task(queued.fd, queued.message);
                } 
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }

        
        uint32_t start_bot_manager_id = add_function("start_bot_manager",[this](Context& ctx){
            std::string unitcode = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            g_ptr<Server> new_server = make<Server>();
            new_server->thread = make<Thread>();
            new_server->unit = uid;
            new_server->sethash("bot_manager");
            servers << new_server;
            new_server->thread->run_blocking([this, unitcode]() mutable {
                manage_bots(unitcode);
            });
        });

        std::string get_compiler_flag(const std::string& flag) {
            for(std::string& a : uargs) {
                if(a.find(flag) == 0) {
                    return a.substr(flag.length()+1); //Everything after the prefix
                }
            }
            return "";
        }

        //This is a thistle specific thing for now, until I can generilize it.
        uint32_t ledgerpool = 0;
        std::string sessionpath;

        void manage_sessions(const std::string& unitcode) {
            sessionpath = get_compiler_flag("--project");
            if(sessionpath.empty()) {
                sessionpath = "web/thistle/";;
            }
            //print("Session path: ",sessionpath);

            while(true) {
                g_ptr<Webcorn_Core> unit = nullptr;
                qeue_request queued;
                bool has_queued = false;
                g_ptr<Server> server = nullptr;
                {
                    std::lock_guard<std::mutex> lock(servers_mutex);
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->needshelp()) {
                            unit = as<Webcorn_Core>(units[servers[i]->unit]);
                            server = servers[i];
                            break;
                        } 
                        if(servers[i]->getfd()==0 && !servers[i]->requests.empty()) {
                            queued = servers[i]->requests.take(0);
                            has_queued = true;
                            server = servers[i];
                            break;
                        }
                    }
                }
                if(unit) {
                    std::string fullreq = unit->types.label.to_std();
                    //print("Unit ",unit->uid," has aksed for ",fullreq);
                    list<std::string> req = split_str(fullreq,':');
                    std::string cmd = req[0];
                    std::string arg = req.length()>1?req[1]:"";
                    if(cmd=="INVEST") {
                        ColCol& unitdata = unit->types[unitdata_col];
                        value_col unit_global_values(unit->uid, unitdata_col, global_value_table_idx);
                        if(unit_global_values.hasKey(arg)) {
                            Ptr dataptr = load_subunit(sessionpath+req[2]);
                            unit_global_values.get(arg).set((void*)&dataptr);
                        } else {
                            print(red("webcorn:manage_sessions no global variable was declared to recive investment of: "+req[2]));
                        }
                    } else {
                        print(red("webcorn:manage_sessions unrecognized command: "+cmd));
                    }
                    unit->types.live = true;
                }   
                else if(has_queued) {
                    //print(green("Fuffiled a qeued request"));
                    #if USE_TLS
                        as<Webcorn_Core>(units[server->unit])->current_tls = queued.tls;
                    #endif
                    server->setlabel(queued.message);
                    server->setfd(queued.fd);
                } 
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        uint32_t start_session_manager_id = add_function("start_session_manager",[this](Context& ctx){
            std::string unitcode = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            g_ptr<Server> new_server = make<Server>();
            new_server->thread = make<Thread>();
            new_server->unit = uid;
            new_server->sethash("session_manager");
            servers << new_server;
            //ledgerpool = load_sheet("web/thistle/ledger");
            new_server->thread->run_blocking([this, unitcode]() mutable {
                manage_sessions(unitcode);
            });
        });




        uint32_t dispatch_unit_id = add_function("dispatch_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string session = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            int server_fd = *(int*)ctx.node().children()[1].value().get();
            std::string message = string(*(Ptr*)ctx.node().children()[2].value().get()).to_std();
            std::string unitcode = string(*(Ptr*)ctx.node().children()[3].value().get()).to_std();

            uint32_t sessionhash = 0;
            bool any_can_serve = false;
            if(!session.empty()) {
                if(session=="ANYSERVE") {
                    any_can_serve = true;
                }
                else if(distributed_tokens.hasKey(session)) {
                    sessionhash = hashBytes(session.data(),session.length());
                }
            }

            g_ptr<Server> server = nullptr;
            {
                std::lock_guard<std::mutex> lock(servers_mutex);
                if(sessionhash!=0) {
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->gethash()==sessionhash) {
                            server = servers[i];
                            //print("Retrived session ",sessionhash," server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        print(red("webcorn:dispatch_unit no server matches the session "+session+", something has gone wrong with the session manager!"));
                        return;
                    }   
                    if(server->getfd()!=0) {
                        //print(yellow("Retrived server is busy, qeueing a request instead"));
                        qeue_request req(session,server_fd,message,unitcode);
                        #if USE_TLS
                            req.tls = current_tls; current_tls = nullptr;
                        #endif
                        server->requests << req;
                        return;
                    }
                } else {
                    for(int i=1;i<servers.length();i++) { //Server 1 is the session manager, so not avaliable for file serving
                        if((servers[i]->gethash()==0||any_can_serve) && servers[i]->getfd()==0) {
                            server = servers[i];
                            //print("Found avaliable server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        // if(servers.length()>10) {
                        //     webcorn_close(server_fd);
                        //     print(red("Refused connection because there are already too many servers"));
                        //     return;
                        // }
                        g_ptr<Server> new_server = make<Server>();
                        new_server->thread = make<Thread>();
                        g_ptr<Webcorn_Core> webcorn = make_unit<Webcorn_Core>();
                        webcorn->uargs << uargs;
                        new_server->unit = webcorn->uid;
                        new_server->sethash(0);
                        servers << new_server;
                        server = new_server;
                        uint16_t uid = webcorn->uid;
                        //print("Dispatched a new server unit ",uid);
                        new_server->thread->run_raw([webcorn, unitcode]() mutable {
                            webcorn->run(webcorn->process(unitcode));
                        });
                    } 
                }
            }
            #if USE_TLS
                as<Webcorn_Core>(units[server->unit])->current_tls = current_tls; current_tls = nullptr;
            #endif
            server->setlabel(message);
            server->setfd(server_fd);
            //print("Server fd is now ",server->getfd());
        });

        void setup_unit(uint32_t server_fd, const std::string& unitcode) {
            g_ptr<Server> server = nullptr;
            {
                std::lock_guard<std::mutex> lock(servers_mutex);
                g_ptr<Server> new_server = make<Server>();
                new_server->thread = make<Thread>();
                g_ptr<Webcorn_Core> webcorn = make_unit<Webcorn_Core>();
                webcorn->uargs << uargs;
                new_server->unit = webcorn->uid;
                new_server->sethash(0);
                servers << new_server;
                server = new_server;
                uint16_t uid = webcorn->uid;
                server->setfd(server_fd);
                new_server->thread->run_blocking_with_stack_size([webcorn, unitcode]() mutable {
                    webcorn->run(webcorn->process(unitcode));
                },1*1024*1024);
            }
        }
        uint32_t setup_unit_id = add_function("setup_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            int server_fd = ctx.node().getInt(0);
            std::string unitcode = ctx.node().getString(1).to_std();
            setup_unit(server_fd,unitcode);
        });

        uint32_t terminate_all_servers = add_function("terminate_all_servers",[this](Context& ctx){
            if(uid==1) {
                {   
                    print("Terminating ",units.length()-2," units "); //0 is the global unit, 1 is the main running unit, so they don't get terminated
                    std::lock_guard<std::mutex> lock(servers_mutex);
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->unit>1) {
                            print("Terminated unit ",servers[i]->unit);
                            g_ptr<Unit> unit = units[servers[i]->unit];
                            unit->suspend();
                            int fd = servers[i]->getfd();
                            if(fd > 0) CLOSE_SOCKET(fd);
                            servers[i]->thread->detach();
                            // for(int s=1;s<unit->subunits.length();s++) {
                            //     ColColCol& subunit = unit->subunits.get(s);
                            //     subunit.unlock();
                            //     bounce_subunit(&subunit);
                            // }
                        }
                    }
                }
                exit(0);
            } else {
                print(red("Webcorn:core UNIT "+std::to_string(uid)+" ATTEMPTED TO TERMINATE ALL SERVERS!"));
            }
        });


        bool is_websocket_upgrade(const std::string& request) {
            return request.find("Upgrade: websocket") != std::string::npos;
        }
        
        std::string get_websocket_key(const std::string& request) {
            size_t pos = request.find("Sec-WebSocket-Key: ");
            if(pos == std::string::npos) return "";
            pos += 19;
            size_t end = request.find("\r\n", pos);
            return request.substr(pos, end - pos);
        }
        
        std::string sha1_base64(const std::string& input) {
            // SHA1 constants
            uint32_t h0 = 0x67452301;
            uint32_t h1 = 0xEFCDAB89;
            uint32_t h2 = 0x98BADCFE;
            uint32_t h3 = 0x10325476;
            uint32_t h4 = 0xC3D2E1F0;
        
            // Pre-processing: adding padding
            std::string msg = input;
            uint64_t bit_len = input.size() * 8;
            msg += (char)0x80;
            while(msg.size() % 64 != 56) msg += (char)0x00;
            for(int i = 7; i >= 0; i--) msg += (char)((bit_len >> (i*8)) & 0xFF);
        
            // Process each 512-bit chunk
            for(size_t chunk = 0; chunk < msg.size(); chunk += 64) {
                uint32_t w[80];
                for(int i = 0; i < 16; i++) {
                    w[i] = ((uint8_t)msg[chunk+i*4]   << 24) |
                           ((uint8_t)msg[chunk+i*4+1] << 16) |
                           ((uint8_t)msg[chunk+i*4+2] << 8)  |
                           ((uint8_t)msg[chunk+i*4+3]);
                }
                for(int i = 16; i < 80; i++) {
                    uint32_t n = w[i-3]^w[i-8]^w[i-14]^w[i-16];
                    w[i] = (n<<1)|(n>>31); // left rotate 1
                }
        
                uint32_t a=h0, b=h1, c=h2, d=h3, e=h4;
        
                for(int i = 0; i < 80; i++) {
                    uint32_t f, k;
                    if(i<20)      { f=(b&c)|((~b)&d); k=0x5A827999; }
                    else if(i<40) { f=b^c^d;          k=0x6ED9EBA1; }
                    else if(i<60) { f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
                    else          { f=b^c^d;          k=0xCA62C1D6; }
        
                    uint32_t temp = ((a<<5)|(a>>27)) + f + e + k + w[i];
                    e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
                }
        
                h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
            }
        
            // Produce 20-byte digest
            uint8_t digest[20];
            for(int i=0;i<4;i++) {
                digest[i]    = (h0>>(24-i*8))&0xFF;
                digest[i+4]  = (h1>>(24-i*8))&0xFF;
                digest[i+8]  = (h2>>(24-i*8))&0xFF;
                digest[i+12] = (h3>>(24-i*8))&0xFF;
                digest[i+16] = (h4>>(24-i*8))&0xFF;
            }
        
            // Base64 encode
            const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out;
            for(int i=0;i<20;i+=3) {
                uint32_t n = ((uint32_t)digest[i]<<16) | 
                             (i+1<20?(uint32_t)digest[i+1]<<8:0) | 
                             (i+2<20?(uint32_t)digest[i+2]:0);
                out += b64[(n>>18)&63];
                out += b64[(n>>12)&63];
                out += (i+1<20) ? b64[(n>>6)&63] : '=';
                out += (i+2<20) ? b64[n&63]      : '=';
            }
            return out;
        }
        uint32_t secure_hash_id = add_function("secure_hash",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            output = sha1_base64(ctx.node().getString(0).to_std());
        },sizeof(Ptr),string_id);

        std::string make_websocket_accept(const std::string& key) {
            return sha1_base64(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        }

        std::string websocket_handshake(const std::string& accept_key) {
            return "HTTP/1.1 101 Switching Protocols\r\n"
                   "Upgrade: websocket\r\n"
                   "Connection: Upgrade\r\n"
                   "Sec-WebSocket-Accept: " + accept_key + "\r\n"
                   "\r\n";
        }

        bool read_exact(int fd, void* out, size_t length) {
            uint8_t* buffer = (uint8_t*)out;
            size_t total = 0;
            while(total < length) {
                int got = READ_SOCKET(fd,(char*)buffer+total,length-total);
                if(got <= 0) {return false;}
                total += got;
            }
            return true;
        }
        std::string websocket_read(int fd) {
            std::string message;
        
            while(true) {
                uint8_t header[2];
        
                if(!read_exact(fd, header, 2)) {
                    return "";
                }
        
                bool fin = header[0] & 0x80;
                uint8_t opcode = header[0] & 0x0F;
        
                if(opcode == 0x8) {
                    return "";
                }
        
                if(opcode != 0x0 && opcode != 0x1 && opcode != 0x2) {
                    return "";
                }
        
                bool masked = header[1] & 0x80;
                uint64_t length = header[1] & 0x7F;
        
                if(length == 126) {
                    uint8_t ext[2];
                    if(!read_exact(fd, ext, 2)) {
                        return "";
                    }
                    length = ((uint64_t)ext[0] << 8) | ((uint64_t)ext[1]);
                } else if(length == 127) {
                    uint8_t ext[8];
                    if(!read_exact(fd, ext, 8)) {
                        return "";
                    }
                    length = 0;
                    for(int i=0;i<8;i++) {
                        length = (length << 8) | ext[i];
                    }
                }
                uint8_t mask[4] = {0};
        
                if(masked) {
                    if(!read_exact(fd, mask, 4)) {
                        return "";
                    }
                }
                size_t old_size = message.size();
                message.resize(old_size + length);
        
                if(length > 0) {
                    if(!read_exact(fd,(uint8_t*)message.data()+old_size,length)) {
                        return "";
                    }
                }
                if(masked) {
                    for(size_t i=0;i<length;i++) {
                        message[old_size+i] ^= mask[i%4];
                    }
                }
                if(fin) {
                    return message;
                }
            }
        }

        void websocket_write(int fd, const std::string& message) {
            std::string frame;
            frame += (char)0x81; 
            size_t len = message.size();
            if(len <= 125) {
                frame += (char)len;
            } else if(len <= 65535) {
                frame += (char)126;
                frame += (char)((len>>8)&0xFF);
                frame += (char)(len&0xFF);
            } else {
                frame += (char)127;
                for(int i=7;i>=0;i--) frame += (char)((len>>(i*8))&0xFF);
            }
            
            frame += message;
            size_t wres = WRITE_SOCKET(fd, frame.data(), frame.size());
        }

        uint32_t is_websocket_id = add_function("is_websocket",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string request = ctx.node().getString(0).to_std();
            bool result = is_websocket_upgrade(request);
            ctx.node().value().set((void*)&result);
        },1,bool_id);
        uint32_t websocket_accept_id = add_function("websocket_accept",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = ctx.node().getInt(0);
            std::string request = ctx.node().getString(1).to_std();
            std::string key = get_websocket_key(request);
            bool result = false;
            if(!key.empty()) {
                std::string accept_key = make_websocket_accept(key);
                std::string response = websocket_handshake(accept_key);
                webcorn_write(fd, response);
                #ifndef _WIN32
                    struct timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;

                    setsockopt(
                        fd,
                        SOL_SOCKET,
                        SO_RCVTIMEO,
                        &tv,
                        sizeof(tv)
                    );
                #endif
                result = true;
            }
            ctx.node().value().set((void*)&result);
        },1,bool_id);
        uint32_t websocket_read_id = add_function("websocket_read",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = ctx.node().getInt(0);
            string output = resolve_string_ticket(ctx.node());
            output = websocket_read(fd);
        },sizeof(Ptr),string_id);
        uint32_t websocket_write_id = add_function("websocket_write",[this](Context& ctx){
            standard_sub_process(ctx);
            int fd = ctx.node().getInt(0);
            std::string message = ctx.node().getString(1).to_std();
            websocket_write(fd, message);
        });

        

        uint32_t make_webcorn_id = add_function("make_webcorn",[this](Context& ctx){
            standard_sub_process(ctx);
            auto unit = make_unit<Webcorn_Core>();
            uint32_t unitid = (uint32_t)unit->uid;
            ctx.node().value().set((void*)&unitid);
        },4,int_id);

        uint32_t compile_unit_id = add_function("compile_unit",[this](Context& ctx){
            //print(node_to_string(ctx.node()));
            standard_sub_process(ctx);
            int unitid = *(int*)ctx.node().children()[0].value().get();
            std::string source = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
            Node root = unit->process(source);
            unit->compile(root);
            //unit->start_logged_stage(unit->x_handlers);

            // ctx.node().scopes() << root;
            // root.owner(ctx.node());

            //unit->end_logged_stage();
            ctx.node().value().set((void*)&root);
        },sizeof(Ptr),node_id);

        // uint32_t run_unit_id = add_function("run_unit",[this](Context& ctx){
        //     //print(node_to_string(ctx.node()));
        //     standard_sub_process(ctx);
        //     int unitid = *(int*)ctx.node().children()[0].value().get();
        //     g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
        //     unit->start_logged_stage(unit->x_handlers);
        //     unit->standard_travel_pass(unit->unit_root);
        //     unit->end_logged_stage();
        // });

        uint32_t run_unit_id = add_function("run_unit",[this](Context& ctx){
            //print(node_to_string(ctx.node()));
            standard_sub_process(ctx);
            int unitid = *(int*)ctx.node().children()[0].value().get();
            std::string source = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            g_ptr<Webcorn_Core> unit = as<Webcorn_Core>(units[unitid]);
            Node root = unit->process(source);
            unit->run(root);
        });


        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined"); uint32_t prefix_inlined_id = reg_id("prefix_inlined");  uint32_t suffix_inlined_id = reg_id("suffix_inlined"); 
        uint32_t invisible_id = reg_id("invisible"); uint32_t prefix_invisible_id = reg_id("prefix_invisible"); uint32_t suffix_invisible_id = reg_id("suffix_invisible");
        uint32_t component_id = reg_id("component"); uint32_t prefix_component_id = reg_id("prefix_component");  uint32_t suffix_component_id = reg_id("suffix_component");
        uint32_t template_qual = add_qual("template");
        uint32_t stateless_qual = add_qual("stateless");
        uint32_t find_node_id = make_tokenized_keyword("find_node");
        // uint32_t capture_id = add_function("capture",[this](Context& ctx){
        //     if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") { //The property normally owns the scope, but if I find a way to change that...
        //         ctx.source() = "";
        //         standard_travel_pass(ctx.node().scopes()[0],ctx);
        //     } else {
        //         standard_sub_process(ctx);
        //         string output = resolve_string_ticket(ctx.node());
        //         output = "run('"+ctx.source().to_std()+"'";
        //         for(int i=0;i<ctx.node().children().length();i++) {
        //             Node arg = ctx.node().children()[i];
        //             output.push(","+string(*(Ptr*)arg.value().get()).to_std());
        //         }
        //         output.push(")");
        //     }
        // },sizeof(Ptr),string_id);

        std::string indent_html_text(const std::string& text) {
            list<std::string> s = split_str(text,'<');
            std::string indent = "";
            for(int i=0;i<s.length();i++) {
                if(s[i].empty()) continue;
                bool deindent = s[i].at(0)=='/';
                s[i].insert(0,"<");
                if(deindent) {
                    indent = indent.substr(0,indent.length()-2);
                    s[i].insert(0,indent);
                } else {
                    s[i].insert(0,indent);
                    indent += "  ";
                    size_t pos = 0;
                    while((pos = s[i].find('\n', pos)) != std::string::npos && pos!=s[i].length()-1) {
                        s[i].replace(pos, 1, "\n" + indent);
                        pos += indent.length() + 1;
                    }
                }
            }
            std::string to_return = "";
            for(auto si : s) to_return += si + "\n";
            return to_return;
        };
        uint32_t indent_html_id = add_function("indent_html",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            output = indent_html_text(ctx.node().getString(0).to_std());
        },sizeof(Ptr),string_id);

        
        std::string to_js_expr(std::string s) {
            std::string str = "(()=>{"+s+"})()";
            return str;
        }
        uint32_t to_js_expr_id = add_function("js_do",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = ctx.node().getString(0);
            CHECK_ERROR("Bad string in js_do!");
            output = to_js_expr(s.to_std());
        },sizeof(Ptr),string_id);

        std::string to_js_lit(std::string s) {
            return "'"+escape_js_string(s)+"'";
        }
        uint32_t to_js_lit_id = add_function("js_lit",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = ctx.node().getString(0);
            CHECK_ERROR("Bad string in js_lit!");
            output = to_js_lit(s.to_std());
        },sizeof(Ptr),string_id);


        Stage& html_handlers = reg_stage("htmlemiting");

        _lookup is_structural{{
            "style",
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload", "onmouseover",
            "onmousedown","onmouseup",
            "role", "lang", "colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster", "oncontextmenu",
            "inputmode", "enterkeyhint",
            "onscroll", "onresize", "ontouchstart", "ontouchend", "ontouchmove",
            "onwheel", "ondblclick", "onkeypress",
            "ondragstart", "ondrop", "ondragover",
            "accept"
        }, false};
        
        _lookup is_svg_structural{{
            "fill", "stroke", "stroke-width", "stroke-opacity",
            "fill-opacity", "d", "viewBox", "cx", "cy", "r",
            "x", "y", "width", "height", "points",
            "transform", "clip-path", "opacity",
            "x1", "y1", "x2", "y2", "rx", "ry",
            "preserveAspectRatio", "xmlns", "marker-end",
            "text-anchor", "dominant-baseline"
        }, false};

        bool in_svg = false;
        uint32_t set_in_svg = add_function("set_in_svg",[this](Context& ctx){
            standard_sub_process(ctx);
            bool b = *(bool*)ctx.node().children()[0].value().get();
            in_svg = b;
        });

        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-" || name.substr(0,5) == "aria-";
        }
        uint32_t is_prop_structural_id = add_function("is_prop_structural",[this](Context& ctx){
            standard_sub_process(ctx);
            string prop = ctx.node().getString(0);
            CHECK_ERROR("Bad property in is_prop_structural!");
            bool b = is_prop_structural(prop.to_std());
            ctx.node().value().set((void*)&b);
        },1,bool_id);
        uint32_t is_prop_structural_in_svg_id = add_function("is_prop_structural_in_svg",[this](Context& ctx){
            bool b = false;
            if(in_svg) {
                standard_sub_process(ctx);
                string prop = ctx.node().getString(0);
                CHECK_ERROR("Bad property in is_prop_structural_in_svg!");
                b = is_svg_structural[prop.to_std()];
            }
            ctx.node().value().set((void*)&b);
        },1,bool_id);

        std::string escape_js_string(const std::string& s) {
            std::string out;
            for(char c : s) {
                switch(c) {
                    case '\\': out += "\\\\"; break;
                    case '\'': out += "\\'"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c; break;
                }
            }
            return out;
        }

        uint32_t escape_js_id = add_function("escape_js",[this](Context& ctx){
            standard_sub_process(ctx);
            string content = resolve_string_ticket(ctx.node().children()[0]);
            std::string escaped = escape_js_string(content.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = escaped;
        },sizeof(Ptr),string_id);

        uint32_t escape_html_id = add_function("escape_html",[this](Context& ctx){
            standard_sub_process(ctx);
            string content = resolve_string_ticket(ctx.node().children()[0]);
            std::string escaped = html_escape_string(content.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = escaped;
        },sizeof(Ptr),string_id);

        uint32_t unescape_html_id = add_function("unescape_html",[this](Context& ctx){
            standard_sub_process(ctx);
            string content = resolve_string_ticket(ctx.node().children()[0]);
            std::string unescaped = html_unescape_string(content.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = unescaped;
        },sizeof(Ptr),string_id);

        bool resolve_prop_names(Context& ctx, Node c, std::string& prop, std::string& val) {
            Node saved_root = ctx.root();
            ctx.root(c);
            std::string old_source = ctx.source().to_std();

            if(c.children()[0].type()==literal_id) {
                prop = c.children()[0].name().to_std();
            } else if(c.children()[0].value().type()==string_id) {
                process_node(ctx,c.children()[0]);
                if(!is_live(c.children()[0].value().data_ptr())) { //For templates and such where we might use an identifer
                    ctx.root(saved_root);
                    ctx.source() = old_source;
                    return true;
                }
                prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
            } else {
                prop = c.children()[0].name().to_std();
            }

            if(c.children().length()>1) {
                if(c.children()[1].type()==literal_id) {
                    val = c.children()[1].name().to_std();
                } else if(c.children()[1].value().type()==string_id) {
                    // if(c.children()[1].name().to_std()=="capture"&&c.children()[1].children().length()==4) {
                    //     print(yellow("BEFORE:\n"),node_to_string(c));
                    // }
                    process_node(ctx,c.children()[1]);
                    // if(c.children()[1].name().to_std()=="capture"&&c.children()[1].children().length()==4) {
                    //     print(magenta("AFTER:\n"),node_to_string(c));
                    // }
                    if(!is_live(c.children()[1].value().data_ptr())) {
                        ctx.root(saved_root);
                        ctx.source() = old_source;
                        return true;
                    }
                    val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                } else {
                    val = c.children()[1].name().to_std();
                }
            } else if(!c.scopes().empty()) {
                val = "run('"+Ptr_to_string(c,c.cachelevel)+"')";
            }
            else {
                ctx.root(saved_root);
                ctx.source() = old_source;
                return true;
            }
            ctx.root(saved_root);
            ctx.source() = old_source;
            return false;
        }

        void gather_inline_props(
            Context& ctx, Node c,
            list<std::string>& structural_prop_labels, list<std::string>& structural_prop_values,
            list<std::string>& style_prop_labels, list<std::string>& style_prop_values
        ) {
            std::string prop = "";
            std::string val = "";

            if(resolve_prop_names(ctx,c,prop,val)) {return;}

            list<std::string>* prop_labels; list<std::string>* prop_values;
            if(is_prop_structural(prop)||(in_svg&&is_svg_structural[prop])) {
                
                prop_labels = &structural_prop_labels; 
                prop_values = &structural_prop_values;
            } else {
                prop_labels = &style_prop_labels; 
                prop_values = &style_prop_values;
            }

            if(!prop_labels->has(prop)) {
                prop_labels->push(prop);
                prop_values->push(val);
            }
        }

        void scan_for_inline_props(
            Context& ctx, Node node,
            list<std::string>& structural_prop_labels, list<std::string>& structural_prop_values,
            list<std::string>& style_prop_labels, list<std::string>& style_prop_values
        ) {
            if(node.type()==property_id||node.type()==to_unary_id(property_id)) {
                gather_inline_props(ctx,node,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
            } else if(node.type()==if_id) {
                process_node(ctx,node.children()[0]);
                if(node.getBool(0))  {
                    for(int j=0;j<node.scopes()[0].children().length();j++) {
                        Node jc = node.scopes()[0].children()[j];
                        scan_for_inline_props(ctx,jc,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                    }
                } else if(node.scopes().length()>1) {
                    for(int j=0;j<node.scopes()[1].children().length();j++) {
                        Node jc = node.scopes()[1].children()[j];
                        scan_for_inline_props(ctx,jc,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                    }
                }
            } else if(node.type()==equals_id) {
                process_node(ctx,node);
            } else if(node.type()==func_call_id&&node.has_qual(inlined_id)) {
                for(int j=0;j<node.scopes()[0].children().length();j++) {
                    Node jc = node.scopes()[0].children()[j];
                    scan_for_inline_props(ctx,jc,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                }
            } else {
                for(int i=0;i<node.children().length();i++) {
                    Node c = node.children()[i];
                    scan_for_inline_props(ctx,c,structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
                }
            }
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            scan_for_inline_props(ctx,ctx.node(),structural_prop_labels,structural_prop_values,style_prop_labels,style_prop_values);
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
            Node old_qual = ctx.qual(); //Consider jus removing the qual here, it can cause problems with the rerouting
            std::string old_source = ctx.source().to_std();
            ctx.source().col().clear();
            ctx.node(node);
            emit_inline_html(ctx);
            std::string to_reutrn = ctx.source().to_std();
            ctx.node(old_node);
            ctx.source() = old_source;
            ctx.qual(old_qual);
            return to_reutrn;
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(type,property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        Node get_prop(Context& ctx, Node node, std::string looking_for) {
            for(int i=0;i<node.children().length();i++) {
                Node c = node.children()[i];
                if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                    std::string prop = "";
                    std::string val = "";
                    if(resolve_prop_names(ctx,c,prop,val)) {
                        continue;
                    }
                    if(prop==looking_for) {
                        return c;
                    }
                }  
            }
            return deadptr;
        }

        Handler get_prop_handler = [this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            Node c = get_prop(ctx,node,looking_for);
            ctx.node().value().set((void*)&c);
        };
        uint32_t get_prop_id = overload_type(component_id,".\"get_prop\"","GET_PROP",make_global_value(node_id,sizeof(Ptr)),get_prop_handler);
        uint32_t get_prop_inlined_id = overload_type(inlined_id,".\"get_prop\"","GET_PROP",make_global_value(node_id,sizeof(Ptr)),get_prop_handler);

        Handler set_prop_handler = [this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            process_node(ctx, ctx.node().children()[1].children()[1]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            std::string set_to = resolve_string_ticket(ctx.node().children()[1].children()[1]).to_std();
            Node c = get_prop(ctx,node,looking_for);
            if(is_live(c)) {
                c.children()[1].name() = set_to;
            } else {
                print(yellow("webcorn:set_prop property "+looking_for+" not found while trying to set to "+set_to));
            }
        };
        uint32_t set_prop_id = overload_type(component_id,".\"set_prop\"","SET_PROP",make_global_value(node_id,sizeof(Ptr)),set_prop_handler);
        uint32_t set_prop_inlined__id = overload_type(inlined_id,".\"set_prop\"","SET_PROP",make_global_value(node_id,sizeof(Ptr)),set_prop_handler);

        Handler has_prop_handler = [this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            bool has = is_live(get_prop(ctx,node,looking_for));
            ctx.node().value().set((void*)&has);
        };
        uint32_t has_prop_id = overload_type(component_id,".\"has_prop\"","HAS_PROP",make_global_value(bool_id,1),has_prop_handler);
        uint32_t has_prop_inlined_id = overload_type(inlined_id,".\"has_prop\"","HAS_PROP",make_global_value(bool_id,1),has_prop_handler);

        int get_propidx(Context& ctx, Node node, std::string looking_for) {
            for(int i=0;i<node.children().length();i++) {
                Node c = node.children()[i];
                if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                    std::string prop = "";
                    std::string val = "";
                    if(resolve_prop_names(ctx,c,prop,val)) {
                        continue;
                    }
                    if(prop==looking_for) {
                        return i;
                    }
                }  
            }
            return -1;
        }
        Handler get_propidx_handler = [this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            int i = get_propidx(ctx,node,looking_for);
            ctx.node().value().set((void*)&i);
        };
        uint32_t get_propidx_id = overload_type(component_id,".\"get_propidx\"","GET_PROPIDX",make_global_value(int_id,4),get_propidx_handler);
        uint32_t get_propidx_inlined_id = overload_type(inlined_id,".\"get_propidx\"","GET_PROPIDX",make_global_value(int_id,4),get_propidx_handler);

        //This should definetly be replaced with a cleaner system eventually
        Node webcorn_node_scan(const std::string& label, Node from) {
            //print("SEARCHING: ",node_info(from));
            if(!from.scopes().empty()) {
                //print("SEARCHING ",from.scopes()[0].quals().length()," QUALS");
                for(int i=0;i<from.scopes()[0].children().length();i++) {
                    Node c = from.scopes()[0].children()[i];
                    //print("   LOOKING AT ",node_to_string(c));
                    if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(c.children()[0],deadptr);
                            if(!is_live(c.children()[0].value().data_ptr())) {
                                if(!is_live(c.children()[0].value().data_ptr())) {
                                    continue;
                                }
                            }
                            //print("Prop is: ",prop);
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(c.children()[1],deadptr);
                            if(!is_live(c.children()[1].value().data_ptr())) {
                                if(!is_live(c.children()[1].value().data_ptr())) {
                                    continue;
                                }
                            }
                            val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                            //print("Val is: ",val);
                        } else {
                            val = c.children()[1].name().to_std();
                        }

                        //print("   ",prop,":",val);
                    
                        if(prop=="id"&&val==label) return from;
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

        uint32_t color_of_string_id = add_function("color_of_string",[this](Context& ctx){
            standard_sub_process(ctx);
            string got = ctx.node().getString(0);
            CHECK_ERROR("Invalid first arg in color_of_string");
            std::string color = color_of_string(got.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = color;
        },sizeof(Ptr),string_id);

        uint32_t list_files_in_directory_id = add_function("list_files_in_directory",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            list<std::string> files = listFilesInDirectory(path);
            while(data.length() > files.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<files.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = files[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = files[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        
        uint32_t list_directories_in_directory_id = add_function("list_directories_in_directory",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            list<std::string> dirs = listDirectoriesInDirectory(path);
            while(data.length() > dirs.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<dirs.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = dirs[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = dirs[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t create_directory_id = add_function("create_directory",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::error_code ec;
            std::filesystem::create_directories(path, ec);
            if(ec) print(red("create_directory failed for "+path+": "+ec.message()));
        });
        uint32_t remove_file_id = add_function("remove_file",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::error_code ec;
            std::filesystem::remove(path, ec);
            if(ec) print(red("remove_file failed for "+path+": "+ec.message()));
        });
        uint32_t remove_directory_id = add_function("remove_directory",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
            if(ec) print(red("remove_directory failed for "+path+": "+ec.message()));
        });
        uint32_t rename_file_id = add_function("rename_file",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string path = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::string new_name = ((string&)(*(Ptr*)ctx.node().children()[1].value().get())).to_std();
            std::error_code ec;
            std::filesystem::rename(path, new_name, ec);
            if(ec) print(red("rename failed for "+path+": "+ec.message()));
        });

        //Just some experiments
        uint32_t clientsidescope_id = reg_id("clientsidescope");
        uint32_t clientside_id = add_function("clientside",[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr callptr = ctx.node().scopes()[0];
            string output = resolve_string_ticket(ctx.node());
            std::string out = "";
            out = "run('"+Ptr_to_string(callptr,callptr.cachelevel)+"'";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node arg = ctx.node().children()[i];
                if(arg.type()==var_decl_id) continue;
                out += ","+string(*(Ptr*)arg.value().get()).to_std();
            }
            out+=");";
            output = out;
        },sizeof(Ptr),string_id); 
        uint32_t serversidescope_id = reg_id("serversidescope");
        uint32_t serverside_id = add_function("serverside",[this](Context& ctx){

        }); 


        uint32_t init_propbin() {
            uint32_t at = types.add_idx();
            types[at].label = "Propbin";
            return at;
        }
        uint32_t propbin_col = init_propbin();
        //list<map<std::string,std::string>> propbin;


                
        void subemit(Context& ctx, std::string tag, bool self_closing, std::string structural, std::string content, bool local = false) {
            uint32_t sctructuralat = create_column(types[propbin_col],sizeof(Ptr),string_id); //Structural
            uint32_t stylisticat = create_column(types[propbin_col],sizeof(Ptr),string_id); //Stylistic
            uint32_t partsat = create_column(types[propbin_col],sizeof(Ptr),string_id); //Part

            Ptr strticket = get_ticket(name_store_id,1,char_id);
            types[propbin_col][partsat].push(&strticket);

            ctx.sub().source() = "";
            Node node = ctx.sub().node();
            if(node.scopes().length()==0) {
                if(is_live(node.left())) {
                    start_stage(html_handlers);
                    process_node(ctx,node.left());
                    start_stage(x_handlers);
                }
            } else {
                Node scope = node.scopes().get(0);
                for(int i=0;i<scope.children().length();i++) {
                    Node child = scope.children().get(i);
                    start_stage(html_handlers);
                    process_node(ctx,child);
                    start_stage(x_handlers);
                }
            }
            uint32_t plen = types[propbin_col].length()-types[propbin_col].free.length();

            Col& structural_props = types[propbin_col][sctructuralat];
            Col& stylistic_props = types[propbin_col][stylisticat];
            Col& part_props = types[propbin_col][partsat];
            
            ((string&)*(Ptr*)part_props[0]).push(ctx.sub().source());

            list<CCol*> structural_props_cells = structural_props.allCells();
            for(uint32_t i=0;i<structural_props_cells.length();i++) {
                CCol& cell = *structural_props_cells[i];
                structural+=((QString&)cell).to_std()+"=\""+((string&)*(Ptr*)structural_props[cell.index]).to_std()+"\" ";
            }
            std::string stylistic = "";
            list<CCol*> stylistic_props_cells = stylistic_props.allCells();
            for(uint32_t i=0;i<stylistic_props_cells.length();i++) {
                CCol& cell = *stylistic_props_cells[i];
                stylistic+=((QString&)cell).to_std()+": "+((string&)*(Ptr*)stylistic_props[cell.index]).to_std()+"; ";
            }
            std::string parts = ((string&)*(Ptr*)part_props[0]).to_std();

            if(stylistic.length()>0) {
                stylistic = " style=\""+stylistic+"\"";
            }
            if(structural.length()>0) {
                structural = " "+structural;
            }

            std::string part = "";
            if(self_closing) {
                part = "<"+tag+structural+stylistic+"/>";
            } else {
                part = "<"+tag+structural+stylistic+">"+content+parts+"</"+tag+">";
            }
            if(local) {
                if(is_live(ctx.sub().node())) {
                    string output = resolve_string_ticket(ctx.sub().node());
                    output = part;
                }
            } else if(plen>3) {
                ((string&)*(Ptr*)(types[propbin_col][plen-4])[0]).push(part);
            } else {
                ctx.sub().source() = part;
            }

            Acorn::recycle_column(types[propbin_col],partsat);
            Acorn::recycle_column(types[propbin_col],stylisticat);
            Acorn::recycle_column(types[propbin_col],sctructuralat);
            recycle_column(strticket);
        }

        void init() override {
            register_type("div",component_id,0);
            register_type("inlined",inlined_id,0);
            register_type("invisible",invisible_id,0);

            r_handlers[clientside_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()) {
                    ctx.node().scopes()[0].type(clientsidescope_id);
                }
            };
            x_handlers[clientsidescope_id] = [this](Context& ctx){
                ctx.state(standard_travel_pass(ctx.node(),ctx));
            };

            x_handlers[to_unary_id(property_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") {
                    ctx.source() = "";
                    standard_travel_pass(ctx.node().scopes()[0],ctx);
                }
            };
            Handler prop_handler = x_handlers[to_unary_id(property_id)];
            x_handlers[property_id] = prop_handler;


            //Potential idea, experiment with later
            //The idea is having the prop emission be driven by the props, making control flows and such more first class
            //But this would have performance issues and introduce extra compleixty in the current state (not to mention it doesn't work like this, source passing breaks it)
            // html_handlers[property_id] = [this](Context& ctx){
            //     std::string prop = ""; std::string val = "";
            //     resolve_prop_names(ctx,ctx.node(),prop,val);
            //     ctx.source().push(prop+"\0"+val+"\1");
            // };

             // void emit_inline_html(Context& ctx) {
            //     if(ctx.node().mute()) return;
            //     std::string s = "";
            //     list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            //     list<std::string> style_prop_labels; list<std::string> style_prop_values;
            //     std::string old_source = ctx.source().to_std();
            //     ctx.source().col().clear();
            //     standard_sub_process(ctx);
            //     list<std::string> prop_groups  = split_str(ctx.source().to_std(),'\1');
            //     for(int i=0;i<prop_groups.length();i++) {
            //         std::string property = prop_groups[i];
            //         list<std::string> props  = split_str(property,'\0');
            //         std::string prop = ""; if(props.length()>0) prop = props[0];
            //         std::string val = ""; if(props.length()>1) val = props[1];
            //         list<std::string>* prop_labels; list<std::string>* prop_values;
            //         if(is_prop_structural(prop)) {
            //             prop_labels = &structural_prop_labels; 
            //             prop_values = &structural_prop_values;
            //         } else {
            //             prop_labels = &style_prop_labels; 
            //             prop_values = &style_prop_values;
            //         }
        
            //         if(!prop_labels->has(prop)) {
            //             prop_labels->push(prop);
            //             prop_values->push(val);
            //         }
            //     }
            //     for(int i=0;i<structural_prop_labels.length();i++) {
            //         s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            //     }   
            //     if(!style_prop_labels.empty()) {
            //         s += " style=\"";
            //         for(int i=0;i<style_prop_labels.length();i++) {
            //             s += style_prop_labels[i]+":"+style_prop_values[i]+";";
            //         }  
            //         s += "\""; 
            //     }
            //     ctx.sub().source().push(s);
            // }

            //M is just the stage when this is most viable, any earlier and we get some issues
            m_handlers[property_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node().scopes().empty()&&ctx.node().children().length()>1) { //Yield the scope
                    ctx.node().children()[1].scopes().push(ctx.node().scopes().take(0));
                    ctx.node().children()[1].scopes()[0].owner(ctx.node().children()[1]);
                }
            };
            t_handlers[capture_id] = [this](Context& ctx){
                ctx.node().value(make_root_value(string_id,sizeof(Ptr)));
                if(ctx.node().scopes().empty()) { 
                    standard_sub_process(ctx);
                } else {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        place_node_in_scope(ctx.node().children()[i],ctx.node().scopes()[0]);
                    }
                }
            };
            x_handlers[capture_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()&&ctx.node().scopes()[0].owner()!=ctx.node()) {
                    ctx.node().scopes()[0].owner(ctx.node());
                }
                if(!ctx.node().scopes().empty()&&ctx.source().to_std()=="go") {
                    ctx.source() = "";
                    standard_travel_pass(ctx.node().scopes()[0],ctx);
                } else {
                    standard_sub_process(ctx);
                    string output = resolve_string_ticket(ctx.node());
                    output = "run('"+Ptr_to_string(ctx.node(),ctx.node().cachelevel)+"'";
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node arg = ctx.node().children()[i];
                        if(arg.type()==var_decl_id) {
                            output.push(",'[VAR]'");
                        } else {
                            //print("ARG: ",node_to_string(arg));
                            output.push(","+string(*(Ptr*)arg.value().get()).to_std());
                        }
                    }
                    output.push(")");
                }
            };

            

            add_function("uspan_flame_chart",[this](Context& ctx){
                string output = resolve_string_ticket(ctx.node());
                output = uspan->print_as_flamechart();
            },sizeof(Ptr),string_id);


            html_handlers.default_function = [this](Context& ctx) {
                if(is_live(ctx.qual())) {
                    //print("Running qual: ",node_info(ctx.qual()),red(" for "),node_info(ctx.node()));
                    if(is_live(ctx.value())) {
                        //print("Rerouting to value");
                        x_handlers.run(to_prefix_id(ctx.qual().type()))(ctx);
                    } else {
                        //print("Rerouting to node");
                        x_handlers.run(to_suffix_id(ctx.qual().type()))(ctx);
                    }
                } else {
                    //print("Running: ",node_info(ctx.node()));
                    //print("Rerouting to X");
                    x_handlers.run(ctx.node().type())(ctx);
                }
            };


            // add_function("subemit",[this](Context& ctx){
            //     standard_sub_process(ctx);
            //     std::string tag = ctx.node().getString(0).to_std();
            //     bool self_closing = ctx.node().getBool(1);
            //     std::string content = ctx.node().getString(3).to_std();

            //     std::string structural = ctx.node().getString(2).to_std();

            //     map<std::string,std::string> structural_props;
            //     map<std::string,std::string> stylistic_props;
            //     map<std::string,std::string> part_props;
                
            //     propbin.push(structural_props); propbin.push(stylistic_props); propbin.push(part_props);

            //     ctx.sub().source() = "";
            //     Node node = ctx.sub().node();
            //     if(node.scopes().length()>0) {
            //         Node scope = node.scopes().get(0);
            //         for(int i=0;i<scope.children().length();i++) {
            //             Node child = scope.children().get(i);
            //             start_stage(html_handlers);
            //             process_node(ctx,child);
            //             start_stage(x_handlers);
            //         }
            //     }
            //     std::string newsrc = ctx.sub().source().to_std();
            //     part_props = propbin.pop();
            //     stylistic_props = propbin.pop(); 
            //     structural_props = propbin.pop(); 
            //     part_props["ANY"]+=newsrc;

            //     for(auto& e : structural_props.entrySet()) {
            //         structural+=e.key+"=\""+e.value+"\" ";
            //     }
            //     std::string stylistic = "";
            //     for(auto& e : stylistic_props.entrySet()) {
            //         stylistic+=e.key+": "+e.value+"; ";
            //     }
            //     std::string parts = part_props["ANY"];

            //     if(stylistic.length()>0) {
            //         stylistic = " style=\""+stylistic+"\"";
            //     }
            //     if(structural.length()>0) {
            //         structural = " "+structural;
            //     }

            //     std::string part = "";
            //     if(self_closing) {
            //         part = "<"+tag+structural+stylistic+"/>";
            //     } else {
            //         part = "<"+tag+structural+stylistic+">"+content+parts+"</"+tag+">";
            //     }
            //     if(propbin.length()>0) {
            //         propbin[propbin.length()-1]["ANY"]+=part;
            //     } else {
            //         ctx.sub().source() = part;
            //     }
            // });
            // html_handlers[property_id] = [this](Context& ctx){
            //     standard_sub_process(ctx);
            //     std::string prop; 
            //     std::string val;

            //     if(ctx.node().children().length()==2) {
            //         prop = ctx.node().getString(0).to_std();
            //         val = ctx.node().getString(1).to_std();
            //     } else if(ctx.node().children().length()==1) {
            //         std::string name = ctx.node().name().to_std();
            //         if(name.at(0)==':') {
            //             prop = name.substr(name.find(':')+1);
            //             val = ctx.node().getString(0).to_std();
            //         } else {
            //             val = name.substr(0, name.find(':'));
            //             prop = ctx.node().getString(0).to_std();
            //         }
            //     } else if(ctx.node().children().length()==0) {
            //         std::string name = ctx.node().name().to_std();
            //         val = name.substr(0,name.find(':'));
            //         prop = name.substr(name.find(':')+1);
            //     }

            //     if(is_prop_structural(prop)||(in_svg&&is_svg_structural[prop])) {
            //         propbin[propbin.length()-3].getOrPut(prop,val);
            //     } else {
            //         propbin[propbin.length()-2].getOrPut(prop,val);
            //     }
            // };


            add_function("subemit",[this](Context& ctx){
                standard_sub_process(ctx);
                std::string tag = ctx.node().getString(0).to_std();
                bool self_closing = ctx.node().getBool(1);
                std::string content = ctx.node().getString(3).to_std();
                std::string structural = ctx.node().getString(2).to_std();
                subemit(ctx,tag,self_closing,structural,content);
            });
            add_function("local_subemit",[this](Context& ctx){
                standard_sub_process(ctx);
                std::string tag = ctx.node().getString(0).to_std();
                bool self_closing = ctx.node().getBool(1);
                std::string content = ctx.node().getString(3).to_std();
                std::string structural = ctx.node().getString(2).to_std();
                subemit(ctx,tag,self_closing,structural,content,true);
            },sizeof(Ptr),string_id);
            add_function("emit_part",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t binat = types[propbin_col].length()-types[propbin_col].free.length();
                ((string&)*(Ptr*)types[propbin_col][binat-1][0]).push(ctx.node().getString(0));
            });
            html_handlers[property_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string prop = ctx.node().getString(0);
                string val = ctx.node().getString(1);
                uint32_t binat = types[propbin_col].length()-types[propbin_col].free.length();
                if(is_prop_structural(prop.to_std())||(in_svg&&is_svg_structural[prop.to_std()])) {
                    binat = binat-3;
                } else {
                    binat = binat-2;
                }
                Col& bin = types[propbin_col][binat];
                if(!bin.hasKey(prop.col().storage,prop.col().size)) {
                    bin.qput(&val,prop.col().storage,prop.col().size,string_id);
                }
            };
            
            
            // r_handlers[property_id] = [this](Context& ctx){
            //     standard_sub_process(ctx);
            //     if(ctx.root().type()!=qmark_id) {
            //         ctx.node().name().col().clear();
            //         for(int i=1;i>=0;i--) {
            //             if(i==0) ctx.node().name().push(':');
            //             Node c = ctx.node().children()[i];
            //             if(c.type()==literal_id) {
            //                 ctx.node().name().push(c.name().to_std());
            //                 recycle_node(c);
            //                 ctx.node().children().removeAt(i);
            //             }
            //         }
            //     }
            // };


            html_handlers[func_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            // html_handlers[func_call_id] = [this](Context& ctx){
            //     uint32_t prelen = ctx.source().length();
            //     fire_quals(ctx,ctx.node().value());
            //     if(ctx.node().mute()) {ctx.node().mute(false); return;}
            //     bool needs_closing = ctx.source().length()!=prelen;
            //     x_handlers.run(func_call_id)(ctx);
            //     if(needs_closing&&prelen>0) {
            //         std::string src = ctx.source().to_std();
            //         size_t tag_start = prelen+1;
            //         size_t tag_end = src.find(' ', tag_start);
            //         if(tag_end == std::string::npos) tag_end = src.find('>', tag_start);
            //         std::string tag = src.substr(tag_start, tag_end - tag_start);
            //         ctx.source().push("</"+tag+">");
            //     }
            // };
            // html_handlers[prefix_component_id] = [this](Context& ctx){
            //     if(ctx.node().type()==func_call_id) {
            //         ctx.qual(deadptr); //Revoke it. Dealing with inner quals and their routing is a minefield!
            //         std::string props = emit_inline_html(ctx,ctx.value().type_scope());
            //         if(!props.empty()) {
            //             ctx.source().push("<div "+props+">");
            //         }
            //     } else if(ctx.node().type()==func_decl_id&&!ctx.node().has_qual(template_qual)) {
            //         if(ctx.node().children().length()==0) { //Implcit case, for when a div is paramatrized and named but not qualifed as a template, don't emit it!
            //             ctx.qual(deadptr); 
            //             html_handlers.run(component_id)(ctx);
            //         }
            //     }
            // };
            // x_handlers[prefix_component_id] = html_handlers[prefix_component_id];

            html_handlers[to_prefix_id(template_qual)] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id&&!ctx.node().has_qual(stateless_qual)) {
                    //print("Instantiating: [",ctx.node().scopes().length(),"] ",node_info(ctx.node()));
                    Node active_instance = deadptr;

                    std::string path = "";
                    Node climb = ctx.node(); //Create a key by climbing the scope, to handle recursion
                    while(is_live(climb.in_scope())&&is_live(climb.in_scope().owner())) {
                        Node climb_scope = climb.in_scope();
                        if(is_live(climb_scope.value())&&climb_scope.value().loc()>0) {
                            path += "-"+std::to_string(climb_scope.value().loc());
                        }
                        climb = climb_scope.owner();
                    }
                    if(!path.empty()) { //If we've been emitted multiple times because this call is in a loop
                        if(ctx.node().scopes().length()==1) { //Push our template scope in as a copy so that scope 1 remains the place to instantiate from
                            ctx.node().scopes() << ctx.node().scopes()[0];
                        }
                        //Add shrinking by checking against the node in 0 later, we can use it to understand what the previous highest path was and cull things that don't reach that watermark at iteration 0 of a given path
                        //That's a future memory optimization.
                        if(ctx.node().scopes().hasKey(path)) {
                            active_instance = ctx.node().scopes().get(path);
                        } else {
                            active_instance = instantiate_template_scope(ctx.node(),ctx.node().scopes()[1].owner(),ctx,true);
                            ctx.node().scopes().put(path,active_instance);
                        }

                        Node decl = active_instance.owner(); //Rebind the arguments
                        for(int i=0;i<ctx.node().children().length();i++) {
                            Node c = ctx.node().children()[i];
                            Node arg = decl.children()[i];
                            c.children().col().set(0,(void*)&arg);
                        }
                        ctx.node().scopes().col().set(0,(void*)&active_instance);
                        //print("Instance ",path,":\n",node_to_string(ctx.node()));
                    } else {
                        Node scope_owner = ctx.node().scopes()[0].owner();
                        if(scope_owner!=ctx.node()) { //If we haven't been instantiated yet
                            active_instance = instantiate_template_scope(ctx.node(),scope_owner,ctx,true);
                        } else {
                            active_instance = ctx.node().scopes()[0];
                        }
                    }
                    ctx.node().scopes().col().set(0,(void*)&active_instance); //Swap the active instance into 0 so it gets called
                }
            };
            x_handlers[to_prefix_id(template_qual)] = html_handlers[to_prefix_id(template_qual)];

            // r_handlers[prefix_inlined_id] = [this](Context& ctx){
            //     if(ctx.node().type()==func_call_id) {
            //         Node func = ctx.value().type_scope();
            //         // func.owner().mute(true); //To stop it from emitting 
            //         bool is_static = ctx.value().has_qual(static_qual);
            //         for(int i=0;i<func.children().length();i++) {
            //             if(is_static) {
            //                 Node copy = make_node(); //Make this a proper deep copy later (placeholder for now)
            //                 copy.copy(func.children()[i]);
            //                 ctx.result().insert(ctx.index(),copy);
            //             } else {
            //                 ctx.result().insert(ctx.index(),func.children()[i]);
            //             }
            //             ctx.index()++;
            //         }
            //     }
            // };

            html_handlers[DEBUG_ROOT_id] = [this](Context& ctx) {
                print("==HTML STAGE==");
                print(node_to_string(ctx.node().in_scope()));
            };

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                print(yellow("webcorn:gather_props this function is deprecated"));
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

            uint32_t display_node_id = make_tokenized_keyword("display_node");
            // r_handlers[display_node_id] = [this](Context& ctx){
            //     ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
            //     Ptr ticket = get_ticket(data_store_id,1,char_id);
            //     string contents(ticket);
                


            //     ctx.node().value().set((void*)&ticket);
            // };
            // x_handlers[display_node_id] = [this](Context& ctx){
            //     string addr(*(Ptr*)ctx.node().children()[0].value().get());
            //     string output(*(Ptr*)ctx.node().value().get());

                

            //     output = ("<p>"+addr.to_std()+"</p>");
            // };

            
            // add_function("stamp_source_to_terminal",[this](Context& ctx){
            //     g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
            //         if(instruction=="compile") {
            //             Log::Line l; l.start();
            //             Node root = twig->process(content);
            //             twig->compile(root);
            //             double a_time = l.end(); l.start();
            //             out += fnodenet_to_string(root,Stamper{[this](Node n, list<int>& offsets){
            //                 std::string to_return = n.name().to_std();
            //                 if(n.type()!=0) {
            //                     std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
            //                     while((int)n.y()>=offsets.length()) {offsets<<0;}
            //                     n.x(n.x()+offsets[(int)n.y()]);
            //                     offsets[(int)n.y()]+=nreturn.length()-to_return.length();
            //                     to_return = nreturn;
            //                 }
            //                 return to_return;
            //             },[this](Node n){
            //                 list<Node> stamps;
            //                 map<uint64_t,bool> visited;
            //                 collect_stamps(n,stamps,visited);
            //                 return stamps;
            //             }});
            //             double b_time = l.end(); 
            //             // l.start();
            //             //print_root(root);
            //             // double c_time = l.end();

            //             print("A: ",ftime(a_time));
            //             print("B: ",ftime(b_time));
            //             //print("C: ",ftime(c_time));
            // });
            

            // x_handlers[make_tokenized_keyword("respond")] = [this](Context& ctx){
            //     int fd = *(int*)ctx.node().children()[0].value().get();
            //     string str = *(Ptr*)ctx.node().children()[1].value().get();
            //     print("RESPONDING TO:\n",str.to_std());
            //     std::string body = "<html><body> <p> hello world </p>  <body></html>";
            //     std::string response = 
            //         "HTTP/1.1 200 OK\r\n"
            //         "Content-Type: text/html\r\n"
            //         "Content-Length: " + std::to_string(body.length()) + "\r\n"
            //         "\r\n" + body;
            //     print("Response:\n",response);
            //     if(::write(fd, response.c_str(), response.length()) < 0) {
            //         print(red("server_id::x_handler write() failed"));
            //     }
            // };


            //Still a WIP, finish later when multi-device scaling becomes a concearn
            g_ptr<Webcorn_Core> transactor = nullptr;
            for(int i=0;i<units.length();i++) {
                if(units[i]->unit_label=="transactor_constructor") {
                    transactor = this;
                    break;
                }
                if(units[i]->unit_label=="transactor") {
                    transactor = as<Webcorn_Core>(units[i]);
                }
            }
            if(!transactor) {
                std::string old_label = unit_label;
                unit_label = "transactor_constructor";
                transactor = make_unit<Webcorn_Core>();
                transactor->unit_label = "transactor";
                unit_label = old_label;
            }

            uint32_t RemotePtr_id = make_YAPA_type("RemotePtr",3,true);
            add_function("make_remote",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr local = ctx.node().getPtr(0);
                Ptr remote = local;
                ctx.node().set((void*)&remote);
            },sizeof(Ptr),RemotePtr_id);
            add_function("make_new_remote",[this](Context& ctx){
                g_ptr<Webcorn_Core> workshop = make_unit<Webcorn_Core>();
                workshop->uargs << uargs;
                Ptr remote(workshop->uid,0,0,0,0);
                remote.cachelevel = 5;
                ctx.node().set((void*)&remote);
            },sizeof(Ptr),RemotePtr_id);

            overload_type(RemotePtr_id,".'transact'(=>)","RemotePtr_TRANSACT",make_global_value(duck_id,0),[this,transactor](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr& ptr = ctx.node().getPtr(0);

                standard_sub_process(ctx);
                if(ctx.node().value().sub_values().empty()) {
                    Node expr = ctx.node().right().c0();
                    Value outval = deadptr;
                    list<Value> bindings;
                    map<uint32_t, bool> internal_values;
                    uint32_t hash = 5381;
                    walk_handlers.default_function = [this,&hash,&bindings,&internal_values](Context& ctx){
                        Col& name = ctx.node().name_col();
                        for(uint32_t i = 0; i < name.length(); i++) {
                            hash = ((hash << 5) + hash) + *(uint8_t*)name.qget(i);
                        }
                        if(ctx.node().type()==var_decl_id&&is_live(ctx.node().value())) {
                            internal_values.put(ctx.node().value().idx, true);
                        }
                        if(ctx.node().type()==identifier_id&&is_live(ctx.node().value())) {
                            if(!internal_values.hasKey(ctx.node().value().idx)) {
                                bindings.push(ctx.node().value());
                            }
                        }
                        standard_sub_process(ctx);
                    };
                    standard_direct_walk(expr);
                    hash = mix32_final(hash);

                    if(transactor->types[value_type_id].hasKeyByHash(hash)) {
                        outval = Ptr(&transactor->types,value_type_id,transactor->types[value_type_id].getidxByHash(hash),0);
                    } else {
                        //Depracted by 0.7.5 revist later
                        // Node out_node = transactor->make_node();
                        // map<uint32_t, Value> value_alias_table;
                        // if(is_live(expr.value())) {
                        //     Value new_return_value = transactor->make_value();
                        //     new_return_value.copy(expr.value(),true);
                        //     value_alias_table.put(expr.value().idx, new_return_value);
                        // }
                        // map<uint32_t, Node> node_alias_table;
                        // deep_copy_node(out_node,expr,value_alias_table,node_alias_table,transactor);
                        // outval = transactor->make_value(function_id,sizeof(Ptr));
                        // transactor->types[value_type_id].addcell(outval.idx,(void*)&hash,4,int_id);
                        // outval.set((void*)&out_node);
                    }
                    ctx.node().value().sub_values().insert(0,outval);

                    Node transaction = *(Node*)outval.get();
                    internal_values.clear();
                    list<Value> transaction_bindings;
                    walk_handlers.default_function = [this,&transaction_bindings,&internal_values](Context& ctx){
                        if(ctx.node().type()==var_decl_id&&is_live(ctx.node().value())) {
                            internal_values.put(ctx.node().value().idx, true);
                        }
                        if(ctx.node().type()==identifier_id&&is_live(ctx.node().value())) {
                            if(!internal_values.hasKey(ctx.node().value().idx)) {
                                transaction_bindings.push(ctx.node().value());
                            }
                        }
                        standard_sub_process(ctx);
                    };
                    standard_direct_walk(transaction);

                    for(int i=0;i<bindings.length();i++) {
                        ctx.node().value().sub_values().push(transaction_bindings[i]);
                        ctx.node().value().sub_values().push(bindings[i]);
                    }
                } 

                Node transaction = *(Node*)ctx.node().value().sub_values()[0].get();

                std::string signature = Ptr_to_string(transaction,transaction.cachelevel)+"@"+capture_ptr(ptr);
                for(int i=1;i<ctx.node().value().sub_values().length();i++) {
                    Value val = ctx.node().value().sub_values()[i];
                    signature+="@";
                    if(i%2==0) {
                        signature+=value_as_string(val);
                    } else {
                        signature+=Ptr_to_string(val,val.cachelevel);
                    }
                }
                
                //On database
                transactor->run(transactor->process("process_transaction('"+signature+"');"));

                //On server
                // ctx.node().value(compile_literal(returned).value());
                // sync_identifier(ctx);
            });
            add_function("process_transaction",[this](Context& ctx){
                standard_sub_process(ctx);
                list<std::string> args = split_str(ctx.node().getString(0).to_std(),'@'); 
                Node expr = string_to_Ptr(args[0]); 
                expr.cache = &types; expr.unit = uid;
                Ptr capabilityptr = string_to_Ptr(args[1]);
                capabilityptr.cachelevel = 3;
                capabilityptr.cache = &types; capabilityptr.unit = uid;
                expr.c0().c0().set((void*)&capabilityptr);
                Value target_val = deadptr;
                for(int i=2;i<args.length();i++) {
                    if(i%2==0) {
                        target_val = string_to_Ptr(args[i]);
                        target_val.cache = &types; target_val.unit = uid;
                    } else {
                        target_val.copy(compile_literal(args[i]).value(),false);
                    }
                }
                start_stage(x_handlers);
                standard_travel_pass(expr.scope());
                if(is_live(expr.value())&&is_live(expr.value().data_ptr())) {
                    resolve_string_ticket(ctx.node()) = value_as_string(expr.value());
                }
            },sizeof(Ptr),string_id);


            x_handlers[make_tokenized_keyword("mem_test")] = [this](Context& ctx){
                uint32_t host_before = 0;
                uint32_t host_after = 0;
                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_before += types[t][c].size;
                    }
                }

                int iterations = 200;
                //readFile("web/webtest.gld");
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

            //DON"T USE UNTIL FIX BEUCASE SERVERS WORK DIFFRENTLY NOW!!!
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
                    // servers.last()->label = target;
                    // print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
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
