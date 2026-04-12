#pragma once

#include "../modules/Twig-Snap.hpp"
#include "../modules/GDSL-Web.hpp"
#include "../modules/GDSL-C.hpp"
#include "../modules/GDSL-LISP.hpp"

namespace GDSL {

    struct Test_Unit : public Web_Unit, public TwigSnap_Unit {
        size_t wub_route = make_route("/wub");
        
        bool wub_active = false;

        std::string build_page() {
            std::string to_write = "<!DOCTYPE html>\n";
            g_ptr<Node> html_root = make<Node>(html_id);
            g_ptr<Node> body = make<Node>(body_id);
            html_root->children << body;
            
            body->children << make_button("wub_btn", "Click Me", "postTo('/wub', this)");
            g_ptr<Node> target_node = make<Node>(div_id, "id=\"target\"");
            body->children << target_node;
            if(wub_active) {
                target_node->children << make<Node>(text_id, paragraph_subtype, "wub wub wub");
            }
            
            body->children << make_jsscript(
                "\nasync function postTo(route, el) {\n"
                "   const response = await fetch(route, {\n"
                "       method: 'POST',\n"
                "       body: el.id\n"
                "   });\n"
                "   document.open();\n"
                "   document.write(await response.text());\n"
                "   document.close();\n"
                "}"
            );

            int depth = 0;
            to_write += html_encode_node(html_root, depth);
            return to_write;
        }

        void init() override {
            Web_Unit::init();
            TwigSnap_Unit::init();

            g_ptr<C_Compiler> comp = make<C_Compiler>();
            comp->init();

            g_ptr<LISP_Unit> lisp = make<LISP_Unit>();
            lisp->init();
            x_handlers[root_route] = [this](Context& ctx){
                ctx.source = readFile("webtext.html");
            };
            p_handlers[root_route] = [this,comp,lisp](Context& ctx){
                print("Source received:\n", ctx.source);
                size_t first_line_idx = ctx.source.find("\n");
                std::string mode = ctx.source.substr(0, first_line_idx);
                std::string code = ctx.source.substr(first_line_idx + 1);
                if(mode=="lisp") {
                    lisp->run(lisp->process(code));
                    ctx.source = lisp->output_string;
                    lisp->output_string = "";
                } else if(mode=="c") {
                    comp->run(comp->process(code));
                    ctx.source = comp->output_string;
                    comp->output_string = "";
                }

            };

            x_handlers[wub_route] = [this](Context& ctx){
                ctx.source = build_page();
            };
            p_handlers[wub_route] = [this](Context& ctx){
                if(ctx.source == "wub_btn")
                    wub_active = !wub_active;
                ctx.source = build_page();
            };
        }

        g_ptr<Node> process(const std::string& code) override {
            g_ptr<Node> server = make<Node>();
            server->type = server_id;
            g_ptr<Node> port_node = make<Node>();
            port_node->type = port_id;
            #ifdef __APPLE__
                port_node->value->set<int>(8080);
            #else 
                port_node->value->set<int>(80);
            #endif
            server->children << port_node;
            return server;
        }

        void run(g_ptr<Node> server) override {
            Web_Unit::run(server);
        }
    };
}