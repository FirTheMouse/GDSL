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
        size_t style_id = reg_id("style");

        size_t text_id = reg_id("text");
        size_t input_id = reg_id("input");

        size_t paragraph_subtype = reg_id("p");
        size_t span_subtype = reg_id("span");

        size_t property_id = reg_id("property");
        size_t properties_id = reg_id("properties");

        _lookup is_structural{{
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload",
            "role","lang","colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        Stage& html_handlers = reg_stage("emitting_html");

        std::string html_encode_node(g_ptr<Node> node) {
            Context ctx;
            ctx.node = node;
            html_handlers.run(node->type)(ctx);
            return ctx.source;
        }

        g_ptr<Node> make_jsscript(const std::string& js) {
            g_ptr<Node> s = make<Node>(script_id);
            s->opt_str = js;
            return s;
        }

        std::string emit_inline_html(Context& ctx) {
            if(ctx.node->mute) return "";
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;

            for(int q=0;q<ctx.node->quals.length();q++) {
                g_ptr<Node> qual = ctx.node->quals[q];
                for(auto c : qual->children) {
                    if(c->type==property_id) {
                        list<std::string>* prop_labels; list<std::string>* prop_values;
                        if(is_prop_structural(c->name)) {
                            prop_labels = &structural_prop_labels; 
                            prop_values = &structural_prop_values;
                        } else {
                            prop_labels = &style_prop_labels; 
                            prop_values = &style_prop_values;
                        }
                        if(q==0||!prop_labels->has(c->name)) {
                            prop_labels->push(c->name);
                            prop_values->push(c->opt_str);
                        }
                    } else {

                    }
                }
            }


            for(int i=0;i<structural_prop_labels.length();i++) {
                s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            }   
            if(!style_prop_labels.empty()) {
                s += " style=\"";
                for(int i=0;i<style_prop_labels.length();i++) {
                    s += style_prop_labels[i]+":"+style_prop_values[i]+";";
                }  
                s += "\""; 
            }
            return s;
        }

        std::string emit_inline_html(Context& ctx, g_ptr<Node> node) {
            g_ptr<Node> old_node = ctx.node;
            ctx.node = node;
            std::string s = emit_inline_html(ctx);
            ctx.node = old_node;
            return s;
        }

        std::string standard_emit_as_html(Context& ctx) {
            std::string s = "";
            s += "<"+labels[ctx.node->type];
            s += emit_inline_html(ctx);
            s += ">\n";
            for(auto c : ctx.node->children) {
                s += html_encode_node(c);
            }

            if(!ctx.node->opt_str.empty()) {
                s += ctx.node->opt_str+"\n";
            } 

            s += "</"+labels[ctx.node->type]+">\n";
            ctx.source = s;
            return ctx.source;
        }

        std::string standard_emit_as_html(Context& ctx, g_ptr<Node> node) {
            g_ptr<Node> old_node = ctx.node;
            ctx.node = node;
            standard_emit_as_html(ctx);
            ctx.node = old_node;
            return ctx.source;
        }

        std::string indent_html_text(const std::string& text) {
            list<std::string> s = split_str(text,'<');
            std::string indent = "";
            for(int i=0;i<s.length();i++) {
                if(s[i].empty()) continue;
                bool deindent = s[i].at(0)=='/';
                s[i].insert(0,"<");
                if(deindent) {
                    indent = indent.substr(0,indent.length()-2);
                    s[i].insert(0,indent);
                } else {
                    s[i].insert(0,indent);
                    indent += "  ";
                    size_t pos = 0;
                    while((pos = s[i].find('\n', pos)) != std::string::npos && pos!=s[i].length()-1) {
                        s[i].replace(pos, 1, "\n" + indent);
                        pos += indent.length() + 1;
                    }
                }
            }
            std::string to_return = "";
            for(auto si : s) to_return+=si;
            return to_return;
        };

        void init() override {
            html_handlers.default_function = [this](Context& ctx){
                standard_emit_as_html(ctx);
            };
            html_handlers[text_id] = [this](Context& ctx){
                std::string s = "";
                s+= "<"+labels[ctx.node->sub_type];
                if(ctx.node->scope()) {
                    s+= emit_inline_html(ctx,ctx.node->scope());
                }
                
                if(ctx.node->left()) {
                    process_node(ctx, ctx.node->left());
                    ctx.node->opt_str = ctx.node->left()->value->get<std::string>();
                }
                
                s+=">\n"+ctx.node->opt_str+"\n</"+labels[ctx.node->sub_type]+">\n";
                ctx.source = s;
            };
            html_handlers[input_id] = [this](Context& ctx) { 
                ctx.source = "<input "+ctx.node->opt_str+">\n"; 
            };
        }
    };
}