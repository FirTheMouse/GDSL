#pragma once
#include "../mixos-acorn/Acorn-Core.hpp"

namespace Acorn {
    struct Compiler_Unit : public virtual Unit {
        Compiler_Unit() {init();}
        

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
                                node.value().copy(v);
                                node.value().reg(-1); //Reset to -1 for cleanliness
                            }
                        }
                    }
                }
                if(is_live(node.value())) {
                    context =  node.value().sub_type();
                }

                a_pass_resolve_keywords(node.children(), context);

                for(int s = 0;s<node.scopes().length();s++) {
                    a_pass_resolve_keywords(node.scopes()[s].children(), context);
                }
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

        void add_type_stamping_handler(uint32_t type) {
            t_handlers[to_prefix_id(type)] = [](Context& ctx){
                if(ctx.value.sub_type() == 0) {
                    ctx.value.sub_type(ctx.qual.sub_type());
                    ctx.value.type(ctx.qual.type());
                    ctx.value.size(ctx.qual.value().size());
                    if(is_live(ctx.qual.value().type_scope()))
                        ctx.value.type_scope(ctx.qual.value().type_scope());
                }
            };
        }

        Value make_type(const std::string& f, size_t size = 0) {
            Value val = make_qual_value(f,size);
            add_type_stamping_handler(val.type());
            return  val;
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

        uint32_t add_function(const std::string& f, uint32_t return_type = 0) {
            Value val = register_value(f,0,return_type);
            keywords.put(f,val);
            return val.sub_type();
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
            char a = ctx.source.at(ctx.index);
            if(!token_combos.hasKey(a)) return 0;

            char c[4] = {a,'\0','\0','\0'};
            for(int i=1;i<4;i++) { //To safely fill without outrunning the length of source
                if(ctx.index+i < ctx.source.length()) {
                    c[i] = ctx.source.at(ctx.index+i);
                }
            }

            uint32_t zero = 0;
            map<uint32_t,uint32_t>& combos = token_combos.get(a);
            for(int i=3;i>0;i--) {
                uint32_t key = combine_tokens(c[0],c[1],c[2],c[3]);
                uint32_t result = combos.getOrDefault(key,zero);
                if(result!=0) {
                    ctx.node.type(result);
                    return i;
                }
                c[i] = '\0';
            }
            return 0;
        }

        size_t add_token(char c, const std::string& f) {
            size_t id = reg_id(f);
            tokenizer_functions[c] = [this,id,c](Context& ctx) {
                ctx.node = make_node();
                int to_skip = find_token_combo(ctx);
                if(to_skip!=0) {
                    ctx.node.name().push(c);
                    for(int i=0;i<to_skip;i++) {
                        ctx.index++;
                        ctx.node.name().push(ctx.source.at(ctx.index));
                    }
                } else {
                    ctx.node.type(id);
                    ctx.node.name().push(c);
                }
                ctx.result.push(ctx.node);
            };
            char_is_split.put(c, true);
            return id;
        }


        list<size_t> discard_types;

        size_t to_decl_id(size_t id) {return id+1;}
        size_t to_unary_id(size_t id) {return id+2;}
        
        size_t colon_id = add_token(':',"COLON");
        size_t lparen_id = add_token('(',"LPAREN");
        size_t rparen_id = add_token(')',"RPAREN");
        size_t comma_id = add_token(',',"COMMA");
        size_t lbracket_id = add_token('[', "LBRACKET");
        size_t rbracket_id = add_token(']', "RBRACKET");
        size_t lbrace_id = add_token('{', "LBRACE");
        size_t rbrace_id = add_token('}', "RBRACE");
        size_t slash_id = add_token('/',"SLASH");
        size_t hash_id = add_token('#',"HASH");

        size_t in_alpha_id = reg_id("IN_ALPHA");
        size_t in_digit_id = reg_id("IN_DIGIT");
        size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return
        size_t quote_id = reg_id("QUOTE");
        size_t comment_id = reg_id("COMMENT");
        size_t single_quote_id = reg_id("SINGLE_QUOTE");

        void add_gather_token(char open, size_t open_id, size_t state_id, size_t type_id) {
            tokenizer_functions[open] = [this, open, open_id, state_id,type_id](Context& ctx) {
                ctx.state = state_id;
                ctx.node = make_node();
                ctx.node.type(type_id);
                ctx.result.push(ctx.node);
            };
        
            tokenizer_state_functions[state_id] = [this, open, open_id](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(c == open) {
                    ctx.state = 0;
                } else if(c == '\\') {
                    char next = ctx.source.at(ctx.index + 1);
                    switch(next) {
                        case 'n':  ctx.node.name().push('\n'); break;
                        case 't':  ctx.node.name().push('\t'); break;
                        case 'r':  ctx.node.name().push('\r'); break;
                        case '"':  ctx.node.name().push('"');  break;
                        case '\\': ctx.node.name().push('\\'); break;
                        default:   ctx.node.name().push(next); break;
                    }
                    at_x += 1.0f;
                    ctx.index++;
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                    // ctx.node.name().push(c);
                } else {
                    ctx.node.name().push(c);
                }
            };
        }


        void add_double_string_token(char open, char open2, size_t open_id, size_t open_id2, size_t state_id, size_t type_id, bool break_on_newline = false, map<std::string,Handler>* parts = nullptr) {      
            Handler open_func = [this, open, open2, open_id, open_id2, state_id, type_id](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                if(c==open2) {
                    ctx.node = make_node(type_id);
                    ctx.result.push(ctx.node);
                    at_x+=1.0f;
                    ctx.index++;
                    ctx.state = state_id;
                }
            };
            Handler open_else_func = [this, open, open_id, state_id](Context& ctx) {
                char c = ctx.source.at(ctx.index+1);
                ctx.node = make_node(open_id);
                ctx.node.name().push(c);
                ctx.result.push(ctx.node);
            };
            Handler full_package_func = [this, state_id, open_func, open_else_func](Context& ctx) {
                open_func(ctx);
                if(ctx.state!=state_id)
                    open_else_func(ctx);
            };
            tokenizer_functions[open] = full_package_func;
            if(parts) {
                parts->put("if",open_func);
                parts->put("else",open_else_func);
                parts->put("full",full_package_func);
            }
        
            Handler state_func = [this, open, open2, open_id, open_id2, state_id, type_id, break_on_newline](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(c == open&&ctx.source.at(ctx.index+1)==open2) {
                    ctx.state=0;
                    ctx.index++;
                } else if(c == '\\') {
                    if(ctx.index + 1 < ctx.source.length()) {
                        ctx.node.name().push(ctx.source.at(ctx.index + 1));
                        at_x += 1.0f;
                        ctx.index++;
                    }
                } else if(c=='\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                    ctx.node.name().push(c);
                    if(break_on_newline)
                        ctx.state = 0;
                }
                else {
                    ctx.node.name().push(c);
                }
            };
            tokenizer_state_functions.put(state_id,state_func);
        }

        Node tokenize(const std::string& code) {
            Node root = make_node();
            root.name("ROOT");
            node_col result = root.children();
            uint32_t state = 0;
            int index = 0;
            at_x = 0.0f;
            at_y = 0.0f;
            Context ctx(result,index);
            ctx.source = code;
            ctx.root = root;

            #if PRINT_ALL
            newline("tokenize pass");
            #endif

            if(!tokenizer_default_function) {
                print("GDSL::tokenize warning! No defined default function, please define one");
            }

            while (index<code.length()) {
                char c = code.at(index);
                Handler* func = nullptr;
                if(ctx.state!=0&&tokenizer_state_functions.hasKey(ctx.state)) {
                    func = &tokenizer_state_functions.get(ctx.state);
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

            return root;
        }

        void init_tokenizer() {
            // Literals_Unit::init();
            char_is_split.put(' ',true);
            tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(char_is_split.getOrDefault(c,false)) {
                    ctx.state = 0; 
                    at_x-=1.0f;
                    --ctx.index;
                    ctx.node.type(tokenized_keywords.getOrDefault(ctx.node.name().to_std(),ctx.node.type()));
                    return;
                } else {
                    ctx.node.name().push(c);
                    if(ctx.index+1==ctx.source.length()) {
                        ctx.state = 0; 
                        ctx.node.type(tokenized_keywords.getOrDefault(ctx.node.name().to_std(),ctx.node.type()));
                    }
                }
            });
    
            tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(char_is_split.getOrDefault(c,false)) {
                    if(c=='.') {
                        ctx.node.type(float_id);
                    } else {
                        ctx.state = 0; 
                        at_x-=1.0f;
                        --ctx.index;
                        return;
                    }
                } else if(std::isalpha(c)) {
                    ctx.state = in_alpha_id;
                }
                ctx.node.name().push(c);
            });

            add_gather_token('"', quote_id, reg_id("IN_STRING"), string_id);
            // add_string_token('\'', single_quote_id, reg_id("IN_SHORT_STRING"));
            map<std::string,Handler> slash_parts;
            add_double_string_token('/', '/', slash_id, slash_id, reg_id("IN_COMMENT"), comment_id, true, &slash_parts);

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
                char c = ctx.source.at(ctx.index);
                if(std::isalpha(c)) {
                    ctx.state = in_alpha_id;
                    ctx.node = make_node();
                    ctx.node.type(identifier_id);
                    ctx.node.name().push(c);
                    ctx.result.push(ctx.node);
                }
                else if(std::isdigit(c)) {
                    ctx.state = in_digit_id;
                    ctx.node = make_node();
                    ctx.node.type(int_id);
                    ctx.node.name().push(c);
                    ctx.result.push(ctx.node);
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
            return false;
        }


        bool find_node_in_scope(Node node) {
            if(node.in_scope().node_table().hasKey(node.name().to_std())) {
                if(node.scopes().length()>0) node.scopes().clear();
                node.scopes().push(node.in_scope().node_table().get(node.name().to_std()));
                return true;
            }
            return false;
        }

        Value distribute_value(Node node, const std::string& label, Value val) {
            if(node.value_table().hasKey(label)) {
                Value table_value = node.value_table().get(label);
                if(table_value.type() == 0) {
                    table_value.copy(val);
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
                            val = distribute_value(scope,label,val);
                        }
                    }
                }
            }
            return val;
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

        Node distribute_node(Node node, const std::string& label, Node carry) {
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
                            carry = distribute_node(scope,label,carry);
                        }
                    }
                }
            }
            return carry;
        }

        Node value_to_qual(Value val) {
            Node to_return = make_node(val.type(),val.sub_type(),"",0.0f,0.0f,0.0f,val);
            return to_return;
        }


        map<char,bool> registered_opperators;
        size_t add_binary_operator(char c, const std::string& f, int use_id = -1) {
            size_t id = use_id;
            if(id==-1) {
                id = add_token(c,f);
            }
            registered_opperators[c] = true;
            size_t decl_id = reg_id(f+"_decl");
            size_t unary_id = reg_id(f+"_unary");

            Handler handler = [this,decl_id,unary_id,c](Context& ctx){
                node_col children = ctx.node.children();
                standard_sub_process(ctx); //This causes us to double distribute because if the left term becomes a var decl from a user defined type it distirbutes itself, we don't overwritte though so its just wasted compute, not a bug
                if(children.length() == 2) {
                    Node type_term = children[0];
                    Node id_term = children[1];

                    ctx.node.name(type_term.name().to_std()+c+id_term.name().to_std());
                    
                    if(type_term.type()==var_decl_id) {
                        ctx.node.type(decl_id);
                        ctx.node.value(type_term.value());
                        ctx.node.name(id_term.name().to_std());
                        ctx.node.value().sub_type(0);
                        ctx.node.value(distribute_value(ctx.node.in_scope(), ctx.node.name().to_std(), ctx.node.value()));
                        ctx.node.children().clear();
    
                        // Node marker = make<Node>(); //Make a muted qual marker
                        // marker->type = decl_id;
                        // marker->mute = true;
                        // ctx.node->value->quals << marker;
                    }
                } else if(children.length() == 1) {
                    Node type_term = children[0];
                    ctx.node.name(c+type_term.name().to_std());
                    ctx.node.type(unary_id);
                    if(is_live(type_term.value())) {
                        ctx.node.value().copy(type_term.value());
                    }
                } 
            };
            t_handlers[id] = handler;
            t_handlers[unary_id] = handler;
    
            return id;
        }

        size_t register_binary_operator(int use_id) {
            return add_binary_operator(' ',labels[use_id],use_id);
        }

        size_t plus_id = add_binary_operator('+',"PLUS");
        size_t dash_id = add_binary_operator('-',"DASH");
        size_t rangle_id = add_binary_operator('>',"RANGLE");
        size_t langle_id = add_binary_operator('<',"LANGLE");
        size_t equals_id = add_binary_operator('=', "EQUALS");
        size_t star_id = add_binary_operator('*',"STAR");
        size_t caret_id = add_binary_operator('^',"CARET");
        size_t amp_id = add_binary_operator('&',"AMPERSAND");
        size_t dot_id = add_binary_operator('.', "DOT");
        size_t pipe_id = add_binary_operator('|', "PIPE");

        uint32_t plus_plus_id = add_token_combo("PLUS_PLUS",'+','+');
        uint32_t plus_plus_plus_id = add_token_combo("PLUS_PLUS_PLUS",'+','+','+');
        uint32_t plus_equals_plus_id = add_token_combo("PLUS_EQUALS_PLUS",'+','=','+');

        uint32_t random_combo_id = add_token_combo("RANDOM",'|','*','^','+');

        //size_t pipe_id = add_token('|',"pipe");


        map<uint32_t,int> left_binding_power;
        map<uint32_t,int> right_binding_power;
        void set_binding_powers(uint32_t id, int lbp, int rbp) {
            left_binding_power.put(id,lbp);
            right_binding_power.put(id,rbp);
        }

        void init_stage_a() {
            discard_types.push_if_absent(undefined_id);
            discard_types.push_if_absent(end_id);
            discard_types.push_if_absent(lparen_id);
            discard_types.push_if_absent(lbrace_id);
            discard_types.push_if_absent(comma_id);

            a_handlers.default_function = [this](Context& ctx) {
                int left_bp = left_binding_power.getOrDefault(ctx.node.type(), -1);
                int right_bp = right_binding_power.getOrDefault(ctx.node.type(), -1);
                
                if(left_bp == -1 && right_bp == -1) return;
                
                if(is_live(ctx.left) && left_bp > 0 && !discard_types.has(ctx.left.type())) {
                    int left_left_bp = left_binding_power.getOrDefault(ctx.left.type(), -1);
                    int left_right_bp = right_binding_power.getOrDefault(ctx.left.type(), -1);
    
                    bool right_associative = right_bp < left_bp; //lbp > rbp means right assoc
                    bool should_steal = left_bp > (right_associative ? left_right_bp : left_left_bp);
                    
                    if(!ctx.left.children().empty()) {
                        if(ctx.left.children().length()==1) {
                            should_steal = true;
                        }
                        else if(discard_types.has(ctx.left.children().last().type())) {
                            goto otter;
                        }
                    }
    
                    if(left_right_bp!=-1 && should_steal) {
                        if(ctx.left.children().length()>1) {
                            ctx.node.children() << ctx.left.children().pop();
                        }
                        ctx.left.children() << ctx.result.take(ctx.index);
                    } else {
                        ctx.node.children() << ctx.left;
                        ctx.result.removeAt(ctx.index - 1);
                    }
                } else {
                    otter:
                    if(!discard_types.has(ctx.node.type()))
                        ctx.node.type(to_unary_id(ctx.node.type()));
                    ctx.index++;
                }
                
                if(right_bp != -1 && ctx.index < ctx.result.length()) {
                    Node next = ctx.result.get(ctx.index);
                    int next_lbp = left_binding_power.getOrDefault(next.type(), -1);
                    if(next_lbp == -1 && !discard_types.has(next.type())) { //It's an atom so we grab it
                        ctx.node.children() << ctx.result.take(ctx.index);
                    } 
                }
                ctx.index--;
            };
    
            for(int m = 0; m<2; m++) {
                uint32_t open_id = m==0?rparen_id:rbracket_id;
                uint32_t close_id = m==0?lparen_id:lbracket_id;

                left_binding_power.put(close_id,10);
    
                a_handlers[open_id] = [this,close_id](Context& ctx) {
                    ctx.result.removeAt(ctx.index);
                    int i = ctx.index-1;
                    list<Node> gathered;
                    while(i>=0) {
                        Node on = ctx.result.get(i);
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
                            // g_ptr<Node> token_on = copy_as_token(on);

                            if(!on.children().empty())
                                on.copy(on.children().take(0));

                            if(!was_given_children) {
                                if(on.children().empty()) {
                                    for(auto g : gathered)
                                        on.children() << g;
                                } else { //This case if for things like int main(int a), where we want the gathered to go under main, not int
                                    for(auto g : gathered)
                                        on.children().last().children() << g;
                                }
                            }
                            ctx.index = i;
                            break;
                        } else {
                            gathered << ctx.result.take(i);
                            i--;
                        }
                    }
                    if(i < 0) {
                        ctx.result.push(ctx.node); //Return the rparen to carry the error
                        print(red("rparen:a_handler unmatched closing paren"));
                    }
                };
            }
    
            a_handlers[identifier_id] = [this](Context& ctx){
                if(is_live(ctx.left) && ctx.left.type() == identifier_id) {
                    while(ctx.index < ctx.result.length() && ctx.result.get(ctx.index).type() == identifier_id) {
                        ctx.left.children() << ctx.result.take(ctx.index);
                    }
                    ctx.index--;
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
                ctx.result.removeAt(ctx.index);
                int i = ctx.index-1;
                list<Node> gathered;
                while(i>=0) {
                    Node on = ctx.result.get(i);
                    Node was_on = ctx.root; //Storing the root for cases where we want to notify once children are gathered
                    while(!on.children().empty()&&on.type()!=lbrace_id) {
                        was_on = on;
                        on = on.children().last(); //Descend to the found lbrace
                    }
                    if(on.type()==lbrace_id) {
                        on.type(scope_id); //Turn it into a scope and hand over the contents
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
                        ctx.index = i;
                        break;
                    } else {
                        gathered << ctx.result.take(i);
                        i--;
                    }
                }
                if(i < 0) {
                    ctx.result.push(ctx.node); //Return the rbrace to carry the error
                    print(red("rbrace:s_handler unmatched closing brace"));
                }
            };
            s_handlers[lbrace_id] = [this](Context& ctx){}; //Do nothing

            s_handlers.default_function = [this](Context& ctx){
                if(ctx.index+1>=ctx.result.length()) return;

                Node right = ctx.result[ctx.index+1];
                if(right.type()==lbrace_id) {
                    ctx.node.children() << ctx.result.take(ctx.index+1);
                }
            };
        }

        void resolve_node_literal(Context& ctx, void* val, uint32_t tag, uint32_t size) {
            standard_sub_process(ctx);
            ctx.node.type(literal_id);
            Value value = make_value(tag,size);
            value.set(val);
            ctx.node.value(value);
        }

        void init_literals() {
            print_handlers[object_id] = [](Context& ctx) {ctx.source = Ptr_as_string(ctx.value.data_ptr());};
            print_handlers[ptr_id] = [](Context& ctx) {ctx.source = Ptr_as_string(ctx.value.data_ptr());};
            print_handlers[float_id] = [](Context& ctx) {ctx.source = std::to_string(*(float*)ctx.value.get());};
            print_handlers[int_id] = [](Context& ctx) {ctx.source = std::to_string(*(int*)ctx.value.get());};
            print_handlers[char_id] = [](Context& ctx) {ctx.source = std::string(1,*(char*)ctx.value.get());};
            print_handlers[bool_id] = [](Context& ctx) {ctx.source = (*(bool*)ctx.value.get()) ? "TRUE" : "FALSE";};
            print_handlers[string_id] = [this](Context& ctx) {ctx.source = string((*(Ptr*)ctx.value.get()));};
                
            t_handlers[float_id] = [this](Context& ctx) {
                float stof = std::stof(ctx.node.name().to_std());
                resolve_node_literal(ctx,(void*)&stof,float_id,4);
            }; 
    
            t_handlers[int_id] = [this](Context& ctx) {
                int stoi = std::stoi(ctx.node.name().to_std());
                resolve_node_literal(ctx,(void*)&stoi,int_id,4);
            }; 
    
            t_handlers[bool_id] = [this](Context& ctx) {
                bool stob = ctx.node.name().to_std() == "true" ? true : false;
                resolve_node_literal(ctx,(void*)&stob,bool_id,1);
            }; 

            t_handlers[char_id] = [this](Context& ctx) {
                char stob = ctx.node.name()[0];
                resolve_node_literal(ctx,(void*)&stob,char_id,1);
            }; 
    
            t_handlers[string_id] = [this](Context& ctx) {
                Ptr ptr = ctx.node.name_ptr();
                resolve_node_literal(ctx,(void*)&ptr,string_id,sizeof(Ptr));
            }; 
        }



        void resolve_identifier(Context& ctx) {
            Node node = ctx.node;
            Value decl_value = make_value();
            // if(is_live(node.value())) decl_value = node.value();
            // else decl_value = make_value();
            bool is_qualifier = is_live(node.value()) && node.value().type()!=0 && node.value().sub_type() != 0; 
            //We count it as a qualifer if it has a fully valid value to stamp

            int root_idx = -1;
            if(is_qualifier) {
                if(is_live(ctx.node.value()))
                    decl_value.quals() << value_to_qual(node.value());
                for(int i = 0; i < node.children().length(); i++) {
                    Node c = node.children()[i];
                    find_value_in_scope(c); //Process forward and consume other qualifers
                    if(c.type()!=identifier_id) {break;}

                    if(is_live(c.value())&&c.value().type()!=0) {
                        decl_value.quals() << value_to_qual(c.value());
                    } else {
                        root_idx = i;
                        break;
                    }
                }
                if(root_idx!=-1) {
                    Node root = node.children()[root_idx];
                    node.name(root.name().to_std());
                    for(int i = root_idx+1; i < node.children().length(); i++) {
                        Node c = node.children()[i];
                        find_value_in_scope(c);
                        if(is_live(c.value())&&c.value().type()!=0) {
                            node.quals() << value_to_qual(c.value());
                        } 
                    }
                    node.children(node.children().take(root_idx).children());
                }
            }
            
            if(node.scopes().empty()) { //Defer, the r_stage will do this later for scoped nodes
                standard_sub_process(ctx);
            }

            if(keywords.hasKey(node.name().to_std())) {
                if(node.value().sub_type()!=0) {
                    node.type(node.value().sub_type());
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
                    node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0]);
                    node.value().type_scope(node.scopes()[0]);
                    node.value().sub_type(0);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
                    if(node.in_scope().type()==type_scope_id) {
                        std::string nname = node.name().to_std();
                        bool has_opp = false;
                        for(auto c : nname) {if(registered_opperators.getOrDefault(c,false)) {has_opp = true; break;}}
                        if(!has_opp) {nname=".\""+nname+"\"";} 
                        overload_type(node.in_scope().owner().value().type(),nname,method_call_id,node.value());
                        //layouts[node.in_scope().owner().value().type()].add_prop(func_call_id,0,node.name().to_std(),node.value().type(),node.value().sub_type(),node.scopes()[0]);
                    }
                } else {
                    node.type(type_decl_id);
                    node.value().copy(make_type(node.name().to_std(),0));
                    node.value().type_scope(node.scopes()[0]);
                    node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
                    node.scopes()[0].type(type_scope_id);
                    add_template(node.value().type());
                }
            } else {
                has_scope = find_node_in_scope(node); //To distinquish func_calls from object identifiers
                if(has_sub_type) {
                    node.type(var_decl_id);
                    if(node.in_scope().type()==type_scope_id) {
                        node.in_scope().value_table().put(node.name().to_std(), decl_value); //So we don't distribute into function bodies, we need to alias later via this, as it's per instance
                        layouts[node.in_scope().owner().value().type()].add_prop(node.value().type(),node.value().size(),node.name().to_std());
                    } else {
                        node.value(distribute_value(node.in_scope(), node.name().to_std(), decl_value));
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
                } else { //Must be a plain identifier
                    find_value_in_scope(node);
                }
            }

        }

        uint64_t make_overload_key(uint32_t root, uint32_t right) {
            return ((uint64_t)root << 32) | right;
        }
        void overload_type(uint32_t type, const std::string& instr, uint32_t overload_to, Value value = deadptr) {
            if(!layouts.hasKey(type)) layouts.put(type,_layout());
            Node expr = tokenize(instr);
            Stage* old_stage = active_stage;
            start_stage(a_handlers);
            standard_direct_pass(expr);
            a_pass_resolve_keywords(expr.children());
            start_stage(old_stage);

            uint32_t root_type = 0; 
            uint32_t right_type = 0;
            if(!expr.children().empty()) {
                Node op = expr.children()[0];
                root_type = op.type();
                if(instr.length()>1&&instr.find(op.name().to_std())==0) {
                    root_type-=2; //Convert to normal version if it's on the left side, so +string parses as plus_unary, but is actually just normal plus
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
            layouts[type].overload.put(make_overload_key(root_type,right_type),type_and_value{overload_to,value});
            recycle_node(expr);

        }

        void what_I_see(Context& ctx) {
            print(bold_str(ctx.node.name().to_std()),": I see my root is ",green(ctx.root.name().to_std()),", my value type is ",blue(labels[ctx.node.value().type()])," and to my left is ",yellow(is_live(ctx.left)?ctx.left.name().to_std():"nothing"));
        }

        void resolve_overload(Context& ctx) {
            standard_sub_process(ctx);
            if(ctx.index==0&&is_live(ctx.node.value())) { //If we're the left term
                if(layouts.hasKey(ctx.node.value().type())) {
                    map<uint64_t,type_and_value>& overload = layouts[ctx.node.value().type()].overload;
                    uint32_t right_type = 0;
                    bool has_overload = false;
                    uint64_t typekey = 0;
                    if(ctx.result.length()>1) {
                        right_type = ctx.result.get(1).value().type();
                    } else {
                        typekey = make_overload_key(ctx.root.type(),0);
                        has_overload = overload.hasKey(typekey);
                    }

                    if(right_type!=0) {
                        typekey = make_overload_key(ctx.root.type(),right_type);
                        has_overload = overload.hasKey(typekey);
                        if(!has_overload) {
                            typekey = make_overload_key(ctx.root.type(),any_id);
                            has_overload = overload.hasKey(typekey);
                        }
                    } else if(!has_overload) {
                        if(ctx.result.length()>1) {
                            right_type = hashString(ctx.result.get(1).name().to_std());
                        }
                        if(right_type!=0) {
                            typekey = make_overload_key(ctx.root.type(),right_type);
                            has_overload = overload.hasKey(typekey);
                        }
                    }
                    if(has_overload) {
                        type_and_value& tnv = overload.get(typekey);
                        ctx.root.type(tnv.type);
                        if(is_live(tnv.value)) {
                            Value& value = (Value&)tnv.value;
                            if((value.type()!=0)) {
                                ctx.root.value(make_value(value.type(),value.size(),value.address(),value.sub_type(),value.sub_size(),value.type_scope()));
                            } else {
                                ctx.root.value(make_value(
                                    ctx.node.value().sub_type(),
                                    ctx.node.value().sub_size()
                                ));
                            }
                        }
                    }
                }
            }
        }

        void sync_args(Context& ctx) {
            if(!ctx.node.scopes().empty()) {
                for(int i = 0; i < ctx.node.children().length(); i++) {
                    Node arg = ctx.node.children()[i];
                    Node param = ctx.node.scopes()[0].owner().children()[i];
                    Node assignment = make_node(equals_id);
                    assignment.children().push(param);
                    assignment.children().push(arg);
                    ctx.node.children().col().set(i,(void*)&assignment);
                }
            }
        }

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

            set_binding_powers(plus_id, 4,6);
            set_binding_powers(dash_id, 4,5);
            set_binding_powers(slash_id, 4,5);
            set_binding_powers(rangle_id, 2,3);
            set_binding_powers(langle_id, 2,3);
            set_binding_powers(equals_id, 1,1);
            set_binding_powers(star_id, 5,7);
            set_binding_powers(caret_id, 8,4);
            set_binding_powers(amp_id, 4,8);
            set_binding_powers(dot_id, 8,9);
            set_binding_powers(pipe_id, 9,8);

            set_binding_powers(random_combo_id,8,9);

            t_handlers[identifier_id] = [this](Context& ctx){resolve_identifier(ctx);};
            t_handlers[equals_id] = [this](Context& ctx){standard_sub_process(ctx);};

            t_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            r_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};
            x_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};

            x_handlers[equals_id] = [this](Context& ctx){
                if(ctx.node.children().length()==2) {
                    backwards_sub_process(ctx);
                    Node left = ctx.node.children()[0];
                    Node right = ctx.node.children()[1];

                    Ptr lp = left.value().data_ptr();
                    Ptr rp = right.value().data_ptr();

                    if(types[lp.pool][lp.idx].heterogenous) {
                        types[lp.pool][lp.idx].qset(lp.sidx,types[rp.pool][rp.idx][rp.sidx],right.value().size());
                    } else {
                        types[lp.pool][lp.idx].set(lp.sidx,types[rp.pool][rp.idx][rp.sidx]);
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
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(ctx.node.type()==dot_id) {
                    Node left = ctx.node.children()[0];
                    Node right = ctx.node.children()[1];
                    uint32_t ltype = left.value().type();
                    if(layouts.hasKey(ltype)) {
                        _layout& layout = layouts.get(ltype);
                        std::string prop = right.name().to_std();
                        if(layout.label_to_index.hasKey(prop)) {
                            uint32_t index = layout.label_to_index.get(prop);
                            ctx.node.value(make_value(layout.tags[index], layout.sizes[index], layout.offsets[index], layout.subtags[index], layout.subsizes[index], layout.ptrs[index]));
                        } else {
                            print(red("r_handlers::dot_id layout of "+labels[ltype]+" does not have prop "+prop));
                            // print(red("root is: "+labels[ctx.root.type()]));
                        }
                        right.value(ctx.node.value());
                    } else {
                        print(red("r_handlers::dot_id no layout found for type "+labels[ltype]));
                    }

                    if(right.type()==identifier_id&&!right.children().empty()) { //Can replace with QValue in the future for an optimization
                        Value value = ctx.node.value();
                        value.type(value.sub_type()); value.sub_type(0);
                        value.size(value.sub_size()); value.sub_size(0);
                        //right.type(temp_get_id);
                    }
                } else if(ctx.node.type()==method_call_id) { //Turn into a function call
                    ctx.node.type(func_call_id);
                    node_col args = ctx.node.children()[1].children();
                    ctx.node.children(args);
                    ctx.node.scopes().push(ctx.node.value().type_scope());
                    sync_args(ctx);
                }
            };
            x_handlers[dot_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node.children()[0];
                Node right = ctx.node.children()[1];
                Value value = ctx.node.value();
                if(right.type()==identifier_id) {
                    Ptr ptr = *(Ptr*)left.value().get();
                    ptr.sidx = value.address();
                    if(!right.children().empty()) { //This is a bit jank, I would prefer to find a way to get = working instead
                        if(types[ptr.pool][ptr.idx].heterogenous) {
                            types[ptr.pool][ptr.idx].qset(ptr.sidx,(void*)&right.children()[0].value().data_ptr(),right.children()[0].value().size());
                        } else {
                            types[ptr.pool][ptr.idx].set(ptr.sidx,(void*)&right.children()[0].value().data_ptr());
                        }
                    } else {
                        if(ctx.root.type()==equals_id&&is_live(ctx.left)) {
                            types[value.pool][value.idx].qset(value_data_offset,(void*)&ptr,sizeof(Ptr)); //Setting the data_ptr itself
                        } else {
                            value.set(types[ptr.pool][ptr.idx].get(ptr.sidx)); //Setting what the data_ptr points to
                        }
                    }
                }
            };

            r_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(bool_id,1));
            };
            x_handlers[langle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                bool result =      
                    *(int*)ctx.node.children()[0].value().get()
                    <
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(bool_id,1));
            };
            x_handlers[rangle_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                bool result =      
                    *(int*)ctx.node.children()[0].value().get()
                    >
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[plus_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                resolve_overload(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(int_id,4));
            };
            x_handlers[plus_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int result =      
                    *(int*)ctx.node.children()[0].value().get()
                    +
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[dash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(int_id,4));
            };
            x_handlers[dash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int result =      
                    *(int*)ctx.node.children()[0].value().get()
                    -
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[star_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(int_id,4));
            };
            x_handlers[star_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int result =      
                    *(int*)ctx.node.children()[0].value().get()
                    *
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[slash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(int_id,4));
            };
            x_handlers[slash_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int result =      
                    *(int*)ctx.node.children()[0].value().get()
                    /
                    *(int*)ctx.node.children()[1].value().get()
                ;
                ctx.node.value().set((void*)&result);
            };

            r_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(!is_live(ctx.node.value())) ctx.node.value(make_value(int_id,4));
            };
            x_handlers[to_unary_id(dash_id)] = [this](Context& ctx){
                int neg = -(*(int*)ctx.node.children()[0].value().get());
                ctx.node.value().set((void*)&neg);
            };

            r_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                sync_args(ctx);
            };
            x_handlers[func_call_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                standard_travel_pass(ctx.node.scopes()[0],ctx.sub);
            };

            x_handlers[add_function("print")] = [this](Context& ctx){
                std::string to_print = "";
                for(int i=0;i<ctx.node.children().length();i++) {
                    Node c = ctx.node.children()[i];
                    if(c.name().to_std()=="ROOTS") {
                        print(node_to_string(ctx.root));
                        continue;
                    }
                    process_node(ctx,c);
                    to_print += value_as_string(c.value());
                }
                print(to_print);
            };

            x_handlers[make_tokenized_keyword("root_name")] = [this](Context& ctx){
                if(ctx.node.children().empty()) {
                    ctx.node.value(make_value(string_id,sizeof(Ptr)));
                    ctx.node.value().set((void*)&ctx.root.name_ptr());
                } else {
                    ctx.root.name() = ctx.node.children()[0].name();
                }
            };
        }
    };
}