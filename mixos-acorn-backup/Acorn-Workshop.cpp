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

        add_function("type_of",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t type = ctx.node().c0().value().type();
            ctx.node().value().set((void*)&type);
        },4,int_id);

        

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

        add_function("max",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().left().value().type()==float_id) {
                float num = std::max(ctx.node().getFloat(0),ctx.node().getFloat(1));
                ctx.node().value().type(float_id);
                ctx.node().value().set((void*)&num);
            } else {
                int num = std::max(ctx.node().getInt(0),ctx.node().getInt(1));
                ctx.node().value().set((void*)&num);
            }
        },int_id);
        add_function("min",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().left().value().type()==float_id) {
                float num = std::min(ctx.node().getFloat(0),ctx.node().getFloat(1));
                ctx.node().value().type(float_id);
                ctx.node().value().set((void*)&num);
            } else {
                int num = std::min(ctx.node().getInt(0),ctx.node().getInt(1));
                ctx.node().value().set((void*)&num);
            }
        },int_id);
        add_function("abs",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().left().value().type()==float_id) {
                float num = std::abs(ctx.node().getFloat(0));
                ctx.node().value().type(float_id);
                ctx.node().value().set((void*)&num);
            } else {
                float num = std::abs(ctx.node().getInt(0));
                ctx.node().value().set((void*)&num);
            }
        },int_id);



        add_function("SIM_flow_connection", [this](Context& ctx) {
            standard_sub_process(ctx);
        
            Ptr connection = ctx.node().getPtr(0);
        
            Col& concol = resolve_to_col(connection);
        
            Ptr from = *(Ptr*)resolve_ptr(*(Ptr*)concol[0]);
            Ptr to   = *(Ptr*)resolve_ptr(*(Ptr*)concol[1]);
        
            double diameter = *(float*)resolve_ptr(*(Ptr*)concol[2]);
            double length   = *(float*)resolve_ptr(*(Ptr*)concol[3]);
        
            if(diameter<=0.0 || length<=0.0) return;
        
            auto kind_amount = [&](Ptr region, const std::string& kind) -> double {
                Col& regcol = resolve_to_col(region);
                double sum = 0.0;
                Col& fluids = resolve_to_col(*(Ptr*)resolve_ptr(*(Ptr*)regcol.get(kind)));
                for(uint32_t i=0;i<fluids.length();i++) {
                    sum+=*(float*)fluids.sget(i);
                }
                return sum;
            };
        
            auto pressure_at = [&](Ptr region, double liquid, double gas) -> double {
                Col& regcol = resolve_to_col(region);
                double volume = *(float*)resolve_ptr(*(Ptr*)regcol.get("Volume"));
                double stiffness = *(float*)resolve_ptr(*(Ptr*)regcol.get("Stiffness"));
        
                double space = std::max(0.001, volume-liquid);
                double gaspressure = gas/space;
                double compression = std::max(0.0, liquid-volume)*stiffness;
        
                return gaspressure+compression;
            };
        
            double fromfluid = kind_amount(from,"Fluid");
            double fromgas = kind_amount(from,"Gas");
            double tofluid = kind_amount(to,"Fluid");
            double togas = kind_amount(to,"Gas");
        
            double pressure_difference = pressure_at(from, fromfluid, fromgas) - pressure_at(to, tofluid, togas);
        
            if(std::abs(pressure_difference)<1e-9) return;
        
            Ptr source = from;
            Ptr target = to;
        
            if(pressure_difference<0.0) {
                std::swap(source,target);
                std::swap(fromfluid,tofluid);
                std::swap(fromgas,togas);
                pressure_difference = -pressure_difference;
            }
        
            double diameter2 = diameter*diameter;
            double conductance = diameter2*diameter2/length;
            double proposed_flow = pressure_difference*conductance;
        
            double sourcevolume = *(float*)resolve_ptr(*(Ptr*)resolve_to_col(source).get("Volume"));
            double gasvolume = fromgas>0.0
                ? std::max(0.001, sourcevolume-fromfluid)
                : 0.0;
        
            double mixturevolume = fromfluid+gasvolume;
        
            if(mixturevolume<=0.0) return;
        
            double maxfraction = std::min(1.0, proposed_flow/mixturevolume);
        
            auto remaining_difference = [&](double fraction) -> double {
                double movedfluid = fromfluid*fraction;
                double movedgas = fromgas*fraction;
        
                double sourcepressure = pressure_at(
                    source,
                    fromfluid-movedfluid,
                    fromgas-movedgas
                );
        
                double targetpressure = pressure_at(
                    target,
                    tofluid+movedfluid,
                    togas+movedgas
                );
        
                return sourcepressure-targetpressure;
            };
        
            double fraction = maxfraction;
        
            // Fast path: the proposed transfer doesn't overshoot.
            if(remaining_difference(maxfraction)<0.0) {
                double low = 0.0;
                double high = maxfraction;
        
                for(int i=0; i<20; ++i) {
                    double middle = (low+high)*0.5;
        
                    if(remaining_difference(middle)>0.0) {
                        low = middle;
                    } else {
                        high = middle;
                    }
                }
        
                fraction = low;
            }
        
            if(fraction<=0.0) return;


            auto transfer_fraction = [&](Ptr source_region,Ptr target_region,const std::string& category,double fraction) {
                    Col& sourceobj = resolve_to_col(source_region);
                    Col& targetobj = resolve_to_col(target_region);

                    Col& src = resolve_to_col(*(Ptr*)resolve_ptr(*(Ptr*)sourceobj.get(category)));
                    Col& dst = resolve_to_col(*(Ptr*)resolve_ptr(*(Ptr*)targetobj.get(category)));

                    auto cells = src.allCells();

                    for(uint32_t i=0; i<cells.length(); i++) {
                        CCol* cell = cells[i];

                        std::string kind = ((QString&)*cell).to_std();

                        float& quantity = *(float*)src.sget(cell->index);

                        float transferred = (float)(quantity*fraction);

                        if(transferred<=0.0f) continue;

                        quantity -= transferred;

                        if(dst.hasKey(kind.data(),kind.size())) {
                            float& target_quantity = *(float*)dst.get(kind);
                            target_quantity += transferred;
                        } else {
                            dst.qput(
                                &transferred,
                                kind.data(),
                                kind.size(),
                                string_id
                            );
                        }
                    }
                };
        
            transfer_fraction(source,target,"Fluid",fraction);
            transfer_fraction(source,target,"Gas",fraction);
        });

        x_handlers[make_tokenized_keyword("repeat")] = [this](Context& ctx) {
            standard_sub_process(ctx);
            Node scope = ctx.node().scopes()[0];
            int times = ctx.node().getInt(0);
            for(int i=0;i<times;i++) {
                uint32_t result = standard_travel_pass(scope, ctx.sub());
                if(result > 0) {
                    uint32_t kind = result % 4;
                    if(kind == 2 || kind == 3) {//Break or continue
                        if(result>=4) {
                            result -= 4;//Consume one magnitude
                            ctx.state(result);//And propagate up
                        } 
                        if(kind == 2) {break;} //Otherwise break away
                        continue;
                    } else { //If it's a return, pass it up
                        ctx.state(result);
                        break;
                    }
                }
            }
        };  


        add_function("string_is_alphanum",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = ctx.node().getString(0);
            Col& col = s.col();
            bool b = true;
            for(uint32_t i=0;i<col.length();i++) {
                unsigned char c = *(unsigned char*)col.sget(i);
                if(!((c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z'))) {
                    b = false;
                    break;
                }
            }
    
            ctx.node().value().set((void*)&b);
        },bool_id);
        add_function("string_is_safe_filename",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = ctx.node().getString(0);
            Col& col = s.col();
            uint32_t len = col.length();
            bool b = (len>0 && len<=32);
            for(uint32_t i=0; b && i<len; i++) {
                unsigned char c = *(unsigned char*)col.sget(i);
                b = (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c=='-' || c=='_';
            }
            ctx.node().value().set((void*)&b);
        },bool_id);

        add_function("random",[this](Context& ctx){
            node_col children = ctx.node().children();
            process_node(ctx,children[randi(0,children.length()-1)]);
        });
        add_function("mult_by_factor",[this](Context& ctx){
            standard_sub_process(ctx);
            int a = ctx.node().getInt(0);
            float f = ctx.node().getFloat(1);
            int r = a*f;
            ctx.node().value().set((void*)&r);
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

        // add_function("CHECK_STACK",[this](Context& ctx){
        //     pthread_t self = pthread_self();
        //     void* stack_addr = pthread_get_stackaddr_np(self); // top of stack (highest address)
        //     size_t stack_size = pthread_get_stacksize_np(self); // total size
        
        //     int local_var;
        //     void* current_sp = &local_var;
        
        //     // stack grows down, stack_addr is the high end, so bottom = stack_addr - stack_size
        //     void* stack_bottom = (char*)stack_addr - stack_size;
        //     ptrdiff_t remaining = (char*)current_sp - (char*)stack_bottom;
        
        //     printf("Stack total: %zu bytes (%.2f MB), remaining to bottom: %ld bytes (%.2f MB)\n",
        //            stack_size, stack_size / (1024.0*1024.0),
        //            remaining, remaining / (1024.0*1024.0));
        // });

        add_function("to_raw_binary_string",[this](Context& ctx){
            standard_sub_process(ctx);
            resolve_string_ticket(ctx.node()) = to_bin(ctx.node().value().get(),ctx.node().value().size());
        },sizeof(Ptr),string_id);

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