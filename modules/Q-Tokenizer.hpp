#pragma once
#include "../modules/Q-Literals.hpp"

namespace GDSL {
    struct Tokenizer_Unit : public virtual Literals_Unit {
        map<std::string,uint32_t> tokenized_keywords;
        map<char,bool> char_is_split;

        size_t make_tokenized_keyword(const std::string& f) {
            size_t id = reg_id(f);
            tokenized_keywords.put(f,id);
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