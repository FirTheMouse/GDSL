#pragma once
#include "../mixos-acorn/Acorn-Blackfeather.hpp"

namespace Acorn {
    struct Compiler_Unit : public virtual Blackfeather_Unit {


        Compiler_Unit(uint16_t _uid) : Unit(_uid)  { init(); }
        Compiler_Unit() {init();}

        uint32_t setup_unit_data() {
            uint32_t idx = types.length();
            ColCol unitdata;
            types.push(unitdata);
            return idx;
        }

        uint32_t unitdata_col = setup_unit_data();
        uint32_t global_value_table_idx = note_value(types[unitdata_col], "global_values", sizeof(Ptr), value_id);
        uint32_t global_node_table_idx = note_value(types[unitdata_col], "global_nodes", sizeof(Ptr), node_id);
        
        Stage& walk_handlers = reg_stage("walking"); 

        Stage& a_handlers = reg_stage("assembling");
        Stage& s_handlers = reg_stage("scoping");
        Stage& t_handlers = reg_stage("typing");
    
        Stage& d_handlers = reg_stage("discovering");
        Stage& r_handlers = reg_stage("resolving");
        Stage& e_handlers = reg_stage("evaluating");
    
        Stage& m_handlers = reg_stage("modeling");
        Stage& i_handlers = reg_stage("interpreting");
        Stage& x_handlers = reg_stage("executing");

        Stage& coerce_handlers = reg_stage("coercing"); 

        map<std::string,Value> keywords;
        map<uint32_t,bool> is_true_type;

        void a_pass_resolve_keywords(node_col nodes, int context = -1) {
            for(int i=0;i<nodes.length();i++) {
                Node node = nodes[i];
                log("Keyword resolving ",node_info(node));
                if(keywords.hasKey(node.name().to_std())) {
                    for(Value v : keywords.getAll(node.name().to_std())) {
                        if(is_live(v)) {
                            if(v.reg()==-1||v.reg()==context) { //By default is -1
                                node.value(make_value()); //Make a value to copy into
                                node.value().copy(v,true);
                                node.value().reg(-1); //Reset to -1 for cleanliness
                            }
                        }
                    }
                }

                if(is_live(node.value())) {
                    context =  node.value().sub_type();
                }

                a_pass_resolve_keywords(node.children(), context);

                //No scopes! To work with precompiling passes and also because there shouldn't be any scopes in a stage
                // for(int s = 0;s<node.scopes().length();s++) {
                //     a_pass_resolve_keywords(node.scopes()[s].children(), context);
                // }
            }
        };


        Value make_qual_value(const std::string& f, uint32_t size = 0) {
            uint32_t id = reg_id(f);
            uint32_t prefix_id = reg_id(f);
            uint32_t suffix_id = reg_id(f);
            Value val = make_value(id,size);
            val.sub_type(id);
            return val;
        }

        uint32_t register_qual_ids(const std::string& f) {
            uint32_t id = reg_id(f);
            uint32_t prefix_id = reg_id(f);
            uint32_t suffix_id = reg_id(f);
            return id;
        }

