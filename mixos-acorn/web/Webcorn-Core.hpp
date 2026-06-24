#pragma once

#include "../Acorn-Script.hpp"
#include "../../ext/g_lib/core/thread.hpp"

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

    #include <mach/mach.h>

    #define _UUID_T
    typedef unsigned char uuid_t[16];

    #define CLOSE_SOCKET(fd) ::close(fd)
    #define READ_SOCKET(fd, buf, len) ::read(fd, buf, len)
    #define WRITE_SOCKET(fd, buf, len) ::write(fd, buf, len)
#endif



size_t current_memory_usage() {
    #ifdef _WIN32
        return 0;
    #else
        struct mach_task_basic_info info;
        mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
        task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
        return info.resident_size; // current RSS in bytes
    #endif
}

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}



        uint32_t session_id = make_type("Session");
        uint32_t init_session_type() {
            ColCol col;
            col.label = "Sessions";
            uint32_t id = types.length();
            types.push(col);

            _layout stemp(add_template(session_id));
            stemp.add_prop(string_id,sizeof(Ptr),"username",char_id,1);
            stemp.add_prop(string_id,sizeof(Ptr),"userpath",char_id,1);
            stemp.add_prop(int_id,4,"timestamp");
            stemp.add_prop(int_id,4,"ip");
            layouts.put(session_id,stemp);
            value_printers[session_id] = [this](Context& ctx) {ctx.source("SESSION:"+Ptr_as_string(*(Ptr*)ctx.value().get()));};
            //writeFile("mixos-acorn/tests/printout.txt",make_wrapper_for_layout(stemp,"Session"));
            return id;
        }
        uint32_t session_col = init_session_type();

        uint32_t datasheet_id = reg_id("datahsheet");
        uint32_t metadatasheet_id = reg_id("metadatasheet");
        uint32_t notesheet_id = reg_id("notesheet");
        uint32_t scriptsheet_id = reg_id("scriptsheet");
        uint32_t storesheet_id = reg_id("storesheet");
        uint32_t formsheet_id = reg_id("formsheet");

        struct Session : Ptr {
            Session() {}
            Session(Ptr p) : Ptr(p) {}
       
            inline Ptr&         username_ptr(){return *(Ptr*)resolve_to_col(*this).qget(sidx+0); }
            inline Col&         username_col(){return resolve_to_col(username_ptr());}
            inline void         username(Ptr p){resolve_to_col(*this).qset(sidx+0, (void*)&p, 32); }
            inline string       username() {return (string&)username_ptr();}
       
            inline Ptr&         userpath_ptr(){return *(Ptr*)resolve_to_col(*this).qget(sidx+32); }
            inline Col&         userpath_col(){return resolve_to_col(userpath_ptr());}
            inline void         userpath(Ptr p){resolve_to_col(*this).qset(sidx+32, (void*)&p, 32); }
            inline string       userpath() {return (string&)userpath_ptr();}
       
            inline int          timestamp() {return *(int*)resolve_to_col(*this).qget(sidx+64); }
            inline void         timestamp(int t){resolve_to_col(*this).qset(sidx+64, (void*)&t, 4); }
       
            inline int          ip()        {return *(int*)resolve_to_col(*this).qget(sidx+68); }
            inline void         ip(int t)   {resolve_to_col(*this).qset(sidx+68, (void*)&t, 4); }
       };

        struct qeue_request {
            qeue_request() {}
            qeue_request(std::string _session, int _fd, std::string _message, std::string _unitcode) : 
            session(_session), fd(_fd), message(_message), unitcode(_unitcode) {}
            std::string session = "";
            int fd = 0; 
            std::string message = "";
            std::string unitcode = "";
        };
        struct Server : q_object {
            g_ptr<Thread> thread = nullptr;
            uint16_t unit = 0;
            list<qeue_request> requests;
            Session session = deadptr;
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
    
        list<g_ptr<Server>> servers;
        std::mutex servers_mutex;


        std::string generate_token() {
            #ifdef _WIN32
                return "";
            #else
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
            std::string request = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
            std::string name = string(*(Ptr*)ctx.node().children()[1].value().get()).to_std();
            string output = resolve_string_ticket(ctx.node());
            output = extract_cookie(request,name);
        },sizeof(Ptr),string_id);

        void cry(const std::string& message) {
            types.label = message;
            types.live = false;
            print(red("Cried for help"));
            while(!types.live) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            print(green("Cries answered"));
        }

        uint32_t validate_login_id = add_function("validate_login",[this](Context& ctx){
            std::string body = ctx.sub().source().to_std();

            std::string username = "";
            std::string password = "";
            size_t u = body.find("username=");
            size_t p = body.find("password=");
            if(u != std::string::npos) username = body.substr(u+9, body.find("&", u) - u - 9);
            if(p != std::string::npos) password = body.substr(p+9, body.find("&", p) - p - 9);
        
            std::string role = "";
            if(username=="employee" && password=="pass123") role = "employee";
            if(username=="manager"  && password=="pass456") role = "manager";
            if(username=="Fir" && password!="NULL") role = "admin";
            if(username=="Reed" && password!="NULL") role = "admin";

            print(yellow("Validating a login")," ",username," ",password);
        
            if(!role.empty()) {
                cry("SESSION:"+username);
                std::string token = types.label.to_std();

                ctx.sub().source() = "HTTP/1.1 302 Found\r\n"
                    "Set-Cookie: session=" + token + "; HttpOnly\r\n"
                    "Location: /\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
            } else {
                std::string body = "invalid";
                ctx.sub().source() = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(body.length()) + "\r\n"
                    "\r\n" + body;
            }
        });

        void copy_session(Session s, Session o) {
            if(is_live(o.username_ptr())) { 
                if(!is_live(s.username_ptr())) {
                    s.username(get_ticket(data_store_id,1,char_id));
                }
                s.username() = o.username();
            }
            if(is_live(o.userpath_ptr())) { 
                if(!is_live(s.userpath_ptr())) {
                    s.userpath(get_ticket(data_store_id,1,char_id));
                }
                s.userpath() = o.userpath();
            }
            s.timestamp(o.timestamp());
            s.ip(o.ip());
        }

        map<std::string,bool> distributed_tokens;

        void save_sheet(uint32_t idx, const std::string& path) {
            uint32_t sheetpool = find_sheet_pools_start(idx);
            auto out = openWriteStream(path);
            types[sheetpool].label =  path.substr(path.find_last_of('/')+1);
            write_raw<uint32_t>(out,sheetpool);
            list<ColCol*> sheet = gather_sheet_pools(sheetpool);
            write_ColColList(out,sheet);
            out.close();
        }
        uint32_t load_sheet(const std::string& path) {
            uint32_t sheetpool = 0;
            bool found = false;
            std::string label = path.substr(path.find_last_of('/')+1);
            for(int p=0;p<types.length();p++) {
                if(types[p].tag==datasheet_id&&types[p].label==label) {
                    sheetpool = p; found = true; break;
                }   
            }
            if(!found) {
                auto in = openReadStream(path);
                print("Loading ",label);
                uint32_t saved_sheetpool = read_raw<uint32_t>(in);
                list<ColCol> loadsheet = read_ColColList(in);
                print("Loaded, adding to unit and normalizing");
                sheetpool = types.length();
                print("Sheetpool ",sheetpool," saved sheetpool ",saved_sheetpool);
                for(int p=0;p<loadsheet.length();p++) {
                    for(int c=0;c<loadsheet[p].length();c++) {
                        Col& col = loadsheet[p][c];
                        if(col.heterogenous) {
                            //Add a scan over the layout and normalization for Ptr members in the future if needed
                        } else if(col.tag==ptr_id||col.tag==string_id) {
                            for(int r=0;r<col.length();r++) {
                               Ptr ptr = *(Ptr*)col[r];
                               if(is_live(ptr)) {
                                    if(ptr.cachelevel==3) {
                                        ptr.cache = &types;
                                    } else if(ptr.cachelevel==0) {
                                        ptr.unit = uid;
                                    }
                                    uint32_t oldpool = ptr.pool;
                                    ptr.pool = sheetpool + (ptr.pool - saved_sheetpool);
                                    //print("Normalized ",oldpool," to ",ptr.pool);
                                    col.set(r,(void*)&ptr);
                               }
                            }
                        }
                    }
                    types.push(loadsheet[p]);
                }   
                print("Unit normalized");
            }
            return sheetpool;
        };

        void manage_sessions(const std::string& unitcode) {
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
                    print("Unit ",unit->uid," has aksed for ",fullreq);
                    list<std::string> req = split_str(fullreq,':');
                    std::string cmd = req[0];
                    std::string arg = req.length()>1?req[1]:"";
                    if(cmd=="SESSION") {
                        if(session_col==0) {
                            print(red("webcorn:manage_sessions no valid session column in the main unit! Ensure a session manager was started"));
                        } else {
                            ColCol& sessions = types[session_col];
                            std::string token = "";
                            print(green("Logging in unit "+std::to_string(unit->uid)+" for "+arg));
                            uint32_t seshid = 0;
                            Session o;
                            if(sessions.hasKey(arg)) {
                                uint32_t seshid = sessions.getidx(arg.data(),arg.length());
                                token = sessions[seshid].label.to_std();
                                Ptr optr(&types,session_col,seshid,0);
                                o = optr;
                                print("Retrived token ",token," for ",arg);
                            } else {
                                token = generate_token();
                                distributed_tokens.put(token, true);
                                Col newsession;
                                newsession.label = token;
                                newsession.index = server->unit;
                                newsession.heterogenous = true;
                                _layout& l = layouts.get(session_id);
                                newsession.tag = session_id; newsession.element_size = l.total_size;
                                newsession.push_default();
                                seshid = sessions.length();
                                sessions.put(arg,newsession); //Do not use the sessions col refrence after this point
                                Ptr optr(&types,session_col,seshid,0);
                                o = optr;
                                o.username(get_ticket(name_store_id,1,char_id));
                                o.username() = arg;
                                o.userpath(get_ticket(name_store_id,1,char_id));
                                o.userpath() = "mixos-acorn/web/thistle/users/"+arg+"/";
                                uint32_t ts = (uint32_t)std::time(nullptr);
                                o.timestamp(ts);
                                server->authourized = true;
                            }
                            server->session = o; //Give it its session
                            unit->types.label = token;
                            server->sethash(token);

                            ColCol& unitdata = unit->types[unitdata_col];
                            value_col unit_global_values(unit->uid, unitdata_col, global_value_table_idx);
                            if(unit_global_values.hasKey("session")) {
                                Value seshv = unit_global_values.get("session");
                                Session s = seshv.data_ptr();
                                unit->copy_session(s,o);
                                print("Session coppied to unit ",unit->uid);
                            }
                            distributed_tokens.put(token,true);
                        }
                        unit->types.live = true;
                    } else if(cmd=="FILE") {
                        if(session_col==0) {
                            print(red("webcorn:manage_sessions:FILE no valid session column in the main unit! Ensure a session manager was started"));
                        } else {
                            if(server->authourized) {
                                if(arg=="LOAD") {
                                    _layout& l = layouts.get(session_id);
                                    ColCol& sessions = types[session_col];
                                    string username = server->session.username();
                                    std::string path = "mixos-acorn/web/thistle/users/"+username.to_std()+"/"+req[2]; //Add a bounds check for this later
                                    print("Loading ",path);
                                    uint32_t sheetpool = unit->load_sheet(path);
                                    unit->types.label = std::to_string(sheetpool);
                                } else if(arg=="SAVE") {
                                    _layout& l = layouts.get(session_id);
                                    ColCol& sessions = types[session_col];
                                    string username = server->session.username();
                                    std::string path = "mixos-acorn/web/thistle/users/"+username.to_std()+"/"+req[3]; //Add a bounds check for this later
                                    print("Saving ",path);
                                    unit->save_sheet(std::stoi(req[2]),path); //And a bounds check for this
                                } else {
                                    print(red("webcorn:manage_sessions:FILE unrecognized argument: "+arg));
                                }
                            } else {
                                unit->types.label = "";
                                print(red("webcorn:manage_sessions:FILE server is not authourized, please log in"));
                            }
                        }
                    } else {
                        print(red("webcorn:manage_sessions unrecognized command: "+cmd));
                    }
                    unit->types.live = true;
                }   
                else if(has_queued) {
                    print(green("Fuffiled a qeued request"));
                    server->setlabel(queued.message);
                    server->setfd(queued.fd);
                } 
                else {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(100));
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
            if(!session.empty()) {
                if(distributed_tokens.hasKey(session)) {
                    print(green("Looking for session "+session));
                    sessionhash = hashBytes(session.data(),session.length());
                }
            }

            g_ptr<Server> server = nullptr;
            {
                std::lock_guard<std::mutex> lock(servers_mutex);
                if(sessionhash) {
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->gethash()==sessionhash) {
                            server = servers[i];
                            print("Retrived session ",sessionhash," server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        print(red("webcorn:dispatch_unit no server matches the session "+session+", something has gone wrong with the session manager!"));
                        return;
                    }   
                    if(server->getfd()!=0) {
                        print(yellow("Retrived server is busy, qeueing a request instead"));
                        qeue_request req(session,server_fd,message,unitcode);
                        server->requests << req;
                        return;
                    }
                } else {
                    for(int i=0;i<servers.length();i++) {
                        if(servers[i]->gethash()==0 && servers[i]->getfd()==0) {
                            server = servers[i];
                            print("Found avaliable server unit ",server->unit);
                            break;
                        }
                    }
                    if(!server) {
                        g_ptr<Server> new_server = make<Server>();
                        new_server->thread = make<Thread>();
                        g_ptr<Webcorn_Core> webcorn = make_unit<Webcorn_Core>();
                        new_server->unit = webcorn->uid;
                        new_server->sethash(0);
                        servers << new_server;
                        server = new_server;
                        uint16_t uid = webcorn->uid;
                        print("Dispatched a new server unit ",uid);
                        new_server->thread->run_blocking([webcorn, unitcode]() mutable {
                            webcorn->run(webcorn->process(unitcode));
                        });
                    } 
                }
            }
            server->setlabel(message);
            server->setfd(server_fd);
            print("Server fd is now ",server->getfd());
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

        std::string websocket_read(int fd) {
            uint8_t header[2];
            READ_SOCKET(fd, (char*)header, 2);
            
            bool masked = header[1] & 0x80;
            uint64_t length = header[1] & 0x7F;
            
            if(length == 126) {
                uint8_t ext[2];
                READ_SOCKET(fd, (char*)ext, 2);
                length = ((uint64_t)ext[0]<<8) | ext[1];
            } else if(length == 127) {
                uint8_t ext[8];
                READ_SOCKET(fd, (char*)ext, 8);
                length = 0;
                for(int i=0;i<8;i++) length = (length<<8)|ext[i];
            }
            
            uint8_t mask[4] = {0};
            if(masked) READ_SOCKET(fd, (char*)mask, 4);
            
            std::string payload(length, 0);
            READ_SOCKET(fd, payload.data(), length);
            
            if(masked) {
                for(size_t i=0;i<length;i++) {
                    payload[i] ^= mask[i%4];
                }
            }
            
            return payload;
        }

        void websocket_write(int fd, const std::string& message) {
            std::string frame;
            frame += (char)0x81; // FIN + text opcode
            
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
            WRITE_SOCKET(fd, frame.data(), frame.size());
        }

        uint32_t thread_sleep_id = add_function("thread_sleep",[this](Context& ctx){std::this_thread::sleep_for(std::chrono::nanoseconds(100));});
        uint32_t unit_sleep_id = add_function("unit_sleep",[this](Context& ctx){types.live = false;});
        uint32_t unit_wake_id = add_function("unit_wake",[this](Context& ctx){types.live = true;});
        uint32_t unit_index_id = add_function("unit_index",[this](Context& ctx){ctx.node().value().set((void*)&types.index);},4,int_id);
        uint32_t unit_uid_id = add_function("unit_uid",[this](Context& ctx){uint32_t tuid = (uint32_t)uid; ctx.node().value().set((void*)&tuid);},4,int_id);
        uint32_t unit_setindex_id = add_function("unit_setindex",[this](Context& ctx){int idx = *(int*)ctx.node().children()[0].value().get(); types.index=idx;});
        uint32_t unit_label_id = add_function("unit_label",[this](Context& ctx){string s = resolve_string_ticket(ctx.node()); s = types.label.to_std();},sizeof(Ptr),string_id);

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

        uint32_t capture_id = make_tokenized_keyword("capture");
        
        std::string to_js_expr(std::string s) {
            std::string str = "(()=>{"+s+"})()";
            return str;
        }
        uint32_t to_js_expr_id = add_function("js_do",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            output = to_js_expr(s.to_std());
        },sizeof(Ptr),string_id);

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
        std::string to_js_lit(std::string s) {
            return "'"+escape_js_string(s)+"'";
        }
        uint32_t to_js_lit_id = add_function("js_lit",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            output = to_js_lit(s.to_std());
        },sizeof(Ptr),string_id);


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
            "download", "controls", "autoplay", "loop", "muted", "poster",  "oncontextmenu"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        uint32_t escape_str_id = add_function("escape_str",[this](Context& ctx){
            standard_sub_process(ctx);
            string content = resolve_string_ticket(ctx.node().children()[0]);
            std::string escaped = html_escape_string(content.to_std());
            string output = resolve_string_ticket(ctx.node());
            output = escaped;
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
                    process_node(ctx,c.children()[1]);
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

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                if(c.type()==property_id||c.type()==to_unary_id(property_id)) {
                    std::string prop = "";
                    std::string val = "";

                    if(resolve_prop_names(ctx,c,prop,val)) {
                        continue;
                    }

                    list<std::string>* prop_labels; list<std::string>* prop_values;
                    if(is_prop_structural(prop)) {
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
            Node prop_node = make_node(property_id);
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
        uint32_t get_prop_id = overload_type(component_id,".\"get_prop\"","GET_PROP",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            Node c = get_prop(ctx,node,looking_for);
            ctx.node().value().set((void*)&c);
        });
        uint32_t set_prop_id = overload_type(component_id,".\"set_prop\"","SET_PROP",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
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
        });
        uint32_t has_prop_id = overload_type(component_id,".\"has_prop\"","HAS_PROP",make_value(bool_id,1),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            bool has = is_live(get_prop(ctx,node,looking_for));
            ctx.node().value().set((void*)&has);
        });
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
        uint32_t get_propidx_id = overload_type(component_id,".\"get_propidx\"","GET_PROPIDX",make_value(int_id,4),[this](Context& ctx){
            Node node = ctx.node().children()[0].scopes()[0];
            process_node(ctx, ctx.node().children()[1].children()[0]);
            std::string looking_for = resolve_string_ticket(ctx.node().children()[1].children()[0]).to_std();
            int i = get_propidx(ctx,node,looking_for);
            ctx.node().value().set((void*)&i);
        });

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

        uint32_t find_sheet_pools_start(uint32_t sheetpool) {
            while(types[sheetpool].tag != datasheet_id) {
                print(sheetpool,": ",labels[types[sheetpool].tag]);
                if(sheetpool == 0) {
                    print(red("webcorn:find_sheet_pools_start unable to find the datasheet")); 
                    return 0;
                }
                sheetpool -= 1;
            }
            return sheetpool;
        }

        list<ColCol*> gather_sheet_pools(uint32_t sheetpool) {
            list<ColCol*> to_return;
            if(sheetpool>=types.length()) {print(red("webcorn:gather_sheet_pools sheetpool "+std::to_string(sheetpool)+" out of bounds for types length "+std::to_string(types.length()))); return to_return;}
            sheetpool = find_sheet_pools_start(sheetpool);
            for(int p=sheetpool;p<types.length();p++) {
                if(p<types.length()) {
                    to_return << &types[p];
                    if(types[p].tag==storesheet_id) {
                        break;
                    }
                } else break;
            }
            return to_return;
        }

        //ADD NORMALIZATION LATER WHEN THIS NEEDS TO WORK WITH MULTIPLE POOLS
        void delete_sheet(uint32_t sheetpool) {
            if(sheetpool>=types.length()) {print(red("webcorn:delete_sheet sheetpool "+std::to_string(sheetpool)+" out of bounds for types length "+std::to_string(types.length()))); return;}
            sheetpool = find_sheet_pools_start(sheetpool);
            while(sheetpool < types.length() && types[sheetpool].tag != storesheet_id) {
                types.removeAt(sheetpool);
            }
            if(sheetpool < types.length()) {
                types.removeAt(sheetpool);
            }
        }

        void rename_sheet(uint32_t sheetpool, const std::string& name) {
            sheetpool = find_sheet_pools_start(sheetpool);
            types[sheetpool].label = name;
        }


        ColCol* find_sheet(list<ColCol*> sheets, uint32_t tag, uint32_t nth = 0) {
            for(auto& sheet : sheets) {
                if(sheet->tag==tag) {
                    if(nth==0) {
                        return sheet;
                    } else nth-=1;
                }
            }
            return nullptr;
        }
        void dump_sheet(list<ColCol*> sheets, uint32_t baseoffset) {
            for(int s = 0;s<sheets.length();s++) {
                dump_pool(*sheets[s],s+baseoffset,s==0);
            }
        }
        void dump_sheet(uint32_t sheetpool) {
            uint32_t baseoffset = find_sheet_pools_start(sheetpool);
            dump_sheet(gather_sheet_pools(sheetpool),baseoffset);
        }
        
        std::string render_debugcolumn(Context& ctx, Ptr ptr, g_ptr<style_manager> styles) {
            std::string out = "";
            out += "<table ";
            out += ">\n";
            ColCol& rendersheet = resolve_to_pool(ptr);
            if(rendersheet.length()<=ptr.idx) return "";
            uint32_t c = ptr.idx;
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            out += "<th ";
            out+=styles->resolve_prop(ctx, "header_style"); 
            out+=">";
            out += rendersheet[c].label.to_std();

            if(rendersheet.tag == storesheet_id) {
                ptr.idx = c;
                out += "<div class='popup' style='"
                        "display:none;position:absolute;top:100%;left:0;"
                        "background:white;border:1px solid #ccc;border-radius:4px;"
                        "padding:4px;z-index:100;white-space:nowrap'>"
                        "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','add_row_col','"+std::to_string(c)+"')\">+</button>"
                        "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','remove_row_col','"+std::to_string(c)+"')\">-</button>"
                        "</div>";
            }

            out += "</th>";
            out += "</tr>";
            uint32_t max_rows = 0;  
            Col& col = rendersheet[c];         
            for(int r = 0; r < col.length(); r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                    std::string tostr = "";
                    if(r<col.length()) {
                        if(rendersheet.tag==storesheet_id) { //This sheet stores direct values
                            tostr = tag_to_str(col.tag,col[r]);
                        } else if(rendersheet.tag!=0&&col.tag==ptr_id) {
                            Ptr p = *(Ptr*)col[r]; 
                            if(is_live(p)) {
                                p.unit = ptr.unit;
                                Col& vcol = resolve_to_col(p);
                                if(ERROR_FLAG) { //Until we make it so metadata displays right
                                    ERROR_FLAG = false;
                                    tostr = tag_to_str(col.tag,col[r]);
                                } else {
                                    tostr = tag_to_str(vcol.tag,vcol[p.sidx]);
                                    if(ERROR_FLAG) { //Until we make it so metadata displays right
                                        ERROR_FLAG = false;
                                        tostr = tag_to_str(col.tag,col[r]);
                                    }
                                }
                            }
                        }
                    }
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\"";
                    out += "onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\"";
                    out += "/>";
                    if(rendersheet.tag == storesheet_id) {
                        std::string str = "";
                        if(col.cells.length()>r) {
                            //Same problem in ColColCol_to_form
                            //This is a bit dangerous and janky, in the future we'll use tags to ensure we're casting the right type of key
                            str = ((QString&)col.cells[r]).to_std(); //To retrive the key as a string
                        } 
                        out += "<div class='popup' style='"
                        "display:none;position:absolute;top:100%;left:0;"
                        "background:white;border:1px solid #ccc;border-radius:4px;"
                        "padding:4px;z-index:100;white-space:nowrap'>"
                        "<input value=\""+str+"\" onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','labelcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>"
                        "</div>";
                    } 
                    out+="\n</td>\n";
                out += "</tr>";
            }
            out += "</table>";
            out+="</div>";
            return out;
        }

        std::string ColColCol_to_DebugSheet(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            if(types.length()<=ptr.pool||ptr.pool<0) return  "<div id='"+ctx.sub().node().name().to_std()+"'><p \"style=color:red\"> OUT OF BOUNDS: "+std::to_string(ptr.pool)+"</p></div>";
            
            //Add a gaurd against rendering non-sheet pools as well

            list<ColCol*> sheets = gather_sheet_pools(ptr.pool);
            if(sheets.empty()) {print(red("webcorn:ColColCol_to_DebugSheet sheetpool "+std::to_string(ptr.pool)+" returned no results")); return "";}
            uint32_t baseoffset = find_sheet_pools_start(ptr.pool);
            uint32_t renderpool = ptr.pool;

            g_ptr<style_manager> styles = make<style_manager>(this);
            std::string out = "";
            out+="<div id='"+ctx.sub().node().name().to_std()+"'>";
            out+="<p>"+std::to_string(ptr.pool)+"</p>";
            out += "<table ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    if(!props[i].scopes().empty()) {
                        styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                    }
                }    
            }
            out += ">\n";
            ColCol& rendersheet = resolve_to_pool(ptr);
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<rendersheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += rendersheet[c].label.to_std();

                if(rendersheet.tag == storesheet_id) { //If it's a store pool or direct values
                    ptr.idx = c;
                    out += "<div class='popup' style='"
                           "display:none;position:absolute;top:100%;left:0;"
                           "background:white;border:1px solid #ccc;border-radius:4px;"
                           "padding:4px;z-index:100;white-space:nowrap'>"
                           "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','add_row_col','"+std::to_string(c)+"')\">+</button>"
                           "<button onclick=\"fragthree('"+ctx.sub().node().name().to_std()+"','remove_row_col','"+std::to_string(c)+"')\">-</button>"
                           "</div>";
                }

                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<rendersheet.length();c++) if(rendersheet[c].length() > max_rows) max_rows = rendersheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<rendersheet.length();c++) {
                    Col& col = rendersheet[c];
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n";
                    std::string tostr = "";
                    int splice_point = out.length();
                    if(r<col.length()) {
                        if(rendersheet.tag==storesheet_id) {
                            if(col.tag==ptr_id||col.tag==string_id) {
                                Ptr p = *(Ptr*)resolve_ptr(ptr);
                                if(is_live(p)) {
                                    tostr = value_as_string(ptr);
                                }
                            } else {
                                tostr = value_as_string(ptr);
                            }
                            out += "<div class='context_menu' ";
                            out += styles->resolve_prop(ctx, "context_menu_style");
                            out += ">";
                            out += "<div ";
                            out += styles->resolve_prop(ctx, "context_menu_header_style");
                            out += ">Cell " + Ptr_to_string(ptr) + "</div>";
                            out += "<div ";
                            out += styles->resolve_prop(ctx, "context_menu_body_style");
                            out += ">";
                            out += "<label ";
                            out += styles->resolve_prop(ctx, "context_menu_label_style");
                            out += ">Label</label>";
                            out += "<input ";
                            out += styles->resolve_prop(ctx, "context_menu_input_style");
                            std::string str = "";
                            if(col.cells.length()>r) {
                                //Same problem in ColColCol_to_form
                                //This is a bit dangerous and janky, in the future we'll use tags to ensure we're casting the right type of key
                                str = ((QString&)col.cells[r]).to_std(); //To retrive the key as a string
                            }
                            out += "value=\""+str+"\" onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','labelcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>";
                            out += "</div></div>";
                        } else if(rendersheet.tag!=0) { //To gaurd against going into a pool we aren't suppoused to (temporary for now)
                            Ptr p = *(Ptr*)col[r]; 
                            if(is_live(p)) {
                                Col& vcol = resolve_to_col(p);
                                if(ERROR_FLAG) { //Until we make it so metadata displays right
                                    ERROR_FLAG = false;
                                    print("CAUGHT ERROR FLAG: ",ERROR_MSG);
                                    tostr = value_as_string(p);
                                } else {
                                    tostr = tag_to_str(vcol.tag,vcol[p.sidx]);
                                    if(ERROR_FLAG) { //Until we make it so metadata displays right
                                        ERROR_FLAG = false;
                                        print("CAUGHT ERROR FLAG: ",ERROR_MSG);
                                        tostr = value_as_string(p);
                                    }
                                    if(rendersheet.tag == datasheet_id || rendersheet.tag == formsheet_id) { //The context menu for the main sheet, this is where we'll edit from
                                        out += "<div class='context_menu' ";
                                        out += styles->resolve_prop(ctx, "context_menu_style");
                                        out += ">";
                                        out += "<div ";
                                        out += styles->resolve_prop(ctx, "context_menu_header_style");
                                        out += ">Cell " + Ptr_to_string(ptr) + "</div>";
                                        out += "<div ";
                                        out += styles->resolve_prop(ctx, "context_menu_body_style");
                                        out += ">";
                
                                        out += "<label ";
                                        out += styles->resolve_prop(ctx, "context_menu_label_style");
                                        out += ">Label</label>";
                                        out += "<input ";
                                        out += styles->resolve_prop(ctx, "context_menu_input_style");
                                        ptr.pool+=1; //To get to metadata
                                        out+="value=\""+Ptr_to_string(*(Ptr*)resolve_ptr(ptr))+"\" ";
                                        out+="onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','run','("+Ptr_to_string(ptr)+").set('+this.value+')')\"/>";
                                        ptr.pool-=1;
                
                                        out += "<label ";
                                        out += styles->resolve_prop(ctx, "context_menu_label_style");
                                        out += ">Note</label>";
                                        out += "<input ";
                                        out += styles->resolve_prop(ctx, "context_menu_input_style");
                                        ptr.pool+=2; //To get to notes
                                        out+="value=\""+value_as_string(*(Ptr*)resolve_ptr(ptr))+"\" ";
                                        out+="onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>";
                                        ptr.pool-=2;
                
                                        out += "<label ";
                                        out += styles->resolve_prop(ctx, "context_menu_label_style");
                                        out += ">Script</label>";
                                        out += "<input ";
                                        out += styles->resolve_prop(ctx, "context_menu_input_style");
                                        ptr.pool+=3; //To get to scripts
                                        out+="value=\""+value_as_string(*(Ptr*)resolve_ptr(ptr))+"\" ";
                                        out+="onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>";
                                        ptr.pool-=3;

                                        out += "<label ";
                                        out += styles->resolve_prop(ctx, "context_menu_label_style");
                                        out += ">Store</label>";
                                        out+=render_debugcolumn(ctx,p,styles);
                                    }
                                }
                            }
                        }

                        std::string splicein = "<input "+styles->resolve_prop(ctx, "input_style")+" value=\""+tostr+"\"";
                        splicein += "onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\"/>";
                        out.insert(splice_point,splicein);
                    }
                    out+="\n</td>\n";
                }
                out += "</tr>";
            }
            out += "</table>";
            out+="</div>";
            return out;
        }

        std::string ColColCol_to_Form(Context& ctx, Ptr ptr) {
            list<ColCol*> sheets = gather_sheet_pools(ptr.pool);
            if(sheets.empty()) {print(red("webcorn:ColColCol_to_Form sheetpool "+std::to_string(ptr.pool)+" returned no results")); return "";}
            ColCol& sheetsheet = *sheets[0];
            ColCol* formsheet = find_sheet(sheets,formsheet_id);
            if(!formsheet) {print(red("webcorn:ColColCol_to_Form formsheet not found")); return "";}
            ColCol& datasheet = *formsheet;
            ColCol& metadatasheet = *find_sheet(sheets,metadatasheet_id,1);
            uint32_t sheetsheetidx = find_sheet_pools_start(ptr.pool);


            g_ptr<style_manager> styles = make<style_manager>(this);
            std::string out = "<div id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    if(!props[i].scopes().empty()) {
                        styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                    }
                }    
            }
            out += ">\n";

            uint32_t max_rows = 0;
            for(int c = 0;c<sheetsheet.length();c++) if(sheetsheet[c].length() > max_rows) max_rows = sheetsheet[c].length();
 
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                for(int c = 0; c < sheetsheet.length(); c++) {
                    ptr.idx = c;
                    Col& sheetcol = sheetsheet[c];
                    Col& datacol = datasheet[c];
                    Col& metacol = metadatasheet[c];
                    
                    std::string label = datacol.label.to_std();
                    Ptr metadata = *(Ptr*)metacol[r];
                    uint32_t widget_type = metadata.pool;

                    Ptr sheetptr = *(Ptr*)sheetcol[r];
                    std::string current = value_as_string(sheetptr);

                    if(datacol.length() > 0) {
                        Ptr p = *(Ptr*)datacol[r];
                        if(is_live(p)) {
                            Col& vcol = resolve_to_col(p);
                            QString sublabel = vcol.label;
                            if(sublabel.empty()) sublabel = std::to_string(c+r);

                            out += "<div ";
                            out += styles->resolve_prop(ctx, "field_style");
                            out += ">\n<label ";
                            out += styles->resolve_prop(ctx, "label_style");
                            out += ">" + sublabel.to_std() + "</label>\n";

                            if(widget_type==0) {
                                out += "<select ";
                                out += styles->resolve_prop(ctx, "select_style");
                                ptr.pool = sheetsheetidx; //Targeting the sheet cell itself
                                out += " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value);";
                                out += "\">\n";
                                for(int v = 0; v < vcol.length(); v++) {
                                    std::string str = std::to_string(v);
                                    if(vcol.cells.length()>v) {
                                        //This is a bit dangerous and janky, in the future we'll use tags to ensure we're casting the right type of key
                                        str = ((QString&)vcol.cells[v]).to_std(); //To retrive the key as a string
                                    }
                                    p.sidx = v;
                                    std::string val = value_as_string(p);
                                    p.sidx = 0; //Could also store the selected in the sidx of p
                                    std::string selected = (val==current) ? " selected" : ""; //For now, in the future add a check against the value at 0 for this column
                                    out += "<option value='"+val+"'"+selected+">"+str+"</option>\n";
                                }
                                out += "</select>\n";
                            } else if(widget_type==1) {
                                out += "<input ";
                                out += styles->resolve_prop(ctx, "form_input_style");
                                out += " value=\""+current+"\"";
                                ptr.pool = sheetsheetidx; //Targeting the sheet cell itself
                                out += " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value);";
                                out += "/>\n";
                            } else {

                            }
                        }
                    }
                }
            }
            out += "</div>";
            return out;
        }

        std::string ColColCol_to_Sheet(Context& ctx, Ptr ptr) {
            list<ColCol*> sheets = gather_sheet_pools(ptr.pool);
            ColCol& datasheet = *sheets[0];
            
            g_ptr<style_manager> styles = make<style_manager>(this);

            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    if(!props[i].scopes().empty()) {
                        styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                    }
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<datasheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += datasheet[c].label.to_std();
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<datasheet.length();c++) if(datasheet[c].length() > max_rows) max_rows = datasheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<datasheet.length();c++) {
                    Col& col = datasheet[c];
                    std::string tostr = "";
                    if(r<col.length()) {
                        Ptr p = *(Ptr*)col[r]; //Since the datasheet stores Ptrs
                        if(is_live(p)) {
                            //print("To string ",Ptr_to_string(p));
                            tostr = value_as_string(p);
                        }
                    }
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\""
                    + " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\""
                    +"/>\n</td>\n";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        //render_sheet(sheetid, poolid, "render as")
        //Render as options: static, sheet, form, debug
        uint32_t render_sheet_id = add_function("render_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().children().length()!=2) {print(red("Wrong number of arguments for render_sheet, expected 3")); return;}
            int sheetid = *(int*)ctx.node().children()[0].value().get();
            string renderas = (string&)*(Ptr*)ctx.node().children()[1].value().get();
            print("RENDERING POOL: ",sheetid," AS ",renderas);
            if(sheetid!=0) {
                Ptr ptr(&types,sheetid,0,0);
                if(renderas.to_std()=="static") {
                    //ctx.sub().source().push(ColCol_to_Static(ctx,ptr));
                } else if(renderas.to_std()=="sheet") {
                    ctx.sub().source().push(ColColCol_to_Sheet(ctx,ptr));
                } else if(renderas.to_std()=="form") {
                    ctx.sub().source().push(ColColCol_to_Form(ctx,ptr));
                } else if(renderas.to_std()=="debug") {
                    ctx.sub().source().push(ColColCol_to_DebugSheet(ctx,ptr));
                } else {
                    print(red("Unrecognized render type for render_sheet "),renderas);
                }
            } else {
                ctx.sub().source().push("<table id='"+ctx.sub().node().name().to_std()+"'></table>");
            }
        });

        uint32_t create_sheet_id = add_function("create_sheet",[this](Context& ctx){
            uint32_t sheetid = types.length();
            ColCol data_pool; data_pool.tag=datasheet_id; types.push(data_pool);
            ColCol metadata_pool; metadata_pool.tag=metadatasheet_id; types.push(metadata_pool); 
            ColCol notes_pool; notes_pool.tag=notesheet_id; types.push(notes_pool);
            ColCol scripts_pool; scripts_pool.tag=scriptsheet_id; types.push(scripts_pool);
            ColCol store_pool; store_pool.tag=storesheet_id; types.push(store_pool);
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t delete_sheet_id = add_function("delete_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(uint32_t*)ctx.node().children()[0].value().get();
            delete_sheet(sheetpool);
        });
        uint32_t rename_sheet_id = add_function("rename_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t sheetpool = *(uint32_t*)ctx.node().children()[0].value().get();
            string name = resolve_string_ticket(ctx.node().children()[1]);
            rename_sheet(sheetpool,name.to_std());
        });
        uint32_t add_form_id = add_function("add_form",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            list<ColCol*> sheets = gather_sheet_pools(idx);
            if(sheets.empty()) {print(red("webcorn::add_form sheetpool "+std::to_string(idx)+" is invalid, unable to add a form")); return;}
            uint32_t baseoffset = find_sheet_pools_start(idx);
            uint32_t storeidx = baseoffset+(sheets.length()-1);
            uint32_t oldstoreidx = storeidx;
            
            //Snapshot shape from first non-store pool
            int ncols = sheets[0]->length();
            int nrows = 0;
            if(!sheets[0]->empty()) {
                nrows = sheets[0]->get(0).length();
            }
        
            auto insert_pool = [&](uint8_t tag) {
                ColCol pool; pool.tag = tag;
                for(int c = 0; c < ncols; c++) {
                    Col col(sizeof(Ptr)); col.tag = ptr_id;
                    for(int r = 0; r < nrows; r++) col.push_default();
                    pool.push(col); 
                }
                types.insert(oldstoreidx,pool); //There's a resize risk here, so probably shouldn't use sheets past this point
                storeidx+=1;
            };
        
            //Order is inverted becuase an insert means entries come in backwards
            insert_pool(scriptsheet_id);
            insert_pool(notesheet_id);
            insert_pool(metadatasheet_id);
            insert_pool(formsheet_id);

            sheets = gather_sheet_pools(idx);

            //Normalize for the newely shifted storesheet index
            for(int p=0;p<sheets.length()-4;p++) {
                for(int c=0;c<sheets[p]->length();c++) {
                    Col& col = sheets[p]->get(c);
                    if(col.heterogenous) {
                        //Add a scan over the layout and normalization for Ptr members in the future if needed
                    } else if(col.tag==ptr_id||col.tag==string_id) {
                        for(int r=0;r<col.length();r++) {
                           Ptr ptr = *(Ptr*)col[r];
                           if(is_live(ptr)) {
                                if(ptr.pool==oldstoreidx) {
                                    ptr.pool = storeidx;
                                }
                                col.set(r,(void*)&ptr);
                           }
                        }
                    }
                }
            }   

            // ColCol& store = types[storeidx];
            // uint32_t a = create_column(store,4,int_id);
            // bool use_corrupt = true;
            // int numA = 5;
            // int numB = 8;
            // int numC = 12;
            // if(use_corrupt) {
            //     store[a].put("opt",(void*)&numA,string_id);
            //     store[a].put("nopt",(void*)&numB,string_id);
            //     store[a].put("bopt",(void*)&numC,string_id);
            // } else {
            //     store[a].push((void*)&numA);
            //     store[a].push((void*)&numB);
            //     store[a].push((void*)&numC);
            // }
            // Ptr to_store(&types,storeidx,store.length()-1,0);
            // types[oldstoreidx][0].qset(0,(void*)&to_store,sizeof(Ptr));
            // print("Added form elements");
            // dump_sheet(sheets,baseoffset);
        });

     
        uint32_t save_sheet_id = add_function("save_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t idx = *(uint32_t*)ctx.node().children()[0].value().get();
            string s(*(Ptr*)ctx.node().children()[1].value().get());
            save_sheet(idx,("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std()));
        });
        uint32_t load_sheet_id = add_function("load_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            string s(*(Ptr*)ctx.node().children()[0].value().get());
            uint32_t sheetpool = load_sheet("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
            ctx.node().value().set((void*)&sheetpool);
            // dump_sheet(sheetpool);
            // cry("FILE:LOAD:sheets/"+s.to_std());
            // if(types.label.empty()) {
            //     print(red("Load sheet failed!"));
            // } else {
            //     uint32_t sheetpool = std::stoi(types.label.to_std());
            //     ctx.node().value().set((void*)&sheetpool);
            // }
        },4,int_id);

        uint32_t add_column_id = add_function("add_column_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            print("Adding column to ",sheetpool);
            for(auto& pool : gather_sheet_pools(sheetpool)) {
                if(pool->tag==storesheet_id) continue;
                Col ncol(sizeof(Ptr)); ncol.tag = ptr_id;
                uint32_t row_count = 0;
                if(!types[sheetpool].empty()) {
                    row_count = types[sheetpool][0].empty() ? 0 : types[sheetpool][0].length();
                } 
                for(int i=0;i<row_count;i++) {
                    ncol.push_default();
                }
                pool->push(ncol);
            }
        });
        uint32_t add_row_id = add_function("add_row_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            for(auto& pool : gather_sheet_pools(sheetpool)) {
                if(pool->tag==storesheet_id) continue;
                for(int i=0;i<pool->length();i++) {
                    pool->get(i).push_default();
                }
            }
        });
        uint32_t add_row_to_col_id = add_function("add_row_to_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            int column = *(int*)ctx.node().children()[1].value().get();
            types[sheetpool][column].push_default();
        });
        uint32_t remove_row_from_col_id = add_function("remove_row_from_col",[this](Context& ctx){
            standard_sub_process(ctx);
            int sheetpool = *(int*)ctx.node().children()[0].value().get();
            int column = *(int*)ctx.node().children()[1].value().get();
            Col& col = types[sheetpool][column];
            if(col.cells.length()==col.length()) col.cells.removeAt(col.cells.length()-1,sizeof(CCol));
            col.removeAt(col.length()-1);
        });

        uint32_t setcell_id = add_function("setcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            if(string_to_cachelevel(terms[0])!=3) {print(red("webcorn:setcell ptr "+terms[0]+" is invalid, it has the wrong cache level")); return;}
            Ptr cellptr = string_to_Ptr(terms[0]); cellptr.cache = &types;
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            list<ColCol*> sheets = gather_sheet_pools(cellptr.pool);
            if(sheets.last()->tag!=storesheet_id) {print(red("webcorn:setcell ptr "+terms[0]+" is invalid, the sheets it gathered did not end with a storesheet")); return;}
            uint32_t storepoolidx = find_sheet_pools_start(cellptr.pool)+(sheets.length()-1);
            Ptr p = cellptr;
            if(cellptr.pool!=storepoolidx) {
                p = *(Ptr*)resolve_ptr(cellptr);
            }
            Node literal = compile_literal(terms[1]);
            Value lv = literal.value();
            if(!is_live(p)) { //If we aren't replacing a live Ptr, create a new spot for its data in the store pool
                p = get_ticket(storepoolidx,lv.size(),lv.type());
                resolve_to_col(cellptr).set(cellptr.sidx,(void*)&p);
            }
            void* data = lv.get();
            Col& tcol = resolve_to_col(p); //Where the value is stored in the store pool

            uint32_t subtype = 0; uint32_t subsize = 0; uint32_t alias = ptr_id;

            if(lv.type()==string_id) {subtype = char_id; subsize = 1; alias = string_id;}
            else if(lv.sub_type()!=0) {subtype = lv.sub_type(); subsize = lv.sub_size(); alias = lv.type();}

            Ptr subp = deadptr; //This can be optimized in the future with finer discernment, such as detecting if we're replacing a Ptr more accurately than with just alias
            //Pending a redesign of how double-hop values work on the language side
            if(tcol.tag==ptr_id||tcol.tag==string_id) {
                if(!tcol.empty()) {
                    subp = *(Ptr*)tcol.get(p.sidx); //The Ptr currently stored to the other collection
                    if(subtype==0||subsize==0&&is_live(subp)) { //Free the subptr if we're realiasing to a scalar
                        print("Recycling subp");
                        recycle_column(subp);
                    } else {
                        if(is_live(subp)) {
                            print("Resetting subp");
                            Col& subcol = resolve_to_col(subp);
                            subcol.clear(); subcol.element_size = subsize; subcol.tag = subtype;
                        } else {
                            print("Regnerating subp");
                            subp = get_ticket(storepoolidx,subsize,subtype);
                            resolve_to_col(p).set(p.sidx,(void*)&subp);
                        }
                    }
                } else if(subtype!=0&&subsize!=0) {
                    print("Replacing subp");
                    subp = get_ticket(storepoolidx,subsize,subtype);
                    resolve_to_col(p).push((void*)&subp);
                }
            }
            Col& col = resolve_to_col(p);
            if(subtype!=0&&subsize!=0) { //If we're a pointer to a collection
                if(col.tag!=alias) {
                    print("Realiasing");
                    col.element_size = sizeof(Ptr); col.tag=alias;
                    col.clear();
                    subp = get_ticket(storepoolidx,subsize,subtype);
                    resolve_to_col(p).push((void*)&subp);
                } else {
                    print("Replacing");
                }
                Ptr dataptr  = *(Ptr*)data;
                Col& datacol = resolve_to_col(dataptr); //Copy over the data to it's new position
                Col& subcol = resolve_to_col(subp);
                subcol.clear();
                for(int i=0;i<datacol.length();i++) {
                    subcol.push(datacol[i]);
                }
            } else { //If we're the direct value in the store pool
                if(col.element_size!=lv.size()||col.tag!=lv.type()) {
                    print("Clearing and pushing");
                    col.clear();
                    col.element_size = lv.size(); col.tag=lv.type();
                    col.push(data);
                } else if(col.empty()) {
                    print("Pushing");
                    col.push(data);
                } else {
                    print("Setting");
                    col.set(p.sidx,data);
                }
            }
        });


        uint32_t labelcell_id = add_function("labelcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            if(string_to_cachelevel(terms[0])!=3) {print(red("webcorn:labelcell ptr "+terms[0]+" is invalid, it has the wrong cache level")); return;}
            Ptr cellptr = string_to_Ptr(terms[0]); cellptr.cache = &types;
            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            Col& cellcol = resolve_to_col(cellptr);
            if(pooltag==storesheet_id) { //We can only label storesheet cells for now
                Node literal = compile_literal(terms[1]);
                if(literal.value().type()==string_id) { 
                    while(cellcol.cells.length()<=cellptr.sidx) {
                        CCol c; //Temporary filler
                        char defc = ' ';
                        c.element_size = 1; 
                        c.tag = string_id;
                        c.hash = hashBytes((void*)&defc, 1);
                        c.index = cellcol.cells.length();
                        c.push((void*)&defc);
                        cellcol.cells.push(c);
                    }
                    CCol& cell = cellcol.cells[cellptr.sidx];
                    string& s = (string&)*(Ptr*)literal.value().get();
                    cell.clear();
                    cell.element_size = s.length();
                    cell.hash = hashBytes(resolve_ptr(s), s.length());
                    cell.push(resolve_ptr(s));
                } else {
                    //We only suppourt string keys for now
                }
            }
        });

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
            x_handlers[property_id] = x_handlers[to_unary_id(property_id)];
            //M is just the stage when this is most viable, any earlier and we get some issues
            m_handlers[property_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!ctx.node().scopes().empty()&&ctx.node().children().length()>1) { //Yield the scope
                    ctx.node().children()[1].scopes().push(ctx.node().scopes().take(0));
                    ctx.node().children()[1].scopes()[0].owner(ctx.node().children()[1]);
                }
            };
            t_handlers[capture_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
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


            add_function("plen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                Ptr pptr(&types,pool,0,0);
                uint32_t plen = resolve_to_pool(pptr).length();
                ctx.node().value().set((void*)&plen);
            },4,int_id);

            add_function("clen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t col = *(int*)ctx.node().children()[1].value().get();
                Ptr cptr(&types,pool,col,0);
                uint32_t clen = resolve_to_col(cptr).length();
                ctx.node().value().set((void*)&clen);
            },4,int_id);

            add_function("clabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t col = *(int*)ctx.node().children()[1].value().get();
                Ptr cptr(&types,pool,col,0);
                string output = resolve_string_ticket(ctx.node());
                output = resolve_to_col(cptr).label.to_std();
            },sizeof(Ptr),string_id);

            add_function("csetlabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t col = *(int*)ctx.node().children()[1].value().get();
                string label = (string&)*(Ptr*)ctx.node().children()[2].value().get();
                Ptr cptr(&types,pool,col,0);
                resolve_to_col(cptr).label = label.to_std();
            });

            add_function("resolve_as_Ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value().set(resolve_ptr(*(Ptr*)ctx.node().children()[0].value().get()));
            },sizeof(Ptr),ptr_id);

            add_function("resolve_as_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr resolved = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set(resolve_ptr(resolved));
            },sizeof(Ptr),node_id);

            add_function("cast_to_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                if(n.cachelevel==3) n.cache = &types; //Figure out later why this isnt' working with string_to_Ptr
                ctx.node().value().set((void*)&n);
            },sizeof(Ptr),node_id);

            add_function("makePtr3",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().value().get();
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t idx = *(int*)ctx.node().children()[1].value().get();
                uint32_t sidx = *(int*)ctx.node().children()[2].value().get();
                p.pool = pool; p.idx = idx; p.sidx = sidx;
                p.cachelevel = 3; p.cache = &types;
            },sizeof(Ptr),ptr_id);

            add_function("value_as_string",[this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                output = value_as_string(*(Ptr*)ctx.node().children()[0].value().get());
            },sizeof(Ptr),string_id);

            add_function("is_live",[this](Context& ctx){
                standard_sub_process(ctx);
                bool b = is_live(*(Ptr*)ctx.node().children()[0].value().get());
                ctx.node().value().set((void*)&b);
            },1,bool_id);

            add_function("call_owner_as_string",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Ptr ownerptr = n.scopes()[0].owner();
                string output = resolve_string_ticket(ctx.node());
                output = Ptr_to_string(ownerptr,ownerptr.cachelevel);
            },sizeof(Ptr),string_id);

            add_function("call_owner",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Node owner = n.scopes()[0].owner();
                ctx.node().value().set((void*)&owner);
            },sizeof(Ptr),node_id);

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


            html_handlers[func_call_id] = [this](Context& ctx){
                uint32_t prelen = ctx.source().length();
                fire_quals(ctx,ctx.node().value());
                if(ctx.node().mute()) {ctx.node().mute(false); return;}
                bool needs_closing = ctx.source().length()!=prelen;
                x_handlers.run(func_call_id)(ctx);
                if(needs_closing) {
                    std::string src = ctx.source().to_std();
                    size_t tag_start = prelen+1;
                    size_t tag_end = src.find(' ', tag_start);
                    if(tag_end == std::string::npos) tag_end = src.find('>', tag_start);
                    std::string tag = src.substr(tag_start, tag_end - tag_start);
                    ctx.source().push("</"+tag+">");
                }
            };
            html_handlers[func_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            html_handlers[prefix_component_id] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id) {
                    ctx.qual(deadptr); //Revoke it. Dealing with inner quals and their routing is a minefield!
                    std::string props = emit_inline_html(ctx,ctx.value().type_scope());
                    if(!props.empty()) {
                        ctx.source().push("<div "+props+">");
                    }
                } else if(ctx.node().type()==func_decl_id&&!ctx.node().has_qual(template_qual)) {
                    if(ctx.node().children().length()==0) { //Implcit case, for when a div is paramatrized and named but not qualifed as a template, don't emit it!
                        ctx.qual(deadptr); 
                        html_handlers.run(component_id)(ctx);
                    }
                }
            };
            x_handlers[prefix_component_id] = html_handlers[prefix_component_id];

            html_handlers[to_prefix_id(template_qual)] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id&&!ctx.node().has_qual(stateless_qual)) {
                    //print("Instantiating: [",ctx.node().scopes().length(),"] ",node_info(ctx.node()));
                    Node scope_owner = ctx.node().scopes()[0].owner();
                    if(scope_owner!=ctx.node()) { //If we haven't been instantiated yet
                        Node zero = ctx.node().scopes()[0];
                        ctx.node().scopes().clear(); ctx.node().scopes() << zero;
                        ctx.node().scopes() << instantiate_template_scope(ctx.node(),scope_owner,ctx,true);
                    } 
                }
            };
            x_handlers[to_prefix_id(template_qual)] = html_handlers[to_prefix_id(template_qual)];

            r_handlers[prefix_inlined_id] = [this](Context& ctx){
                if(ctx.node().type()==func_call_id) {
                    Node func = ctx.value().type_scope();
                    // func.owner().mute(true); //To stop it from emitting 
                    bool is_static = ctx.value().has_qual(static_qual);
                    for(int i=0;i<func.children().length();i++) {
                        if(is_static) {
                            Node copy = make_node(); //Make this a proper deep copy later (placeholder for now)
                            copy.copy(func.children()[i]);
                            ctx.result().insert(ctx.index(),copy);
                        } else {
                            ctx.result().insert(ctx.index(),func.children()[i]);
                        }
                        ctx.index()++;
                    }
                }
            };

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
                //print("TARGET: ",target," FROM: ",node_info(from));
                Node result = webcorn_node_scan(target,from);
                // if(!is_live(result)) {
                //     while(is_live(from.in_scope())&&is_live(from.in_scope().owner())&&from.in_scope().owner().type()==func_decl_id) {
                //         from = from.in_scope().owner();
                //     }
                //     print("NOW SEARCHING FROM: ",node_info(from));
                // }

                ctx.node().value().set((void*)&result);
                if(is_live(result)) {
                    //print("FOUND: ",node_to_string(result));
                } else {
                    print(red("COULD NOT FIND "+target));
                }
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
                #ifdef _WIN32
                    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
                #endif
                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(server_fd < 0) { print(red("socket() failed")); return; }
                ctx.node().value().set((void*)&server_fd);
            };
            
            uint32_t bind_id = make_tokenized_keyword("bind");
            r_handlers[bind_id] = make_int_node;
            x_handlers[bind_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int port = *(int*)ctx.node().children()[1].value().get();
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
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
                if(client_fd == -1) { throw_error("accept failed"); return; }
                ctx.node().value().set((void*)&client_fd);
            };
            
            uint32_t read_id = make_tokenized_keyword("read");
            r_handlers[read_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id, sizeof(Ptr)));
            };
            x_handlers[read_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                char buffer[4096];
                std::string request;
                
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
                
                Ptr ticket = get_ticket(name_store_id, 1, char_id);
                for(auto c : request) types[name_store_id][ticket.idx].push((void*)&c);
                ctx.node().value().set((void*)&ticket);
            };
            
            uint32_t write_id = make_tokenized_keyword("write");
            r_handlers[write_id] = make_int_node;
            x_handlers[write_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                Ptr strptr = *(Ptr*)ctx.node().children()[1].value().get();
                Col& col = resolve_to_col(strptr);
                WRITE_SOCKET(fd, (const char*)col.storage, col.size);
            };
            
            uint32_t close_id = make_tokenized_keyword("close");
            r_handlers[close_id] = make_int_node;
            x_handlers[close_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                CLOSE_SOCKET(fd);
            };

            

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