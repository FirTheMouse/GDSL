#include "../mixos-acorn/Acorn-Workshop.hpp"

namespace Acorn {
    
    void Workshop_Unit::sub_init() {


        add_function("this",[this](Context& ctx){
            if(is_live(ctx.sub())) {
                ctx.node().value(ctx.sub().node().left().value());
                sync_identifier(ctx);
            }
        });
        add_function("fill_capture",[this](Context& ctx){
            standard_sub_process(ctx);
            string output = resolve_string_ticket(ctx.node());
            std::string result = ctx.node().getString(0).to_std(); //<- the run fragment
            for(int i=1;i<ctx.node().children().length();i++) {
                std::string part = ctx.node().getString(i).to_std();
                size_t at = result.find(",,");
                if(at == std::string::npos) {
                    throw_error("fill_capture: not enough empty capture slots");
                    return;
                }
                result.insert(at + 1, part);
            }
            output = result;
        },sizeof(Ptr),string_id);

        

        add_function("run_in_new_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            g_ptr<Workshop_Unit> workshop = make_unit<Workshop_Unit>();
            workshop->uargs << uargs;
            std::string unitcode = ctx.node().getString(0).to_std();
            workshop->start_thread([workshop, unitcode]() mutable {
                workshop->run(workshop->process(unitcode));
            });
        });

        add_function("is_value_ptr",[this](Context& ctx){
            standard_sub_process(ctx);
            bool b = is_ptr_alias(ctx.node().c0().value().type());
            ctx.node().set((void*)&b);
        },1,bool_id);
        add_function("YAPA_level_of_value",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t myid = ctx.node().c0().value().type();
            int i = -1;
            for(int YAPA_level=0;YAPA_level<YAPAs.length();YAPA_level++) {
                if(i>0) {break;}
                for(int y=0;y<YAPAs[YAPA_level].length();y++) {
                    if(YAPAs[YAPA_level][y]==myid) {
                        i = YAPA_level; break;
                    }
                }
            }
            ctx.node().set((void*)&i);
        },4,int_id);

        r_handlers[group_id] = [this](Context& ctx){
            if(is_live(ctx.node().value()) && ctx.node().value().type() != 0) return;
            standard_sub_process(ctx);
            resolve_overload(ctx);
            if(!is_live(ctx.node().value())) {
                if(ctx.node().children().length()>0) {
                    Value firstval = ctx.node().c0().value();
                    ctx.node().value(make_value(ptr_id,sizeof(Ptr),0,firstval.type(),firstval.size()));
                } else {
                    ctx.node().value(make_value(ptr_id,sizeof(Ptr),0,duck_id,0));
                }
            }
        };
        x_handlers[group_id] = [this](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            
            uint32_t children_count = ctx.node().children().length();
            if(ctx.node().sub_type()==duck_id) {
                for(uint32_t i=0;i<children_count;i++) {
                    Value childval = ctx.node().children()[i].value();
                    if(childval.type()!=duck_id) {
                        ctx.node().value().sub_type(childval.type());
                        ctx.node().value().sub_size(childval.size());
                        break;
                    }
                }
            }
            if(ctx.node().sub_type()==duck_id) {
                throw_error("Group can not intilize because none of it's values have a discernible type");
                return;
            }   

            Col& col = resolve_to_col(resolve_ticket(ctx.node(),ctx.node().value().sub_size(),ctx.node().value().sub_type()));
            col.clear();
            col.reserve(children_count*ctx.node().value().sub_size());
            for(uint32_t i=0;i<children_count;i++) {
                col.push(ctx.node().get(i));
            }
        };

        add_function("children_to_string",[this](Context& ctx){
            resolve_string_ticket(ctx.node()) = children_to_string(ctx.node().getContext(0));
        },sizeof(Ptr),string_id);

        add_function("CHECK_STACK",[this](Context& ctx){
            pthread_t self = pthread_self();
            void* stack_addr = pthread_get_stackaddr_np(self); // top of stack (highest address)
            size_t stack_size = pthread_get_stacksize_np(self); // total size
        
            int local_var;
            void* current_sp = &local_var;
        
            // stack grows down, stack_addr is the high end, so bottom = stack_addr - stack_size
            void* stack_bottom = (char*)stack_addr - stack_size;
            ptrdiff_t remaining = (char*)current_sp - (char*)stack_bottom;
        
            printf("Stack total: %zu bytes (%.2f MB), remaining to bottom: %ld bytes (%.2f MB)\n",
                   stack_size, stack_size / (1024.0*1024.0),
                   remaining, remaining / (1024.0*1024.0));
        });

        add_function("pebble_refragment",[this](Context& ctx){
            standard_sub_process(ctx);
            string fragment = ctx.node().getString(0);
            string nodeid = ctx.node().getString(1);
            string domid = ctx.node().getString(2);
            string source_text = ctx.node().getString(3);

            g_ptr<Workshop_Unit> compiler = nullptr;
            {
                std::lock_guard<std::mutex> lock(units_mutex);
                for(int i=uid;i<units.length();i++) {
                    if(units[i]->unit_label == "FirsCompiler") {
                        compiler = as<Workshop_Unit>(units[i]);
                        break;
                    }
                }
            }
            if(!compiler) {
                compiler = make_unit<Workshop_Unit>();
                compiler->unit_label = "FirsCompiler";
            }

            print("Compiler at ",compiler->uid);

            Node root = compiler->process(source_text.to_std());
            compiler->compile(root);

            print(compiler->node_to_string(root));
            if(compiler->UERROR_FLAG) {
                for(auto& s : compiler->UERRORS) {
                    print(red("UERROR: "),s);
                }
                compiler->UERRORS.clear();
                compiler->UERROR_FLAG = false;
            }

            std::string html = "";
            // walk_handlers.default_function = [&](Context& ctx){
            //     html+="<span class=\""+labels[ctx.node().type()]+"\">"+ctx.node().name().to_std()+"</span>";
            //     standard_sub_process(ctx);
            //     if(is_live(ctx.node().scope())) {
            //         standard_travel_pass(ctx.node().scope(),ctx.sub());
            //     }
            // };
            // standard_travel_walk(root);

            html = fnodenet_to_string(root,Stamper{[this](Node n, list<int>& offsets){
                std::string to_return = n.name().to_std();
                if(n.type()!=0) {
                    std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
                    while((int)n.y()>=offsets.length()) {offsets<<0;}
                    n.x(n.x()+offsets[(int)n.y()]);
                    offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                    to_return = nreturn;
                }
                return to_return;
            },[this](Node n){
                list<Node> stamps;
                map<uint64_t,bool> visited;
                collect_stamps(n,stamps,visited);
                return stamps;
            }});
            std::string behaviour = fill_capture(fragment.to_std(),nodeid.to_std(),"'"+domid.to_std()+"'","this.innerText");
            // resolve_string_ticket(ctx.node()) = "<div id=\""+domid.to_std()+"\"><div contenteditable=\"true\" oninput=\""+behaviour+"\">"+html+"</div></div>";
            resolve_string_ticket(ctx.node()) = html;
        },sizeof(Ptr),string_id);
    }
}