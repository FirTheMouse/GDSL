#pragma once
#include "../mixos-acorn/Acorn-Core.hpp"
#include "../ext/g_lib/core/thread.hpp"
#include <csignal>

#include <termios.h>
#include <unistd.h>

struct TerminalLantern {
    termios old_termios;
    
    TerminalLantern() {
        tcgetattr(STDIN_FILENO, &old_termios);
        termios raw = old_termios;
        raw.c_lflag &= ~(ECHO | ICANON); //Disable echo and line buffering
        raw.c_cc[VMIN] = 1;  //Read one char at a time
        raw.c_cc[VTIME] = 0; //No timeout
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    
    ~TerminalLantern() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios); //Restore on destruction
    }
};

char read_key() {
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

//q = -100
//^ = 2
//v = -2
//> = 1
//< = -1
//s = 3
//f = 4
int read_arrow() {
    char c = read_key();
    if(c == '\x1b') {
        char seq[2];
        read(STDIN_FILENO, &seq[0], 1);
        read(STDIN_FILENO, &seq[1], 1);
        if(seq[0]=='[') {
            if(seq[1]=='C') return 1; //>
            if(seq[1]=='D') return -1; //<
            if(seq[1]=='A') return 2; //^
            if(seq[1]=='B') return -2; //v
        }
    }
    if(c=='q') return -100; // quit
    if(c=='s') return 3;
    if(c=='f') return 4;
    return 0;
}

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

        void stamp_onto_page(Node node, list<std::string>& lines) {
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                // print("STAMPING: ",node_info(node));
                int x = (int)node.x();
                int y = (int)node.y();
                while(y>=lines.length()) {lines << "";}
                while((x+node.name().length())>=lines[y].length()) lines[y]+=" ";
                for(char c : node.name().to_std()) lines[y][x++] = c;
                // for(auto l : lines) {
                //     print(escape_string(l,false));
                // }
            }
            for(int i=0;i<node.children().length();i++) stamp_onto_page(node.children()[i],lines);
            for(int i=0;i<node.quals().length();i++) stamp_onto_page(node.quals()[i],lines);
            for(int i=0;i<node.scopes().length();i++) stamp_onto_page(node.scopes()[i],lines);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) stamp_onto_page(node.value().quals()[i],lines);
            }
        }
        std::string nodenet_to_string(Node root) {
            list<std::string> lines;
            stamp_onto_page(root,lines);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        std::string idx_to_color(const std::string& num, int idx) {
            float hue = fmod(idx * 137.508f, 360.0f);
            float s = 0.8f, v = 0.9f;
            
            float c = v * s;
            float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
            float m = v - c;
            float r,g,b;
            if(hue<60)       {r=c;g=x;b=0;}
            else if(hue<120) {r=x;g=c;b=0;}
            else if(hue<180) {r=0;g=c;b=x;}
            else if(hue<240) {r=0;g=x;b=c;}
            else if(hue<300) {r=x;g=0;b=c;}
            else             {r=c;g=0;b=x;}
            return rgb(num, (int)((r+m)*255), (int)((g+m)*255), (int)((b+m)*255));
        }
        std::string colorize_numbers(const std::string& line) {
            if(line.empty()) {return "";}
            std::string out = "";
            int i = 0;
            while(i < line.length()) {
                if(std::isdigit(line[i])) {
                    std::string num = "";
                    while(i < line.length() && std::isdigit(line[i])) {
                        num += line[i++];
                    }
                    int idx = std::stoi(num);
                    out+=idx_to_color(num,idx);
                } else {
                    out += line[i++];
                }
            }
            return out;
        }
        void stamp_lifetimes(Node node, list<std::string>& lines) {
            if(is_live(node.value())&&is_live(node.value().data_ptr())) {
                int x = 0;
                int y = 0;
                if(node.x()>=0.0f&&node.y()>=0.0f) {
                    x = (int)node.x();
                    y = (int)node.y();
                } else if(!node.quals().empty()) {
                    Node q = node.quals()[0];
                    x = (int)q.x();
                    y = (int)q.y();
                }
                if(x>=0.0f&&y>=0.0f) {
                    std::string to_stamp = std::to_string(node.value().data_ptr().idx);
                    while(y>=lines.length()) {lines << "";}
                    while((x+to_stamp.length())>=lines[y].length()) lines[y]+=" ";
                    for(char c : to_stamp) lines[y][x++] = c;
                }
            }
            for(int i=0;i<node.children().length();i++) stamp_lifetimes(node.children()[i],lines);
            for(int i=0;i<node.scopes().length();i++) stamp_lifetimes(node.scopes()[i],lines);
        }
        std::string nodenet_to_lifetimes(Node root) {
            list<std::string> lifetime_lines;
            stamp_lifetimes(root,lifetime_lines);
            list<std::string> main_lines;
            stamp_onto_page(root,main_lines);
            std::string out = "";
            list<std::string> lines;
            for(int i=0;i<main_lines.length();i++) {
                lines << main_lines[i]+"\n";
                if(i<lifetime_lines.length()) {
                    if(!lifetime_lines[i].empty())
                        lines << colorize_numbers(lifetime_lines[i])+"\n";
                }
            }
            for(auto l : lines) {
                out += l;
            }
            return out;
        }



        void collect_stamps(Node node, list<Node>& nodes) {
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                int x = (int)node.x();
                int y = (int)node.y();
                int insert_at = nodes.length();
                for(int i=0;i<nodes.length();i++) {
                    int ny = (int)nodes[i].y();
                    int nx = (int)nodes[i].x();
                    if(y<ny||(y==ny&&x<nx)) {
                        insert_at = i;
                        break;
                    }
                }
                nodes.insert(node, insert_at);
            }
            for(int i=0;i<node.children().length();i++) collect_stamps(node.children()[i],nodes);
            for(int i=0;i<node.quals().length();i++) collect_stamps(node.quals()[i],nodes);
            for(int i=0;i<node.scopes().length();i++) collect_stamps(node.scopes()[i],nodes);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) collect_stamps(node.value().quals()[i],nodes);
            }
        }
        list<std::string> fstamp(Node root, std::function<std::string(Node,int&)> format) {
            list<std::string> lines;
            list<int> offsets;
            list<Node> stamps;
            collect_stamps(root,stamps);
            for(int i=0;i<stamps.length();i++) {
                Node stamp = stamps[i];
                int x = (int)stamp.x();
                int y = (int)stamp.y();
                while(y>=offsets.length()) {offsets<<0;}
                x+=offsets[y];
                std::string to_stamp = format(stamp,offsets[y]);
                while(y>=lines.length()) {lines << "";}
                while((x+to_stamp.length())>=lines[y].length()) lines[y]+=" ";
                for(char c : to_stamp) lines[y][x++] = c;
            }
            return lines;
        }
        std::string fnodenet_to_string(Node root,std::function<std::string(Node,int&)> format) {
            list<std::string> lines = fstamp(root,format);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        struct Flipbook : q_object {
            Flipbook() {}
            Flipbook(std::string _label) : label(_label) {}
            std::string label = "";
            std::ofstream out;
            size_t len = 0;
            void open() {
                std::string path = "mixos-acorn/flipbooks/"+label;
                out.open(path, std::ios::binary);
            }
            void close() {
                out.close();
            }
            void add_page(const std::string& page) {
                write_string(out,page);
                len++;
            }
            list<std::string> pages() {
                list<std::string> to_return;
                std::string path = "mixos-acorn/flipbooks/"+label;
                std::ifstream in(path, std::ios::binary);
                for(int i=0;i<len;i++) {
                    to_return << read_string(in);
                }
                in.close();
                return to_return;
            }
        };
        list<g_ptr<Flipbook>> flipbooks;

        g_ptr<Flipbook> get_flipbook(const std::string& label) {
            for(int i=0;i<flipbooks.length();i++) {
                if(flipbooks[i]->label==label) return flipbooks[i];
            }
            return nullptr;
        }
      
        int page_num = 0;
        void setup_resolution_flipbook() {
            Watcher w;
            w.stagestart = [this](Context& ctx){
                g_ptr<Flipbook> b = make<Flipbook>("resolution_"+active_stage->label);
                b->open();
                flipbooks << b;
            };
            w.stagend = [this](Context& ctx){
                g_ptr<Flipbook> b = get_flipbook("resolution_"+active_stage->label);
                if(b) b->close();
                ptr_colors.clear();
                page_num = 0;
            };
            
            w.prefix = [this](Context& ctx) {
                g_ptr<Flipbook> b = get_flipbook("resolution_"+active_stage->label);    
                b->add_page(
                    blue(std::to_string(page_num++))+"> "+node_info(ctx.node())
                    +"\n"
                    +fnodenet_to_string(unit_root,[&ctx](Node n, int& offset){
                        std::string s1 = n.name().to_std();
                        std::string s2 = s1;
                        if(n==ctx.node()) {
                            s2 = blue(s2);
                        } else {
                            if(n.resolved()) {
                                s2 = green(s2);
                            } else {
                                s2 = red(s2);
                            }
                        }
                        offset+=s2.length()-s1.length();
                        s1 = s2;
                        return s1;
                }));
            };
            w.suffix = [this](Context& ctx) {
                ctx.node().resolved(true);
                for(int i=0;i<ctx.node().quals().length();i++)  {
                    Node q = ctx.node().quals()[i]; 
                    if(q.mute()) {q.resolved(true);}
                }

                g_ptr<Flipbook> b = get_flipbook("resolution_"+active_stage->label); 
                b->add_page(
                    cyan(std::to_string(page_num++))+"> "+node_info(ctx.node())
                    +"\n"
                    +fnodenet_to_string(unit_root,[&ctx](Node n, int& offset){
                        std::string s1 = n.name().to_std();
                        std::string s2 = s1;
                        if(n==ctx.node()) {
                            s2 = cyan(s2);
                        } else {
                            if(n.resolved()) {
                                s2 = green(s2);
                            } else {
                                s2 = red(s2);
                            }
                        }
                        offset+=s2.length()-s1.length();
                        s1 = s2;
                        return s1;
                }));
            };
            watchers << w;
        }

        void setup_resolution_trace_flipbook() {
            Watcher w("resolution_trace_flipbook");
            w.stagestart = [this](Context& ctx){
                g_ptr<Flipbook> b = make<Flipbook>("resolution_trace_"+active_stage->label);
                b->open();
                flipbooks << b;
            };
            w.stagend = [this](Context& ctx){
                g_ptr<Flipbook> b = get_flipbook("resolution_trace_"+active_stage->label);
                if(b) b->close();
                ptr_colors.clear();
                page_num = 0;
            };
    
            w.prefix = [this](Context& ctx) {                
                ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = white("> "+s);}; //Ptr on
                g_ptr<Flipbook> b = get_flipbook("resolution_trace_"+active_stage->label);    
                b->add_page("\n\n\n\n\n\n\n"+node_to_string(unit_root)+"\n"+blue(std::to_string(page_num++)));
                ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = pine("~ "+s);}; //Ptr resolving
            };
            w.suffix = [this](Context& ctx) {
                // ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = green("> "+s);}; //Ptr finished resolving
                // g_ptr<Flipbook> b = get_flipbook("resolution_trace_"+active_stage->label);    
                // b->add_page("\n\n\n\n\n\n\n"+node_to_string(unit_root)+"\n"+blue(std::to_string(page_num++)));
                // for(int i=0;i<ctx.node().quals().length();i++)  {
                //     Node q = ctx.node().quals()[i]; 
                //     if(q.mute()) {
                //         ptr_colors[Ptr_to_key(q)] = [](std::string& s){s = gray(s);};
                //     }
                // }
                ptr_colors[Ptr_to_key(ctx.node())] = [](std::string& s){s = gray(". "+s);}; //Ptr resolved
            };
            watchers << w;
        }

        void navigate_flipbook(g_ptr<Flipbook> flipbook) {
            list<std::string> book = flipbook->pages();
            if(book.empty()) {print("Flipbook ",flipbook->label," is empty"); return;}
            int on_page = 0;
            TerminalLantern lantern;
            std::string next = book[on_page];
            while(true) {
                print(next);
                int key = read_arrow();
                if(key == 1 && on_page < book.length()-1) {on_page++; next = book[on_page];} //>
                if(key == -1 && on_page > 0) {on_page--; next = book[on_page];} //<
                if(key == -2 && on_page-5 > 0) {on_page-=5; next = book[on_page];} //v
                if(key == 2 && on_page+5 < book.length()-1) {on_page+=5; next = book[on_page];} //^
                if(key == -100) break;
            }
            print("Exited navigation");
        }


        float flip_speed = 0.1f;
        int flip_pages(list<std::string> book, int start_page = 0, int flip_to = -1) {
            if(book.empty()) return 0;
            if(flip_to==-1) flip_to = book.length()-1;
            int on_page = start_page;
            Log::Line l; l.start();
            while(on_page!=flip_to) {
                if(ERROR_FLAG) break;
                if(l.time_s()>=flip_speed) {
                    std::string p = book[on_page];
                    print(p);
                    if(flip_to>on_page) {
                        on_page++;
                    } else if(flip_to<on_page) {
                        on_page--;
                    }
                    if(on_page>=book.length()||on_page<=0) {
                        break;
                    }
                    l.start();
                }
            }
            ERROR_FLAG = false;
            return on_page;
        }


        bool silence_blackfeather = false;
        void launch_blackfeather(list<Node> roots) {
            if(silence_blackfeather) return;
            std::string line;
            Node on_node = deadptr;
            while(std::getline(std::cin, line)) {
                if(line.empty()) break;
                if(line == "exit") break;
                if(line == "cont") break;

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
                        } else if(cmds[1]=="stamp") {
                            if(cmds.length()==2) {is_invalid = true; continue;}
                            if(cmds[2]=="all") {
                                for(auto r : roots) {
                                    print(nodenet_to_string(r));
                                }
                            } else if(cmds[2]=="lifetimes") {
                                for(auto r : roots) {
                                    print(nodenet_to_lifetimes(r));
                                }
                            } else if(cmds[2]=="test") {
                                print(fnodenet_to_string(roots[0],[](Node n, int& offset){
                                    std::string to_return = n.name().to_std();
                                    if(n.mute()) {
                                        std::string nreturn = gray(to_return);
                                        offset+=nreturn.length()-to_return.length();
                                        to_return = nreturn;
                                    }
                                    return to_return;
                                }));
                            } else {
                                if(!is_str_num(cmds[2])) {is_invalid = true; continue;}
                                int root_id = std::stoi(cmds[2]);
                                if(root_id<roots.length()) {
                                    print(nodenet_to_string(roots[root_id]));
                                } else {
                                    print(red("ROOT INDEX OUT OF BOUNDS"));
                                }
                            }
                            
                            echo = true;
                        } else if(cmds[1]=="live") {
                            for(auto r : roots) {
                                print(nodenet_to_lifetimes(r));
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
                    } else if(cmds[0]=="flipbook"||cmds[0]=="flip"||cmds[0]=="f") {
                        if(cmds.length()==1) {is_invalid = true; continue;}
                        if(cmds[1]=="list") {
                            print("Listing ",flipbooks.length()," flipbooks");
                            for(int i=0;i<flipbooks.length();i++) {
                                print(i,": ",flipbooks[i]->label);
                            }
                        } else if(cmds[1]=="play"||cmds[1]=="open") {
                            if(cmds.length()>2) {
                                print("Running ",cmds[1],"-",cmds[2]);
                                g_ptr<Flipbook> b = nullptr;
                                if(is_str_num(cmds[2])) {
                                    int f_idx = std::stoi(cmds[2]);
                                    if(f_idx<flipbooks.length()&&f_idx>=0) b = flipbooks[f_idx];
                                } else {
                                    get_flipbook(cmds[2]);
                                }
                                if(b) {              
                                    if(cmds[1]=="play") {
                                        print("Playing: ",b->label);
                                        print("Pages: ",b->len);
                                        flip_pages(b->pages());
                                        echo = true;
                                    }
                                    if(cmds[1]=="open") {
                                        print("Opened: ",b->label);
                                        print("Pages: ",b->len);
                                        navigate_flipbook(b);
                                        echo = true;
                                    }
                                } else {
                                    print("Could not find flipbook ",cmds[2]);
                                }
                            }
                        } else if(cmds[1]=="all") {
                            for(auto b : flipbooks) {
                                print_and_pause(0.7f,"\n\n\n\n\n\n",b->label," pages: ",b->len,"\n\n\n\n\n\n");
                                flip_pages(b->pages());
                            }
                        } else if(cmds[1]=="speed") {
                            if(cmds.length()>2) {
                                if(is_str_num(cmds[2])) {
                                    flip_speed = std::stof(cmds[2]);
                                }
                            } else {
                                print("Flip speed: ",flip_speed);
                            }   
                        } else {
                            is_invalid = true;
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
            list<Node> roots = {root};
            launch_blackfeather(roots);
        }

        void launch_blackfeather() {
            list<Node> roots = {unit_root};
            launch_blackfeather(roots);
        }
    };
}