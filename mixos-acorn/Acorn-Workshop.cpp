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