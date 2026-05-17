#pragma once

#include "../mixos-acorn/Acorn-Script.hpp"

namespace Acorn {
    struct Acorn_Kernel : public virtual Acorn_Script {
        Acorn_Kernel() {init();}
        
        Stage& sys_handlers = reg_stage("system");

        uint32_t load_id = make_tokenized_keyword("load");
        uint32_t stop_id = make_tokenized_keyword("STOP");

        void init() override {
            // a_handlers[test_id] = [this](Context& ctx){
            //     print(ctx.node.name());
            //     print(ctx.node.children()[0].name());
            //     print(ctx.node.scopes()[0].name());
            // };

            // Context ctx;
            // ctx.node = n;

            // start_stage(a_handlers);
            // standard_process(ctx);

            // print("Hello world");

            x_handlers[load_id] = [this](Context& ctx){
                if(unit_root.idx==1) {
                    ctx.node.value(make_value(string_id,sizeof(Ptr))); //Allocate a space to recive a value
                    unit_root = ctx.node;
                    unit_root.idx = 0;
                    while(unit_root.idx==0) {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                    }
                    print("I LIVE!");
                }


                // if(!ctx.node.children().empty()) {
                //     process_node(ctx,ctx.node.children()[0]);
                //     ctx.node.value(make_value(this,string_id,sizeof(Ptr)));
                //     std::string path = string(*(Ptr*)ctx.node.children()[0].value().get(),this).to_std();
                //     uint32_t store_to = types[data_store_id].note_value(path,sizeof(char),char_id);
                //     for(auto c : readFile(path))
                //         types[data_store_id][store_to].push((void*)&c);
                //     Ptr ticket(data_store_id,store_to,0);
                //     ctx.node.value().set((void*)&ticket);
                // }
            };

            sys_handlers.default_function = [this](Context& ctx){
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            };  

            // sys_handlers[load_id] = [this](Context& ctx){
            //     if(!ctx.unit) return;
            //     Unit& acorn = *ctx.unit;
            //     if(!acorn.unit_root.live) {
            //         std::string path = string(*(Ptr*)acorn.unit_root.children()[0].value().get(),&acorn).to_std();
            //         uint32_t store_to = acorn.types[acorn.data_store_id].note_value(path,sizeof(char),char_id);
            //         for(auto c : readFile(path))
            //             acorn.types[acorn.data_store_id][store_to].push((void*)&c);
            //         Ptr ticket(acorn.data_store_id,store_to,0);
            //         acorn.unit_root.value().set((void*)&ticket);
            //         ctx.source = "sys:115 loaded file "+path+" for a process";
            //         acorn.unit_root.live = true;
            //     }
            // };
            // x_handlers[stop_id] = [this](Context& ctx){
            //     unit_root = ctx.node;
            // };
            // sys_handlers[stop_id] = [this](Context& ctx){
            //     ctx.source = "sys:122 stopping";
            //     ctx.flag = true;
            // };

            // x_handlers[make_tokenized_keyword("run")] = [this](Context& ctx){
            //     if(!ctx.node.children().empty()) {
            //         process_node(ctx,ctx.node.children()[0]);
            //         std::string torun = string(*(Ptr*)ctx.node.children()[0].value().get(),this).to_std();
            //         Acorn_Script acorn;
            //         acorn.process(torun);
            //         acorn.unit_root.children() << make_node(&acorn,stop_id);
            //         g_ptr<Thread> t = make<Thread>();
            //         t->run_blocking([&acorn]() mutable {
            //             acorn.run(acorn.unit_root);
            //         });
            //         Context actx;
            //         actx.unit = &acorn;
            //         while(true) {
            //             actx.node = acorn.unit_root;
            //             sys_handlers.run(acorn.unit_root.type())(actx);
            //             if(!actx.source.empty()) {
            //                 print(actx.source);
            //                 actx.source = "";
            //             }
            //             if(actx.flag) {
            //                 t->stop();
            //                 break;
            //             }
            //         }
            //     }
            // };
        }

        virtual Node process(std::string code) override {
            // Acorn_Script p1;
            // Acorn_Script p2;

            // p1.process(readFile("mixos-acorn/tests/p1.gld"));
            // print("P1: ");
            //     p1.run(p1.unit_root);
                
            // print("NODENETS");
            // print("P1: ");
            //     print(p1.node_to_string(p1.unit_root,2,0,true));
            
            Acorn_Script p3;
            Node root = p3.process(readFile("mixos-acorn/tests/acorntest.gld"));
            p3.run(root);


            return make_node();
        }
    
        virtual void run(Node root) override {
            
        }
    };
}