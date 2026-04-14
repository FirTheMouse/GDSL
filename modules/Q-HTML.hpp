#pragma once

#include "../core/GDSL.hpp"

namespace GDSL {
    struct HTML_Unit : public virtual Unit {
        HTML_Unit() { init(); }

        size_t html_id = reg_id("html");
        size_t body_id = reg_id("body");
        size_t div_id = reg_id("div");
        size_t button_id = reg_id("button");
        size_t script_id = reg_id("script");

        size_t text_id = reg_id("text");
        size_t input_id = reg_id("input");

        size_t paragraph_subtype = reg_id("p");
        size_t span_subtype = reg_id("span");

        size_t style_id = reg_id("style");
        size_t prefix_style_id = reg_id("style");
        size_t suffix_style_id = reg_id("style");
        size_t property_id = reg_id("property");

        size_t properties_id = reg_id("properties");

        map<uint32_t,Handler> html_handlers; Handler html_default_function;
        std::string html_encode_node(g_ptr<Node> node, int& depth) {
            Context ctx(depth);
            ctx.node = node;
            depth++;
            std::string indent(depth * 3, ' ');
            html_handlers.getOrDefault(node->type, html_default_function)(ctx);
            ctx.source.insert(0,indent);
            return ctx.source;
        }

        g_ptr<Node> make_jsscript(const std::string& js) {
            g_ptr<Node> s = make<Node>(script_id);
            s->opt_str = js;
            return s;
        }

        void init() override {
            html_default_function = [this](Context& ctx){
                std::string s = "";
                s += "<"+labels[ctx.node->type]+(ctx.node->name.empty()?"":" "+ctx.node->name);
                int zero_depth = -1;
                for(auto q : ctx.node->quals) {
                    s += " "+html_encode_node(q,zero_depth);
                    zero_depth = -1;
                }
                s+=">\n";

                for(auto c : ctx.node->children) {
                    if(c->type == style_id) continue;
                    s += html_encode_node(c,ctx.index);
                }

                std::string indent(ctx.index * 3, ' ');
                if(!ctx.node->opt_str.empty()) {
                    std::string old_str = ctx.node->opt_str;
                    indent_multiline(ctx.node->opt_str,indent);
                    s += ctx.node->opt_str+"\n";
                    ctx.node->opt_str = old_str;
                } 

                if(ctx.index>0) ctx.index--;
                indent = std::string(ctx.index * 3, ' ');
                s += indent;
                s += "</"+labels[ctx.node->type]+">\n";
                ctx.source = s;
            };

            html_handlers[text_id] = [this](Context& ctx){
                std::string s = "";
                s+= "<"+labels[ctx.node->sub_type];
                int zero_depth = -1;
                for(auto q : ctx.node->quals) {
                    s += " "+html_encode_node(q,zero_depth);
                    zero_depth = -1;
                }

                if(ctx.node->left()) {
                    process_node(ctx, ctx.node->left());
                    ctx.node->name = ctx.node->left()->value->get<std::string>();
                }
                
                s+=">"+ctx.node->name+"</"+labels[ctx.node->sub_type]+">\n";
                ctx.source = s;
                // ctx.index--;
            };

            html_handlers[input_id] = [this](Context& ctx) { 
                ctx.source = "<input "+ctx.node->name+">\n"; 
                // ctx.index--;
            };

            html_handlers[style_id] = [this](Context& ctx) {
                std::string s = "style=\"";
                for(auto c : ctx.node->children) {
                    s += c->name + ":" + c->opt_str + ";";
                }
                s+="\"";
                ctx.source = s;
            };
            html_handlers[prefix_style_id] = [this](Context& ctx) {};
            html_handlers[suffix_style_id] = [this](Context& ctx) {};

            html_handlers[properties_id] = [this](Context& ctx) {
                std::string s = "";
                for(auto c : ctx.node->children) {
                    s += c->name + "=\"" + c->opt_str + "\" ";
                }
                ctx.source = s;
            };
        }
    };
}