#pragma once
#include "../mixos-acorn/Acorn-Core.hpp"
#include <csignal>
namespace Acorn {

    void signal_handler(int signal) {
        print("\nRECIVED SIGNAL: ",signal);
        ERROR_FLAG = true;
        ERROR_MSG = "Console interrupt";
    }

    void setup_signals() {
        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
    }

    struct Blackfeather_Unit : public virtual Unit {
        Blackfeather_Unit() {init();}
        
        void init() override {
            setup_signals();
        }

        void launch_blackfeather(list<Node> roots) {
            std::string line;
            Node on_node = deadptr;
            while(std::getline(std::cin, line)) {
                if(line.empty()) break;
                if(line == "exit") break;

                for(int i=0;i<line.length();i++) {
                    if(line.at(i)=='|') {
                        if(line.at(i+1)==' ') {line.erase(i+1,1);}
                        if(line.at(i-1)==' ') {line.erase(i-1,1); i--;}
                    }
                }

                list<std::string> piped_cmds = split_str(line,'|');
                std::string mode = "";
                bool echo = false;
                bool is_invalid = false;
                for(auto pcmd : piped_cmds) {
                    list<std::string> cmds = split_str(pcmd,' ');
                    if(pcmd.empty()) {is_invalid = true; continue;}

                    if(mode.empty()) { //This is to let us pipe without constantly redeclaring the first scope
                        mode=cmds[0];
                    } else {
                        cmds.insert(mode,0);
                    }

                    if(cmds[0]=="trace") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="count") {
                            print(roots.length()); 
                        } else if(cmds[1]=="print") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                for(auto r : roots) {
                                    print(node_to_string(r));
                                }
                            }  else {
                                if(!is_str_num(cmds[2])) {is_invalid = true; continue;}
                                int root_id = std::stoi(cmds[2]);
                                if(root_id<roots.length()) {
                                    print(node_to_string(roots[root_id]));
                                } else {
                                    print(red("ROOT INDEX OUT OF BOUNDS"));
                                }
                            }
                            echo = true;
                        }
                    } else if(cmds[0]=="span") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="print") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                span->print_all();
                            } else {
                                //Nothing here yet
                            }
                            echo = true;
                        }
                    } else if(cmds[0]=="unit") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="print"||cmds[1]=="dump") {
                            bool do_print = cmds[1]=="print";
                            if(do_print) echo = true;
                            if(cmds.length()>2) {
                                uint32_t addr0 = 0;
                                if(is_str_num(cmds[2])) {
                                    addr0 = std::stoi(cmds[2]);
                                } else {
                                    for(int i=0;i<types.length();i++) {
                                        if(types[i].type_name==cmds[2]) {
                                            addr0 = i; break;
                                        }
                                    }
                                }
                                TypePool& ptype = types[addr0];
                                if(cmds.length()==3) {
                                    if(do_print) {
                                        print(type_to_string(ptype));
                                    } else {
                                        writeFile("mixos-acorn/tests/printout.txt",type_to_string(ptype));
                                    }
                                } else {
                                    uint32_t addr1 = 0;
                                    if(is_str_num(cmds[3])) {
                                        addr1 = std::stoi(cmds[3]);
                                    } else {
                                        if(cmds[3]=="on") {addr1 = on_node.idx;}
                                    }
                                    if(cmds.length()==4) {
                                        if(do_print) {
                                            print(type_to_string(ptype));
                                        } else {
                                            list<list<std::string>> plines = type_to_lines(ptype);
                                            list<std::string> tline = plines[addr1];
                                            writeFile("mixos-acorn/tests/printout.txt","");
                                            editTextFile("mixos-acorn/tests/printout.txt",[tline](std::string& src){
                                                list<list<std::string>> col = {tline};
                                                src = print_columnar_table(col);
                                            });
                                        }
                                    } else {
                                        uint32_t addr2 = std::stoi(cmds[4]);
                                    }
                                }
                            } else {
                                if(!do_print) {
                                    dump_unit(true);
                                }
                            }
                        }
                    } else if(cmds[0]=="node") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(is_str_num(cmds[1])) {
                            uint32_t node_id = std::stoi(cmds[1]);
                            Ptr nptr = {node_type_id,node_id,0};
                            on_node = (Node&)nptr;
                            print("on_node: ",node_info(on_node));
                        } else {
                            if(cmds[1]=="print") {
                                if(cmds.length()>2) {
                                    _layout& ntemp = layouts[node_id];
                                    uint32_t idx = ntemp.label_to_index[cmds[2]];
                                    print(tag_to_str(ntemp.tags[idx],resolve_to_col(on_node).qget(ntemp.offsets[idx])));
                                } else {
                                    print(node_to_string(on_node));
                                    echo = true;
                                }
                            } else {
                                if(cmds[1]=="in_scope") {
                                    on_node = on_node.in_scope();
                                } else if(cmds[1]=="owner") {
                                    on_node = on_node.owner();
                                } else if(cmds[1]=="child") {
                                    uint32_t cidx = std::stoi(cmds[2]);
                                    on_node = on_node.children()[cidx];
                                } else if(cmds[1]=="qual") {
                                    uint32_t qidx = std::stoi(cmds[2]);
                                    on_node = on_node.quals()[qidx];
                                } else if(cmds[1]=="scope") {
                                    uint32_t scidx = std::stoi(cmds[2]);
                                    on_node = on_node.scopes()[scidx];
                                }
                                print("on_node: ",node_info(on_node));
                            }
                        }
                    } else {
                        is_invalid = true;
                    }
                }
                if(echo) {
                    print("[",line,"]");
                }
                if(is_invalid) {
                    print(red("Invalid command: "+line));
                }
            }
        }

        void launch_blackfeather(Node root) {
            launch_blackfeather({root});
        }
    };
}