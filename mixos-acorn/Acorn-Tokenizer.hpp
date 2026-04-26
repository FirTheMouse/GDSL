// #pragma once
// #include "../mixos-acorn/Acorn-Core.hpp"

// namespace Acorn {
//     struct Tokenizer_Unit : public virtual Unit {
//         map<std::string,uint32_t> tokenized_keywords;
//         map<char,bool> char_is_split;

//         map<char, Handler> tokenizer_functions;
//         map<uint32_t, Handler> tokenizer_state_functions;
//         Handler tokenizer_default_function = nullptr;

//         float at_x = 0.0f;
//         float at_y = 0.0f;

//         size_t make_tokenized_keyword(const std::string& token_name, size_t default_id = 0) {
//             size_t id = default_id;
//             if(default_id==0) {
//                 id = reg_id(token_name);
//             }
//             tokenized_keywords.put(token_name,id);
//             return id;
//         }

//         size_t add_token(char c, const std::string& f) {
//             size_t id = reg_id(f);
//             tokenizer_functions[c] = [this,id,c](Context& ctx) {
//                 ctx.node = make<Node>(id,c);
//                 ctx.node->x = at_x; 
//                 ctx.node->y = at_y;
//                 ctx.result->push(ctx.node);
//             };
//             char_is_split.put(c, true);
//             return id;
//         }


//         size_t to_decl_id(size_t id) {return id+1;}
//         size_t to_unary_id(size_t id) {return id+2;}
        
//         size_t colon_id = add_token(':',"COLON");
//         size_t lparen_id = add_token('(',"LPAREN");
//         size_t rparen_id = add_token(')',"RPAREN");
//         size_t comma_id = add_token(',',"COMMA");
//         size_t lbracket_id = add_token('[', "LBRACKET");
//         size_t rbracket_id = add_token(']', "RBRACKET");
//         size_t lbrace_id = add_token('{', "LBRACE");
//         size_t rbrace_id = add_token('}', "RBRACE");
//         size_t slash_id = add_token('/',"SLASH");

//         size_t in_alpha_id = reg_id("IN_ALPHA");
//         size_t in_digit_id = reg_id("IN_DIGIT");
//         size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return
//         size_t quote_id = reg_id("QUOTE");
//         size_t comment_id = reg_id("COMMENT");
//         size_t single_quote_id = reg_id("SINGLE_QUOTE");

//         void add_string_token(char open, size_t open_id, size_t state_id) {
//             tokenizer_functions.put(open,[this, open, open_id, state_id](Context& ctx) {
//                 ctx.state = state_id;
//                 ctx.node = make<Node>();
//                 ctx.node->x = at_x + 1;
//                 ctx.node->y = at_y;
//                 ctx.node->type = string_id;
//                 ctx.node->name = "";
//                 ctx.result->push(ctx.node);
        
//                 g_ptr<Node> quote = make<Node>(open_id, open);
//                 quote->x = at_x; quote->y = at_y;
//                 quote->mute = true;
//                 ctx.result->last()->quals << quote;
//             });
        
//             tokenizer_state_functions.put(state_id,[this, open, open_id](Context& ctx) {
//                 char c = ctx.source.at(ctx.index);
//                 if(c == open) {
//                     ctx.state = 0;
//                     g_ptr<Node> quote = make<Node>(open_id, c);
//                     quote->x = at_x; quote->y = at_y;
//                     quote->mute = true;
//                     ctx.result->last()->quals << quote;
//                 } else if(c == '\\') {
//                     if(ctx.index + 1 < ctx.source.length()) {
//                         ctx.node->name += ctx.source.at(ctx.index + 1);
//                         at_x += 1.0f;
//                         ctx.index++;
//                     }
//                 } else if(c == '\n') {
//                     at_y += 1.0f;
//                     at_x = -1.0f;
//                     ctx.node->name += c;
//                 } else {
//                     ctx.node->name += c;
//                 }
//             });
//         }


