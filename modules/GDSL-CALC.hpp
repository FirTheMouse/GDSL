#include "../core/GDSL.hpp"

namespace GDSL {

    void my_module() {

        size_t int_id = reg_id("int");
        
        size_t plus_id = reg_id("plus");
        tokenizer_functions['+'] = [&](Context& ctx) {
            ctx.node = make<Node>();
            ctx.node->name += '+';
            ctx.node->type = plus_id;
            ctx.result->push(ctx.node);
        };

        tokenizer_state_functions[int_id] = [&](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            bool is_digit = std::isdigit(c);
            bool is_space = std::isspace(c);
            if(is_digit) {
                ctx.node->name += c;
            } else if(is_space) {
                ctx.state = 0; 
            } else {
                ctx.state = 0; 
                --ctx.index;
            }
        };

        tokenizer_default_function = [&](Context& ctx) {
            char c = ctx.source.at(ctx.index);
            bool is_digit = std::isdigit(c);
            bool is_space = std::isspace(c);
            if(is_digit) {
                ctx.state = int_id;
                ctx.node = make<Node>();
                ctx.node->name = c;
                ctx.node->type = int_id;
                ctx.result->push(ctx.node);
            } else if(is_space) {

            } else {
                
            }
          
        };

        list<g_ptr<Node>> tokens = tokenize("1+2+3");
        for(int i = 0; i<tokens.length(); i++) {
            print(tokens[i]->to_string());
        }
        

        a_parse_function = [&](Context& ctx) {
            g_ptr<Node> left = ctx.nodes[ctx.index];
            ctx.index++;
            
            while(ctx.index < ctx.nodes.length()) {
                g_ptr<Node> op = ctx.nodes[ctx.index];
                ctx.index++;
                
                if(ctx.index >= ctx.nodes.length()) break;
                g_ptr<Node> right = ctx.nodes[ctx.index];
                ctx.index++;
                
                op->children << left;
                op->children << right;
                left = op;
            }
            
            ctx.result->push(left);
        };


        list<g_ptr<Node>> nodes = parse_tokens(tokens);
        for(auto n : nodes) {
            print(n->to_string());
        }

    }
}

