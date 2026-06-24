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
        
        
        Stage& a_handlers = reg_stage("assembling");
        Stage& s_handlers = reg_stage("scoping");
        Stage& t_handlers = reg_stage("typing");
    
        Stage& d_handlers = reg_stage("discovering");
        Stage& r_handlers = reg_stage("resolving");
        Stage& e_handlers = reg_stage("evaluating");
    
        Stage& m_handlers = reg_stage("modeling");
        Stage& i_handlers = reg_stage("inspecting");
        Stage& x_handlers = reg_stage("executing");

        map<std::string,Value> keywords;
        //Qual handlers which act on the value
        size_t to_prefix_id(size_t id) {return id+1;}
        //Qual handlers which act on the node
        size_t to_suffix_id(size_t id) {return id+2;}

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
            t_handlers[to_prefix_id(type)] = [](Context& ctx){
                if(ctx.value().sub_type() == 0) {
                    ctx.value().sub_type(ctx.qual().sub_type());
                    ctx.value().type(ctx.qual().type());
                    ctx.value().size(ctx.qual().value().size());
                    if(is_live(ctx.qual().value().type_scope()))
                        ctx.value().type_scope(ctx.qual().value().type_scope());
                }
            };
            // r_handlers[to_prefix_id(type)] = [](Context& ctx){
            //     if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
            //         ctx.value().type(ctx.qual().value().type());
            //         ctx.value().size(ctx.qual().value().size());
            //     }
            // };
        }

        Value make_type_value(const std::string& f, size_t size = 0) {
            Value val = make_qual_value(f,size);
            add_type_stamping_handler(val.type());
            return  val;
        }

        uint32_t add_qual(const std::string& f, uint32_t size = 0) {
            Value val = make_qual_value(f,size);
            keywords.put(f,val);
            return val.type();
        }


        void register_type_initilizers(uint32_t prefix_type) {
            r_handlers[prefix_type] = [this](Context& ctx){
                if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                    ctx.value().size(layouts.get(ctx.value().type()).total_size);
                }
            };
            x_handlers[prefix_type] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0].type()!=ptr_id) { //Beause Ptrs store subtypes in their quals
                    ctx.value().data_col().push_default();
                    ctx.value().data_col().heterogenous = true;
                }
            }; 
        }
        uint32_t make_type(const std::string& f, uint32_t size = 0) {
            Value val = make_type_value(f,size);
            keywords.put(f,val);
            register_type_initilizers(to_prefix_id(val.type()));
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

        float at_x = 0.0f;
        float at_y = 0.0f;

        size_t make_tokenized_keyword(const std::string& token_name, size_t default_id = 0) {
            size_t id = default_id;
            if(default_id==0) {
                id = reg_id(token_name);
            }
            tokenized_keywords.put(token_name,id);
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
                ctx.node(make_node(0,0,"",at_x,at_y));
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
        size_t hash_id = add_token('#',"HASH");

        size_t in_alpha_id = reg_id("IN_ALPHA");
        size_t in_digit_id = reg_id("IN_DIGIT");
        size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return
        size_t quote_id = add_token('"',"QUOTE");
        size_t comment_id = reg_id("COMMENT");
        size_t single_quote_id = add_token('\'',"SINGLE_QUOTE");

        Node tokenize(const std::string& code) {
            Node root = make_node();
            root.name("ROOT");
            node_col result = root.children();
            uint32_t state = 0;
            at_x = 0.0f;
            at_y = 0.0f;
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

            while (index<code.length()) {
                char c = code.at(index);
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
            deep_recycle_context(ctx);
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
                    ctx.node().type(tokenized_keywords.getOrDefault(ctx.node().name().to_std(),ctx.node().type()));
                    return;
                } else {
                    ctx.node().name().push(c);
                    if(ctx.index()+1==ctx.source().length()) {
                        ctx.state(0); 
                        ctx.node().type(tokenized_keywords.getOrDefault(ctx.node().name().to_std(),ctx.node().type()));
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
                    ctx.node(make_node(identifier_id,0,std::string(1,c),at_x,at_y));
                    ctx.result().push(ctx.node());
                }
                else if(std::isdigit(c)) {
                    ctx.state(in_digit_id);
                    ctx.node(make_node(int_id,0,std::string(1,c),at_x,at_y));
                    ctx.result().push(ctx.node());
                }  else {
                    print("tokenize:default_function missing handling for char: ",c);
                }
            };
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
        uint32_t gloabl_qual = add_qual("global");
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
            }
            return val;
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
            }
            return carry;
        }

        //Make this cleaner later, probably when I do the normalization update and get more equipment
        void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
            n.type(o.type());
            n.sub_type(o.sub_type());
            n.name(o.name().to_std());
            n.x(o.x());
            n.y(o.y());
            n.z(o.z());
            n.mute(o.mute());
            n.resolved(o.resolved());
        
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
        
            n.quals().clear();
            for(int i = 0; i < o.quals().length(); i++) {
                Node newq = make_node();
                deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
                n.quals() << newq;
            }
            
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
                    //print("Deep copying as ",news.idx);
                    deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
                    news.owner(n);
                    n.scopes() << news;
                } else {
                    //print("Leaving untouched");
                    n.scopes() << o.scopes()[i];
                }
            }
        
            if(value_alias_table.hasKey(o.value().idx)) {
                Value aliased = value_alias_table.get(o.value().idx);
                n.value(aliased);
            } else {
                if(is_live(o.value())) {
                    if(!o.has_qual(gloabl_qual)) {
                        if(!is_live(n.value())) {
                            n.value(make_value());
                        }
                        n.value().copy(o.value(),true);
                    } else {
                        n.value(o.value());
                    }
                }
            }
        
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
            } else if(n.type()==func_decl_id) {
                n.value().type_scope(n.scopes()[0]);
                n.scopes()[0].owner(n);
                value_alias_table.put(o.value().idx, n.value());
                node_alias_table.put(o.scopes()[0].idx, n.scopes()[0]);
                // print("Put ",o.scopes()[0].idx," node alias for : ",node_info(n.scopes()[0]));
            }
        }
        

        Node value_to_qual(Value val, std::string name = "", float x = -1.0f, float y = -1.0f) {
            Node to_return = make_node(val.type(),val.sub_type(),name,x,y,0.0f,val);
            return to_return;
        }

        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power[id] = lbp;
            right_binding_power[id] = rbp;
        }

        map<char,bool> registered_opperators;
        list<uint32_t>  registered_opperator_ids;
        size_t add_binary_operator(char c, const std::string& f, int lbp, int rbp, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            set_binding_powers(id,lbp,rbp);
            registered_opperators[c] = true;
            registered_opperator_ids << id;
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");

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
                    if(type_term.type()==var_decl_id||(is_live(type_term.value())&&type_term.value().sub_type()==type_term.type())) {
                        ctx.node().type(decl_id);
                        ctx.node().value(make_value());
                        ctx.node().value().copy(type_term.value(),true);
                        ctx.node().value().quals().push(value_to_qual(type_term.value()));
                        ctx.node().name(id_term.name().to_std());
                        ctx.node().value().sub_type(0);
                        ctx.node().value(distribute_value(ctx.node().in_scope(), ctx.node().name().to_std(),ctx.node().value(),ctx.node().count_qual(hoisted_id)));
                        ctx.node().children().clear();
    
                        
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
    
            return id;
        }

        size_t register_binary_operator(int use_id, int lbp, int rbp) {
            return add_binary_operator(' ',labels[use_id],lbp,rbp,use_id);
        }

        size_t plus_id = add_binary_operator('+',"PLUS", 4, 6);
        size_t dash_id = add_binary_operator('-',"DASH", 4, 5);
        size_t rangle_id = add_binary_operator('>',"RANGLE", 2, 3);
        size_t langle_id = add_binary_operator('<',"LANGLE", 2, 3);
        size_t bang_id = add_binary_operator('!',"BANG", 2, 3);
        size_t equals_id = add_binary_operator('=', "EQUALS", 1, 1);
        size_t star_id = add_binary_operator('*',"STAR", 5, 7);
        size_t slash_id = add_binary_operator('/',"SLASH", 4, 5);
        size_t caret_id = add_binary_operator('^',"CARET", 8, 4);
        size_t amp_id = add_binary_operator('&',"AMPERSAND", 4, 8);
        size_t dot_id = add_binary_operator('.', "DOT", 8, 9);
        size_t pipe_id = add_binary_operator('|', "PIPE", 9, 8);
        uint32_t qmark_id = add_binary_operator('?',"QMARK",1,3);
        uint32_t property_id = add_binary_operator(':',"COLON",5,6);

        uint32_t  add_binding_token_combo(const std::string& f, int lbp, int rbp, char a, char b, char c = '\0', char d = '\0') {
            uint32_t id = add_token_combo(f,a,b,c,d);
            set_binding_powers(id,lbp,rbp);
            return id;
        }

        uint32_t plus_plus_id = add_binding_token_combo("PLUS_PLUS",2,-1,'+','+');
        uint32_t plus_plus_plus_id = add_binding_token_combo("PLUS_PLUS_PLUS",2,-1,'+','+','+');
        uint32_t plus_equals_plus_id = add_binding_token_combo("PLUS_EQUALS_PLUS",2,-1,'+','=','+');
        uint32_t plus_equals_id = add_binding_token_combo("PLUS_EQUALS",2,3,'+','=');

        uint32_t langle_langle_id = add_binding_token_combo("LANGLE_LANGLE",8,9,'<','<');
        uint32_t rangle_rangle_id = add_binding_token_combo("RANGLE_RANGLE",8,9,'>','>');

        uint32_t equals_equals_id =  add_binding_token_combo("EQUALS_EQUALS",2,3,'=','=');
        uint32_t bang_equals_id =  add_binding_token_combo("BANG_EQUALS",2,3,'!','=');
        uint32_t langle_equals_id =  add_binding_token_combo("LANGLE_EQUALS",2,3,'<','=');
        uint32_t rangle_equals_id =  add_binding_token_combo("RANGLE__EQUALS",2,3,'>','=');
        uint32_t amp_amp_id =  add_binding_token_combo("AMP_AMP",1,1,'&','&');
        uint32_t pipe_pipe_id =  add_binding_token_combo("PIPE_PIPE",1,1,'|','|');

        uint32_t random_combo_id = add_token_combo("RANDOM",'|','*','^','+');

        uint32_t assign_into_id = reg_id("ASSIGN_INTO"); //For function calls

        void init_stage_a() {
            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(comma_id);

            a_handlers.default_function = [this](Context& ctx) {
                int left_bp = left_binding_power.getOrDefault(ctx.node().type(), -1);
                int right_bp = right_binding_power.getOrDefault(ctx.node().type(), -1);

                if(left_bp == -1 && right_bp == -1) return;
                
                if(is_live(ctx.left()) && left_bp > 0 && !discard_types.has(ctx.left().type())) {
                    int left_left_bp = left_binding_power.getOrDefault(ctx.left().type(), -1);
                    int left_right_bp = right_binding_power.getOrDefault(ctx.left().type(), -1);
    
                    bool right_associative = right_bp < left_bp; //lbp > rbp means right assoc
                    bool should_steal = left_bp > (right_associative ? left_right_bp : left_left_bp);

                    //LOG_W(ctx,"ON "+ctx.node().name().to_std());
                    
                    if(!ctx.left().children().empty()) {
                        if(ctx.left().children().length()==1) {
                            should_steal = true;
                        }
                        else if(discard_types.has(ctx.left().children().last().type())) {
                            goto otter;
                        }
                    }
    
                    //LOG_W(ctx,"SHOULD STEAL "+std::to_string((int)should_steal));
                    if(left_right_bp!=-1 && should_steal) {
                        if(ctx.left().children().length()>1) {
                            //LOG_W(ctx,"TAKING: "+node_info(ctx.left().children().last()));
                            ctx.node().children() << ctx.left().children().pop();
                            //LOG_W(ctx,"TOOK: "+node_info(ctx.node().children().last()));
                            if(ERROR_FLAG) return;
                        }
                        ctx.left().children() << ctx.result().take(ctx.index());
                    } else {
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

                //LOG_W(ctx,"MIDDLE "+ctx.node().name().to_std());
                
                if(right_bp != -1 && ctx.index() < ctx.result().length()) {
                    Node next = ctx.result().get(ctx.index());
                    int next_lbp = left_binding_power.getOrDefault(next.type(), -1);
                    if(next_lbp == -1 && !discard_types.has(next.type())) { //It's an atom so we grab it
                        ctx.node().children() << ctx.result().take(ctx.index());
                    } 
                }
                ctx.index()--;

                //LOG_W(ctx,"AFTER "+ctx.node().name().to_std());
            };
    
            for(int m = 0; m<2; m++) {
                uint32_t open_id = m==0?rparen_id:rbracket_id;
                uint32_t close_id = m==0?lparen_id:lbracket_id;

                left_binding_power.put(close_id,10);
    
                a_handlers[open_id] = [this,close_id](Context& ctx) {
                    ctx.result().removeAt(ctx.index());
                    int i = ctx.index()-1;
                    list<Node> gathered;
                    while(i>=0) {
                        Node on = ctx.result().get(i);
                        Node was_on = on; //Storing the root for cases where we want to notify once children are gathered
                        while(!on.children().empty()&&on.type()!=close_id) {
                            on = on.children().last();
                        }
                        if(on.type()==close_id) {
                            gathered.reverse();
                            bool was_given_children = false;
                            if(on.children().empty()) {
                                for(auto g : gathered)
                                    on.children() << g;
                                was_given_children = true;
                            }
                            // g_ptr<Node> token_on = copy_as_token(on);s

                            Node token_on = copy_as_token(on);

                            if(!on.children().empty())
                                on.copy(on.children().take(0));

                            on.quals() << token_on; //Copy the lparen
                            on.quals() << turn_into_token(ctx.node()); //Copy the rparen

                            if(!was_given_children) {
                                if(on.children().empty()) {
                                    for(auto g : gathered)
                                        on.children() << g;
                                } else { //This case if for things like int main(int a), where we want the gathered to go under main, not int
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
            value_printers[object_id] = [this](Context& ctx) {ctx.source(Ptr_as_string(ctx.value().data_ptr()));};
            value_printers[ptr_id] = [this](Context& ctx) {Ptr p = *(Ptr*)ctx.value().get(); ctx.source(Ptr_to_string(p,p.cachelevel));};
            value_printers[float_id] = [](Context& ctx) {ctx.source(std::to_string(*(float*)ctx.value().get()));};
            value_printers[int_id] = [](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source(std::to_string(*(int*)p));};
            value_printers[char_id] = [](Context& ctx) {ctx.source(std::string(1,*(char*)ctx.value().get()));};
            value_printers[bool_id] = [](Context& ctx) {ctx.source((*(bool*)ctx.value().get()) ? "TRUE" : "FALSE");};
            value_printers[string_id] = [this](Context& ctx) {void* p = ctx.value().get(); DEBUG_ONLY(if(ERROR_FLAG) {return;}) ctx.source() = *(string*)p;};
            value_printers[node_id] = [this](Context& ctx) {ctx.source(node_to_string((Node&)(*(Ptr*)ctx.value().get())));};
            value_printers[value_id] = [this](Context& ctx) {ctx.source(value_info((Value&)(*(Ptr*)ctx.value().get())));};
            value_printers[context_id] = [this](Context& ctx) {Context context = (Context&)(*(Ptr*)ctx.value().get()); std::string src = "Source ptr of "+Ptr_as_string(context)+": "+Ptr_as_string(context.source_ptr()); ctx.source(src);};
                
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
                    decl_value.quals() << value_to_qual(node.value(),node.name().to_std(),node.x(),node.y());
                }
                for(int i = 0; i < node.children().length(); i++) {
                    Node c = node.children()[i];
                    find_value_in_scope(c); //Process forward and consume other qualifers
                    if(c.type()!=identifier_id) {break;}

                    if(is_live(c.value())&&c.value().type()!=0) {
                        decl_value.quals() << value_to_qual(c.value(),c.name().to_std(),c.x(),c.y());
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
                        if(is_live(c.value())&&c.value().type()!=0) {
                            node.quals() << value_to_qual(c.value(),c.name().to_std(),c.x(),c.y());
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
            bool has_type_scope = is_live(node.value().type_scope());
            bool has_sub_type = node.value().sub_type() != 0;
            
            if(has_scope) {
                node.scopes()[0].owner(node);
                node.scopes()[0].name(node.name().to_std());
                if(has_sub_type) {
                    node.type(func_decl_id);
                    node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0],node.count_qual(hoisted_id));
                    node.value().type_scope(node.scopes()[0]);
                    node.value().sub_type(0);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value(),node.count_qual(hoisted_id)));
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
                } else {
                    node.type(type_decl_id);
                    node.value(make_type_value(node.name().to_std(),0));
                    node.value().type_scope(node.scopes()[0]);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value(), node.count_qual(hoisted_id)));
                    node.scopes()[0].type(type_scope_id);
                    add_template(node.value().type());
                    r_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(ctx.value().size()==0&&layouts.hasKey(ctx.value().type())) {
                            ctx.value().size(layouts.get(ctx.value().type()).total_size);
                        }
                    };
                    x_handlers[to_prefix_id(node.value().type())] = [this](Context& ctx){
                        if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                            ctx.value().data_col().push_default();
                            ctx.value().data_col().heterogenous = true;
                        }
                    }; 
                }
            } else {
                has_scope = find_node_in_scope(node); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node.type(var_decl_id);
                    if(node.in_scope().type()==type_scope_id) {
                        node.in_scope().value_table().put(node.name().to_std(), decl_value); //So we don't distribute into function bodies, we need to alias later via this, as it's per instance
                        layouts[node.in_scope().owner().value().type()].add_prop(node.value().type(),node.value().size(),node.name().to_std(),0,0,decl_value);
                    } else {
                        node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value, node.count_qual(hoisted_id)));
                    }
                    node.value().sub_type(0);
                } else if(has_scope) {
                    node.type(func_call_id);
                    find_value_in_scope(node); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
                    if(is_live(node.value().type_scope()))
                        node.scopes()[0] = node.value().type_scope(); //Swap to the type scope
                    // if(!node->children.empty()) {
                    //     node->name.append("(");
                    //     for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
                    // }
                } else if(found_a_value) { //If we already had a value and nothing interesting happened to us, reclaim it
                    find_value_in_scope(node);
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
                        //No clue what this could be
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


        void overload_type(uint32_t type, const std::string& instr, uint32_t overload_to, Value value = deadptr) {
            if(!layouts.hasKey(type)) {
                layouts.put(type,_layout(add_template(type)));
            }
            Node expr = tokenize(instr);
            Stage* old_stage = active_stage;
            start_stage(a_handlers);
            standard_direct_pass(expr);
            a_pass_resolve_keywords(expr.children());
            start_stage(old_stage);

            //print(node_to_string(expr));

            uint32_t root_type = 0; 
            uint32_t right_type = 0;
            if(!expr.children().empty()) {
                Node op = expr.children()[0];
                root_type = op.type();
                if(op.name().length()==1&&instr.length()>1&&instr.find(op.name().to_std())==0) {
                    root_type-=2; //Convert to normal version if it's on the left side, so +string parses as plus_unary, but is actually just normal plus
                    //Only single char ops can be unary form so token combos don't need this
                }

                if(!op.children().empty()) {
                    if(!is_live(op.children()[0].value())) {
                        right_type = op.children()[0].type();
                        if(right_type==string_id) {
                            right_type = hashString(op.children()[0].name().to_std());
                        }
                    } else {
                        right_type = op.children()[0].value().type();
                    }
                }
            }
            layouts.get(type).add_overload(make_overload_key(root_type,right_type),overload_to,value);
            recycle_node(expr);
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

        void what_I_see(Context& ctx) {
            print(bold_str(ctx.node().name().to_std()),": I see my root is ",green(ctx.root().name().to_std()),", my value type is ",blue(labels[ctx.node().value().type()])," and to my left is ",yellow(is_live(ctx.left())?ctx.left().name().to_std():"nothing"));
        }

        void resolve_overload(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to resolve overloads while another error was flagged")); return;})
            if(!is_node_opperator(ctx.root())) return;
            //LOG_W(ctx," resolving overloads");
            // print("RESOLVING: ",node_to_string(ctx.node()));
            standard_sub_process(ctx); //Consider not doing this
            if(ctx.index()==0&&is_live(ctx.node().value())) { //If we're the left term
                if(layouts.hasKey(ctx.node().value().type())) {
                    _layout& l = layouts[ctx.node().value().type()];
                    uint32_t right_type = 0;
                    bool has_overload = false;
                    uint64_t typekey = 0;
                    if(ctx.result().length()>1) {
                        ctx.index() = 1; //Because process node will just blindly carry index
                        process_node(ctx,ctx.result().get(1));
                        ctx.index() = 0;
                        if(is_live(ctx.result().get(1).value())) {
                            //print("Deriving value from right_type");
                            right_type = ctx.result().get(1).value().type();
                        } else {
                            log(yellow("resolve_overload: right term has a dead value: "),node_info(ctx.result().get(1)));
                            //print(span->on_line->parent->to_string());
                        }
                    } else {
                        typekey = make_overload_key(ctx.root().type(),0);
                        has_overload = l.has_overload(typekey);
                    }

                    if(right_type!=0) {
                        typekey = make_overload_key(ctx.root().type(),right_type);
                        has_overload = l.has_overload(typekey);
                        if(!has_overload) {
                            //print("False overload, checking any");
                            typekey = make_overload_key(ctx.root().type(),any_id);
                            has_overload = l.has_overload(typekey);
                        }
                    } else if(!has_overload) {
                        if(ctx.result().length()>1) {
                            right_type = hashString(ctx.result().get(1).name().to_std());
                            //print("Overloading name: ",ctx.result().get(1).name().to_std());
                        }
                        if(right_type!=0) {
                            typekey = make_overload_key(ctx.root().type(),right_type);
                            has_overload = l.has_overload(typekey);
                        }
                    }
                    //print("Has overload: ",has_overload?"Yes":"No");
                    if(has_overload) {
                        type_and_value tnv = l.get_overload(typekey);
                        ctx.root().type(tnv.type);
                        if(is_live(tnv.value)) {
                            Value& value = (Value&)tnv.value;
                            if((value.type()!=0)) {
                                ctx.root().value(make_value(value.type(),value.size(),value.address(),value.sub_type(),value.sub_size(),value.type_scope()));
                            } else {
                                if(ctx.node().value().sub_type()==0) {
                                    fire_quals(ctx,ctx.node().value());
                                } 

                                ctx.root().value(make_value(
                                    ctx.node().value().sub_type(),
                                    ctx.node().value().sub_size()
                                ));
                                if(ctx.node().value().quals().length()>1) {
                                    for(int i=1;i<ctx.node().value().quals().length();i++) {
                                        ctx.root().value().quals() << ctx.node().value().quals()[i];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        uint32_t static_qual = add_qual("static");

        void sync_args(Context& ctx) {
            if(!ctx.node().scopes().empty()) {
                DEBUG_ONLY(if(ctx.node().children().length()!=ctx.node().scopes()[0].owner().children().length()) {throw_error("Wrong number of arguments for function: ",node_to_string(ctx.node())); return;})
                for(int i = 0; i < ctx.node().children().length(); i++) {
                    Node arg = ctx.node().children()[i];
                    if(arg.type()==equals_id) continue;
                    Node param = ctx.node().scopes()[0].owner().children()[i];
                    Node assignment = make_node(equals_id);
                    assignment.children().push(param);
                    assignment.children().push(arg);
                    ctx.node().children().col().set(i,(void*)&assignment);
                }
            }
        }

        void gather_all_values_in_scope(value_col& subvals, Node scope) {
            for(int i=0;i<scope.children().length();i++) {
                Node c = scope.children()[i];
                if(is_live(c.value())) {
                    for(int n=0;n<subvals.length();n++) {
                        if(subvals.get(n).idx==c.value().idx) goto skipatom;
                    }
                    subvals.push(c.value());
                }
                skipatom:
                gather_all_values_in_scope(subvals,c);
            }
            for(int s=0;s<scope.scopes().length();s++) {
                Node subscope = scope.scopes()[s];
                if(!is_live(subscope.owner())||subscope.owner().idx==scope.idx) {
                    gather_all_values_in_scope(subvals,subscope);
                }
            }
        }

        //Add another row to each data column for function calls
        int descend_call_scope(Context& ctx, Node scope) {
            Value sv = scope.value();
            int loc = sv.loc()+1;
            sv.loc(loc);
            value_col subvals = sv.sub_values();
            if(subvals.empty()) {gather_all_values_in_scope(subvals,scope);}
            for(int i=0;i<subvals.length();i++) {
                Value sval = subvals.get(i); //Static values just stay where they are, they don't descend and ascend
                if(is_live(sval.data_ptr())&&!sval.has_qual(static_qual)) { //This ceremony is becuse if we just did col.push(col.get(0) it would invalidate the column as we push thus breaking the get, so we have to save as temps
                    Ptr dataptr = sval.data_ptr();
                    if(dataptr.pool!=data_store_id) continue;
                    uint32_t elem_size = resolve_to_col(dataptr).element_size;
                    list<uint8_t> temp(elem_size);
                    if(resolve_to_col(dataptr).empty()) {
                        if(resolve_to_col(dataptr).element_size == 0) continue; //Skip uninitialized cols
                        resolve_to_col(dataptr).push_default();
                    }
                    memcpy(temp.data(), resolve_to_col(dataptr).get((uint32_t)0), elem_size);
                    if(resolve_to_col(dataptr).length() <= loc) {
                        //These shouldn't be getting out of sync in the first place, in the future investigate this deeper
                        int depth_check = 0;
                        //This could be due to us trying this on other func calls, perhaps replace loc with the return of the data ptr to make this work?
                        while(resolve_to_col(dataptr).length() <= loc && depth_check++ < 100) {
                            resolve_to_col(dataptr).push(temp.data());
                        }
                        if(depth_check>=90) {
                            print(red("Infinite loop in loc catchup on "+Ptr_as_string(dataptr)+": this shouldn't even be happening in the first place!"));
                        }
                    } else {
                        resolve_to_col(dataptr).set(loc, temp.data());
                    }
                    dataptr.sidx = loc;
                    resolve_to_col(sval).qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
                } else {
                    log(yellow(Ptr_as_string(sval)+" is not live, and can not be descended"));
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
                if(is_live(sval.data_ptr())&&!sval.has_qual(static_qual)) {
                    Ptr newptr = sval.data_ptr();
                    if(newptr.pool!=data_store_id) continue;
                    newptr.sidx = loc;
                    resolve_to_col(sval).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
                }
            }
        }

        void desync_args(Node root) {
            if(is_live(root.value().data_ptr())) {
                Ptr newptr = root.value().data_ptr();
                newptr.sidx = newptr.sidx-1;
                resolve_to_col(root.value()).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
            }
            for(int i=0;i<root.children().length();i++) {
                desync_args(root.children()[i]);
            }
        }
        void resync_args(Node root) {
            if(is_live(root.value().data_ptr())) {
                Ptr newptr = root.value().data_ptr();
                newptr.sidx = newptr.sidx+1;
                resolve_to_col(root.value()).qset(value_data_offset,(void*)&newptr,sizeof(Ptr));
            }
            for(int i=0;i<root.children().length();i++) {
                resync_args(root.children()[i]);
            }
        }

        void call_func(Context& ctx, Node scope) {
            list<list<uint8_t>> temps;
            for(int i=0;i<ctx.node().children().length();i++) {
                Node rightterm = ctx.node().children()[i].children()[1];
                process_node(ctx, rightterm);
                Value rv = rightterm.value();
                list<uint8_t> snap; snap.resize(rv.size());
                memcpy(snap.data(), rv.get(), rv.size());
                temps << snap;
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("ABORTING FUNCTION CALL BEFORE DESCENT")); return;})
            //print("RUNNING: ",node_to_string(ctx.node()));
            int stack_depth = descend_call_scope(ctx,scope);
            DEBUG_ONLY(if(stack_depth>500) {throw_error("Stack overflow on function call: ",node_info(ctx.node())); return;})
            for(int i=0;i<ctx.node().children().length();i++) {
                Node leftterm = ctx.node().children()[i].children()[0];
                leftterm.value().set(temps[i].data());
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("ABORTING FUNCTION CALL BEFORE PASS")); return;})
            if(!standard_travel_pass(scope,ctx.sub())) { //If the return didn't already ascend
                ascend_call_scope(scope);
            }
        }


        Node instantiate_template_scope(Node call, Node decl, Context& ctx, bool args_already_synced = false) {
            Node new_scope = make_node(decl.scopes()[0].type(), 0, decl.name().to_std());

            if(is_live(decl.scopes()[0].value())) {
                new_scope.value(make_value());
                new_scope.value().copy(decl.scopes()[0].value(), true);
            }
            new_scope.owner(call);
        
            for(int i = 0; i < decl.scopes()[0].quals().length(); i++) {
                new_scope.quals() << decl.scopes()[0].quals()[i];
            }
        
            map<uint32_t, Value> value_alias_table;
            map<uint32_t, Node> node_alias_table;
        
            Stage* oldstage = active_stage;
            start_stage(x_handlers); //Because we're trying to derive the value, this may not be the right long term solution though
            //This was a bit of an accident born from how things were working in Webcorn's standard_gather_from_scope
            //And an anomaly with FUNC_DECLs revelead when trying to make templating work
            if(args_already_synced) { //Because they've already been turned into the assignment form by sync_args
                for(int i = 0; i < call.children().length(); i++) {
                    Node c = call.children()[i];
                    process_node(ctx, c.children()[1]);
                    value_alias_table.put(c.children()[0].value().idx, c.children()[1].value());
                }
            } else {
                for(int i = 0; i < call.children().length(); i++) {
                    process_node(ctx, call.children()[i]);
                    if(i < decl.children().length()) {
                        value_alias_table.put(decl.children()[i].value().idx, call.children()[i].value());
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
                    resolve_overload(ctx);
                };
            }
            x_handlers[id] = x_handler;
            return id;
        }

        uint32_t print_id = add_function("print",[this](Context& ctx){ 
            std::string to_print = "";
            for(int i=0;i<ctx.node().children().length();i++) {
                Node c = ctx.node().children()[i];
                process_node(ctx,c);
                to_print += value_as_string(c.value());
            }
            print(to_print);
        });
        uint32_t return_id = make_tokenized_keyword("return");

        uint32_t true_id = add_function("true",[this](Context& ctx){
            bool t = true;
            ctx.node().value().set((void*)&t);
        },1,bool_id);
        uint32_t false_id = add_function("false",[this](Context& ctx){
            bool f = false;
            ctx.node().value().set((void*)&f);
        },1,bool_id);

        void init() override {
            init_literals();
            init_tokenizer();
            init_stage_a();
            init_stage_s();

            register_type("int",int_id,4);
            register_type("float",float_id,4);
            register_type("bool",bool_id,1);
            register_type("string",string_id,sizeof(Ptr));
            register_type("Node",node_id,sizeof(Ptr));
            register_type("Value",value_id,sizeof(Ptr));
            register_type("Context",context_id,sizeof(Ptr));
            register_type("Ptr",ptr_id,sizeof(Ptr));

            set_binding_powers(random_combo_id,8,9);

            t_handlers[identifier_id] = [this](Context& ctx){resolve_identifier(ctx);};
            t_handlers[equals_id] = [this](Context& ctx){standard_sub_process(ctx);};

            t_handlers.default_function = [this](Context& ctx){if(ctx.node().scopes().empty()) {standard_sub_process(ctx);}}; //Because resolving passes will already cover the sub process for scoped nodes
            r_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            x_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};

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
            r_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                resolve_overload(ctx);
                fire_quals(ctx,ctx.node().value());
                sync_args(ctx);
                //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
            };
            x_handlers[func_call_id] = [this](Context& ctx) {
                if(ctx.node().scopes().length()==1) {
                    Node scope = ctx.node().scopes()[0];
                    call_func(ctx,scope);
                } else if(ctx.node().scopes().length()>1) {
                    for(int i=1;i<ctx.node().scopes().length();i++) {
                        standard_travel_pass(ctx.node().scopes()[i],ctx.sub());
                    }
                }
            };

            t_handlers[return_id] = [this](Context& ctx){
                if(ctx.index()+1<ctx.result().length()) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                }
                standard_sub_process(ctx);
            };
            r_handlers[return_id] = [this](Context& ctx){
                Node climb = ctx.node();
                while(is_live(climb)&&climb.type()!=func_decl_id) { //WARNING: we need to make sure things like in blocks which also use returns are safe with this! 
                    //This might try to bind to some random function via climbing when it does this, so when metaprogramming becomes visible in the compielr, add gaurds.
                    climb = climb.in_scope().owner();
                }
                ctx.node().parent(climb);
                if(is_live(ctx.node().parent())) {
                    ctx.node().value(ctx.node().parent().value());
                }
                standard_sub_process(ctx);
            };  
            x_handlers[return_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(is_live(ctx.node().parent())) {
                    if(!ctx.node().children().empty()) {
                        void* snap = ctx.node().children()[0].value().get();
                        ascend_call_scope(ctx.node().parent().scopes()[0]);
                        ctx.node().value().set(snap);
                    } else {
                        ascend_call_scope(ctx.node().parent().scopes()[0]);
                    }
                }
                ctx.state(1);
                return;
            };

            r_handlers[equals_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
            };
            x_handlers[equals_id] = [this](Context& ctx){
                if(ctx.node().children().length()==2) {
                    backwards_sub_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute equals while another error was flagged")); return;})
                    Node left = ctx.node().children()[0];
                    Node right = ctx.node().children()[1];

                    Ptr lp = left.value().data_ptr();
                    Ptr rp = right.value().data_ptr();

                    DEBUG_ONLY(if(!is_live(lp)) {throw_error("left term of equals is invalid"); return;})
                    DEBUG_ONLY(if(!is_live(rp)) {throw_error("right term of equals is invalid"); return;})
                    DEBUG_ONLY(if(left.value().size()!=right.value().size()) {throw_error("Mismatched sizes for assignment from:\n",node_to_string(ctx.node())); return;})
                    
                    //Col& lcol = resolve_to_col(lp);
                    if(resolve_to_col(lp).heterogenous) {
                        resolve_to_col(lp).qset(lp.sidx,resolve_ptr(rp),right.value().size());
                    } else {
                        resolve_to_col(lp).set(lp.sidx,resolve_ptr(rp));
                    }
                }
            };

            make_tokenized_keyword("any",any_id);
            make_tokenized_keyword("null",null_id);

            // add_gather_token('#', hash_id, identifier_id, identifier_id); //REMEMBER TO FIX ## AS WELL LATER!

            r_handlers[identifier_id] = [this](Context& ctx){
                resolve_overload(ctx);
            };
            r_handlers[literal_id] = r_handlers[identifier_id];

            r_handlers[dot_id] = [this](Context& ctx){
                if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(ctx.node().type()==dot_id) {
                    Node left = ctx.node().children()[0];
                    Node right = ctx.node().children()[1];
                    uint32_t ltype = left.value().type();
                    if(layouts.hasKey(ltype)) {
                        _layout& layout = layouts.get(ltype);
                        std::string prop = right.name().to_std();
                        if(layout.label_to_index.hasKey(prop)) {
                            uint32_t index = layout.label_to_index.get(prop);
                            if(is_live(layout.ptrs[index])) { //If we were handed a full value just copy that over (why not just always use this though... mark for later)
                                ctx.node().value(make_value()); ctx.node().value().copy(layout.ptrs[index],true);
                            } else {
                                ctx.node().value(make_value(layout.tags[index], layout.sizes[index], layout.offsets[index], layout.subtags[index], layout.subsizes[index]));
                            }
                        } else {
                            print(red("r_handlers::dot_id layout of "+labels[ltype]+" does not have prop "+prop));
                            // print(red("root is: "+labels[ctx.root().type()])); 
                        }
                        //right.value(ctx.node().value());
                    } else {
                        print(red("r_handlers::dot_id no layout found for type "+labels[ltype]));
                    }

                    //This is mean to be for inline get syntax like children(0), probably going to be replaced with a proper overload in the future
                    if(right.type()==identifier_id&&!right.children().empty()) { //Can replace with QValue in the future for an optimization
                        Value value = ctx.node().value();
                        value.type(value.sub_type()); value.sub_type(0);
                        value.size(value.sub_size()); value.sub_size(0);
                        //right.type(temp_get_id);
                    }

                    resolve_overload(ctx); //Going around a second time
                } else if(ctx.node().type()==method_call_id) { //Turn into a function call
                    ctx.node().type(func_call_id);
                    Node amp = make_node(to_unary_id(amp_id));
                    amp.value(make_value(ptr_id,sizeof(Ptr)));
                    Node match_this = make_node(identifier_id,"match_this",ctx.node().children()[0].value(),ctx.node().in_scope());
                    amp.children().push(match_this);
                    process_node(ctx,amp); //Resolve this
                    node_col args = ctx.node().children()[1].children();
                    ctx.node().children(args);
                    ctx.node().children().insert(0,amp);
                    ctx.node().scopes().push(ctx.node().value().type_scope());
                    sync_args(ctx);
                    ctx.node().value(ctx.node().value().type_scope().owner().value());
                }
            };
            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Value value = ctx.node().value();
                if(right.type()==identifier_id) {
                    Ptr ptr = deadptr;
                    uint32_t rvt = left.value().type();
                    if(rvt==ptr_id||rvt==node_id||rvt==value_id||rvt==context_id||rvt==string_id) {
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
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '"') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        case '"':  ctx.node().name().push('"');  break;
                        case '\\': ctx.node().name().push('\\'); break;
                        default:   ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
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
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '\'') {
                    ctx.state(0);
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;
                } else if(c == '\\' && ctx.index()+1<ctx.source().length()) {
                    char next = ctx.source().at(ctx.index() + 1);
                    switch(next) {
                        case 'n':  ctx.node().name().push('\n'); break;
                        case 't':  ctx.node().name().push('\t'); break;
                        case 'r':  ctx.node().name().push('\r'); break;
                        case '"':  ctx.node().name().push('"');  break;
                        case '\'':  ctx.node().name().push('\'');  break;
                        case '\\': ctx.node().name().push('\\'); break;
                        default:   ctx.node().name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index()++;
                } else if(c == '\n') {
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
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
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
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                int result =      
                    *(int*)p1
                    >
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
                standard_sub_process(ctx);
                void* p1 = ctx.node().children()[0].value().get();
                void* p2 = ctx.node().children()[1].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
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
                standard_sub_process(ctx);
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
                standard_sub_process(ctx);
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
                standard_sub_process(ctx);
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

            x_handlers[make_tokenized_keyword("root_name")] = [this](Context& ctx){
                if(ctx.node().children().empty()) {
                    ctx.node().value(make_value(string_id,sizeof(Ptr)));
                    ctx.node().value().set((void*)&ctx.root().name_ptr());
                } else {
                    ctx.root().name() = ctx.node().children()[0].name();
                }
            };
        }
    };
}