#pragma once

#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Object.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    
    struct Context {
        Context() : index(_ctx_dummy_index) {}
        Context(int& _index) : index(_index) {}
        Context(Col& _result, int& _index) : result(&_result), index(_index) {}
    
        Ptr node;
        Ptr qual;
        Ptr left;
        Ptr out;
        Ptr root;
        Col* result = nullptr;
        list<Ptr> nodes;
        Ptr value;
        int& index;
        uint32_t state = 0;
        bool flag = false;
    
        Context* sub;
    
        std::string source;
    
        Context duplicate() {
            return Context((*result), index);
        }
    };

    using Handler = std::function<void(Context&)>;
        
    struct Unit : public q_object {
    
        map<uint32_t,std::string> labels;
        list<TypePool> types;
    
        const uint32_t node_type_id = 0; TypePool node_type = make_node_type(); 
        const uint32_t value_type_id = 1; TypePool value_type = make_value_type(); 
        const uint32_t handler_type_id = 2; TypePool handler_type = make_handler_type(); 

        const uint32_t name_store_id = 3; TypePool name_store = make_store_type(); 
        const uint32_t children_store_id = 4; TypePool children_store = make_store_type(); 
        const uint32_t quals_store_id = 5; TypePool quals_store = make_store_type(); 
        const uint32_t node_table_store_id = 6; TypePool node_table_store = make_store_type(); 
        const uint32_t value_table_store_id = 7; TypePool value_table_store = make_store_type(); 
        const uint32_t scopes_store_id = 8; TypePool scopes_store = make_store_type(); 
        const uint32_t opt_str_store_id = 9; TypePool opt_str_store = make_store_type(); 

        TypePool new_type() {
            TypePool to_return;
            types << to_return;
            return to_return;
        }

        Ptr new_type_handle() {
            TypePool new_type;
            Ptr to_return{(uint32_t)types.length(),0,0};
            types << new_type;
            return to_return;
        }

        void set_handler(const Ptr& ptr, const Ptr& handle) {
            handler_type.set(ptr.idx, ptr.sidx, (void*)&handle);
        }

        size_t undefined_id = 0;
        size_t pointer_id = 1;

        uint32_t reg_id(const std::string& label) {
            uint32_t at = handler_type.columns.length();
            Ptr ptr{handler_type_id,at,0};
            handler_type.new_column(label,&ptr,sizeof(Ptr),pointer_id);
            labels.put(at,label);
            return at;
        }

        size_t float_id = reg_id("float");
        size_t int_id = reg_id("int");
        size_t bool_id = reg_id("bool");
        size_t string_id = reg_id("string");


        TypePool make_handler_type() {
            TypePool t;
            Ptr undefined_ptr{handler_type_id,0,0};
            t.new_column("UNDEFINED",&undefined_ptr,sizeof(Ptr),0);
            Ptr ptr_ptr{handler_type_id,1,0};
            t.new_column("ptr",&ptr_ptr,sizeof(Ptr),1);
            return t;
        }

        TypePool make_store_type() {
            TypePool t;
            return t;
        }
        
        size_t tombstone_col = 0; 
        size_t refs_col = 0;

        TypePool make_object_type() {
            TypePool t;
            tombstone_col = t.new_column<bool>("tombstone",false,bool_id);
            refs_col = t.new_column<int>("refs",0,int_id);
            return t;
        }

        size_t list_id = 0;
        size_t map_id = 0;
        size_t weakptr_id = 0;

        size_t type_col = 0;
        size_t sub_type_col = 0;
        size_t name_col = 0;
        size_t x_col = 0;
        size_t y_col = 0;
        size_t z_col = 0;
        size_t value_col = 0;
        size_t children_col = 0;
        size_t quals_col = 0;
        size_t node_table_col = 0;
        size_t value_table_col = 0;
        size_t scopes_col = 0;
        size_t parent_col = 0;
        size_t owner_col = 0;
        size_t in_scope_col = 0;
        size_t is_scope_col = 0;
        size_t opt_str_col = 0;
        size_t mute_col = 0;

        TypePool make_node_type() {
            TypePool t = make_object_type();
            type_col = t.note_value("type",4,int_id).index;
            sub_type_col = t.note_value("sub_type",4,int_id).index;
            name_col = t.note_value("name",sizeof(Ptr),string_id).index;
            x_col = t.note_value("x",4,float_id).index;
            y_col = t.note_value("y",4,float_id).index;
            z_col = t.note_value("z",4,float_id).index;
            value_col = t.note_value("value",sizeof(Ptr),pointer_id).index;
            children_col = t.note_value("children",sizeof(Ptr),list_id).index;
            quals_col = t.note_value("quals",sizeof(Ptr),list_id).index;
            node_table_col = t.note_value("node_table",sizeof(Ptr),map_id).index;
            value_table_col = t.note_value("value_table",sizeof(Ptr),map_id).index;
            scopes_col = t.note_value("scopes",sizeof(Ptr),list_id).index;
            parent_col = t.note_value("parent",sizeof(Ptr),weakptr_id).index;
            owner_col = t.note_value("owner",sizeof(Ptr),weakptr_id).index;
            in_scope_col = t.note_value("in_scope",sizeof(Ptr),weakptr_id).index;
            is_scope_col = t.note_value("is_scope",1,bool_id).index;
            opt_str_col = t.note_value("opt_str",sizeof(Ptr),string_id).index;
            mute_col = t.note_value("mute",1,bool_id).index;

            t.init_funcs << [this,&t](Object& obj) {
                Ptr nameptr{name_store_id, (uint32_t)name_store.add_column(sizeof(Ptr)), 0};
                t.set(name_col, obj.sidx, (void*)&nameptr);
            
                Ptr childrenptr{children_store_id, (uint32_t)children_store.add_column(sizeof(Ptr)), 0};
                t.set(children_col, obj.sidx, (void*)&childrenptr);
            
                Ptr qualsptr{quals_store_id, (uint32_t)quals_store.add_column(sizeof(Ptr)), 0};
                t.set(quals_col, obj.sidx, (void*)&qualsptr);
            
                Ptr scopesptr{scopes_store_id, (uint32_t)scopes_store.add_column(sizeof(Ptr)), 0};
                t.set(scopes_col, obj.sidx, (void*)&scopesptr);
            
                Ptr nodetableptr{node_table_store_id, (uint32_t)node_table_store.add_column(sizeof(Ptr)), 0};
                t.set(node_table_col, obj.sidx, (void*)&nodetableptr);
            
                Ptr valuetableptr{value_table_store_id, (uint32_t)value_table_store.add_column(sizeof(Ptr)), 0};
                t.set(value_table_col, obj.sidx, (void*)&valuetableptr);
            
                Ptr optstrptr{opt_str_store_id, (uint32_t)opt_str_store.add_column(sizeof(char)), 0};
                t.set(opt_str_col, obj.sidx, (void*)&optstrptr);
        
                Ptr valueptr{value_type_id, 0, 0};
                t.set(value_col, obj.sidx, (void*)&valueptr);
        
                Ptr noptr{0, 0, 0};
                t.set(parent_col, obj.sidx, (void*)&noptr);
                t.set(owner_col, obj.sidx, (void*)&noptr);
                t.set(in_scope_col, obj.sidx, (void*)&noptr);
        
                bool f = false;
                t.set(is_scope_col, obj.sidx, (void*)&f);
                t.set(mute_col, obj.sidx, (void*)&f);
        
                float zero = 0.0f;
                t.set(x_col, obj.sidx, (void*)&zero);
                t.set(y_col, obj.sidx, (void*)&zero);
                t.set(z_col, obj.sidx, (void*)&zero);
            };
            return t;
        }

        TypePool make_value_type() {
            TypePool t = make_object_type();

            return t;
        }

        inline void* resolve_ptr(const Ptr& ptr) {
            return types[ptr.pool].columns[ptr.idx].get(ptr.sidx);
        }

        inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {
            return types[ptr.pool].columns[idx].get(ptr.sidx);
        }

        inline Ptr& resolve_to_ptr(const Ptr& ptr) {
            return *(Ptr*)types[ptr.pool].columns[ptr.idx].get(ptr.sidx);
        }

        inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {
            return *(Ptr*)types[ptr.pool].columns[idx].get(ptr.sidx);
        }

        inline Col& resolve_to_col(const Ptr& ptr) {
            return types[ptr.pool].columns[ptr.idx];
        }

        inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {
            return types[ptr.pool].columns[idx];
        }

        Ptr make_node() {
            return Ptr{node_type_id,0,node_type.create().sidx};
        }

        struct Stage : q_object {
            Stage() {
                default_function = [](Context& ctx){};
            }
    
            map<uint32_t,Handler> handlers;
            Handler default_function;
    
            std::string label;
    
            bool has(uint32_t key){
                return handlers.hasKey(key);
            }
    
            Handler& run(uint32_t key){
                return handlers.getOrDefault(key,default_function);
            }
    
            Handler& getOrDefault(uint32_t key, Handler& fallback){
                return handlers.getOrDefault(key,fallback);
            }
    
            Handler& operator[](uint32_t key) {
                return handlers[key];
            }
        };
    
        map<std::string,g_ptr<Stage>> stages;
    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);
            return *new_stage.getPtr();
        }
    
        Stage& a_handlers = reg_stage("assembling");
        Stage& s_handlers = reg_stage("scoping");
        Stage& t_handlers = reg_stage("typing");
    
        Stage& d_handlers = reg_stage("discovering");
        Stage& r_handlers = reg_stage("resolving");
        Stage& e_handlers = reg_stage("evaluating");
    
        Stage& m_handlers = reg_stage("modeling");
        Stage& i_handlers = reg_stage("inspecting");
        Stage& x_handlers = reg_stage("executing");
    
        Stage* active_stage;
    
        void start_stage(Stage& stage) {
            active_stage = &stage;
        }
    
        void start_stage(g_ptr<Stage> stage_ptr) {
            start_stage(*stage_ptr.getPtr());
        }



        std::string value_info(Ptr& value) {
            std::string to_return = "";
            // to_return += "["+ptr_to_string(value.getPtr())+"]"
            // + "(type: " + labels[value->type]
            // + (value->reg!=-1?", reg: "+std::to_string(value->reg):"")
            // + (value->data?", value: "+value_as_string(value)+" @"+ptr_to_string(value->data):"")
            // + (value->sub_type!=0?", sub_type: "+labels[value->sub_type]:"")
            // + (value->type_scope?", has scope":"")
            // + (value->size!=0?", size: "+std::to_string(value->size):"")
            // + (value->address!=0?", address: "+std::to_string(value->address):"")
            // + (value->store?", has store":"")
            // + (!value->sub_values.empty()?", sub: "+std::to_string(value->sub_values.length()):"");
            // if(!value->quals.empty()) {
            //     to_return += ", Quals: ";
            //     for(int i=0;i<value->quals.length();i++) { //Drop in v +ptr_to_string(value->quals[i].getPtr())
            //         to_return += labels[type_of_node(value->quals[i])]+(i!=value->quals.length()-1?", ":"");
            //     }
            // }
            // to_return += ")";
            return to_return;
        }
        
        std::string node_info(const Ptr& node) {
            std::string to_return = "";
            
            to_return += labels[node.idx];
            // + (node->sub_type==0?"":":"+labels[node->sub_type])
            // + (node->name.empty()?"":" "+green(node->name)+" ")  
            // + (node->value?value_info(node->value):"")
            // + (node->x!=-1.0f?"("+std::to_string((int)node->x)+","+std::to_string((int)node->y)+")":"")
            // + (!node->children.empty()?"[C:"+std::to_string(node->children.length())+"]":"")  
            // + (node->in_scope?"{"+node->in_scope->name+"}":"");
            return to_return;
        }
    
        #define MUTE_TABLES 1
        #define MUTE_MUTED_QUALS 1
    
        std::string node_to_string(const Ptr& node, int depth = 0, int index = 0, bool print_sub_scopes = false) {
            std::string indent(depth * 2, ' ');
            std::string to_return = "";
            
            // to_return += indent + std::to_string(index) + ": " + node_info(node);
            
            // if(!node->quals.empty()) {
            //     to_return += " [";
            //     for(int i=0;i<node->quals.length();i++) {
            //         #if MUTE_MUTED_QUALS
            //         if(!node->quals[i]->mute)
            //             to_return += labels[node->quals[i]->type]+(i!=node->quals.length()-1?", ":"");
            //         #else
            //             to_return += labels[node->quals[i]->type]+(i!=node->quals.length()-1?", ":"");
            //         #endif
            //     }
            //     to_return += "]";
            // }
    
            // #if !MUTE_TABLES 
            // if(node->value_table.size()>0) {
            //     to_return += "\n" + indent + "   Value table:";
            //     for(auto [key,val] : node->value_table.entrySet()) {
            //         to_return += "\n" + indent + "     Key: "+key+" | "+value_info(val);
            //     }
            // }
    
            // if(node->node_table.size()>0) {
            //     to_return += "\n" + indent + "   Node table:";
            //     for(auto [key,val] : node->node_table.entrySet()) {
            //         to_return += "\n" + indent + "     Key: "+key+" | "+node_info(val);
            //     }
            // }
            // #endif
        
    
    
            // if(!node->opt_str.empty()) {
            //     to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
            // }
    
            // if(!node->scopes.empty()) {
            //     to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node->scopes.size());
            //     int i = 0;
            //     for(auto& scope : node->scopes) {
            //         to_return += "\n" + indent; 
            //         if(scope) {
            //             if(print_sub_scopes) {
            //                 to_return += node_to_string(scope, depth + 3,index,print_sub_scopes);
            //             }
            //             else {
            //                 to_return += "    " + scope->name;
            //             }
            //         } else {
            //             to_return += "[NULL]";
            //         }
            //     }
            // }
            // if(node->owner) {
            //     to_return +=  "\n" + indent + "  Owner: " + node_info(node->owner);
            // }
    
            // if(!node->children.empty()) {
            //     //to_return += " ["+std::to_string(children.length())+"]";
            //     for(int i=0;i<node->children.length();i++) {
            //         if(node->children[i]) {
            //             to_return += "\n " + node_to_string(node->children[i], depth + 1, i, print_sub_scopes);
            //         }
            //         else {
            //             to_return += "\n" + indent + "[NULL]";
            //         }
            //     }
            // }
        
            return to_return;
        }


        virtual void init() {
            types << node_type;
            types << value_type;
            types << handler_type;

            types << name_store;
            types << children_store;
            types << quals_store;
            types << node_table_store;
            types << value_table_store;
            types << scopes_store;
            types << opt_str_store;

            labels[undefined_id] = "UDEFINED";
            labels[pointer_id] = "ptr";
        }
    
        virtual g_ptr<Node> process(const std::string& path) {
            return nullptr;
        }
    
        virtual void run(g_ptr<Node> root) {
            
        }
    
        inline void standard_process(Context& ctx) {
            newline(active_stage->label+": "+node_info(ctx.node));
            active_stage->run(ctx.node.idx)(ctx);
            endline();
        }
    
        inline void standard_process(Context& ctx, size_t type) {
            active_stage->run(type)(ctx);
        }

        void process_node(Context& ctx, Ptr& node) {
            Ptr saved_node = ctx.node;
            Context* saved_sub = ctx.sub;
            ctx.node = node;
            standard_process(ctx);
            ctx.node = saved_node;
            ctx.sub = saved_sub;
        }
    
        void process_node(Context& ctx, Ptr& node, Ptr& left) {
            Ptr saved_node = ctx.node;
            Ptr saved_left = ctx.left;
            Context* saved_sub = ctx.sub;
            ctx.node = node;
            ctx.left = left;
            standard_process(ctx);
            ctx.node = saved_node;
            ctx.left = saved_left;
            ctx.sub = saved_sub;
        }
    
        void process_node(Ptr& node, Ptr& left) {
            Context ctx;
            process_node(ctx,node,left);
        }
    
        void standard_sub_process_node(Ptr& root) {
            Context ctx;
            ctx.node = root;
            standard_sub_process(ctx);
        }


    
        void standard_sub_process(Context& ctx) {
            int i = 0;
            Context sub_ctx(resolve_to_col(resolve_to_ptr(ctx.node,children_col)),i);
            sub_ctx.root = ctx.node;
            sub_ctx.sub = ctx.sub;
            while(i < sub_ctx.result->length()) {
                if(i==0) {
                    process_node(sub_ctx, *(Ptr*)sub_ctx.result->get(i));
                } else {
                    process_node(sub_ctx, *(Ptr*)sub_ctx.result->get(i), *(Ptr*)sub_ctx.result->get(i-1));
                }
                i++;
            }
            ctx.flag = sub_ctx.flag;
        }

        void backwards_sub_process(Context& ctx) { 
            Col& children = resolve_to_col(resolve_to_ptr(ctx.node,children_col));
            int i = children.length()-1;
            Context sub_ctx(children,i);
            sub_ctx.root = ctx.node;
            sub_ctx.sub = ctx.sub;
            while(i >= 0) {
                if(i==children.length()-1) {
                    process_node(sub_ctx, *(Ptr*)sub_ctx.result->get(i));
                } else {
                    process_node(sub_ctx, *(Ptr*)sub_ctx.result->get(i), *(Ptr*)sub_ctx.result->get(i+1));
                }
                i--;
            }
            ctx.flag = sub_ctx.flag;
        }
    
        // void fire_quals(Context& ctx, g_ptr<Value> value) {
        //     g_ptr<Value> saved_value = ctx.value;
        //     ctx.value = value;
        //     for(auto qual : value->quals) {
        //         if(qual->mute) continue;
        //         ctx.qual = qual;
        //         if(active_stage->has(qual->type+1))
        //             (*active_stage)[qual->type+1](ctx);
        //     }
        //     ctx.value = saved_value;
        // }
        // void fire_quals(Context& ctx, g_ptr<Node> node) {
        //     g_ptr<Node> saved_node = ctx.node;
        //     ctx.node = node;
        //     for(auto qual : node->quals) {
        //         if(qual->mute) continue;
        //         ctx.qual = qual;
        //         if(active_stage->has(qual->type+2))
        //             (*active_stage)[qual->type+2](ctx);
        //     }
        //     ctx.node = saved_node;
        // }
    
        // g_ptr<Node> scan_for_node_via_node_table(const std::string& label, g_ptr<Node> from) {
        //     if(from->node_table.hasKey(label)) {
        //         return from->node_table.get(label);
        //     } else {
        //         for(auto scope : from->scopes) {
        //             g_ptr<Node> found = scan_for_node_via_node_table(label,scope);
        //             if(found) {
        //                 return found;
        //             }
        //         }
        //         return nullptr;
        //     }
        // }
    
        // g_ptr<Node> scan_for_node(const std::string& label, g_ptr<Node> from) {
        //         for(auto c : from->children) {
        //             if(c->name==label) {
        //                 return c;
        //             }
        //         }
        //         for(auto scope : from->scopes) {
        //             g_ptr<Node> found = scan_for_node(label,scope);
        //             if(found) {
        //                 return found;
        //             }
        //         }
        //         return nullptr;
        // }
    
        // g_ptr<Node> find_scope(g_ptr<Node> start, std::function<bool(g_ptr<Node>)> check) {
        //     g_ptr<Node> on_scope = start;
        //     while(true) {
        //         if(check(on_scope)) {
        //             return on_scope;
        //         }
        //         for(auto c : on_scope->scopes) {
        //             if(c==start||c==on_scope) continue;
        //             if(check(c)) {
        //             return c;
        //             }
        //         }
        //         if(on_scope->parent) {
        //             on_scope = on_scope->parent;
        //         }
        //         else 
        //             break;
        //     }
        //     return nullptr;
        // }
    
        // g_ptr<Node> find_scope_name(const std::string& match,g_ptr<Node> start) {
        //     return find_scope(start,[match](g_ptr<Node> c){
        //         return c->name == match;
        //     });
        // }
    
        // void discover_symbol(g_ptr<Node> node, g_ptr<Node> root) {
        //     Context ctx;
        //     ctx.root = root;
        //     ctx.node = node;
        //     newline("Discovering: "+node_info(node));
        //     d_handlers.run(node->type)(ctx);
        //     endline();
        // }
    
        // void discover_symbols(g_ptr<Node> root) {
        //     newline("Discover symbols pass over "+std::to_string(root.children.size())+" nodes");   
        //     for(int i = 0; i < root.children.size(); i++) {
        //         discover_symbol(root.children[i], root);
        //     }
            
        //     for(auto child_scope : root.scopes) {
        //         discover_symbols(child_scope);
        //     }
        //     endline();
        // }
        
        // void standard_resolving_pass(g_ptr<Node> root) {
        //     newline("Resolving pass over "+std::to_string(root.children.size())+" nodes");
        //     int i = 0;
        //     Context ctx(root.children,i);
        //     ctx.root = root;
        //     while(i < root.children.size()) {
        //         if(root.children[i]->scope()) {
        //             ctx.node = root.children[i];
        //             standard_process(ctx);
        //             ctx.left = root.children[i];
        //         }
        //         i++;
        //     }
        //     i = 0;
        //     while(i < root.children.size()) {
        //         if(!root.children[i]->scope()) {
        //             ctx.node = root.children[i];
        //             standard_process(ctx);
        //             ctx.left = root.children[i];
        //         }
        //         i++;
        //     }
        //     i = 0;
        //     while(i < root.children.size()) {
        //         if(root.children[i]->scope()) {
        //             standard_sub_process_node(root.children[i]);
        //         }
        //         i++;
        //     }
        //     for(auto child_scope : root.scopes) {
        //         standard_resolving_pass(child_scope);
        //     }
        //     endline();
        // }
    
        void standard_direct_pass(Ptr& root) {
            Col& children = resolve_to_col(resolve_to_ptr(root,children_col));
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            int i = 0;
            Context ctx(children,i);
            ctx.root = root;
            while(i < ctx.result->length()) {
                ctx.node = *(Ptr*)ctx.result->get(i);
                standard_process(ctx);
                ctx.left = *(Ptr*)ctx.result->get(i);
                i++;
            }
    
            Col& scopes = resolve_to_col(resolve_to_ptr(root,children_col));
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(*(Ptr*)scopes.get(i));
            }
            endline();
        }
    
        //Returns true if flagged for a return/break
        // bool standard_travel_pass(g_ptr<Node> root, Context* sub = nullptr) {
        //     newline("Travel pass over "+std::to_string(root.children.size())+" nodes");
        //     int i = 0;
        //     Context ctx(root.children, i);
        //     ctx.root = root;
        //     if(sub) ctx.sub = sub;
        //     while(i < root.children.size()) {
        //         ctx.node = root.children[i];
        //         standard_process(ctx);
        //         ctx.left = root.children[i];
        //         if(ctx.flag) { //This is the return/break process
        //             endline();
        //             return true;
        //         }
        //         i++;
        //     }
        //     endline();
        //     return false;
        // }
    
        // void standard_backwards_pass(g_ptr<Node> root){
        //     newline("Backwards pass over "+std::to_string(root.children.size())+" nodes");
        //     Context ctx;
        //     ctx.root = root;
        //     ctx.result = &root.children;
        //     for(int i=root.children.length()-1;i>=0;i--) {
        //         ctx.index = i;
        //         ctx.node = root.children[i];
        //         standard_process(ctx);
        //         if(!ctx.node) {
        //             root.children.removeAt(i);
        //         } else {
        //             for(auto scope : root.children[i]->scopes) {
        //                 if(scope->owner&&scope->owner==root.children[i]) {
        //                     standard_backwards_pass(scope);
        //                 }
        //             }
        //             ctx.left = root.children[i];
        //         }
        //     }
        //     endline();
        // }
    
        // void memory_backwards_pass(g_ptr<Node> root){
        //     newline("Memory pass over "+std::to_string(root.children.size())+" nodes");
        //     Context ctx;
        //     ctx.root = root;
        //     ctx.result = &root.children;
    
        //     for(int i=root.children.length()-1;i>=0;i--) {
        //         ctx.node = root.children[i];
        //         for(auto scope : root.children[i]->scopes) {
        //             if(scope->owner==root.children[i])
        //                 memory_backwards_pass(scope);
        //         }
        //         standard_process(ctx);
        //         ctx.left = root.children[i];
        //     }
        //     endline();
        // }
    
        // list<g_ptr<Node>> node_buffer;
        // list<g_ptr<Value>> value_buffer;
        // list<g_ptr<Type>> type_buffer;
    
        // void serialize_node(g_ptr<Node> node) {
        //     if(node->save_idx==-1) {
        //         node->save_idx = node_buffer.length();
        //         node_buffer << node;
    
        //         if(node->value) serialize_value(node->value);
    
        //         for(auto e : node->value_table.entrySet()) {
        //             serialize_value(e.value);
        //         }
    
        //         for(auto q : node->quals) {
        //             serialize_node(q);
        //         }
    
        //         list<g_ptr<Node>> to_serialize; 
        //         to_serialize << node->children;
        //         to_serialize << node->scopes;
        //         for(auto s: to_serialize) {
        //             serialize_node(s);
        //         }
        //     }
        // }
    
        // void serialize_type(g_ptr<Type> type) {
        //     if(type.save_idx == -1) {
        //         type.save_idx = type_buffer.length();
        //         type_buffer << type;
        //     }
        // }
    
        // void serialize_value(g_ptr<Value> value) {
        //     if(value->save_idx == -1) {
        //         value->save_idx = value_buffer.length();
        //         value_buffer << value;
                
        //         for(auto q : value->quals) {
        //             serialize_node(q);
        //         }
        //         for(auto v : value->sub_values) {
        //             serialize_value(v);
        //         }
    
        //         if(value->type_scope) serialize_node(value->type_scope);
        //         if(value->store) serialize_type(value->store);
        //     }
        // }
    
        // void write_value(g_ptr<Value> value, std::ostream& out) {
        //     write_raw(out, value->type);
        //     write_raw(out, value->sub_type);
        //     write_raw(out, value->size);
        //     write_raw(out, value->sub_size);
        //     write_raw(out, value->reg);
        //     write_raw(out, value->address);
        //     write_raw(out, value->loc);
    
        //     bool has_data = value->data != nullptr;
        //     write_raw(out, has_data);
        //     if(has_data) {
        //         out.write((const char*)value->data, value->size);
        //     }
    
        //     write_raw<uint32_t>(out, value->quals.length());
        //     for(auto q : value->quals) {
        //         write_raw<int>(out, q->save_idx);
        //     }
    
        //     write_raw<uint32_t>(out, value->sub_values.length());
        //     for(auto v : value->sub_values) {
        //         write_raw<int>(out, v->save_idx);
        //     }
    
        //     write_raw<int>(out, value->type_scope ? value->type_scope->save_idx : -1);
    
        //     write_raw<int>(out, value->store ? value->store.save_idx : -1);
        // }
    
        // void write_node(g_ptr<Node> node, std::ostream& out) {
        //     write_raw(out, node->type);
        //     write_string(out, node->name);
        //     write_raw<float>(out, node->x);
        //     write_raw<float>(out, node->y);
        //     write_raw<float>(out, node->z);
        //     write_string(out, node->opt_str);
        //     write_raw(out, node->is_scope);
    
        //     write_raw<int>(out, node->value ? node->value->save_idx : -1);
    
        //     write_raw<uint32_t>(out, node->children.length());
        //     for(auto c : node->children) {
        //         write_raw<int>(out, c->save_idx);
        //     }
    
        //     write_raw<uint32_t>(out, node->scopes.length());
        //     for(auto s : node->scopes) {
        //         write_raw<int>(out, s->save_idx);
        //     }
    
        //     write_raw<uint32_t>(out, node->quals.length());
        //     for(auto q : node->quals) {
        //         write_raw<int>(out, q->save_idx);
        //     }
    
        //     write_raw<uint32_t>(out, node->value_table.size());
        //     for(auto e : node->value_table.entrySet()) {
        //         write_string(out, e.key);
        //         write_raw<int>(out, e.value ? e.value->save_idx : -1);
        //     }
    
        //     write_raw<uint32_t>(out, node->node_table.size());
        //     for(auto e : node->node_table.entrySet()) {
        //         write_string(out, e.key);
        //         write_raw<int>(out, e.value ? e.value->save_idx : -1);
        //     }
    
        //     write_raw<int>(out, node->parent ? node->parent.save_idx : -1);
        //     write_raw<int>(out, node->owner ? node->owner->save_idx : -1);
        //     write_raw<int>(out, node->in_scope ? node->in_scope->save_idx : -1);
        // }
    
        // void read_value(g_ptr<Value> value, std::istream& in, map<uint32_t,uint32_t>& id_remap) {
        //     value->type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        //     value->sub_type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        //     value->size = read_raw<size_t>(in);
        //     value->sub_size = read_raw<int>(in);
        //     value->reg = read_raw<int>(in);
        //     value->address = read_raw<int>(in);
        //     value->loc = read_raw<int>(in);
    
        //     bool has_data = read_raw<bool>(in);
        //     if(has_data) {
        //         value->data = malloc(value->size);
        //         in.read((char*)value->data, value->size);
        //     }
    
        //     uint32_t qual_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < qual_count; i++) {
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) value->quals << node_buffer[idx];
        //     }
    
        //     uint32_t sub_value_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < sub_value_count; i++) {
        //         int idx = read_raw<int>(in);
        //         if(idx >= 0) value->sub_values << value_buffer[idx];
        //     }
    
        //     int type_scope_idx = read_raw<int>(in);
        //     value->type_scope = type_scope_idx != -1 ? node_buffer[type_scope_idx].getPtr() : nullptr;
    
        //     int store_idx = read_raw<int>(in);
        //     value->store = store_idx != -1 ? type_buffer[store_idx] : nullptr;
        // }
    
        // void read_node(g_ptr<Node> node, std::istream& in, map<uint32_t,uint32_t>& id_remap) {
        //     node->type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        //     node->name = read_string(in);
        //     node->x = read_raw<float>(in);
        //     node->y = read_raw<float>(in);
        //     node->z = read_raw<float>(in);
        //     node->opt_str = read_string(in);
        //     node->is_scope = read_raw<bool>(in);
    
        //     int value_idx = read_raw<int>(in);
        //     node->value = value_idx != -1 ? value_buffer[value_idx] : nullptr;
    
        //     uint32_t child_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < child_count; i++) {
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) node->children << node_buffer[idx];
        //     }
    
        //     uint32_t scope_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < scope_count; i++) {
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) node->scopes << node_buffer[idx];
        //     }
    
        //     uint32_t qual_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < qual_count; i++) {
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) node->quals << node_buffer[idx];
        //     }
    
        //     uint32_t value_table_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < value_table_count; i++) {
        //         std::string key = read_string(in);
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) node->value_table.put(key, value_buffer[idx]);
        //     }
    
        //     uint32_t node_table_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < node_table_count; i++) {
        //         std::string key = read_string(in);
        //         int idx = read_raw<int>(in);
        //         if(idx != -1) node->node_table.put(key, node_buffer[idx]);
        //     }
    
        //     int parent_idx = read_raw<int>(in);
        //     node->parent = parent_idx != -1 ? node_buffer[parent_idx].getPtr() : nullptr;
    
        //     int owner_idx = read_raw<int>(in);
        //     node->owner = owner_idx != -1 ? node_buffer[owner_idx].getPtr() : nullptr;
    
        //     int in_scope_idx = read_raw<int>(in);
        //     node->in_scope = in_scope_idx != -1 ? node_buffer[in_scope_idx].getPtr() : nullptr;
        // }
    
        // void serialize(g_ptr<Node> root) {
        //     node_buffer.clear();
        //     value_buffer.clear();
        //     serialize_node(root);
        // }
    
        // void saveBinary(std::ostream& out) {
        //     write_raw<uint32_t>(out, labels.size());
        //     for(auto& e : labels.entrySet()) {
        //         write_raw<uint32_t>(out, e.key);
        //         write_string(out, e.value);
        //     }
    
        //     write_raw<uint32_t>(out, type_buffer.length());
        //     write_raw<uint32_t>(out, value_buffer.length());
        //     write_raw<uint32_t>(out, node_buffer.length());
    
        //     for(auto t : type_buffer) write_type(t, out);
        //     for(auto v : value_buffer) write_value(v, out);
        //     for(auto n : node_buffer) write_node(n, out);
        // }
    
        // g_ptr<Node> loadBinary(std::istream& in) {
        //     node_buffer.clear();
        //     value_buffer.clear();
    
        //     //Remap the ids into a map for ease of acess 
        //     map<uint32_t, uint32_t> id_remap;
        //     uint32_t label_count = read_raw<uint32_t>(in);
        //     for(uint32_t i = 0; i < label_count; i++) {
        //         uint32_t saved_id = read_raw<uint32_t>(in);
        //         std::string str = read_string(in);
        //         bool already_exists = false;
        //         for(auto e : labels.entrySet()) {
        //             if(e.value == str) { 
        //                 id_remap.put(saved_id, e.key); 
        //                 already_exists = true;
        //                 break; 
        //             }
        //         }
        //         if(!already_exists) {
        //             id_remap.put(saved_id,reg_id(str));
        //         }
        //     }
    
        //     uint32_t type_count = read_raw<uint32_t>(in);
        //     uint32_t value_count = read_raw<uint32_t>(in);
        //     uint32_t node_count = read_raw<uint32_t>(in);
    
        //     //Pre allocate
        //     for(uint32_t i = 0; i < type_count; i++) {
        //         auto t = make<Type>();
        //         t.save_idx = i;
        //         type_buffer << t;
        //     }
        //     for(uint32_t i = 0; i < value_count; i++) {
        //         auto v = make<Value>();
        //         v->save_idx = i;
        //         value_buffer << v;
        //     }
        //     for(uint32_t i = 0; i < node_count; i++) {
        //         auto n = make<Node>();
        //         n->save_idx = i;
        //         node_buffer << n;
        //     }
        //     //Annotate
        //     for(uint32_t i = 0; i < type_count; i++) read_type(type_buffer[i], in);
        //     for(uint32_t i = 0; i < value_count; i++) read_value(value_buffer[i], in, id_remap);
        //     for(uint32_t i = 0; i < node_count; i++) read_node(node_buffer[i], in, id_remap);
    
    
        //     return node_buffer.empty() ? nullptr : node_buffer[0];
        // }
    
        // void saveBinary(const std::string& path) {
        //     std::ofstream out(path, std::ios::binary);
        //     if (!out) throw std::runtime_error("Can't write to file: " + path);
        //     saveBinary(out);
        //     out.close();
        // }
    
        // g_ptr<Node> loadBinary(const std::string& path) {
        //     std::ifstream in(path, std::ios::binary);
        //     if (!in) throw std::runtime_error("Can't read from file: " + path);
        //     g_ptr<Node> to_return = loadBinary(in);
        //     in.close();
        //     return to_return;
        // }
    
    };
}