//         void add_double_string_token(char open, char open2, size_t open_id, size_t open_id2, size_t state_id, size_t type_id, bool break_on_newline = false, map<std::string,Handler>* parts = nullptr) {      
//             Handler open_func = [this, open, open2, open_id, open_id2, state_id, type_id](Context& ctx) {
//                 char c = ctx.source.at(ctx.index+1);
//                 if(c==open2) {
//                     ctx.node = make<Node>(type_id);
//                     g_ptr<Node> slashes = make<Node>(type_id);
//                     slashes->name = std::string({open,open2});
//                     slashes->x = at_x; slashes->y = at_y;
//                     slashes->mute = true;
//                     ctx.node->quals << slashes;
//                     ctx.result->push(ctx.node);
//                     at_x+=1.0f;
//                     ctx.index++;
//                     ctx.state = state_id;
//                 }
//             };
//             Handler open_else_func = [this, open, open_id, state_id](Context& ctx) {
//                 char c = ctx.source.at(ctx.index+1);
//                 ctx.node = make<Node>(open_id,c);
//                 ctx.node->x = at_x;
//                 ctx.node->y = at_y;
//                 ctx.result->push(ctx.node);
//             };
//             Handler full_package_func = [this, state_id, open_func, open_else_func](Context& ctx) {
//                 open_func(ctx);
//                 if(ctx.state!=state_id)
//                     open_else_func(ctx);
//             };
//             tokenizer_functions[open] = full_package_func;
//             if(parts) {
//                 parts->put("if",open_func);
//                 parts->put("else",open_else_func);
//                 parts->put("full",full_package_func);
//             }
        
//             Handler state_func = [this, open, open2, open_id, open_id2, state_id, type_id, break_on_newline](Context& ctx) {
//                 char c = ctx.source.at(ctx.index);
//                 if(c == open&&ctx.source.at(ctx.index+1)==open2) {
//                     ctx.state=0;
//                     g_ptr<Node> slashes = make<Node>(type_id);
//                     slashes->name = std::string({open,open2});
//                     slashes->x = at_x; slashes->y = at_y;
//                     slashes->mute = true;
//                     ctx.result->last()->quals << slashes;
//                     ctx.index++;
//                 } else if(c == '\\') {
//                     if(ctx.index + 1 < ctx.source.length()) {
//                         ctx.node->name += ctx.source.at(ctx.index + 1);
//                         at_x += 1.0f;
//                         ctx.index++;
//                     }
//                 } else if(c=='\n') {
//                     at_y += 1.0f;
//                     at_x = -1.0f;
//                     ctx.node->name += c;
//                     if(break_on_newline)
//                         ctx.state = 0;
//                 }
//                 else {
//                     ctx.node->name += c;
//                 }
//             };
//             tokenizer_state_functions.put(state_id,state_func);
//         }

//         list<g_ptr<Node>> tokenize(const std::string& code) {
//             list<g_ptr<Node>> result;
//             uint32_t state = 0;
//             int index = 0;
//             at_x = 0.0f;
//             at_y = 0.0f;
//             Context ctx(result,index);
//             ctx.source = code;

//             #if PRINT_ALL
//             newline("tokenize pass");
//             #endif

//             if(!tokenizer_default_function) {
//                 print("GDSL::tokenize warning! No defined default function, please define one");
//             }

//             while (index<code.length()) {
//                 char c = code.at(index);
//                 Handler* func = nullptr;
//                 if(ctx.state!=0&&tokenizer_state_functions.hasKey(ctx.state)) {
//                     func = &tokenizer_state_functions.get(ctx.state);
//                 } else {
//                     func = &tokenizer_functions.getOrDefault(c,tokenizer_default_function);
//                 }

//                 if(func) {
//                     (*func)(ctx);
//                 }

//                 at_x += 1.0f;
//                 ++index;
//             }  

//             #if PRINT_ALL
//             int i = 0;
//             for(auto t : result) {
//                 if(t->getType()) {
//                     log(i++," ",labels[t->getType()],": ",t->name);
//                 }
//             }
//             endline();
//             #endif

//             return result;
//         }

//         void init() {
//             // Literals_Unit::init();
//             // char_is_split.put(' ',true);
//             // tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
//             //     char c = ctx.source.at(ctx.index);
//             //     if(char_is_split.getOrDefault(c,false)) {
//             //         ctx.state = 0; 
//             //         at_x-=1.0f;
//             //         --ctx.index;
//             //         ctx.node->type = tokenized_keywords.getOrDefault(ctx.node->name,ctx.node->type);
//             //         return;
//             //     } else {
//             //         ctx.node->name += c;
//             //     }
//             // });
    