        void add_type_stamping_handler(uint32_t type) {
            is_true_type[type] = true;
            t_handlers[to_prefix_id(type)] = [](Context& ctx){
                if(ctx.value().sub_type() == 0) {
                    ctx.value().sub_type(ctx.qual().sub_type());
                    ctx.value().type(ctx.qual().type());
                    ctx.value().size(ctx.qual().value().size());
                    if(is_live(ctx.qual().value().type_scope())) //<- not sure what this is for, it could screw with type scopes role in provenace, this was written before the new meaning for type scope in 0.7.1
                        ctx.value().type_scope(ctx.qual().value().type_scope());
                }
            };
            x_handlers[type] = [this](Context& ctx){ //For casting
                standard_sub_process(ctx);
                if(!ctx.node().children().empty()) {
                    ctx.node().value().copy(ctx.node().children()[0].value(),false);
                    ctx.node().value().type(ctx.node().type());
                }
            };
        }
        void register_type_initilizers(uint32_t prefix_type) {
            r_handlers[prefix_type] = [this](Context& ctx){
                if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                    ctx.value().size(layouts.get(ctx.value().type()).total_size);
                }
            };
            //v this is probably what was causing problems with query, very suspect, investigate later!
            x_handlers[prefix_type] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0].type()!=ptr_id) { //Beause Ptrs store subtypes in their quals
                    ctx.value().data_col().push_default();
                    ctx.value().data_col().heterogenous = true;
                }
            }; 
        }

        Value make_type_value(const std::string& f, size_t size = 0) {
            Value val = make_qual_value(f,size);
            add_type_stamping_handler(val.type());
            return  val;
        }

        map<uint32_t,bool> typeless_quals;
        uint32_t add_qual(const std::string& f, uint32_t size = 0) {
            Value val = make_qual_value(f,size);
            keywords.put(f,val);
            typeless_quals[val.type()]=true;
            return val.type();
        }

        uint32_t make_type(const std::string& f, uint32_t size = 0, Handler value_print = nullptr) {
            Value val = make_type_value(f,size);
            keywords.put(f,val);
            register_type_initilizers(to_prefix_id(val.type()));
            if(value_print) {
                value_printers[val.type()] = value_print;
            }
            return val.type();
        }

        void register_type(const std::string& label, uint32_t type, uint32_t size) {
            Value val = make_value(type,size); val.sub_type(type);
            add_type_stamping_handler(type);
            keywords.put(label,val);
        }

        Value register_value(const std::string& name, uint32_t size = 0,uint32_t type = 0) {
            Value v = make_value(type,size);
            v.sub_type(reg_id(name));
            return v;
        }

        uint32_t make_keyword(const std::string& name, uint32_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
            Value val = register_value(type_name==""?name:type_name,size,sub_type);
            keywords.put(name,val);
            return val.sub_type();
        }

        void add_alias(const std::string& name, uint32_t type) {
            keywords.put(name,keywords.get(labels[type]));
        }



        map<std::string,uint32_t> tokenized_keywords;

        map<char,bool> char_is_split;

        map<char, Handler> tokenizer_functions;
        map<uint32_t, Handler> tokenizer_state_functions;
        Handler tokenizer_default_function = nullptr;



        map<uint32_t,list<std::function<void()>>> token_registers;
        size_t make_tokenized_keyword(const std::string& token_name, size_t default_id = 0, uint32_t registeryid = 0) {
            size_t id = default_id;
            if(default_id==0) {
                id = reg_id(token_name);
            }

            if(registeryid==0) {
                tokenized_keywords.put(token_name,id);
            }
            token_registers[registeryid] << [this,token_name,id]() {
                tokenized_keywords[token_name] = id;
            };
            return id;
        }

        uint32_t make_registering_tokenized_keyword(const std::string& token_name, size_t default_id = 0, uint32_t registeryid = 0) {
            uint32_t id = make_tokenized_keyword(token_name,default_id,registeryid);
            tokenizer_state_functions[id] = [this](Context& ctx){
                for(auto& f : token_registers[ctx.node().type()]) {f();}
                ctx.state(0); at_x-=1.0f; --ctx.index();
            };
            return id;
        }

        map<uint32_t,map<uint32_t,uint32_t>> token_combos;
        inline uint32_t combine_tokens(char a, char b, char c = '\0', char d = '\0') {
            return  (uint32_t(uint8_t(a)) << 24) |
                    (uint32_t(uint8_t(b)) << 16) |
                    (uint32_t(uint8_t(c)) << 8)  |
                    (uint32_t(uint8_t(d)));
        }
        uint32_t add_token_combo(const std::string& f, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = reg_id(f);
            token_combos[a].put(combine_tokens(a,b,c,d),id);
            return id;
        }
        int find_token_combo(Context& ctx) {
            char a = ctx.source().at(ctx.index());
            if(!token_combos.hasKey(a)) return 0;

            char c[4] = {a,'\0','\0','\0'};
            for(int i=1;i<4;i++) { //To safely fill without outrunning the length of source
                if(ctx.index()+i < ctx.source().length()) {
                    c[i] = ctx.source().at(ctx.index()+i);
                }
            }

            uint32_t zero = 0;
            map<uint32_t,uint32_t>& combos = token_combos.get(a);
            for(int i=3;i>0;i--) {
                uint32_t key = combine_tokens(c[0],c[1],c[2],c[3]);
                uint32_t result = combos.getOrDefault(key,zero);

                // printnl(" ",i,": ");
                // for(int m=0;m<4;m++) {
                //     printnl(c[m],"[",std::to_string(m),"]");
                // }
                // print(" : ",labels[result]);

                if(result!=0) {
                    ctx.node().type(result);
                    return i;
                }
                c[i] = '\0';
            }
            return 0;
        }

        size_t add_token(char c, const std::string& f) {
            size_t id = reg_id(f);
            tokenizer_functions[c] = [this,id,c](Context& ctx) {
                ctx.node(make_node(0,0,"",at_x,at_y,at_z));
                int to_skip = find_token_combo(ctx);
                if(to_skip!=0) { 
                    ctx.node().name().push(c);
                    for(int i=0;i<to_skip;i++) {
                        ctx.index()++;
                        if(ctx.index()<ctx.source().length()) {
                            at_x+=1.0f;
                            ctx.node().name().push(ctx.source().at(ctx.index()));
                        }
                    }
                } else {
                    ctx.node().type(id);
                    ctx.node().name().push(c);
                }
                if(tokenizer_state_functions.hasKey(ctx.node().type())) {
                    ctx.state(ctx.node().type());
                }
                ctx.result().push(ctx.node());
            };
            char_is_split.put(c, true);
            return id;
        }


        list<size_t> discard_types;

        size_t to_decl_id(size_t id) {return id+1;}
        size_t to_unary_id(size_t id) {return id+2;}
        
        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        size_t lbracket_id = add_token('[', "LBRACKET");
        size_t rbracket_id = add_token(']', "RBRACKET");
        size_t lbrace_id = add_token('{', "LBRACE");
        size_t rbrace_id = add_token('}', "RBRACE");

        size_t in_alpha_id = reg_id("IN_ALPHA");
        size_t in_digit_id = reg_id("IN_DIGIT");
        size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return
        size_t quote_id = add_token('"',"QUOTE");
        size_t comment_id = reg_id("COMMENT");
        size_t single_quote_id = add_token('\'',"SINGLE_QUOTE");

        Node tokenize(const std::string& code) {

            tokenized_keywords.clear(); 
            for(auto& f : token_registers[0]) {f();}

            Node root = make_node(root_id);
            root.name("ROOT");
            root.z(at_z);
            node_col result = root.children();
            uint32_t state = 0;
            Context ctx = make_context(result);
            int& index = ctx.index();

            ctx.source(code);
            ctx.root(root);

            #if PRINT_ALL
            newline("tokenize pass");
            #endif

            if(!tokenizer_default_function) {
                print("GDSL::tokenize warning! No defined default function, please define one");
            }

            while (index<ctx.source().length()) {
                char c = ctx.source().at(index);
                Handler* func = nullptr;
                if(ctx.state()!=0&&tokenizer_state_functions.hasKey(ctx.state())) {
                    func = &tokenizer_state_functions.get(ctx.state());
                } else {
                    func = &tokenizer_functions.getOrDefault(c,tokenizer_default_function);
                }

                if(func) {
                    (*func)(ctx);
                }

                at_x += 1.0f;
                ++index;
            }  

            #if PRINT_ALL
            int i = 0;
            for(int t=0;t<result.length();t++) {
                log(i++," ",labels[result.get(t).type()],": ",result.get(t).name());
            }
            endline();
            #endif
            recycle_column(ctx.source_ptr());
            recycle_context(ctx);
            return root;
        }

        void init_tokenizer() {
            // Literals_Unit::init();
            char_is_split.put(' ',true);
            tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(char_is_split.getOrDefault(c,false)) {
                    ctx.state(0); 
                    at_x-=1.0f;
                    --ctx.index();
                    if(tokenized_keywords.hasKey(ctx.node().name().to_std())) {
                        uint32_t tokenized_type = tokenized_keywords.get(ctx.node().name().to_std());
                        ctx.node().type(tokenized_type);
                        if(tokenizer_state_functions.hasKey(tokenized_type)) {ctx.state(tokenized_type);}
                    } else {
                        ctx.node().type(ctx.node().type());
                    }
                    return;
                } else {
                    ctx.node().name().push(c);
                    if(ctx.index()+1==ctx.source().length()) {
                        ctx.state(0); 
                        if(tokenized_keywords.hasKey(ctx.node().name().to_std())) {
                            uint32_t tokenized_type = tokenized_keywords.get(ctx.node().name().to_std());
                            ctx.node().type(tokenized_type);
                            if(tokenizer_state_functions.hasKey(tokenized_type)) {ctx.state(tokenized_type);}
                        } else {
                            ctx.node().type(ctx.node().type());
                        }
                    }
                }
            });
    
            tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(c=='.') {
                    ctx.node().type(float_id);
                } else if(c=='|') {
                    ctx.node().type(ptr_id);
                } else if(char_is_split.getOrDefault(c,false)) {
                    ctx.state(0); 
                    at_x-=1.0f;
                    --ctx.index();
                    return;
                } else if(std::isalpha(c)) {
                    ctx.state(in_alpha_id);
                }
                ctx.node().name().push(c);
            });


            tokenizer_functions[' '] = [this](Context& ctx) {
                //Just skip
            };
            tokenizer_functions['\t'] = [this](Context& ctx) {
                //Just skip
            };
            tokenizer_functions['\n'] = [this](Context& ctx) {
                at_y += 1.0f;
                at_x = -1.0f;
            };
    
            tokenizer_default_function = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(std::isalpha(c)) {
                    ctx.state(in_alpha_id);
                    ctx.node(make_node(identifier_id,0,std::string(1,c),at_x,at_y,at_z));
                    ctx.result().push(ctx.node());
                }
                else if(std::isdigit(c)) {
                    ctx.state(in_digit_id);
                    ctx.node(make_node(int_id,0,std::string(1,c),at_x,at_y,at_z));
                    ctx.result().push(ctx.node());
                }  else {
                    print("tokenize:default_function missing handling for char: ",c);
                }

                int to_skip = find_token_combo(ctx);
                if(to_skip!=0) {
                    ctx.state(0);
                    for(int i=0;i<to_skip;i++) {
                        ctx.index()++;
                        if(ctx.index()<ctx.source().length()) {
                            at_x+=1.0f;
                            ctx.node().name().push(ctx.source().at(ctx.index()));
                        }
                    }
                }
            };

            // tokenizer_default_function = [this](Context& ctx) {
            //     char c = ctx.source().at(ctx.index());
            //     int to_skip = find_token_combo(ctx);
            //     if(to_skip!=0) {
            //         ctx.node(make_node(0,0,"",at_x,at_y));
            //         ctx.node().name().push(c);
            //         for(int i=0;i<to_skip;i++) {
            //             ctx.index()++;
            //             if(ctx.index()<ctx.source().length()) {
            //                 at_x+=1.0f;
            //                 ctx.node().name().push(ctx.source().at(ctx.index()));
            //             }
            //         }
            //         ctx.result().push(ctx.node());
            //     } else {
            //         if(std::isalpha(c)) {
            //             ctx.state(in_alpha_id);
            //             ctx.node(make_node(identifier_id,0,std::string(1,c),at_x,at_y));
            //             ctx.result().push(ctx.node());
            //         } else if(std::isdigit(c)) {
            //             ctx.state(in_digit_id);
            //             ctx.node(make_node(int_id,0,std::string(1,c),at_x,at_y));
            //             ctx.result().push(ctx.node());
            //         }  else {
            //             print("tokenize:default_function missing handling for char: ",c);
            //         }
            //     }               
            // };
        }



        bool find_value_in_scope(Node node) {
            if(node.in_scope().value_table().hasKey(node.name().to_std())) {
                node.value(node.in_scope().value_table().get(node.name().to_std()));
                return true;
            }
            value_col vcol(uid, unitdata_col, global_value_table_idx);
            if(vcol.hasKey(node.name().to_std())) {
                node.value(vcol.get(node.name().to_std()));
                return true;
            }
            return false;
        }


        bool find_node_in_scope(Node node) {
            if(node.in_scope().node_table().hasKey(node.name().to_std())) {
                if(node.scopes().length()>0) node.scopes().clear();
                node.scopes().push(node.in_scope().node_table().get(node.name().to_std()));
                return true;
            }
            node_col ncol(uid, unitdata_col, global_node_table_idx);
            if(ncol.hasKey(node.name().to_std())) {
                node.scopes().push(ncol.get(node.name().to_std()));
                return true;
            }
            return false;
        }

        // Value distribute_value(QNode& node, const std::string& label, Value val) {
        //     if(node.value_table().hasKey(label)) {
        //         Value table_value = node.value_table().get(label);
        //         if(table_value.type() == 0) {
        //             table_value.copy(val);
        //             val = table_value;
        //         }
        //     } else {
        //         node.value_table().put(label, val);
        //     }
        //     for(int c = 0;c<node.children().length();c++) {
        //         QNode& child = node.children()[c].toQ();
        //         if(!child.scopes().empty()) {
        //             for(int s = 0;s<child.scopes().length();s++) {
        //                 QNode& scope = child.scopes().get(s).toQ();
        //                 if(scope.owner().idx==child.idx) {
        //                     val = distribute_value(scope,label,val);
        //                 }
        //             }
        //         }
        //     }
        //     return val;
        // }

        uint32_t hoisted_id = add_qual("hoisted");
        uint32_t global_qual = add_qual("global");

        Value distribute_value_children(Node node, const std::string& label, Value val) {
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                for(int s = 0;s<child.scopes().length();s++) {
                    Node scope = child.scopes().get(s);
                    if(scope.owner().idx==child.idx) {
                        val = distribute_value(scope,label,val,0);
                    }
                }
                val = distribute_value_children(child, label, val);
            }
            return val;
        }

        Value distribute_value(Node node, const std::string& label, Value val, int initial_hoist) {
            if(initial_hoist!=0) {
                Node climb = node;
                for(int i = 0; i < initial_hoist && is_live(climb.owner()); i++) {
                    climb = climb.owner().in_scope();
                }
                return distribute_value(climb, label, val, 0);
            }

            if(node.value_table().hasKey(label)) {
                Value table_value = node.value_table().get(label);
                if(table_value.type() == 0) {
                    table_value.copy(val,false);
                    val = table_value;
                }
            } else {
                node.value_table().put(label, val);
            }
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0;s<node.children()[c].scopes().length();s++) {
                        Node scope = child.scopes().get(s);
                        if(scope.owner().idx==child.idx) {
                            val = distribute_value(scope,label,val,0);
                        }
                    }
                }
                val = distribute_value_children(child,label,val);
            }
            return val;
        }

        Node distribute_node_children(Node node, const std::string& label, Node carry) {
            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                for(int s = 0;s<child.scopes().length();s++) {
                    Node scope = child.scopes().get(s);
                    if(scope.owner().idx==child.idx) {
                        carry = distribute_node(scope,label,carry,0);
                    }
                }
                carry = distribute_node_children(child, label, carry);
            }
            return carry;
        }

        Node distribute_node(Node node, const std::string& label, Node carry, int inital_hoist) {
            if(inital_hoist!=0) {
                Node climb = node;
                for(int i = 0; i < inital_hoist && is_live(climb.owner()); i++) {
                    climb = climb.owner().in_scope();
                }
                return distribute_node(climb, label, carry, 0);
            }

            if(node.node_table().hasKey(label)) {
                Node table_node = node.node_table().get(label);
                if(table_node.name().length()==0) {
                    table_node.copy(carry);
                    carry = table_node;
                }
            } else {
                node.node_table().put(label, carry);
            }

            for(int c = 0;c<node.children().length();c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0;s<node.children()[c].scopes().length();s++) {
                        Node scope = child.scopes().get(s);
                        if(scope.owner().idx==child.idx) {
                            carry = distribute_node(scope,label,carry,0);
                        }
                    }
                }
                carry = distribute_node_children(child,label,carry);
            }
            return carry;
        }
 
        //Logging for the deep copy node
        #define LOG_DCN 0
        #if LOG_DCN
            #define UDCN(x) x
        #else
            #define UDCN(x)
        #endif

        //Make this cleaner later, probably when I do the normalization update and get more equipment
        //Experiment with a version that doesn't copy *evrything*, this is the biggest performance problem  right now in TwigSnap
        void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
            UDCN(uspan->newline("Deep copying "+node_info(o));)
            UDCN(uspan->newline("Initial fields");)
            n.type(o.type());
            n.sub_type(o.sub_type());
            n.name(o.name().to_std());
            n.x(o.x());
            n.y(o.y());
            n.z(o.z());
            n.mute(o.mute());
            n.resolved(o.resolved());
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy children");)
            n.children().clear();
            for(int i = 0; i < o.children().length(); i++) {
                if(n.type()==equals_id&&i==0&&o.children()[0].in_scope()!=o.children()[1].in_scope()) {
                    n.children() << o.children()[i]; //For function calls, don't deep copy the left argumnet.
                    //This again needs to be fxied up as *local* function calls do need this copy for aliasing
                    //Add to list of things to fix when normalization rolls around
                } else {
                    Node newc = make_node();
                    deep_copy_node(newc, o.children()[i], value_alias_table, node_alias_table);
                    n.children() << newc;
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy quals");)
            n.quals().clear();
            for(int i = 0; i < o.quals().length(); i++) {
                if(o.quals()[i].mute()) {
                    n.quals() << o.quals()[i];
                } else {
                    Node newq = make_node();
                    deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
                    n.quals() << newq;
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy value");)
            if(value_alias_table.hasKey(o.value().idx)) {
                Value aliased = value_alias_table.get(o.value().idx);
                n.value(aliased);
            } else {
                if(is_live(o.value())) {
                    if(!o.has_qual(global_qual)) {
                        if(!is_live(n.value())) {
                            n.value(make_value());
                        }
                        deep_copy_value(n.value(),o.value());
                    } else {
                        n.value(o.value());
                    }
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Copy scopes");)
            n.scopes().clear();
            for(int i = 0; i < o.scopes().length(); i++) {
                //print("Deciding to alias scope ",o.scopes()[i].idx);
                //print(node_to_string(o));
                if(node_alias_table.hasKey(o.scopes()[i].idx)) {
                    Node aliased = node_alias_table.get(o.scopes()[i].idx);
                    //print("Aliasing as ",aliased.idx);
                    n.scopes() << aliased;
                } else if(o.scopes()[i].owner()==o) {
                    Node news = make_node();
                    n.scopes() << news;
                    //print("Deep copying as ",news.idx);
                    if(n.type()==func_decl_id) {
                        n.scopes()[i].owner(n);
                        value_alias_table.put(o.value().idx, n.value());
                        node_alias_table.put(o.scopes()[i].idx, n.scopes()[i]);
                        //print("Put ",o.scopes()[i].idx," node alias for : ",node_info(n.scopes()[i]));
                    }
                    deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
                    news.owner(n);
                } else {
                    //print("Leaving untouched");
                    n.scopes() << o.scopes()[i];
                }
            }
            UDCN(uspan->endline();)
            UDCN(uspan->newline("Resolve tables");)
            resolve_to_col(n).qset(node_value_table_offset,
                resolve_to_col(o).qget(node_value_table_offset), sizeof(Ptr));
            resolve_to_col(n).qset(node_node_table_offset,
                resolve_to_col(o).qget(node_node_table_offset), sizeof(Ptr));
        
            n.parent(o.parent_ptr());
            n.owner(o.owner_ptr());
            n.in_scope(o.in_scope_ptr());
            n.opt_str() = o.opt_str().to_std();

            if(n.type()==var_decl_id) {
                value_alias_table.put(o.value().idx, n.value());
            } 
            UDCN(uspan->endline();)
            UDCN(uspan->endline();)
        }
        

        Node value_to_qual(Value val, std::string name = "") {
            Node to_return = make_node(val.type(),val.sub_type(),name,-1.0f,-1.0f,0.0f,val);
            return to_return;
        }

        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power[id] = lbp;
            right_binding_power[id] = rbp;
            if(labels[to_unary_id(id)]==labels[id]+"_unary") {
                left_binding_power[to_unary_id(id)] = lbp;
                right_binding_power[to_unary_id(id)] = rbp;
            }
            if(labels[to_decl_id(id)]==labels[id]+"_decl") {
                left_binding_power[to_decl_id(id)] = lbp;
                right_binding_power[to_decl_id(id)] = rbp;
            }
        }

        void declare_variable(Node node, Value decl_value) {
            decl_value.sub_type(0);
            node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value, node.count_qual(hoisted_id)));
            node.value().type_scope(node.in_scope()); //The intent here is to provide provenace information, unsure if this should go before the distirbution or afer, and, if it should opperate on decl_value or not.
        }

        map<char,bool> registered_opperators;
        map<uint32_t,bool>  registered_operator_ids;
        map<uint32_t,bool>  overloaded_operator_ids;
        inline bool is_operator(uint32_t type) {
            return registered_operator_ids.getOrDefault(type,false);
        }
        inline bool is_pure_operator(uint32_t type) {
            return (registered_operator_ids.getOrDefault(type,false)&&!overloaded_operator_ids.getOrDefault(type,false));
        }

        size_t add_binary_operator(char c, const std::string& f, int lbp, int rbp, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");
            set_binding_powers(id,lbp,rbp);
            registered_opperators[c] = true;
            registered_operator_ids.put(id,true);
            registered_operator_ids.put(decl_id,true);
            registered_operator_ids.put(unary_id,true);

            Handler handler = [this,decl_id,unary_id,c](Context& ctx){
                node_col children = ctx.node().children();
                standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
                
                ctx.node().quals() << copy_as_token(ctx.node());
                ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                
                if(children.length() == 2) {
                    Node type_term = children[0];
                    Node id_term = children[1];

                    ctx.node().name(type_term.name().to_std()+c+id_term.name().to_std());
                    //May need to commit the decls as tokens, check the stamp later when it isn't almost midnight
                    if(type_term.type()==var_decl_id||(is_live(type_term.value())&&type_term.value().type()==type_term.type())) {
                        ctx.node().type(decl_id);
                        ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                        ctx.node().value().quals().push(value_to_qual(type_term.value()));
                        ctx.node().name(id_term.name().to_std());
                        if(id_term.value().type()==0) { //If this is a decleration
                            declare_variable(ctx.node(),ctx.node().value());
                            ctx.node().children().clear();
                        } else { //If this is a reinterpret like for a cast
                            ctx.node().children().removeAt(0);
                        }
    
                        
                    }
                } else if(children.length() == 1) {
                    Node type_term = children[0];
                    ctx.node().name(c+type_term.name().to_std());
                    ctx.node().type(unary_id);
                    if(is_live(type_term.value())) {
                        if(!is_live(ctx.node().value())) ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                    }
                } 
            };
            t_handlers[id] = handler;
            t_handlers[unary_id] = handler;

            r_handlers[id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(void_id,0));
            };

            Handler shandler = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }

                if(ctx.index()+1>=ctx.result().length()) {
                    return;
                }

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    if(!ctx.node().children().empty()) {
                        ctx.node().children().last().children() << ctx.result().take(ctx.index()+1);
                    } else {
                        ctx.node().children() << ctx.result().take(ctx.index()+1);
                    }
                }                
            };
            s_handlers[id] = shandler;
            s_handlers[unary_id] = shandler;

            Handler xhandler = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            };
            x_handlers[id] = xhandler;
            x_handlers[unary_id] = xhandler;
            x_handlers[decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            return id;
        }

        size_t register_binary_operator(int use_id, int lbp, int rbp) {
            return add_binary_operator(' ',labels[use_id],lbp,rbp,use_id);
        }

        size_t plus_id = add_binary_operator('+',"PLUS", 4, 6);
        size_t dash_id = add_binary_operator('-',"DASH", 4, 5);
        size_t rangle_id = add_binary_operator('>',"RANGLE", 4, 5);
        size_t langle_id = add_binary_operator('<',"LANGLE", 4, 5);
        size_t bang_id = add_binary_operator('!',"BANG", 2, 3);
        size_t equals_id = add_binary_operator('=', "EQUALS", 1, 1);
        size_t star_id = add_binary_operator('*',"STAR", 5, 7);
        size_t slash_id = add_binary_operator('/',"SLASH", 4, 5);
        size_t caret_id = add_binary_operator('^',"CARET", 8, 4);
        size_t dollar_id = add_binary_operator('$',"DOLLAR", 8, 9);
        size_t amp_id = add_binary_operator('&',"AMPERSAND", 4, 8);
        size_t tilde_id = add_binary_operator('~', "TILDE", 4, 8);
        size_t dot_id = add_binary_operator('.', "DOT", 8, 9);
        size_t pipe_id = add_binary_operator('|', "PIPE", 9, 8);
        uint32_t qmark_id = add_binary_operator('?',"QMARK",1,3);
        uint32_t property_id = add_binary_operator(':',"COLON",3,6);
        uint32_t hash_id = add_binary_operator('#',"HASH",5,3); //This may also be set in acorn_script right now because I was testing it out

        uint32_t group_id = add_binary_operator('[',"GROUP",11,12,reg_id("GROUP"));

        uint32_t  add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = add_token_combo(f,a,b,c,d);
            set_binding_powers(id,lbp,rbp);
            registered_operator_ids[id] = true;
            r_handlers[id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
            };
            Handler shandler = [this](Context& ctx){ //This is nessecary for closures
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }

                if(ctx.index()+1>=ctx.result().length()) {
                    return;
                }

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    if(!ctx.node().children().empty()) {
                        ctx.node().children().last().children() << ctx.result().take(ctx.index()+1);
                    } else {
                        ctx.node().children() << ctx.result().take(ctx.index()+1);
                    }
                }                
            };
            s_handlers[id] = shandler;

            Handler xhandler = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            };
            x_handlers[id] = xhandler;
            return id;
        }
        uint32_t add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c, char d, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            uint32_t id = add_binding_token_combo(f,lbp,rbp,a,b,c,d);
            if(type!=0) {
                r_handlers[id] = [this,size,type](Context& ctx){
                    if(is_live(ctx.node().value())) return;
                    standard_sub_process(ctx);
                    ctx.node().value(make_value(type,size));
                    resolve_overload(ctx);
                };
            }
            x_handlers[id] = xhandler;
            return id;
        }
        uint32_t add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            return add_binding_token_combo(f,lbp,rbp,a,b,c,'\0',xhandler,size,type);
        }
        uint32_t add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            return add_binding_token_combo(f,lbp,rbp,a,b,'\0','\0',xhandler,size,type);
        }

        uint32_t add_binding_unary_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = add_binding_token_combo(f,lbp,rbp,a,b,c,d);
            intentional_unary[id] = true;
            return id;
        }
        uint32_t add_binding_unary_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c, char d, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            uint32_t id = add_binding_token_combo(f,lbp,rbp,a,b,c,d,xhandler,size,type);
            intentional_unary[id] = true;
            return id;
        }
        uint32_t add_binding_unary_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            uint32_t id = add_binding_token_combo(f,lbp,rbp,a,b,c,'\0',xhandler,size,type);
            intentional_unary[id] = true;
            return id;
        }
        uint32_t add_binding_unary_token_combo(const std::string& f, int lbp, int rbp, char a, char b, Handler xhandler, uint32_t size = 0, uint32_t type = 0) {
            uint32_t id = add_binding_token_combo(f,lbp,rbp,a,b,'\0','\0',xhandler,size,type);
            intentional_unary[id] = true;
            return id;
        }

        uint32_t dash_rangle_id = add_binding_token_combo("DASH_RANGLE",8,9,'-','>');

        uint32_t plus_plus_id = add_binding_token_combo("PLUS_PLUS",2,-1,'+','+');
        uint32_t dash_dash_id = add_binding_token_combo("DASH_DASH",2,-1,'-','-');
        uint32_t plus_plus_plus_id = add_binding_token_combo("PLUS_PLUS_PLUS",2,-1,'+','+','+');
        uint32_t plus_equals_plus_id = add_binding_token_combo("PLUS_EQUALS_PLUS",2,-1,'+','=','+');
        uint32_t plus_equals_id = add_binding_token_combo("PLUS_EQUALS",2,3,'+','=');

        uint32_t dot_dot_dot_id = add_binding_token_combo("DOT_DOT_DOT",8,9,'.','.','.');
        uint32_t dot_query_id = add_binding_token_combo("DOT_QUERY",8,9,'.','?');

        uint32_t langle_langle_id = add_binding_token_combo("LANGLE_LANGLE",8,9,'<','<');
        uint32_t rangle_rangle_id = add_binding_token_combo("RANGLE_RANGLE",8,9,'>','>');

        uint32_t equals_equals_id =  add_binding_token_combo("EQUALS_EQUALS",4,5,'=','=');
        uint32_t bang_equals_id =  add_binding_token_combo("BANG_EQUALS",4,5,'!','=');
        uint32_t langle_equals_id =  add_binding_token_combo("LANGLE_EQUALS",4,5,'<','=');
        uint32_t rangle_equals_id =  add_binding_token_combo("RANGLE_EQUALS",4,5,'>','=');
        uint32_t amp_amp_id =  add_binding_token_combo("AMP_AMP",3,3,'&','&');
        uint32_t pipe_pipe_id =  add_binding_token_combo("PIPE_PIPE",2,2,'|','|');

        uint32_t equals_rangle_id =  add_binding_token_combo("EQUALS_RANGLE",1,-1,'=','>');

        uint32_t random_combo_id = add_token_combo("RANDOM",'|','*','^','+');

        uint32_t arguments_id = reg_id("ARGUMENTS"); //For function calls

        void take_right(Context ctx, int amt) {
            for(int i = 0; i<amt; i++) {
                ctx.index()+=(i+1);
                if(ctx.index() < ctx.result().length()) {
                    process_node(ctx,ctx.result().get(ctx.index()));
                }
                for(int r=0;r<ctx.result().length();r++) {
                    if(ctx.result()[r]==ctx.node()) {ctx.index() = r;}
                }
            }

            for(int i = 0; i<amt; i++) {
                if(ctx.index() + 1 < ctx.result().length()) {
                    ctx.node().children() << ctx.result().take(ctx.index() + 1);
                } 
            }
        }
        void take_left(Context ctx, int amt) {
            for(int i = 0; i<amt; i++) {
                if(ctx.index() - 1 >= 0) {
                    ctx.node().children().insert(0,ctx.result().take(ctx.index() - 1));
                    ctx.index()--;
                }
            }
        }

        map<uint32_t,bool> intentional_unary;
        inline bool is_unary_id(uint32_t id) {
            if(intentional_unary.getOrDefault(id,false)) {return true;}

            const std::string& label = labels[id];
            const std::string suffix = "_unary";
            return label.size() >= suffix.size() && 
                   label.compare(label.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        void init_stage_a() {
            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(rparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(rbrace_id);
            discard_types.push_if_absent(lbracket_id);
            discard_types.push_if_absent(rbracket_id);
            //discard_types.push_if_absent(rbrace_id);
            discard_types.push_if_absent(comma_id);

            discard_types.push_if_absent(return_id);



            a_handlers.default_function = [this](Context& ctx) {
                int left_bp = left_binding_power.getOrDefault(ctx.node().type(), -1);
                int right_bp = right_binding_power.getOrDefault(ctx.node().type(), -1);
                // uspan->newline("On "+ctx.node().name().to_std());
                // uspan->log("Binding powers, left: ",left_bp," right: ",right_bp);
                // uspan->log("Before:\n",node_to_string(ctx.root()));

                if(left_bp == -1 && right_bp == -1) {                
                    // uspan->endline();
                    return;
                }
                
                if(is_live(ctx.left()) && left_bp > 0 && !discard_types.has(ctx.left().type())) {
                    int left_left_bp = left_binding_power.getOrDefault(ctx.left().type(), -1);
                    int left_right_bp = right_binding_power.getOrDefault(ctx.left().type(), -1);
                    // uspan->log("To my left: ",node_info(ctx.left()));
                    // uspan->log("Left's binding powers, left: ",left_left_bp," right: ",left_right_bp);
    
                    bool right_associative = right_bp < left_bp; //lbp > rbp means right assoc
                    bool should_steal = left_bp > (right_associative ? left_right_bp : left_left_bp);
                    if(should_steal) {
                        // uspan->log("May insert into left because my left binding power is greater than left's ",right_associative?"right binding power":"left binding power");
                    }

                    //!intentional_unary.getOrDefault(ctx.left().type(),false

                    if(!ctx.left().children().empty()) {
                        if(!is_unary_id(ctx.left().type())&&ctx.left().children().length()==1) {
                            should_steal = true;
                            // uspan->log("May insert into left because it has one child");
                        }
                        else if(discard_types.has(ctx.left().children().last().type())) {
                            // uspan->log("Going to otter because the last child of left is a discard type");
                            goto otter;
                        }
                    }

                    if(left_right_bp!=-1 && should_steal && !ctx.left().has_qual(lparen_id) && (ctx.node().type()!=lparen_id||ctx.left().type()!=group_id)) {
                        if((is_unary_id(ctx.left().type())&&ctx.left().children().length()==1)||ctx.left().children().length()>1) {
                            // uspan->log("Swapping because ",is_unary_id(ctx.left().type())?"left has one child and is unary":"left has more than one child",": "+node_info(ctx.left().children().last()));
                            ctx.node().children() << ctx.left().children().pop();
                        }
                        // uspan->log("Inserting into left ",(left_right_bp!=-1)?"because it has a right binding power":"because we should swap",": "+node_info(ctx.left()));
                        ctx.left().children() << ctx.result().take(ctx.index());
                    } else {
                        // uspan->log("Taking left ",((left_right_bp==-1)?"because left has no right binding power":(ctx.left().has_qual(lparen_id)?"because left was formed by parens":"")),": "+node_info(ctx.left()));
                        ctx.node().children() << ctx.left();
                        ctx.result().removeAt(ctx.index() - 1);
                    }
                } else {
                    otter:
                    if(!discard_types.has(ctx.node().type())) {
                        if(ctx.node().name().length()==1) { //Only single char opperators can be made unary like this
                            ctx.node().type(to_unary_id(ctx.node().type()));
                        }
                    }
                    ctx.index()++;
                }

                if(right_bp != -1 && ctx.index() < ctx.result().length()) {
                    Node next = ctx.result().get(ctx.index());
                    int next_lbp = left_binding_power.getOrDefault(next.type(), -1);
                    // uspan->log("To my right: ",node_info(next));
                    // uspan->log("Right's binding powers, left: ",next_lbp," right: ",right_binding_power.getOrDefault(next.type(), -1));
                    if(next_lbp == -1 && !discard_types.has(next.type())) { //It's an atom so we grab it
                        ctx.node().children() << ctx.result().take(ctx.index());
                        // uspan->log("Taking right because I have a right binding power and my left does not have a left binding power: "+node_info(ctx.node().children().last()));
                    } else {
                        // uspan->log("Leaving right because ",(next_lbp==-1)?"it has no left binding power":"it is a discard type");
                    }
                }
                ctx.index()--;

                // uspan->log("After:\n"+node_to_string(ctx.root()));
                // uspan->endline();
            };
    
            for(int m = 0; m<2; m++) {
                uint32_t open_id = m==0?rparen_id:rbracket_id;
                uint32_t close_id = m==0?lparen_id:lbracket_id;

                left_binding_power.put(close_id,10);

                a_handlers[open_id] = [this,close_id](Context& ctx) {
                    // uspan->newline("On "+labels[close_id]);
                    // uspan->log("Paren before:\n",node_to_string(ctx.root()));
                    ctx.result().removeAt(ctx.index());
                    Node on_right = deadptr;
                    if(ctx.result().length()>ctx.index()) {
                        on_right = ctx.result().get(ctx.index());
                        // uspan->log("On right: ",node_info(on_right));
                    }
                    int i = ctx.index()-1;
                    list<Node> gathered;
                    while(i>=0) {
                        Node on = ctx.result().get(i);
                        // uspan->log("On: ",node_info(on));
                        node_col on_from = ctx.result();
                        while(!on.children().empty()&&on.type()!=close_id) {
                            on_from = on.children();
                            on = on.children().last();
                            // uspan->log("Checking: ",node_info(on));
                        }
                        Node on_left = deadptr; //This logic was added just to handle the lambda arguments case, it's subject to future correction as nessecary
                        node_col left_from = ctx.result();
                        if(i>0) {
                            on_left = ctx.result().get(i-1);
                            while(!on_left.children().empty()&&on_left.type()!=group_id&&on_left.type()!=lparen_id&&on_left.type()!=lbracket_id) {
                                left_from = on_left.children();
                                on_left = on_left.children().last();
                            }
                        }
                        if(is_live(on_left)) {
                            // uspan->log("On left: ",node_info(on_left));
                        }
                        if(on.type()==close_id) {
                            gathered.reverse();

                            // uspan->log("On closing type");
                            
                            //If the paren holds a group, we're a lambda argument like [l, n](int i) and so we need to create the arguments structure.
                            if(close_id==lparen_id&&(on.children().length()==1&&on.children()[0].type()==group_id)) {
                                Node grouper = on.children().take(0);
                                grouper.children().push(on);
                                for(int j=0;j<on_from.length();j++) {
                                    if(on_from[j]==on) {on_from.col().set(j,(void*)&grouper); break;}
                                }
                                for(auto g : gathered) on.children() << g;

                                on.type(arguments_id);
                                grouper.type(lambda_id);

                                ctx.index(i);
                                on.quals() << turn_into_token(ctx.node());
                                break;
                            }

                            // if(close_id!=lparen_id||(ctx.result().length()<i+1&&ctx.result().get(i+1).type()!=rbracket_id)) { //So we can call functions inside brackets like arr[stoi(s)];
                            //     if(is_live(on_left)&&(on_left.type()==group_id||on_left.type()==lbracket_id)) { //For lambda arguments and lambda calls from indexed lists like arr[i](args)
                            //         for(auto g : gathered) on.children() << g;
                            //         if(close_id==lparen_id) {
                            //             on.type(arguments_id);
                            //             on_left.children().push(on_from==ctx.result()?ctx.result().take(i):on_from.pop());
                            //             ctx.index(i+(on_from==ctx.result()?1:0)); //I'm not sure if this is the right index or not, it should be noticble if it causes issues though
                            //         } else if(close_id==lbracket_id) { //For nested arrays like arr[0][1][2] 
                            //             //^ this is probably unessecary now that we promote lbrackets to groups.
                            //             on.children().insert(0, left_from==ctx.result()?ctx.result().take(i-1):left_from.pop());
                            //             ctx.index(i-1);
                            //             // uspan->log(green("PROMOTION LOGIC"));
                            //         }
                            //         break;
                            //     }
                            // }


                            bool was_given_children = false;
                            if(on.children().empty()) {
                                // uspan->log("Giving children to on because it has none");
                                for(auto g : gathered)
                                    on.children() << g;
                                was_given_children = true;
                            }
                            if(close_id!=lbracket_id) { //Brackets remain after a gather
                                Node token_on = copy_as_token(on);

                                if(!on.children().empty()) {
                                    on.copy(on.children().take(0));
                                    // uspan->log("Copying first child, now: ",node_info(on));
                                    if(was_given_children) {
                                        // uspan->log("Adding children again because was given them before");
                                        for(int g=1;g<gathered.length();g++) {
                                            on.children() << gathered[g];
                                        }
                                    }
                                }

                                on.quals() << token_on; //Copy the lparen
                            } else {
                                on.type(group_id); //Promote the lbracket to a group instead, so it can be bound
                            }
                            on.quals() << turn_into_token(ctx.node()); //Copy the rparen

                            if(!was_given_children) {
                                if(on.children().empty()||close_id==lbracket_id) {
                                    // uspan->log("Taking children because was not given children and ",close_id==lbracket_id?"I'm a bracket":"I had no children");
                                    for(auto g : gathered)
                                        on.children() << g;
                                } else { //This case if for things like int main(int a), where we want the gathered to go under main, not int
                                    // uspan->log("Giving children to last child because was not given children and ",close_id==lbracket_id?"I'm not a bracket":"I had children");
                                    for(auto g : gathered)
                                        on.children().last().children() << g;
                                }
                            }
                            ctx.index(i);
                            break;
                        } else {
                            gathered << ctx.result().take(i);
                            i--;
                        }
                    }
                    if(i < 0) {
                        ctx.result().push(ctx.node()); //Return the rparen to carry the error
                        print(red("rparen:a_handler unmatched closing paren"));
                    }
                    // uspan->log("Paren after:\n",node_to_string(ctx.root()));
                    // uspan->endline();
                };
            }
    
            a_handlers[identifier_id] = [this](Context& ctx){
                if(is_live(ctx.left()) && ctx.left().type() == identifier_id) {
                    while(ctx.index() < ctx.result().length() && ctx.result().get(ctx.index()).type() == identifier_id) {
                        ctx.left().children() << ctx.result().take(ctx.index());
                    }
                    ctx.index()--;
                } 
            };
        }

        uint32_t scope_id = reg_id("scope");
        uint32_t type_scope_id = reg_id("type_scope");


        void place_node_in_scope(Node node, Node insc) {
            node.in_scope(insc);
            for(int i=0;i<node.children().length();i++) {
                place_node_in_scope(node.children()[i],insc);
            }
        }

        void init_stage_s() {
            s_handlers[rbrace_id] = [this](Context& ctx){
                ctx.result().removeAt(ctx.index());
                int i = ctx.index()-1;
                list<Node> gathered;
                while(i>=0) {
                    Node on = ctx.result().get(i);
                    Node was_on = ctx.root(); //Storing the root for cases where we want to notify once children are gathered
                    while(!on.children().empty()&&on.type()!=lbrace_id) {
                        was_on = on;
                        on = on.children().last(); //Descend to the found lbrace
                    }
                    if(on.type()!=lbrace_id&&!on.scopes().empty()) {
                        for(int i=0;i<on.scopes().length();i++) {
                            on = on.scopes()[i];
                            while(!on.children().empty()&&on.type()!=lbrace_id) {
                                was_on = on;
                                on = on.children().last(); //Descend to the found lbrace
                            }
                            if(on.type()==lbrace_id) break;
                        }
                    }
                    if(on.type()==lbrace_id) {
                        on.quals().push(copy_as_token(on));
                        on.quals().push(turn_into_token(ctx.node()));
                        on.type(scope_id); //Turn it into a scope and hand over the contents
                        on.x(-1.0f); on.y(-1.0f);
                        gathered.reverse();
                        for(auto g : gathered) on.children() << g;

                        for(int c=0;c<was_on.children().length();c++) { //Promote to a scope
                            if(was_on.children()[c].idx==on.idx) {
                                Node newscope = was_on.children().take(c);
                                was_on.scopes() << newscope;
                                newscope.owner(was_on);
                                newscope.name() = was_on.name().to_std();
                                for(int n=0;n<newscope.children().length();n++){
                                   place_node_in_scope(newscope.children()[n],newscope);
                                }
                                break;
                            }
                        }
                        ctx.index(i);
                        break;
                    } else {
                        gathered << ctx.result().take(i);
                        i--;
                    }
                }
                if(i < 0) {
                    ctx.result().push(ctx.node()); //Return the rbrace to carry the error
                    print(red("rbrace:s_handler unmatched closing brace"));
                }
            };
            s_handlers[lbrace_id] = [this](Context& ctx){}; //Do nothing

            s_handlers.default_function = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_direct_pass(ctx.node());
                }
                
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }

            };
        }

    void resolve_node_literal(Context& ctx, void* val, uint32_t tag, uint32_t size) {
            standard_sub_process(ctx);
            ctx.node().type(literal_id);
            Value value = make_value(tag,size);
            // value.set(val);
            ctx.node().value(value);
        }

        void init_literals() {
            value_printers[ptr_id] = [this](Context& ctx) {
                Ptr p = *(Ptr*)ctx.value().get(); 
                ctx.source(Ptr_to_string(p,p.cachelevel));
            };
            value_printers[float_id] = [](Context& ctx) {ctx.source(std::to_string(*(float*)ctx.value().get()));};
            value_printers[int_id] = [](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source(std::to_string(*(int*)p));};
            value_printers[char_id] = [](Context& ctx) {ctx.source(std::string(1,*(char*)ctx.value().get()));};
            value_printers[bool_id] = [](Context& ctx) {ctx.source((*(bool*)ctx.value().get()) ? "true" : "false");};
            value_printers[string_id] = [this](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source() = *(string*)p;};
            value_printers[node_id] = [this](Context& ctx) {ctx.source(node_to_string((Node&)(*(Ptr*)ctx.value().get())));};
            value_printers[value_id] = [this](Context& ctx) {ctx.source(value_info((Value&)(*(Ptr*)ctx.value().get())));};
            value_printers[context_id] = [this](Context& ctx) {Context context = (Context&)(*(Ptr*)ctx.value().get()); std::string src = context_trace_to_string(context); ctx.source(src);};
                
            t_handlers[ptr_id] = [this](Context& ctx) {
                Ptr p = string_to_Ptr(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&p,ptr_id,sizeof(Ptr));
            }; 

            t_handlers[float_id] = [this](Context& ctx) {
                float stof = std::stof(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&stof,float_id,4);
            }; 
    
            t_handlers[int_id] = [this](Context& ctx) {
                int stoi = std::stoi(ctx.node().name().to_std());
                resolve_node_literal(ctx,(void*)&stoi,int_id,4);
            }; 
    
            t_handlers[bool_id] = [this](Context& ctx) {
                bool stob = ctx.node().name().to_std() == "true" ? true : false;
                resolve_node_literal(ctx,(void*)&stob,bool_id,1);
            }; 

            t_handlers[char_id] = [this](Context& ctx) {
                char stob = ctx.node().name()[0];
                resolve_node_literal(ctx,(void*)&stob,char_id,1);
            }; 
    
            t_handlers[string_id] = [this](Context& ctx) {
                Ptr ptr = ctx.node().name_ptr();
                resolve_node_literal(ctx,(void*)&ptr,string_id,sizeof(Ptr));
            }; 
        }

        //Scan for if this qual is the first one that's a type stamper (or rather, not a typeless)
        bool is_first_type_qual(Context& ctx) {
            for(uint32_t i=0;i<ctx.value().quals().length();i++){
                Node q = ctx.value().quals()[i];
                if(q==ctx.qual()) {
                    return true;
                } //v if it isn't a qual like static, hoisted, or global that doesn't imbue a type.
                if(!typeless_quals.hasKey(q.type())) return false;
            }
            return false;
        }

        void resolve_identifier(Context& ctx) {
            Node node = ctx.node();
            
            Value decl_value = make_value();
            bool found_a_value = find_value_in_scope(ctx.node());
            // if(is_live(node.value())) decl_value = node.value();
            // else decl_value = make_value();
            bool is_qualifier = is_live(node.value()) && node.value().type()!=0 && node.value().sub_type() != 0; 
            //We count it as a qualifer if it has a fully valid value to stamp

            int root_idx = -1;
            if(is_qualifier) {
                if(is_live(ctx.node().value())) {
                    decl_value.quals() << value_to_qual(node.value(),node.name().to_std());
                    node.quals() << copy_as_token(decl_value.quals().last(),node.x(),node.y(),node.z()); 
                }
                for(int i = 0; i < node.children().length(); i++) {
                    Node c = node.children()[i];
                    find_value_in_scope(c); //Process forward and consume other qualifers
                    if(c.type()!=identifier_id) {break;}

                    if(is_live(c.value())&&c.value().type()!=0&&c.value().type()!=duck_id) {
                        decl_value.quals() << value_to_qual(c.value(),c.name().to_std());
                        node.quals() << copy_as_token(decl_value.quals().last(),c.x(),c.y(),c.z()); 
                    } else {
                        root_idx = i;
                        break;
                    }
                }
                if(root_idx!=-1) {
                    Node root = node.children()[root_idx];
                    node.name(root.name().to_std());
                    node.x(root.x());
                    node.y(root.y());
                    for(int i = root_idx+1; i < node.children().length(); i++) {
                        Node c = node.children()[i];
                        find_value_in_scope(c);
                        if(is_live(c.value())&&c.value().type()!=0&&c.value().type()!=duck_id) {
                            node.quals() << value_to_qual(c.value(),c.name().to_std());
                            node.quals() << copy_as_token(decl_value.quals().last(),c.x(),c.y(),c.z());
                        } 
                    }
                    node.children(node.children().take(root_idx).children());
                }
            }
            
            if(node.scopes().empty()) { //Defer, the r_stage will do this later for scoped nodes
                standard_sub_process(ctx);
            }

            //For builtin functions and such
            if(keywords.hasKey(node.name().to_std())) {
                if(node.value().sub_type()!=0) {
                    node.type(node.value().sub_type());
                    node.value().sub_type(0);
                    return;
                }
            }

            // recycle_value(node.value()); //Figure out how to deal with lifetimes like this later
            node.value(decl_value);

            fire_quals(ctx, decl_value);

            bool has_scope = !node.scopes().empty();
            bool has_sub_type = node.value().sub_type() != 0;
            
            if(has_scope) {
                node.scopes()[0].owner(node);
                node.scopes()[0].name(node.name().to_std());
                if(has_sub_type) {
                    node.type(func_decl_id);
                    node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0],node.count_qual(hoisted_id));
                    declare_variable(node,node.value());
                    // node.value().type_scope(node.scopes()[0]); <- I don't belive the return value of functions should be descendable by that function, since the return is typically temporary
                    if(node.in_scope().type()==type_scope_id) {
                        std::string nname = node.name().to_std();
                        bool has_opp = false;
                        for(auto c : nname) {if(registered_opperators.getOrDefault(c,false)) {has_opp = true; break;}}
                        if(!has_opp) {nname=".\""+nname+"\"";} 
                        overload_type(node.in_scope().owner().value().type(),nname,method_call_id,node.value());

                        Node star = make_node(star_id);
                        Node type_term = make_node(identifier_id,node.in_scope().owner().name().to_std(),deadptr,node.scopes()[0]);
                        Node id_term = make_node(identifier_id,"this",deadptr,node.scopes()[0]);
                        star.children().push(type_term);
                        star.children().push(id_term);
                        node.children().insert(0,star);
                    }
                    for(int c=0;c<node.children().length();c++) {
                        place_node_in_scope(node.children()[c],node.scopes()[0]);
                    }
                } else if(ctx.root().type()!=dot_id) { //To stop scoped dot overloads from registering as type declerations
                    node.type(type_decl_id);
                    node.value(make_type_value(node.name().to_std(),0));
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value(), node.count_qual(hoisted_id)));
                    node.scopes()[0].type(type_scope_id);
                    _layout temp(add_template(node.value().type()));
                    layouts.put(node.value().type(),temp);
                    keywords.put(node.name().to_std(),node.value());
                    r_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                            ctx.value().size(layouts.get(ctx.value().type()).total_size);
                        }
                    };
                    x_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(is_live(ctx.value())&&is_first_type_qual(ctx)) {
                            ctx.value().data_col().push_default();
                            ctx.value().data_col().heterogenous = true;
                        }
                    }; 
                    value_printers[node.value().type()] = [this](Context& ctx){
                        ctx.source() = heterogenous_col_to_string(ctx.value().data_col());
                    };
                }
            } else {
                has_scope = find_node_in_scope(node); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node.type(var_decl_id);
                    //Shadowing will need a pass later, better coordination with how distirbute value returns and such.
                    //A noted other issue beyond the duck typing collisions is that for loops are polluting sibling scopes
                    if(find_value_in_scope(node)) { //If this node was duck typed
                        if(node.value().type()==duck_id) {
                            node.value().copy(decl_value,false);
                        }   
                    } else {
                        if(node.in_scope().type()==type_scope_id) {
                            node.in_scope().value_table().put(node.name().to_std(), decl_value); //So we don't distribute into function bodies, we need to alias later via this, as it's per instance
                            layouts[node.in_scope().owner().value().type()].add_prop(node.value().type(),node.value().size(),node.name().to_std(),0,0,decl_value);
                        } else {
                            declare_variable(node,decl_value);
                        }
                    }
                    node.value().sub_type(0);
                } else if(has_scope) {
                    node.type(func_call_id);
                    node.value(node.scope().owner().value());
                    // find_value_in_scope(node); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    // if(is_live(node.value().type_scope()))
                    //     node.scopes()[0] = node.value().type_scope(); //Swap to the type scope
                    // if(!node->children.empty()) {
                    //     node->name.append("(");
                    //     for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
                    // }
                } else if(found_a_value) { //If we already had a value and nothing interesting happened to us, reclaim it
                    find_value_in_scope(node);
                    if(node.value().type()==function_id) {
                        node.type(lambda_call_id);
                    }
                } else {                                   
                    if(node.in_scope().value_table().hasKey("this")) { //The has check is so we don't inject this on the names of declared variables at the top
                        bool children_has_node = false;
                        for(int i=0;i<node.in_scope().children().length();i++){
                            if(node.in_scope().children()==node) {children_has_node=true; break;}
                        }

                        if(node.in_scope().owner().type()==func_decl_id&&!children_has_node) { //These are arguments in the function decleration, the children_has_node check is becuase the args are in the scope of the decleration but not actually in it's children list
                            
                        } else {
                            Node climb = node;
                            while(is_live(climb)&&climb.type()!=func_decl_id) { 
                                climb = climb.in_scope().owner();
                            }
                            if(is_live(climb)&&climb.type()==func_decl_id) { //Checking if this is a member of the method or not
                                if(climb.in_scope().value_table().hasKey(node.name().to_std())) {
                                    //node.value().copy(climb.in_scope().value_table().get(node.name().to_std()),true);

                                    Node accessor = make_node(dot_id);
                                    place_node_in_scope(accessor,node.in_scope());
                                    Node star = make_node(star_id);
                                    Node this_node = make_node(identifier_id,"this",node.in_scope().value_table().get("this"),node.in_scope());
                                    star.children().push(this_node);
                                    accessor.children().push(star);
                                    accessor.children().push(node);
                                    ctx.result().removeAt(ctx.index()); ctx.result().insert(ctx.index(),accessor); 
                                    process_node(ctx,star); //Because this won't get processed again like the scope children do, it's up to us to resolve it here
                                } else {
                                    //Just a plain identifer, not a member
                                }
                            } else {
                               //It isn't in a method, something gave it this as a glitch probably
                            }
                        }
                    } else {
                        //No clue what this could be, duck type it
                        if(
                            (ctx.root().type()==arguments_id)||
                            (ctx.root().type()==equals_id&&ctx.root().children().length()>1&&ctx.root().children()[1]==node)) 
                        {
                            //print(yellow("DUCK EQUALS: "),node_to_string(ctx.root()));
                            node.type(var_decl_id);
                            decl_value.type(duck_id);
                            declare_variable(node,decl_value);
                        } else {
                            //print(magenta("NO  DUCK EQUALS: "),node_info(node));
                        }
                    }
                }
            }
        }

        bool is_node_opperator(Node n) {
            for(int i=0;i<n.name().length();i++) {
                if(registered_opperators[n.name().at(i)]) return true;
            }
            return false;
        }



        void standard_travel_walk(Node root) {
            Stage* old_stage = active_stage;
            if(!walk_handlers.default_function) {
                walk_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            }
            start_stage(walk_handlers);
            standard_travel_pass(root);
            walk_handlers.handlers.clear();
            walk_handlers.default_function = nullptr;
            start_stage(old_stage);
        }


        inline void open_signature(std::string& signature) {
            if(!signature.empty()) {
                char d = signature.at(signature.length()-1);
                if(d!=','&&d!=';'&&d!='('&&d!=')'){signature+="-";}
            }
        }
        inline void continue_signature(Context& ctx, std::string& signature) {
            if(!ctx.node().children().empty()) {
                signature+="(";
                standard_sub_process(ctx);
                signature+=")";
            }
        }

        void overload_type(uint32_t type, const std::string& instr, uint32_t overload_to, Value value = deadptr) {
            float old_at_x = at_x; float old_at_y = at_y;
            at_x = 0.0f; at_y =0.0f;
            Node expr = tokenize(labels[type]+instr);
            at_x = old_at_x; at_y = old_at_y;

            Stage* old_stage = active_stage;
            start_stage(a_handlers);
            standard_direct_pass(expr);
            walk_handlers[identifier_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().type(labels_lookup.getOrDefault(ctx.node().name().to_std(),undefined_id));
            };
            walk_handlers[string_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().type(identifier_id);
            };
            standard_travel_walk(expr);
            std::string signature = "";
            walk_handlers.default_function = [this,&signature](Context& ctx) {
                open_signature(signature);
                signature+=labels[ctx.node().type()];
                continue_signature(ctx,signature);
            };
            // walk_handlers[arguments_id] = [this,&signature](Context& ctx) {
            //     standard_sub_process(ctx);
            // };
            walk_handlers[comma_id] = [this,&signature](Context& ctx) {signature+=",";};
            walk_handlers[dot_dot_dot_id] = [this,&signature](Context& ctx) {signature+="...";};
            walk_handlers[end_id] = [this,&signature](Context& ctx) {signature+=";";};
            walk_handlers[identifier_id] = [this,&signature](Context& ctx) {
                open_signature(signature);
                signature+="'"+ctx.node().name().to_std()+"'";
                continue_signature(ctx,signature);
            };
            standard_travel_walk(expr);
            start_stage(old_stage);
            registered_operator_ids[overload_to] = true;
            overloaded_operator_ids[overload_to] = true;
            recycle_node(expr);

            if(!layouts.hasKey(type)) {
                layouts.put(type,_layout(add_template(type)));
            }
            layouts.get(type).add_overload(signature,overload_to,value);
        }
        uint32_t overload_type(uint32_t type, const std::string& instr, const std::string& f, Value value = deadptr) {
            uint32_t id = reg_id(f);
            overload_type(type,instr,id,value);
            return id;
        }

        uint32_t overload_type(uint32_t type, const std::string& instr, const std::string& f, Value value, Handler xhandler) {
            uint32_t id = reg_id(f);
            overload_type(type,instr,id,value);
            x_handlers[id] = xhandler;
            return id;
        }

        uint32_t overload_type(uint32_t type, list<std::string> instrs, const std::string& f, Value value, Handler xhandler) {
            uint32_t id = reg_id(f);
            for(int i=0;i<instrs.length();i++) {
                overload_type(type,instrs[i],id,value);
            }
            x_handlers[id] = xhandler;
            return id;
        }

        void what_I_see(Context& ctx) {
            print(bold_str(ctx.node().name().to_std()),": I see my root is ",green(ctx.root().name().to_std()),", my value type is ",blue(labels[ctx.node().value().type()])," and to my left is ",yellow(is_live(ctx.left())?ctx.left().name().to_std():"nothing"));
        }

        map<uint32_t,bool> init_signature_transparent_ids(){
            map<uint32_t,bool> to_return;
            to_return.put(group_id,true); to_return.put(property_id,true); 
            to_return.put(identifier_id,true); to_return.put(literal_id,true); 
            to_return.put(lambda_id,true); to_return.put(arguments_id,true);
            return to_return;
        }
        map<uint32_t,bool> signature_transparent_ids = init_signature_transparent_ids();
        inline bool is_signature_transparent(uint32_t type) {
            return signature_transparent_ids.getOrDefault(type,false);
        }

        list<std::string> derive_signatures(Node root) { 
            list<std::string> signatures;
            signatures << labels[root.type()]+"(";
            walk_handlers.default_function = [this,&signatures,&root](Context& ctx) {
                if(signatures.length()>=100) {return;} //<- SIGNATURE LIMIT, beacuse constructors can get chunky, may be a more permenant solution in the future, may not.
                if(ctx.node().type()!=arguments_id&&ctx.root().type()==lambda_id) return;
                list<std::string> next;
                for(int i=0;i<signatures.length();i++) {
                    std::string base = signatures[i];
                    std::string opened = base+(base.empty()||base.back()==','||base.back()==';'||base.back()=='('||base.back()==')'?"":"-");

                    bool is_not_left = ctx.root()!=root||ctx.index()>0;
                    if(is_not_left) {
                        if(ctx.node().type()==identifier_id) {
                            next << opened+"'"+ctx.node().name().to_std()+"'";
                        } else if(ctx.node().type()!=literal_id) {
                            next << opened+labels[ctx.node().type()];
                        }
                    } 
                    if(is_live(ctx.node().value())) {next << opened+labels[ctx.node().value().type()];} else {next<<opened+"UNDEFINED";}
                    if(is_not_left) {next << opened+"any";}
                }
                signatures = next;
                if(is_signature_transparent(ctx.node().type())&&!ctx.node().children().empty()) {
                    for(int i=0;i<signatures.length();i++) {signatures[i]+="(";}
                    standard_sub_process(ctx);
                    for(int i=0;i<signatures.length();i++) signatures[i]+=")";
                    for(int i=0;i<next.length();i++) {
                        signatures << next[i]+"(...)";
                    }
                }
                if(ctx.node().has_qual(comma_id)) {
                    for(int i=0;i<signatures.length();i++) signatures[i]+=",";
                }
                if(ctx.node().has_qual(end_id)) {
                    for(int i=0;i<signatures.length();i++) signatures[i]+=";";
                }
            };
            standard_travel_walk(root);
            for(int i=0;i<signatures.length();i++) {
                signatures[i]+=")";
            }
            CHECK_ERROR_VAL(signatures," error while deriving signatures for ",node_basic_info_with_children_and_position(root));
            return signatures;
        }
        
        void print_overloads(uint32_t type) {
            _layout& l = layouts.get(type);
            list<CCol*> overloads = l.overloads.allCells();
            for(int i=0;i<overloads.length();i++) {
                print((*(QString*)overloads[i]).to_std()," : ",labels[(*(type_and_value*)l.overloads[overloads[i]->index]).type]);
            }
        }

        bool resolve_overload(Node root) {
            CHECK_ERROR_VAL(false,"Attempted to resolve overloads while another error was flagged");

            uint32_t old_root_type = 0;
            if(!is_pure_operator(root.type())) {
                if(is_pure_operator(root.sub_type())) {
                    old_root_type = root.type();
                    root.type(root.sub_type());
                } else {
                    return false;
                }
            }
            if(!is_live(root.left())||!is_live(root.left().value())) return false;
            if(!layouts.hasKey(root.left().value().type())) return false;

            // print("Resolving overload on: ",node_to_string(root));

            list<std::string> signatures = derive_signatures(root);

            // for(int i=0;i<signatures.length();i++) {
            //     print(i,": ",signatures[i]);
            // }
            // print_overloads(root.left().value().type());

            _layout& l = layouts.get(root.left().value().type());
            for(int i=0;i<signatures.length();i++) {
                std::string signature = signatures[i];
                if(l.has_overload(signature)){
                    type_and_value tnv = l.get_overload(signature);
                    if(root.type()==tnv.type) {return false;}
                    if(old_root_type==tnv.type) {root.type(old_root_type); return false;}
                    if(!is_pure_operator(root.sub_type())) {
                        root.sub_type(root.type());
                    }
                    root.type(tnv.type);
                    if(is_live(tnv.value)) {
                        Value value = tnv.value;
                        if(value.type()!=0) {
                            Value copy = root.value();
                            if(!is_live(copy)) {
                                copy = make_value();
                                root.value(copy);  
                            }
                            copy.copy(value,true);
                        } else {
                            // if(root.sub_type()==0) {
                            //     fire_quals(ctx,root.value());
                            // } 
                            root.value(make_value(
                                root.left().value().sub_type(),
                                root.left().value().sub_size()
                            ));
                        }
                    }
                    return true;
                }
            }

            if(root.type()==dot_id) {
                if(is_live(root.left())&&is_live(root.left().value())) {
                    uint32_t ltype = root.left().value().type();
                    if(layouts.hasKey(ltype)) {
                        _layout& l = layouts.get(ltype);
                        std::string prop = root.right().name().to_std();
                        if(l.label_to_index.hasKey(prop)) {
                            uint32_t index = l.label_to_index.get(prop);
                            if(is_live(l.ptrs[index])) { //If we were handed a full value just copy that over (why not just always use this though... mark for later)
                                root.value(make_value()); root.value().copy(l.ptrs[index],true);
                            } else {
                                root.value(make_value(l.tags[index], l.sizes[index], l.offsets[index], l.subtags[index], l.subsizes[index]));
                            }
                            return true;
                        } else {
                            // std::string error_str = "Layout of "+labels[ltype]+" does not have prop "+prop+" Signatures: ";
                            // for(int i=0;i<signatures.length();i++) {
                            //     error_str+="\n"+std::to_string(i)+": "+signatures[i];
                            // }
                            // throw_error(error_str);
                            print_overloads(ltype);
                            throw_error("Layout of "+labels[ltype]+" does not have prop "+prop);
                            return false;
                        }
                    } 
                }
            } else if(root.type()==method_call_id) { //Turn into a function call (revise later)
                // root.type(func_call_id);
                // Node amp = make_node(to_unary_id(amp_id));
                // amp.value(make_value(ptr_id,sizeof(Ptr)));
                // Node match_this = make_node(identifier_id,"match_this",root.children()[0].value(),root.in_scope());
                // amp.children().push(match_this);
                // process_node(amp); //Resolve this
                // node_col args = root.children()[1].children();
                // root.children(args);
                // root.children().insert(0,amp);
                // root.scopes().push(root.value().type_scope()); //<- REVIST THIS LATER! Type scope's meaning was changed to mean where a variable was declared, so this is going to be broken
                // sync_args(ctx);
                // root.value(root.value().type_scope().owner().value());
            }
            CHECK_ERROR_VAL(false,"An error occured while resolving overloads");
            return false;


        }

        inline bool resolve_overload(Context ctx, bool do_subprocess = true) {
            return resolve_overload(ctx.node());
        }

        void mark_and_skip(Context ctx) {
            for(int i=ctx.index();i>=0;i--) {
                ctx.result()[i].resolved(true);
            }
            ctx.index(ctx.result().length());
        }

        void sync_value(Context ctx, Col* col = nullptr) {
            if(!is_live(ctx.node().value())) return;
            if(!col) {
                if(is_live(ctx.node().value().data_ptr())) {
                    col = &ctx.node().value().data_col();
                } else {
                    return;
                }
            }
            if(col->tag!=ctx.node().value().type()) {
                ctx.node().value().type(col->tag);
                ctx.node().value().size(col->element_size);
                if(resolve_overload(ctx.root())) {
                    mark_and_skip(ctx);
                }
            }
        }

        void sync_identifier(Context& ctx) {
            if(!is_live(ctx.node().value())||!is_live(ctx.node().value().data_ptr())) return;

            if(ctx.node().value().type() != ctx.node().sub_type()) {
                ctx.node().sub_type(ctx.node().value().type());
                if(resolve_overload(ctx.root())) {
                    mark_and_skip(ctx);
                }
            }


            // Col& col = ctx.node().value().data_col();
            // if(col.tag != ctx.node().sub_type()) {
            //     ctx.node().sub_type(col.tag);
            //     ctx.node().value().type(col.tag);
            //     ctx.node().value().size(col.element_size);
            //     if(resolve_overload(ctx.root())) {
            //         mark_and_skip(ctx);
            //     }
            // }
        }


        uint32_t static_qual = add_qual("static");
        uint32_t capture_id = make_tokenized_keyword("capture");

        void bind_args(Node call, Node decl) {
            DEBUG_ONLY(if(call.children().length()!=decl.children().length()) {
                throw_error("Wrong number of arguments for function:\n",node_to_string(call),"\n",node_to_string(decl)); 
                return;
            })
            for(int i = 0; i < call.children().length(); i++) {
                Node arg = call.children()[i];
                if(arg.type()==equals_id||arg.sub_type()==equals_id||arg.type()==arguments_id) continue;
                Node param = decl.children()[i];
                Node assignment = make_node(equals_id);
                assignment.children().push(param);
                assignment.children().push(arg);
                call.children().col().set(i,(void*)&assignment);
            }
        }

        void sync_args(Context& ctx) {
            if(!ctx.node().scopes().empty()) {

                Node owner = ctx.node().scopes()[0].owner();
                if(owner.type()==lambda_id&&!owner.children().empty()&&owner.children()[0].type()==to_unary_id(amp_id)) {
                    //Special case for self refrenetial lambdas [&](){} which don't bind any arguments, until a more general solution can be found
                    return;
                }

                bind_args(ctx.node(),ctx.node().scopes()[0].owner());
            }
        }

        void gather_all_values_in_scope(value_col& subvals, Node scope) {
            for(int i=0;i<scope.children().length();i++) {
                Node c = scope.children()[i];
                if(c.type()==capture_id||c.type()==lambda_id) continue;
                if(is_live(c.value())) {
                    for(int n=0;n<subvals.length();n++) {
                        if(subvals.get(n)==c.value()) goto skipatom;
                    }
                    subvals.push(c.value());
                }
                skipatom:
                gather_all_values_in_scope(subvals,c);
            }
            for(int s=0;s<scope.scopes().length();s++) {
                Node subscope = scope.scopes()[s];
                if(!is_live(subscope.owner())||subscope.owner()==scope) {
                    gather_all_values_in_scope(subvals,subscope);
                }
            }
        }

        bool data_ptr_is_in_my(Ptr& p, uint32_t type_id) {
            if(p.cachelevel==3) {
                return (p.cache==&types)&&p.pool==type_id;
            } else if(p.cachelevel==2) {
                return p.cache==(&types[type_id]);
            } else if(p.cachelevel==4) {
                return (p.cache==this)&&p.subunit==0&&p.pool==type_id;
            } else if(p.cachelevel>4) {
                return p.unit==uid&&p.subunit==0&&p.pool==type_id;
            }
            return false;
        }
        bool data_ptr_is_stackable(Ptr& p) {
            return data_ptr_is_in_my(p,data_store_id);
        }
        bool data_ptr_is_in_names(Ptr& p) {
            return data_ptr_is_in_my(p,name_store_id);
        }

        bool value_is_descendable(Value sval, Node scope) {
            if(is_live(sval.type_scope())) { //Only variables with a type scope need to be analyzed, everything else is local and temporary
                if(is_live(sval.data_ptr())&&!sval.has_qual(static_qual)) { //Static values just stay where they are, they don't descend and ascend
                    Node climb = sval.type_scope().owner(); //Discern if the variable decleration is inside of this function decleration
                    while(is_live(climb)) { //Possibly break on another func decl, might cause confusion with closures, experiment with this later.
                        if(climb.scope()==scope) {
                           return true;
                        }
                        climb = climb.climb();
                    }
                }
            }
            return false;
        }

        //Add another row to each data column for function calls
        int descend_call_scope(Context& ctx, Node scope) {
            Value sv = scope.value();
            int loc = sv.loc()+1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            if(subvals.empty()) {gather_all_values_in_scope(subvals,scope);}
            for(uint32_t i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i); //Static values just stay where they are, they don't descend and ascend
                if(value_is_descendable(sval,scope)) {
                    Ptr& dataptr = sval.data_ptr();
                    if(data_ptr_is_stackable(dataptr)) {
                        Col& datacol = resolve_to_col(dataptr);
                        if(datacol.length()<=loc) {
                            datacol.push_default(); //Make space on the stack for this value by descending it's column
                        } 
                        dataptr.sidx = loc;
                    }
                }
            }
            return loc;
        }

        void ascend_call_scope(Node scope) {
            Value sv = scope.value();
            int loc = sv.loc()-1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            for(int i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i);
                if(value_is_descendable(sval,scope)) {
                    Ptr& newptr = sval.data_ptr();
                    if(!data_ptr_is_stackable(newptr)) continue;
                    newptr.sidx = loc;
                }
            }
        }

        void ascend_all_values(Node n, Node scope, list<Value>& ascended) {
            if(is_live(n.value())) {
                if(!ascended.has(n.value())&&value_is_descendable(n.value(),scope)) {
                    n.value().data_ptr().sidx-=1;
                    ascended << n.value();
                }
            }
            for(uint32_t i=0;i<n.children().length();i++) {
                ascend_all_values(n.children()[i],scope,ascended);
            }
        }

        void call_func(Context& ctx, Node scope) {
            int stack_depth = descend_call_scope(ctx,scope);
            DEBUG_ONLY(if(stack_depth>500) {throw_error("Stack overflow on function call: ",node_info(ctx.node())); return;})
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                list<Value> ascended;
                ascend_all_values(c.right(),scope,ascended);
                if(ascended.empty()||!ascended.has(c.left().value())) {
                    process_node(ctx,c);
                } else { //If we're assigning the same value back into itself we need to grab a version of it that's at the newely descended location and then take the ascended version and assign in from that.
                    process_node(ctx,c.left());
                    Value copyval = make_value();
                    copyval.copy(c.left().value(),false);
                    copyval.data_ptr().sidx+=1;
                    process_node(ctx,c.right());
                    assign(copyval,c.right().value());
                    recycle_value(copyval,false);
                }
                for(uint32_t j=0;j<ascended.length();j++) {
                    ascended[j].data_ptr().sidx+=1;
                }
            }
            CHECK_ERROR("ABORTING FUNCTION CALL BEFORE PASS");
            if(!standard_travel_pass(scope,ctx.sub())) { //If the return didn't already ascend
                ascend_call_scope(scope);
            }
        }

        bool resolve_types(Value lv, Value rv, uint32_t ltype) {
            uint32_t rtype = rv.type();

            if(ltype == duck_id) return true; //Ducks accept anything
            if(rtype == duck_id) return true; //Ducks also go into anything
            if(rtype == 0) return false; //If the type is undefined it's probably a bug, check where this manifests in the future and throw_error if always a terminable error
            if(ltype == rtype) return true; //An exact match, just pass on
            
            //Add coercion handlers in the future

            //throw_error("compiler:resolve_types unhandled type mismatch, ltype: ", labels[ltype]," rtype: ", labels[rtype]);
            return false;
        }

        void assign(Value lv, Value rv) {
            if(rv.data_col().empty()) return; //No data to copy

            Ptr lp = lv.data_ptr();
            Ptr rp = rv.data_ptr();

            if(!is_live(lp)) return; //Normally caused by something being delcared but never used, and thus missed by the m pass
            DEBUG_ONLY(if(!is_live(rp)) {throw_error("right term of equals is invalid"); return;})
            
            //Deal with this later when it becomes more useful
            // if(is_live(lv)) {
            //     for(int q=0;q<lv.quals().length();q++) {
            //         Node qual = lv.quals()[q];
            //         if(is_true_type.getOrDefault(qual.type(),false)) {
            //             if(resolve_types(lv, rv, qual.type())) {break;}
            //             else {return;}
            //         }
            //     }
            // }
            Col& lcol = resolve_to_col(lp);

            uint32_t subtype = 0; uint32_t subsize = 0; uint32_t alias = ptr_id;

            Col& rcol = resolve_to_col(rp);
            if(rcol.tag==string_id) {subtype = char_id; subsize = 1; alias = string_id;}
            else if(rv.sub_type()!=0) {subtype = rv.sub_type(); subsize = rv.sub_size(); alias = rv.type();}
            // else if(rcol.tag==ptr_id&&!rcol.empty()) {
            //     //Alias through one of it's pointers to discern what's at that location
            //     print(node_to_string(ctx.node()));
            //     print("(Implment later) Checking column through: ",Ptr_as_string(*(Ptr*)rcol[0]));
            // }   

            if(is_ptr_alias(lcol.tag)&&is_ptr_alias(rcol.tag)) { //Temporary kludge, figure out Ptr assignment later (this is meant for raw ptrs only, not aliases like string)
                if(resolve_to_col(lp).heterogenous) {
                    resolve_to_col(lp).qset(lp.sidx,resolve_ptr(rp),rv.size());
                } else {
                    resolve_to_col(lp).set(lp.sidx,resolve_ptr(rp));
                }
                return;
            }

            Ptr subp = deadptr;
            if(lcol.tag==ptr_id||lcol.tag==string_id||(lcol.heterogenous&&(lv.type()==string_id||lv.type()==ptr_id))) { //Properly fix heterogenaity's interaction with assignment later
                if(!lcol.empty()) {
                    if(!lcol.heterogenous&&lp.sidx>=lcol.length()) {
                        throw_error("compiler:assign left value sidx is out of bounds for col length, L: ",value_info(lv)," R: ",value_info(rv)); return;
                    }
                    subp = *(Ptr*)lcol.get(lp.sidx); //The Ptr currently stored to the other collection
                    if(subtype==0||subsize==0&&is_live(subp)) { //Free the subptr if we're realiasing to a scalar
                        //print("Recycling subp");
                        recycle_column(subp);
                    } else {
                        if(is_live(subp)) {
                            //print("Resetting subp");
                            Col& subcol = resolve_to_col(subp);
                            subcol.clear(); subcol.element_size = subsize; subcol.tag = subtype;
                        } else {
                            //print("Regnerating subp");
                            subp = get_ticket(lp.pool,subsize,subtype,&resolve_to_subunit(lp));
                            resolve_to_col(lp).set(lp.sidx,(void*)&subp);
                        }
                    }
                } else if(subtype!=0&&subsize!=0) {
                    //print("Replacing subp");
                    subp = get_ticket(lp.pool,subsize,subtype,&resolve_to_subunit(lp));
                    resolve_to_col(lp).push((void*)&subp);
                }
            }
            Col& col = resolve_to_col(lp); //Realias because the push may have invalidated it earlier
            if(subtype!=0&&subsize!=0) { //If right is a pointer to a collection
                if(!col.heterogenous&&col.tag!=alias) { //This was accidentally firing on contexts
                    //print("Realiasing");
                    col.element_size = sizeof(Ptr); col.tag=alias;
                    lv.size(sizeof(Ptr)); lv.type(alias);
                    col.clear();
                    subp = get_ticket(lp.pool,subsize,subtype,&resolve_to_subunit(lp));
                    resolve_to_col(lp).push((void*)&subp);
                } else {
                    //print("Replacing");
                }
                Ptr dataptr  = *(Ptr*)rv.get();
                Col& datacol = resolve_to_col(dataptr); //Copy over the data to it's new position
                Col& subcol = resolve_to_col(subp);
                subcol.clear();
                for(int i=0;i<datacol.length();i++) {
                    subcol.push(datacol[i]);
                }
            } else { //If we're the direct value in the store pool
                if(col.element_size!=rv.size()||col.tag!=rv.type()||lv.type()!=rv.type()) {
                    // print("Clearing and pushing");
                    col.clear();
                    col.element_size = rv.size(); col.tag=rv.type();
                    lv.size(rv.size()); lv.type(rv.type());
                    col.push(rv.get());
                } else if(col.empty()) {
                    //print("Pushing");
                    col.push(rv.get());
                } else {
                    //print("Setting");
                    if(rv.data_ptr().sidx>=rv.data_col().length()) {
                        print(yellow("core:assign right value data pointer sub index is out of bounds: "),Ptr_as_string(rv.data_ptr()));
                    } else {
                        col.set(lp.sidx,rv.get());
                    }
                }
            }
        }
        void assign(Node left, Node right) {
            Value rv = right.value();
            Value lv = left.value();
            if(left.type()==to_decl_id(amp_id)) {
                left.value().data_ptr(right.value().data_ptr());
            } else if(left.type()==var_decl_id&&right.value().type()!=string_id&&left.value().type()!=duck_id) {
                left.set(right.get());
            } else {
                assign(lv,rv);
            }
        }

        void deep_copy_value(Value v, Value o) {
            v.copy(o,true); //This already does 90% of the work, all we're really doign here is marshaling ptr reallocation
            //assign(v,o); //This was causing problems, come back later and revise this.
        }


        Node instantiate_template_scope(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
            Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());

            if(is_live(decl.scopes()[0].value())) {
                new_scope.value(make_value());
                new_scope.value().copy(decl.scopes()[0].value(), true);
            }
            //new_scope.owner(call);
        
            for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
                new_scope.quals() << decl.scopes()[0].quals()[i];
            }
        
            map<uint32_t, Value> value_alias_table;
            map<uint32_t, Node> node_alias_table;

            Node puppet = make_node(func_decl_id);
            puppet.value(decl.scopes()[0].value());
            puppet.scopes() << new_scope;
            new_scope.owner(puppet);
        
            Stage* oldstage = active_stage;
            start_stage(x_handlers); //Because we're trying to derive the value, this may not be the right long term solution though
            //This was a bit of an accident born from how things were working in Webcorn's standard_gather_from_scope
            //And an anomaly with FUNC_DECLs revelead when trying to make templating work
            if(args_already_synced) { //Because they've already been turned into the assignment form by sync_args
                for(int i = 0; i < decl.children().length(); i++) {
                    Node c = decl.children()[i];
                    //The puppet gets a copy of c[0], the variable decleration
                    Node puppet_arg = make_node();
                    deep_copy_node(puppet_arg, c, value_alias_table, node_alias_table);
                    puppet.children() << puppet_arg;
                    //print("Aliasing values|",c.value().idx," as values|",puppet.children()[i].value().idx);
                    //Alias c[0] to the puppet's copy, so we can re-call this instnatiation in the future with fresh args
                    value_alias_table.put(c.value().idx, puppet.children()[i].value());
                }
            } else {
                node_col decl_args = decl.children();
                for(int i=0;i<decl_args.length();i++) { //For lambdas and such where we have arguments within the children body
                    if(decl_args[i].type()==arguments_id){
                        for(int j=0;j<decl_args.length();j++) {
                            if(j!=i) {
                                Value newv = make_value();
                                deep_copy_value(newv,decl_args[j].value());
                                value_alias_table.put(decl_args[j].value().idx, newv);
                            }   
                        }
                        decl_args = decl_args[i].children(); 
                        break;
                    }
                } 
                for(int i = 0; i < call.children().length(); i++) { 
                    process_node(ctx, call.children()[i]);
                    if(i < decl_args.length()) {
                        value_alias_table.put(decl_args[i].value().idx, call.children()[i].value());
                    }
                }
            }

            active_stage = oldstage;
        
            for(int i = 0; i < decl.scopes()[0].children().length(); i++) {
                Node copy = make_node();
                copy.in_scope(new_scope);
                deep_copy_node(copy, decl.scopes()[0].children()[i], value_alias_table, node_alias_table);
                new_scope.children() << copy;
            }
        
            return new_scope;
        }

        Node instantiate_function(Node func, Context& ctx){
            Node new_scope = make_node(func.scopes()[0].type(), 0, func.name().to_std());
            if(is_live(func.scopes()[0].value())) {
                new_scope.value(make_value());
                new_scope.value().copy(func.scopes()[0].value(), true);
            }
            for(int i = 0; i < func.scopes()[0].quals().length(); i++) {
                new_scope.quals() << func.scopes()[0].quals()[i];
            }
        
            map<uint32_t, Value> value_alias_table;
            map<uint32_t, Node> node_alias_table;
            
            Node puppet = make_node(func_decl_id);
            puppet.value(func.value());
            if(!puppet.value().sub_values().empty()) {
                value_alias_table[puppet.value().sub_values()[0].idx] = puppet.value().sub_values()[0];
            }
            puppet.scopes() << new_scope;
            new_scope.owner(puppet);

            node_col decl_args = func.children();
            int arguments_idx = -1;
            for(int i=0;i<decl_args.length();i++) {
                if(decl_args[i].type()==arguments_id) {
                    arguments_idx = i;
                    break;
                }
            }
            for(int i=0;i<decl_args.length();i++) {
                if(i==arguments_idx) continue;
                Node cap = decl_args[i];
                if(!is_live(cap.value())) continue;
                Node copy = make_node();
                deep_copy_node(copy,cap,value_alias_table,node_alias_table);
                puppet.children() << copy;
                value_alias_table[cap.value().idx] = puppet.children().last().value();
            }
            if(arguments_idx!=-1) {
                Node args_node = func.children()[arguments_idx];
                Node args_copy = make_node();
                deep_copy_node(args_copy,args_node,value_alias_table,node_alias_table);
                puppet.children() << args_copy;
                for(int i=0;i<args_node.children().length();i++) {
                    value_alias_table[args_node.children()[i].value().idx] = args_copy.children()[i].value();
                }
            }
        
            for(int i = 0; i < func.scopes()[0].children().length(); i++) {
                Node copy = make_node();
                copy.in_scope(new_scope);
                deep_copy_node(copy, func.scopes()[0].children()[i], value_alias_table, node_alias_table);
                new_scope.children() << copy;
            }
            return puppet;
        }

        void instantiate_template(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
            Node new_scope = instantiate_template_scope(call, decl, ctx, args_already_synced);
            call.scopes_col().set(0, (void*)&new_scope);
            new_scope.owner(call);
        }

        // void instantiate_template(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
        //     if(!call.scopes().empty()) {
        //         Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());
        //         call.scopes_col().set(0,(void*)&new_scope);
                
        //         if(is_live(decl.scopes()[0].value())) {
        //             call.scopes()[0].value(make_value());
        //             call.scopes()[0].value().copy(decl.scopes()[0].value(),true);
        //         }
        //         call.scopes()[0].owner(call);
                
        //         for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
        //             call.scopes()[0].quals() << decl.scopes()[0].quals()[i];
        //         }

        //         map<uint32_t,Value> value_alias_table;
        //         map<uint32_t,Node> node_alias_table;
        
        //         Stage* oldstage = active_stage;
        //         start_stage(x_handlers); //Because we're trying to derive the value, this may not be the right long term solution though
        //         //This was a bit of an accident born from how things were working in Webcorn's standard_gather_from_scope
        //         //And an anomaly with FUNC_DECLs revelead when trying to make templating work
        //         if(args_already_synced) { //Because they've already been turned into the assignment form by sync_args
        //             for(int i = 0; i < call.children().length(); i++) {
        //                 Node c = call.children()[i];
        //                 process_node(ctx, c.children()[1]);
        //                 value_alias_table.put(c.children()[0].value().idx, c.children()[1].value());
        //             }
        //         } else {
        //             for(int i = 0; i < call.children().length(); i++) {
        //                 process_node(ctx, call.children()[i]);
        //                 if(i < decl.children().length()) {
        //                     value_alias_table.put(decl.children()[i].value().idx, call.children()[i].value());
        //                 }
        //             }
        //         }
        //         active_stage = oldstage;
        
        //         for(int i = 0; i < decl.scopes()[0].children().length(); i++) {
        //             Node copy = make_node();
        //             copy.in_scope(call.scopes()[0]);
        //             //print("Deep copying:\n",node_to_string(decl.scopes()[0].children()[i],2));
        //             deep_copy_node(copy, decl.scopes()[0].children()[i], value_alias_table, node_alias_table);
        //             //print("Deep copy done, copy:\n",node_to_string(copy,2));
        //             call.scopes()[0].children() << copy;
        //         }
        //     } else {
        //         print("CALL HAS NO SCOPE");
        //     }
        // }



        uint32_t add_function(const std::string& f, Handler x_handler, uint32_t size = 0, uint32_t return_type = 0) {
            Value val = register_value(f,size,return_type);
            keywords.put(f,val);
            uint32_t id = val.sub_type();
            if(return_type!=0) {
                r_handlers[id] = [this](Context& ctx) {
                    standard_sub_process(ctx);
                    //:/
                };
            }
            x_handlers[id] = x_handler;
            return id;
        }
        void also_called(const std::string& alias, const std::string& f) {
            if(keywords.hasKey(f)) {
                keywords.put(alias,keywords.get(f));
            }
        }

        std::string children_to_string(Context& ctx, node_col children) {
            std::string to_print = "";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                process_node(ctx,c);
                if(is_live(c.value())) {
                    to_print += value_as_string(c.value());
                } else {
                    print(red("compiler:children_to_string Can not print dead value: "));
                    print(node_to_string(c,1,0,1));
                    CHECK_ERROR_VAL(to_print,"crash during debug print");
                }
            }
            return to_print;
        }

        uint32_t print_id = add_function("print",[this](Context& ctx){ 
            print(children_to_string(ctx,ctx.node().children()));
        });
        uint32_t return_id = make_tokenized_keyword("return");
        uint32_t stageprint_id = add_function("stageprint",[this](Context& ctx){ 
            print("[",active_stage->label,"] ",children_to_string(ctx,ctx.node().children()));
        });

        uint32_t true_id = add_function("true",[this](Context& ctx){
            bool t = true;
            ctx.node().value().set((void*)&t);
        },1,bool_id);
        uint32_t false_id = add_function("false",[this](Context& ctx){
            bool f = false;
            ctx.node().value().set((void*)&f);
        },1,bool_id);

        int CURSOR_FIRST = 100;
        int CURSOR_LAST  = -100;
        Ptr cursor_get(Context& ctx, int delta, bool is_unary) {
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return deadptr;}
            Ptr p = (is_unary ? ctx.node().c0().value().data_ptr() : ctx.node().getPtr(0));
            if(delta==CURSOR_FIRST) {p.sidx=0;}
            else if(delta==CURSOR_LAST) {p.sidx=resolve_to_col(p).length()-1;}
            else if((delta<0&&(int)p.sidx+delta>=0)||(delta>0&&p.sidx+delta<resolve_to_col(p).length())) {p.sidx+=delta;}
            else {p = deadptr;}
            return p;
        };
        uint32_t amp_dash_id = add_binding_unary_token_combo("AMP_DASH",4,8,'&','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_dash_id = add_binding_token_combo("GROUP_DASH",11,12,'[','-',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t amp_plus_id = add_binding_unary_token_combo("AMP_PLUS",4,8,'&','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_plus_id = add_binding_token_combo("GROUP_PLUS",11,12,'[','+',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t amp_carat_id = add_binding_unary_token_combo("AMP_CARAT",4,8,'&','^',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_FIRST,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_carat_id = add_binding_token_combo("GROUP_CARAT",11,12,'[','^',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_FIRST,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t amp_dollar_id = add_binding_unary_token_combo("AMP_DOLLAR",4,8,'&','$',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_LAST,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_dollar_id = add_binding_token_combo("GROUP_DOLLAR",11,12,'[','$',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_LAST,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t amp_dash_dash_id = add_binding_unary_token_combo("AMP_DASH_DASH",4,8,'&','-','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_dash_dash_id = add_binding_token_combo("GROUP_DASH_DASH",11,12,'[','-','-',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t amp_plus_plus_id = add_binding_unary_token_combo("AMP_PLUS_PLUS",4,8,'&','+','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,true); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        uint32_t group_plus_plus_id = add_binding_token_combo("GROUP_PLUS_PLUS",11,12,'[','+','+',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,false); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);
        
        uint32_t star_dash_id = add_binding_unary_token_combo("STAR_DASH",4,8,'*','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        uint32_t star_plus_id = add_binding_unary_token_combo("STAR_PLUS",4,8,'*','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        uint32_t star_carat_id = add_binding_unary_token_combo("STAR_CARAT",4,8,'*','^',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_FIRST,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        uint32_t star_dollar_id = add_binding_unary_token_combo("STAR_DOLLAR",4,8,'*','$',[this](Context& ctx){
            Ptr p = cursor_get(ctx,CURSOR_LAST,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        uint32_t star_dash_dash_id = add_binding_unary_token_combo("STAR_DASH_DASH",4,8,'*','-','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        uint32_t star_plus_plus_id = add_binding_unary_token_combo("STAR_PLUS_PLUS",4,8,'*','+','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,true); ctx.node().value().data_ptr(p); sync_value(ctx);
        },0,duck_id);
        
        uint32_t query_plus_id = add_binding_unary_token_combo("QUERY_PLUS",4,8,'?','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t query_dash_id = add_binding_unary_token_combo("QUERY_DASH",4,8,'?','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t query_dash_dash_id = add_binding_unary_token_combo("QUERY_DASH_DASH",4,8,'?','-','-',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t query_plus_plus_id = add_binding_unary_token_combo("QUERY_PLUS_PLUS",4,8,'?','+','+',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t dash_dash_query_id = add_binding_unary_token_combo("DASH_DASH_QUERY",4,8,'-','-','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t plus_plus_query_id = add_binding_unary_token_combo("PLUS_PLUS_QUERY",4,8,'+','+','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t dash_query_id = add_binding_unary_token_combo("DASH_QUERY",4,8,'-','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t plus_query_id = add_binding_unary_token_combo("PLUS_QUERY",4,8,'+','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,true); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_query_plus_id = add_binding_token_combo("GROUP_QUERY_PLUS",11,12,'[','?','+',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_query_dash_id = add_binding_token_combo("GROUP_QUERY_DASH",11,12,'[','?','-',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_dash_query_id = add_binding_token_combo("GROUP_DASH_QUERY",11,12,'[','-','?',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-1,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_plus_query_id = add_binding_token_combo("GROUP_PLUS_QUERY",11,12,'[','+','?',']',[this](Context& ctx){
            Ptr p = cursor_get(ctx,1,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_query_dash_dash_id = add_binding_token_combo("GROUP_QUERY_DASH_DASH",11,12,'?','-','-','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,-2,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);
        uint32_t group_query_plus_plus_id = add_binding_token_combo("GROUP_QUERY_PLUS_PLUS",11,12,'?','+','+','?',[this](Context& ctx){
            Ptr p = cursor_get(ctx,2,false); bool v = is_live(p); ctx.node().value().set((void*)&v);
        },sizeof(bool),bool_id);

        void init() override {
            init_literals();
            init_tokenizer();
            init_stage_a();
            init_stage_s();

            register_type("int",int_id,4);
            register_type("float",float_id,4);
            register_type("bool",bool_id,1);
            register_type("char",char_id,1);
            register_type("string",string_id,sizeof(Ptr));
            register_type("Node",node_id,sizeof(Ptr));
            register_type("Value",value_id,sizeof(Ptr));
            register_type("Context",context_id,sizeof(Ptr));
            register_type("Ptr",ptr_id,sizeof(Ptr));

            register_type("Col",col_id,sizeof(Ptr));
            value_printers[col_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};
            register_type("ColCol",colcol_id,sizeof(Ptr));
            value_printers[colcol_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};
            register_type("ColColCol",colcolcol_id,sizeof(Ptr));
            value_printers[colcolcol_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};
            register_type("PtrColColCol",subunit_id,sizeof(Ptr));
            value_printers[subunit_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};

            register_type("func",function_id,sizeof(Ptr));
            value_printers[function_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};
            register_type("duck",duck_id,0);
            value_printers[duck_id] = [this](Context& ctx){ctx.source()="QUACK!";};
            register_type("void",void_id,0);
            value_printers[void_id] = [this](Context& ctx){ctx.source()="void!";};

            register_type("Header",header_id,sizeof(Ptr));
            value_printers[header_id] = [this](Context& ctx){ctx.source(Ptr_as_string(*(Ptr*)ctx.value().get()));};

            set_binding_powers(random_combo_id,8,9);

            t_handlers[identifier_id] = [this](Context& ctx){resolve_identifier(ctx);};
            t_handlers[equals_id] = [this](Context& ctx){standard_sub_process(ctx);};

            t_handlers.default_function = [this](Context& ctx){if(ctx.node().scopes().empty()) {standard_sub_process(ctx);}}; //Because resolving passes will already cover the sub process for scoped nodes
            r_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            x_handlers.default_function = [this](Context& ctx){};
            x_handlers[0] = [this](Context& ctx){ //For roots and such
                standard_sub_process(ctx);
            };
            x_handlers[scope_id] = [this](Context& ctx){ //For scopes
                standard_sub_process(ctx);
            };

            t_handlers[root_id] = [this](Context& ctx){
                if(is_live(ctx.node().node_table_ptr())) {
                    list<CCol*> node_cells = ctx.node().node_table().col().allCells();
                    for(int i=0;i<node_cells.length();i++) {
                        distribute_node(ctx.node(),((QString&)(*(node_cells[i]))).to_std(),ctx.node().node_table()[node_cells[i]->index],0);
                    }
                }
                if(is_live(ctx.node().value_table_ptr())) {
                    list<CCol*> value_cells = ctx.node().value_table().col().allCells();
                    for(int i=0;i<value_cells.length();i++) {
                        distribute_value(ctx.node(),((QString&)(*(value_cells[i]))).to_std(),ctx.node().value_table()[value_cells[i]->index],0);
                    }
                }
                standard_sub_process(ctx);
            };
            x_handlers[root_id] = [this](Context& ctx){standard_sub_process(ctx);};


            s_handlers[equals_rangle_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;
                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                } else if(is_live(right)) {
                    ctx.index()++;
                    process_node(ctx,right);
                    ctx.index()--;
                    Node newscope = make_node(scope_id,ctx.node().name().to_std(),deadptr,deadptr);
                    ctx.node().scopes() << newscope;
                    newscope.owner(ctx.node());
                    place_node_in_scope(right,newscope);
                    newscope.children() << ctx.result().take(ctx.index()+1);
                }
            };
            t_handlers[equals_rangle_id] = [this](Context& ctx){
                if(is_live(ctx.node().scope())) {
                    Node args = make_node(arguments_id);
                    Node arg = ctx.node().left();
                    args.children().push(arg);
                    while(!arg.children().empty()) {
                        args.children().push(arg.children().take(0));
                    }
                    ctx.node().children().col().set(0,(void*)&args);
                    place_node_in_scope(args,ctx.node().scope());
                } else {
                    standard_sub_process(ctx);
                }
            };

            r_handlers[func_decl_id] = [this](Context& ctx) {
                fire_quals(ctx,ctx.node().value());
                Node scope = ctx.node().scopes()[0];
                if(!is_live(scope.value())) {
                    scope.value(make_value()); 
                    scope.value().loc(0); //Set location for stack depth
                }
            };
            x_handlers[func_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };

            t_handlers[lambda_id] = [this](Context& ctx){
                if(is_live(ctx.node().scope())) {
                    ctx.node().value(make_value(function_id,sizeof(Ptr)));
                    Node scope = ctx.node().scope();
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                    place_node_in_scope(ctx.node().children().last(),scope);
                } else {
                    standard_sub_process(ctx);
                }
            };  
            t_handlers[group_id] = [this](Context& ctx){
                if(!ctx.node().scopes().empty()) {
                    ctx.node().type(lambda_id);
                    ctx.node().value(make_value(function_id,sizeof(Ptr)));
                    Node scope = ctx.node().scopes()[0];
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                } else {
                    standard_sub_process(ctx);
                }
            };  
            r_handlers[group_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                // if(r_handlers.has(ctx.node().type())) {
                //     standard_process(ctx);
                // }
            };
            r_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                //:/
                fire_quals(ctx,ctx.node().value());
                sync_args(ctx);
                //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
            };
            r_handlers[lambda_call_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                fire_quals(ctx,ctx.node().value());
            };
            x_handlers[lambda_id] = [this](Context& ctx){
                if(is_live(ctx.node().value())) {
                    Node& puppet =  (Node&)ctx.node().getPtr();
                    if(!ctx.node().children().empty()&&ctx.node().children()[0].type()==to_unary_id(amp_id)) {
                        puppet = ctx.node(); //For [&](){} type lambdas, instead of copying each time it just means direct refrence, like a statless template
                    } else {
                        if(!is_live(puppet)) {
                            puppet = instantiate_function(ctx.node(),ctx);
                            if(puppet.children().length()>0) {
                                bind_args(ctx.node(),puppet);
                            }
                        } else {
                            standard_sub_process(ctx);
                        }   
                    }
                }
            };
            x_handlers[lambda_call_id] = [this](Context& ctx) {
                if(ctx.node().has_qual(lparen_id)) {
                    Node func = ctx.node().getNode();
                    Value decl_val = func.value();
                    Node func_scope = func.scope();
                    if(!decl_val.sub_values().empty()) {
                        ctx.node().value(decl_val.sub_values()[0]);
                    }
                    if(ctx.node().scopes().empty()) { //We're a lambda being called for the first time
                        ctx.node().scopes() << func_scope;
                        if(!func.children().empty()) {
                            bind_args(ctx.node(),func.children().last());
                        }
                    } else {
                        ctx.node().scopes().col().set(0,(void*)&func_scope);
                    }
                } else {
                    return;
                }
                fire_quals(ctx,ctx.node().value());
                Node scope = ctx.node().scopes()[0];
                call_func(ctx,scope);
            };
            x_handlers[func_call_id] = [this](Context& ctx) {
                fire_quals(ctx,ctx.node().value());
                Node scope = ctx.node().scopes()[0];
                //print("Calling function");
                call_func(ctx,scope);
                //print("Call succeded");
            };


            t_handlers[return_id] = [this](Context& ctx){
                if(ctx.index()+1<ctx.result().length()) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }
                standard_sub_process(ctx);
            };
            r_handlers[return_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node climb = ctx.node();
                while(is_live(climb)&&climb.type()!=func_decl_id&&climb.type()!=lambda_id) { //WARNING: we need to make sure things like in blocks which also use returns are safe with this! 
                    //This might try to bind to some random function via climbing when it does this, so when metaprogramming becomes visible in the compielr, add gaurds.
                    climb = climb.in_scope().owner();
                }
                ctx.node().parent(climb);
                if(is_live(ctx.node().parent())) {
                    if(ctx.node().parent().type()==lambda_id) {
                        if(!ctx.node().children().empty()) {
                            ctx.node().value(make_value());
                            ctx.node().value().copy(ctx.node().children()[0].value(),true);
                            ctx.node().parent().value().sub_values().push(ctx.node().value());
                        }
                    } else {
                        ctx.node().value(ctx.node().parent().value());
                    }
                }
            };  
            x_handlers[return_id] = [this](Context& ctx){
                //uspan->log("BEFORE:\n",node_to_string(ctx.node()));
                standard_sub_process(ctx);
                //uspan->log("AFTER SUB PROCESS:\n",node_to_string(ctx.node()));
                if(is_live(ctx.node().parent())) {
                    if(!ctx.node().children().empty()) {
                        assign(ctx.node(),ctx.node().left());
                        //uspan->log("AFTER ASSIGN:\n",node_to_string(ctx.node()));
                    }
                    ascend_call_scope(ctx.node().parent().scopes()[0]);
                }
                //uspan->log("AFTER:\n",node_to_string(ctx.node()));
                ctx.state(1);
                return;
            };

            r_handlers[equals_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
            };
            x_handlers[equals_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                CHECK_ERROR("Attempted to execute equals while another error was flagged");
                assign(ctx.node().left(),ctx.node().right());
                // if(ctx.node().children().length()==2) {
                //     backwards_sub_process(ctx);
                //     DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute equals while another error was flagged")); return;})
                //     assign(ctx.node().left(),ctx.node().right());
                // }
            };

            make_tokenized_keyword("any",any_id);
            make_tokenized_keyword("null",null_id);

            r_handlers[to_unary_id(bang_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(bool_id,1));
                resolve_overload(ctx);
            };
            x_handlers[to_unary_id(bang_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                bool b = *(bool*)ctx.node().children()[0].value().get();
                b = !b;
                ctx.node().value().set((void*)&b);
            };

            r_handlers[identifier_id] = [this](Context& ctx){
                standard_sub_process(ctx); //This may be why resolve_overload had a standard sub process in the first place, I think I forgot to include it here on identifer when I first wrote it!
                //So I just gave it to identifer and disabled the sub process on resolve overload.
                //:/
            };
            r_handlers[literal_id] = r_handlers[identifier_id];
            x_handlers[identifier_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                sync_identifier(ctx);
            };

            t_handlers[dash_rangle_id] = [this](Context& ctx){
                if(ctx.node().children().length()==2) {
                    Node star = make_node(to_unary_id(star_id));
                    place_node_in_scope(star, ctx.node().in_scope());
                    star.children().push(ctx.node().left());
                    ctx.node().children().col().set(0,(void*)&star);
                    ctx.node().type(dot_id);
                    standard_process(ctx);
                } else {
                    standard_sub_process(ctx);
                }
            };

            r_handlers[dot_id] = [this](Context& ctx){
                if(ctx.node().right().type()!=identifier_id&&!is_operator(ctx.node().right().type())) { //Clear it if it's keyword to free up the namespace
                    ctx.node().right().type(identifier_id);
                    if(!is_live(ctx.node().right().value())) {
                        ctx.node().right().value(make_value(0));
                    }
                    if(ctx.node().right().value().type()!=0) {
                        ctx.node().right().value().type(0);
                        ctx.node().right().value().size(0);
                        ctx.node().right().value().sub_type(0);
                        ctx.node().right().value().sub_size(0);
                    }
                }
                standard_sub_process(ctx);
                resolve_overload(ctx);
                // if(r_handlers.has(ctx.node().type())) {
                //     standard_process(ctx);
                // }
            };
            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(ctx.node().type()!=dot_id) {standard_process(ctx); return;}
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value value = ctx.node().value();

                if(right.type()==identifier_id&&is_live(value)) {
                    Ptr ptr = deadptr;
                    uint32_t rvt = left.value().type();
                    if(is_ptr_alias(rvt)) {
                        void* p = left.value().get();
                        DEBUG_ONLY(if(ERROR_FLAG) {return;});
                        ptr = *(Ptr*)p;
                    } else {
                        ptr = left.value().data_ptr();
                    }
                    ptr.sidx = value.address();
                    if(!right.children().empty()) { //This is a bit jank, I would prefer to find a way to get = working instead
                        if(resolve_to_col(ptr).heterogenous) {
                            resolve_to_col(ptr).qset(ptr.sidx,(void*)&right.children()[0].value().data_ptr(),right.children()[0].value().size());
                        } else {
                            resolve_to_col(ptr).set(ptr.sidx,(void*)&right.children()[0].value().data_ptr());
                        }
                    } else {
                        //if(is_assignment.getOrDefault(ctx.root().type(),false)&&is_live(ctx.left())) {
                            value.data_ptr(ptr); //Setting the data pointer itself
                        // } else {
                        //     value.set(resolve_ptr(ptr)); //Setting what the data_ptr points to
                        // }
                    }
                }
            };


            tokenizer_state_functions[quote_id] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(string_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                }

                if(c == '"') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0],at_x,at_y,at_z);

                    std::string name = ctx.node().name().to_std();
                    size_t last_nl = name.rfind('\n');
                    size_t line_start = (last_nl == std::string::npos) ? 0 : last_nl + 1;
                    std::string line_content = name.substr(line_start);
                    if(!line_content.empty()) {
                        float line_x = at_x - (float)line_content.length();
                        Node line_token = make_node(string_id, 0, line_content, line_x, at_y, at_z);
                        line_token.mute(true);
                        ctx.node().quals() << line_token;
                    }


                    ctx.node().quals() << closer;
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        default:  at_x -= 1.0f; ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
                    std::string name = ctx.node().name().to_std();
                    size_t last_nl = name.rfind('\n');
                    size_t line_start = (last_nl == std::string::npos) ? 0 : last_nl + 1;
                    std::string line_content = name.substr(line_start);
                    if(!line_content.empty()) {
                        float line_x = at_x - (float)line_content.length();
                        Node line_token = make_node(string_id, 0, line_content, line_x, at_y, at_z);
                        line_token.mute(true);
                        ctx.node().quals() << line_token;
                    }
                    
                    ctx.node().name().push("\n");
                    at_y += 1.0f;
                    at_x = -1.0f;
                } else {
                    ctx.node().name().push(c);
                }
            };

            //Add handeling as a single char later if we only ever iterate over one item, for now this is just another way to make strings (for TwigSnap)
            tokenizer_state_functions[single_quote_id] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(string_id);
                    ctx.node().name().col().clear();
                }

                if(c == '\'') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;
                    if(ctx.node().name().length()==1) {
                        ctx.node().type(char_id);
                    }
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        default: at_x -= 1.0f; ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
                    ctx.node().name().push('\n');
                    at_y += 1.0f;
                    at_x = -1.0f;
                } else {
                    ctx.node().name().push(c);
                }
            };
 
            r_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[langle_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                bool result =      
                    *(int*)p1
                    <
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[rangle_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                bool result =      
                    *(int*)p1
                    >
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };


            r_handlers[rangle_equals_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[rangle_equals_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    >=
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[langle_equals_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            x_handlers[langle_equals_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    <=
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };


            // r_handlers[plus_equals_id] = [this](Context& ctx){
            //     if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
            //     standard_sub_process(ctx);
            //     resolve_overload(ctx);
            // };

            r_handlers[plus_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };

            x_handlers[plus_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                CHECK_ERROR("Left arg of plus is invalid");
                void* p2 = ctx.node().children()[1].value().get();
                CHECK_ERROR("Right arg of plus is invalid");
                int result =      
                    *(int*)p1
                    +
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[dash_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[dash_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    -
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[star_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[star_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    *
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[slash_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[slash_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    /
                    *(int*)p2
                ;
                ctx.node().value().set((void*)&result);
            };

            r_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(int_id,4));
            };
            x_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                int neg = -(*(int*)ctx.node().children()[0].value().get());
                ctx.node().value().set((void*)&neg);
            };

            uint32_t tilde_amp_id = add_binding_unary_token_combo("TILDE_AMP",9,10,'~','&');
            r_handlers[tilde_amp_id] = [this](Context& ctx){
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(node_id,sizeof(Ptr)));
                else return;
                resolve_overload(ctx);
            };
            x_handlers[tilde_amp_id] = [this](Context& ctx){
                Node n = deadptr;
                if(is_live(ctx.node().c0().scope())) {
                    n = ctx.node().c0().scope().owner();
                }
                CHECK_ERROR("Error in tilde_amp");
                ctx.node().value().set((void*)&n);
            };

            x_handlers[plus_plus_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                if(ctx.node().c0().value().size()!=sizeof(Ptr)) {throw_error("can't plus_plus because the node does not have storage for a Ptr"); return;}
                Ptr& p = ctx.node().getPtr(0);
                p.sidx+=1;
            };
            x_handlers[dash_dash_id] = [this](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                if(ctx.node().c0().value().size()!=sizeof(Ptr)) {throw_error("can't dash_dash because the node does not have storage for a Ptr"); return;}
                Ptr& p = ctx.node().getPtr(0);
                p.sidx-=1;
            };

            r_handlers[amp_amp_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!is_live(ctx.node().value())) ctx.node().value(make_value(bool_id,1));
            };
            r_handlers[pipe_pipe_id] = r_handlers[amp_amp_id];
            x_handlers[amp_amp_id] = [this](Context& ctx){
                process_node(ctx, ctx.node().children()[0]);
                bool left = *(bool*)ctx.node().children()[0].value().get();
                if(!left) {
                    ctx.node().value().set((void*)&left);
                    return;
                }
                process_node(ctx, ctx.node().children()[1]);
                bool result = *(bool*)ctx.node().children()[1].value().get();
                ctx.node().value().set((void*)&result);
            };
            x_handlers[pipe_pipe_id] = [this](Context& ctx){
                process_node(ctx, ctx.node().children()[0]);
                bool left = *(bool*)ctx.node().children()[0].value().get();
                if(left) {
                    ctx.node().value().set((void*)&left);
                    return;
                }
                process_node(ctx, ctx.node().children()[1]);
                bool result = *(bool*)ctx.node().children()[1].value().get();
                ctx.node().value().set((void*)&result);
            };



            x_handlers[make_tokenized_keyword("root_name")] = [this](Context& ctx){
                if(ctx.node().children().empty()) {
                    ctx.node().value(make_value(string_id,sizeof(Ptr)));
                    ctx.node().value().set((void*)&ctx.root().name_ptr());
                } else {
                    ctx.root().name() = ctx.node().children()[0].name();
                }
            };
        }


        void test_compiler() {
            Node root = tokenize("[](int i, int b)");
            start_stage(a_handlers);
            standard_direct_pass(root);
            uspan->print_all();
            print(node_to_string(root));
        }
    };
}