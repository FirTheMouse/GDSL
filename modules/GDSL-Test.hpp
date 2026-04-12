#pragma once

#include "../modules/GDSL-Web.hpp"
#include "../modules/GDSL-C.hpp"
#include "../modules/GDSL-LISP.hpp"

namespace GDSL {

    struct Test_Unit : public Web_Unit {
        size_t wub_route = make_route("/wub");
        
        void init() override {
            Web_Unit::init();

            x_handlers[root_route] = [this](Context& ctx){
                ctx.source = readFile("webtext.html");
            };
            x_handlers[wub_route] = [this](Context& ctx){
                ctx.source = "wub wub";
            };
    
            g_ptr<C_Compiler> comp = make<C_Compiler>();
            comp->init();

            g_ptr<LISP_Unit> lisp = make<LISP_Unit>();
            lisp->init();
    
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
    
            p_handlers[wub_route] = [this](Context& ctx){
                ctx.source = "wub wub";
            };
        }
    };


}