//             // tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
//             //     char c = ctx.source.at(ctx.index);
//             //     if(char_is_split.getOrDefault(c,false)) {
//             //         if(c=='.') {
//             //             ctx.node->type = float_id;
//             //         } else {
//             //             ctx.state = 0; 
//             //             at_x-=1.0f;
//             //             --ctx.index;
//             //             return;
//             //         }
//             //     } else if(std::isalpha(c)) {
//             //         ctx.state = in_alpha_id;
//             //     }
//             //     ctx.node->name += c;
//             // });

//             // add_string_token('"', quote_id, reg_id("IN_STRING"));
//             // add_string_token('\'', single_quote_id, reg_id("IN_SHORT_STRING"));
//             // map<std::string,Handler> slash_parts;
//             // add_double_string_token('/', '/', slash_id, slash_id, reg_id("IN_COMMENT"), comment_id, true, &slash_parts);

//             // // tokenizer_state_functions.put(in_comment_id,[this](Context& ctx) {
//             // //     char c = ctx.source.at(ctx.index);
//             // //     if(c=='/'&&ctx.source.at(ctx.index+1)=='/') {
//             // //         ctx.state=0;
//             // //         g_ptr<Node> slashes = make<Node>(slash_id,c);
//             // //         slashes->x = at_x; slashes->y = at_y;
//             // //         slashes->mute = true;
//             // //         ctx.result->last()->quals << slashes;
//             // //         ctx.index++;
//             // //     } else if(c=='\n') {
//             // //         at_y += 1.0f;
//             // //         at_x = -1.0f;
//             // //         ctx.node->name += c;
//             // //         ctx.state = 0;
//             // //     }
//             // //     else {
//             // //         ctx.node->name += c;
//             // //     }
//             // // });
//             // // tokenizer_functions['/'] = [this](Context& ctx){
//             // //     char c = ctx.source.at(ctx.index+1);
//             // //     if(c=='/') {
//             // //         ctx.node = make<Node>(comment_id);
//             // //         g_ptr<Node> slashes = make<Node>(comment_id);
//             // //         slashes->name = "//";
//             // //         slashes->x = at_x; slashes->y = at_y;
//             // //         slashes->mute = true;
//             // //         ctx.node->quals << slashes;
//             // //         ctx.result->push(ctx.node);
//             // //         at_x+=1.0f;
//             // //         ctx.index++;
//             // //         ctx.state = in_comment_id;
//             // //     } else {
//             // //         ctx.node = make<Node>(slash_id,c);
//             // //         ctx.node->x = at_x;
//             // //         ctx.node->y = at_y;
//             // //         ctx.result->push(ctx.node);
//             // //     }
//             // // };

//             // tokenizer_functions[' '] = [this](Context& ctx) {
//             //     //Just skip
//             // };
//             // tokenizer_functions['\t'] = [this](Context& ctx) {
//             //     //Just skip
//             // };
//             // tokenizer_functions['\n'] = [this](Context& ctx) {
//             //     at_y += 1.0f;
//             //     at_x = -1.0f;
//             // };
    
//             // tokenizer_default_function = [this](Context& ctx) {
//             //     char c = ctx.source.at(ctx.index);
//             //     if(std::isalpha(c)) {
//             //         ctx.state = in_alpha_id;
//             //         ctx.node = make<Node>(identifier_id,c);
//             //         ctx.node->x = at_x;
//             //         ctx.node->y = at_y;
//             //         ctx.result->push(ctx.node);
//             //     }
//             //     else if(std::isdigit(c)) {
//             //         ctx.state = in_digit_id;
//             //         ctx.node = make<Node>(int_id,c);
//             //         ctx.node->x = at_x;
//             //         ctx.node->y = at_y;
//             //         ctx.result->push(ctx.node);
//             //     }  else {
//             //         if(!ctx.result->empty()) {
//             //             attach_error(ctx.result->last(), trivial_error, "tokenize:default_function missing handling for char: "+std::string(1, c));
//             //         }
//             //         print("tokenize:default_function missing handling for char: ",c);
//             //     }
//             // };
//         }
//     };
// }