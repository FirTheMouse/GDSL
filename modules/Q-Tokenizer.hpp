#pragma once
#include "../modules/Q-Literals.hpp"

namespace GDSL {
    struct Tokenizer_Unit : public virtual Literals_Unit {
        map<std::string,uint32_t> tokenized_keywords;
        map<char,bool> char_is_split;

        map<char, Handler> tokenizer_functions;
        map<uint32_t, Handler> tokenizer_state_functions;
        Handler tokenizer_default_function = nullptr;

        size_t make_tokenized_keyword(const std::string& token_name, size_t default_id = 0) {
            size_t id = default_id;
            if(default_id==0) {
                id = reg_id(token_name);
            }
            tokenized_keywords.put(token_name,id);
            return id;
        }

        size_t add_token(char c, const std::string& f) {
            size_t id = reg_id(f);
            tokenizer_functions[c] =  [id,c](Context& ctx) {
                ctx.node = make<Node>(id,c);
                ctx.result->push(ctx.node);
            };
            char_is_split.put(c, true);
            return id;
        }


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

        size_t in_string_id = reg_id("IN_STRING_KEY");
        size_t in_alpha_id = reg_id("IN_ALPHA");
        size_t in_digit_id = reg_id("IN_DIGIT");
        size_t end_id = add_token(';',"END"); //Can commonly be changed to be a line return

        list<g_ptr<Node>> tokenize(const std::string& code) {
            list<g_ptr<Node>> result;
            uint32_t state = 0;
            int index = 0;
            Context ctx(result,index);
            ctx.source = code;

            #if PRINT_ALL
            newline("tokenize pass");
            #endif

            if(!tokenizer_default_function) {
                print("GDSL::tokenize warning! No defined default function, please define one");
            }

            while (index<code.length()) {
                char c = code.at(index);
                if(ctx.state!=0&&tokenizer_state_functions.hasKey(ctx.state)) {
                    auto state_func = tokenizer_state_functions.get(ctx.state);
                    state_func(ctx);
                } else {
                    auto func = tokenizer_functions.getOrDefault(c,tokenizer_default_function);
                    func(ctx);
                }
                ++index;
            }  

            #if PRINT_ALL
            int i = 0;
            for(auto t : result) {
                if(t->getType()) {
                    log(i++," ",labels[t->getType()],": ",t->name);
                }
            }
            endline();
            #endif

            return result;
        }

        void init() {
            Literals_Unit::init();
            char_is_split.put(' ',true);
            tokenizer_state_functions.put(in_alpha_id,[this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(char_is_split.getOrDefault(c,false)) {
                    ctx.state = 0; 
                    --ctx.index;
                    ctx.node->type = tokenized_keywords.getOrDefault(ctx.node->name,ctx.node->type);
                    return;
                } else {
                    ctx.node->name += c;
                }
            });
    
            tokenizer_state_functions.put(in_digit_id,[this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(char_is_split.getOrDefault(c,false)) {
                    if(c=='.') {
                        ctx.node->type = float_id;
                    } else {
                        ctx.state = 0; 
                        --ctx.index;
                        return;
                    }
                } else if(std::isalpha(c)) {
                    ctx.state = in_alpha_id;
                }
                ctx.node->name += c;
            });

            tokenizer_state_functions.put(in_string_id,[](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(c=='"') {
                    ctx.state=0;
                }
                else {
                    ctx.node->name += c;
                }
            });
            tokenizer_functions.put('"',[this](Context& ctx) {
                ctx.state = in_string_id;
                ctx.node = make<Node>();
                ctx.node->type = string_id;
                ctx.node->name = "";
                ctx.result->push(ctx.node);
            });
    
            tokenizer_default_function = [this](Context& ctx) {
                char c = ctx.source.at(ctx.index);
                if(std::isalpha(c)) {
                    ctx.state = in_alpha_id;
                    ctx.node = make<Node>(identifier_id,c);
                    ctx.result->push(ctx.node);
                }
                else if(std::isdigit(c)) {
                    ctx.state = in_digit_id;
                    ctx.node = make<Node>(int_id,c);
                    ctx.result->push(ctx.node);
                } else if(c==' '||c=='\t'||c=='\n') {
                    //just skip
                }
                else {
                    print("tokenize::default_function missing handling for char: ",c);
                }
            };
        }
    };
}