#pragma once

#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;

    struct string {
        string(Col& _col) : col(_col) {}
        Col& col;
        inline char at(uint32_t idx) {return *(char*)col[idx];}
        inline uint32_t length() {return col.length();}
        inline void push(char c) { col.push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col.push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void operator=(const std::string& s){ col.storage.clear(); push(s);}
        inline void operator=(string s){ col.storage.clear(); push((const char*)s.col.storage.data(), s.length());}
        inline void operator=(const char* s) { col.storage.clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col.get(idx); }
        std::string to_std() {return std::string((char*)col.storage.data(), length());}
    };

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col.storage.data(), s.length());
        return os;
    }

    struct ptr_table {
        ptr_table(Col& _col) : col(_col) {}
        Col& col;
        
        inline bool hasKey(const std::string& key) { return col.hasKey(key); }
        
        inline Ptr& get(const std::string& key) { return col.get<Ptr>(key); }
        inline Ptr& operator[](const std::string& key) { return col.get<Ptr>(key); }
        
        inline void put(const std::string& key, const Ptr& node) { col.put(key, &node); }
    };

    struct node_list {
        Ptr col_ptr;
        Unit* unit = nullptr;
        
        node_list(Ptr _col_ptr, Unit* _unit) : col_ptr(_col_ptr), unit(_unit) {}
        node_list() {};
        
        inline Col& col();
        
        inline uint32_t length() { return col().length(); }
        inline bool empty() { return col().empty(); }
        
        inline Node operator[](uint32_t idx);
        inline Node get(uint32_t idx);
        inline Node last();

        inline void removeAt(uint32_t idx);
        inline Node take(uint32_t idx);
        inline Node pop();

        inline void push(Node n);
        inline void operator<<(Node n);
    };
    
    struct Node : public opt_ptr {
        Unit* unit = nullptr;
    
        inline uint32_t&      type();
        inline void           type(uint32_t t);
        inline uint32_t&      sub_type();
        inline void           sub_type(uint32_t st);
    
        inline Ptr&           name_ptr();
        inline Col&           name_col();
        inline string         name();
        inline void           name(std::string s);
    
        inline float&         x();
        inline float&         y();
        inline float&         z();
    
        inline Ptr&           value();
    
        inline Ptr&           children_ptr();
        inline Col&           children_col();
        inline node_list      children();
    
        inline Ptr&           quals_ptr();
        inline Col&           quals_col();
        inline node_list      quals();
    
        inline Ptr&           node_table_ptr();
        inline Col&           node_table_col();
        inline ptr_table      node_table();
    
        inline Ptr&           value_table_ptr();
        inline Col&           value_table_col();
        inline ptr_table      value_table();
    
        inline Ptr&           scopes_ptr();
        inline Col&           scopes_col();
        inline node_list      scopes();
    
        inline Ptr&           parent();
        inline Ptr&           owner();
        inline Ptr&           in_scope();
        inline bool&          is_scope();
    
        inline Ptr&           opt_str_ptr();
        inline Col&           opt_str_col();
        inline string         opt_str();
    
        inline bool&          mute();

        inline void           copy(Node o);
    };
    
    struct Context {
        Context() : index(_ctx_dummy_index) {}
        Context(int& _index) : index(_index) {}
        Context(node_list _result, int& _index) : result(_result), index(_index) {}
    
        Node node;
        Node qual;
        Node left;
        Node out;
        Node root;
        node_list result;
        list<Ptr> nodes;
        Ptr value;
        int& index;
        uint32_t state = 0;
        bool flag = false;
    
        Context* sub;
    
        std::string source;
    
        Context duplicate() {
            return Context(result, index);
        }
    };

    using Handler = std::function<void(Context&)>;
        
    struct Unit : public q_object {
    
        map<uint32_t,std::string> labels;
        list<TypePool> types;

        uint32_t undefined_id = 0;
        uint32_t ptr_id = 1;
    
        uint32_t add_type() {
            uint32_t at = types.length();
            TypePool to_return;
            types << to_return;
            return at;
        }

        Ptr add_type_for_handle() {
            Ptr to_return{add_type(),0,0};
            return to_return;
        }

        uint32_t init_handler_type() {
            TypePool t;
            uint32_t at = types.length();
            Ptr undefined_ptr{at,0,0};
            t.new_column("UNDEFINED",&undefined_ptr,sizeof(Ptr),0);
            Ptr ptr_ptr{at,1,0};
            t.new_column("ptr",&ptr_ptr,sizeof(Ptr),1);
            types << t;
            return at;
        }

        uint32_t handler_type_id = init_handler_type(); 

        void set_handler(const Ptr& ptr, const Ptr& handle) {
            types[handler_type_id].set(ptr.idx, ptr.sidx, (void*)&handle);
        }

        uint32_t reg_id(const std::string& label) {
            uint32_t at = types[handler_type_id].columns.length();
            Ptr ptr{handler_type_id,at,0};
            types[handler_type_id].new_column(label,&ptr,sizeof(Ptr),ptr_id);
            labels.put(at,label);
            return at;
        }

        uint32_t float_id = reg_id("float");
        uint32_t int_id = reg_id("int");
        uint32_t bool_id = reg_id("bool");
        uint32_t string_id = reg_id("string");
        size_t list_id = reg_id("list");
        size_t map_id = reg_id("map");
        size_t weakptr_id = reg_id("weakptr");

        size_t identifier_id = reg_id("IDENTIFIER");
        size_t object_id = reg_id("OBJECT");
        size_t literal_id = reg_id("LITERAL");

        size_t var_decl_id = reg_id("VAR_DECL");
        size_t func_call_id = reg_id("FUNC_CALL");
        size_t function_id = reg_id("FUNCTION");
        size_t method_id = reg_id("METHOD");
        size_t func_decl_id = reg_id("FUNC_DECL");
        size_t type_decl_id = reg_id("TYPE_DECL");

        size_t tombstone_col = 0; 
        size_t refs_col = 0;

        uint32_t make_object_type() {
            uint32_t at = add_type();
            TypePool& t = types[at];
            tombstone_col = t.note_value("tombstone",1,bool_id);
            refs_col = t.note_value("refs",4,int_id);
            return at;
        }

        uint32_t make_store_type() {
            uint32_t at = add_type();
            TypePool& t = types[at];
            return at;
        }
        
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

        uint32_t node_type_id = init_node_type();
        uint32_t value_type_id = init_value_type();
        uint32_t name_store_id = make_store_type();
        uint32_t children_store_id = make_store_type();
        uint32_t quals_store_id = make_store_type();
        uint32_t node_table_store_id = make_store_type(); 
        uint32_t value_table_store_id = make_store_type(); 
        uint32_t scopes_store_id = make_store_type(); 
        uint32_t opt_str_store_id = make_store_type();

        uint32_t init_node_type() {
            uint32_t at = make_object_type();
            TypePool& t = types.last();
            type_col = t.note_value("type",4,int_id);
            sub_type_col = t.note_value("sub_type",4,int_id);
            name_col = t.note_value("name",sizeof(Ptr),string_id);
            x_col = t.note_value("x",4,float_id);
            y_col = t.note_value("y",4,float_id);
            z_col = t.note_value("z",4,float_id);
            value_col = t.note_value("value",sizeof(Ptr),ptr_id);
            children_col = t.note_value("children",sizeof(Ptr),list_id);
            quals_col = t.note_value("quals",sizeof(Ptr),list_id);
            node_table_col = t.note_value("node_table",sizeof(Ptr),map_id);
            value_table_col = t.note_value("value_table",sizeof(Ptr),map_id);
            scopes_col = t.note_value("scopes",sizeof(Ptr),list_id);
            parent_col = t.note_value("parent",sizeof(Ptr),weakptr_id);
            owner_col = t.note_value("owner",sizeof(Ptr),weakptr_id);
            in_scope_col = t.note_value("in_scope",sizeof(Ptr),weakptr_id);
            is_scope_col = t.note_value("is_scope",1,bool_id);
            opt_str_col = t.note_value("opt_str",sizeof(Ptr),string_id);
            mute_col = t.note_value("mute",1,bool_id);

            t.init_funcs << [this,at](opt_ptr& optr) {
                TypePool& t = types[at];
                optr.pool = at;
                Ptr nameptr{name_store_id, (uint32_t)types[name_store_id].add_column(sizeof(char)), 0};
                t.set(name_col, optr.sidx, (void*)&nameptr);
            
                Ptr childrenptr{children_store_id, (uint32_t)types[children_store_id].add_column(sizeof(opt_ptr)), 0};
                t.set(children_col, optr.sidx, (void*)&childrenptr);
            
                Ptr qualsptr{quals_store_id, (uint32_t)types[quals_store_id].add_column(sizeof(opt_ptr)), 0};
                t.set(quals_col, optr.sidx, (void*)&qualsptr);
            
                Ptr scopesptr{scopes_store_id, (uint32_t)types[scopes_store_id].add_column(sizeof(opt_ptr)), 0};
                t.set(scopes_col, optr.sidx, (void*)&scopesptr);
            
                Ptr nodetableptr{node_table_store_id, (uint32_t)types[node_table_store_id].add_column(sizeof(opt_ptr)), 0};
                t.set(node_table_col, optr.sidx, (void*)&nodetableptr);
            
                Ptr valuetableptr{value_table_store_id, (uint32_t)types[value_table_store_id].add_column(sizeof(opt_ptr)), 0};
                t.set(value_table_col, optr.sidx, (void*)&valuetableptr);
            
                Ptr optstrptr{opt_str_store_id, (uint32_t)types[opt_str_store_id].add_column(sizeof(char)), 0};
                t.set(opt_str_col, optr.sidx, (void*)&optstrptr);
        
                Ptr valueptr{value_type_id, 0, 0};
                t.set(value_col, optr.sidx, (void*)&valueptr);
        
                Ptr noptr{0, 0, 0};
                t.set(parent_col, optr.sidx, (void*)&noptr);
                t.set(owner_col, optr.sidx, (void*)&noptr);
                t.set(in_scope_col, optr.sidx, (void*)&noptr);
        
                bool f = false;
                t.set(is_scope_col, optr.sidx, (void*)&f);
                t.set(mute_col, optr.sidx, (void*)&f);
        
                float zero = 0.0f;
                t.set(x_col, optr.sidx, (void*)&zero);
                t.set(y_col, optr.sidx, (void*)&zero);
                t.set(z_col, optr.sidx, (void*)&zero);
            };
            return at;
        }

        uint32_t init_value_type() {
            uint32_t at = make_object_type();
            TypePool& t = types.last();

            return at;
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

        inline void set_ptr(const Ptr& ptr, void* to) {
            types[ptr.pool].columns[ptr.idx].set(ptr.sidx,to);
        }

        inline void set_ptr(const Ptr& ptr, const uint32_t& idx, void* to) {
            types[ptr.pool].columns[idx].set(ptr.sidx,to);
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
        
        std::string node_info(Node node) {
            std::string to_return = "";
            
            to_return += labels[node.type()]
            + (node.sub_type()==0?"":":"+labels[node.sub_type()])
            + (node.name().length()==0?"":" "+green(node.name().to_std())+" ")  
            // + (node->value?value_info(node->value):"")
            // + (node->x!=-1.0f?"("+std::to_string((int)node->x)+","+std::to_string((int)node->y)+")":"")
            + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"") ; 
            //+ (node.in_scope().idx!=0?"{"+node.in_scope().name+"}":"");
            return to_return;
        }
    
        // #define MUTE_TABLES 1
        // #define MUTE_MUTED_QUALS 1
    
        std::string node_to_string(Node node, int depth = 0, int index = 0, bool print_sub_scopes = false) {
            std::string indent(depth * 2, ' ');
            std::string to_return = "";
            
            to_return += indent + std::to_string(index) + ": " + node_info(node);
            
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
    
            if(!node.children().empty()) {
                for(int i=0;i<node.children().length();i++) {
                    if(node.children()[i].unit) {
                        to_return += "\n " + node_to_string(node.children()[i], depth + 1, i, print_sub_scopes);
                    }
                    else {
                        to_return += "\n" + indent + "[NULL]";
                    }
                }
            }
        
            return to_return;
        }


        virtual void init() {
            labels[undefined_id] = "UDEFINED";
            labels[ptr_id] = "ptr";
        }
    
        virtual Node process(std::string path) {
            opt_ptr obj = types[node_type_id].create();
            Node n;
            n.sidx = obj.sidx;
            n.pool = node_type_id;
            n.unit = this;
            return n;
        }
    
        virtual void run(Node root) {
            
        }
    
        inline void standard_process(Context& ctx) {
            newline(active_stage->label+": "+node_info(ctx.node));
            active_stage->run(ctx.node.type())(ctx);
            endline();
        }
    
        inline void standard_process(Context& ctx, size_t type) {
            active_stage->run(type)(ctx);
        }

        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node;
            Context* saved_sub = ctx.sub;
            ctx.node = node;
            standard_process(ctx);
            ctx.node = saved_node;
            ctx.sub = saved_sub;
        }
    
        void process_node(Context& ctx, Node node, Node left) {
            Node saved_node = ctx.node;
            Node saved_left = ctx.left;
            Context* saved_sub = ctx.sub;
            ctx.node = node;
            ctx.left = left;
            standard_process(ctx);
            ctx.node = saved_node;
            ctx.left = saved_left;
            ctx.sub = saved_sub;
        }
    
        void process_node(Node node, Node left) {
            Context ctx;
            process_node(ctx,node,left);
        }
    
        void standard_sub_process_node(Node root) {
            Context ctx;
            ctx.node = root;
            standard_sub_process(ctx);
        }

        void standard_sub_process(Context& ctx) {
            int i = 0;
            node_list children = ctx.node.children();
            Context sub_ctx(children,i);
            sub_ctx.root = ctx.node;
            sub_ctx.sub = ctx.sub;
            while(i < sub_ctx.result.length()) {
                if(i==0) {
                    process_node(sub_ctx, sub_ctx.result.get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result.get(i), sub_ctx.result.get(i-1));
                }
                i++;
            }
            ctx.flag = sub_ctx.flag;
        }

        void backwards_sub_process(Context& ctx) { 
            node_list children = ctx.node.children();
            int i = children.length()-1;
            Context sub_ctx(children,i);
            sub_ctx.root = ctx.node;
            sub_ctx.sub = ctx.sub;
            while(i >= 0) {
                if(i==children.length()-1) {
                    process_node(sub_ctx, sub_ctx.result.get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result.get(i), sub_ctx.result.get(i+1));
                }
                i--;
            }
            ctx.flag = sub_ctx.flag;
        }
    
        void standard_direct_pass(Node root) {
            node_list children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            int i = 0;
            Context ctx(children,i);
            ctx.root = root;
            while(i < ctx.result.length()) {
                ctx.node = ctx.result.get(i);
                standard_process(ctx);
                ctx.left = ctx.result.get(i);
                i++;
            }
    
            node_list scopes = root.scopes();
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(scopes.get(i));
            }
            endline();
        }
    
        //Returns true if flagged for a return/break
        bool standard_travel_pass(Node& root, Context* sub = nullptr) {
            node_list children = root.children();
            newline("Travel pass over "+std::to_string(children.length())+" nodes");
            int i = 0;
            Context ctx(children, i);
            ctx.root = root;
            if(sub) ctx.sub = sub;
            while(i < ctx.result.length()) {
                ctx.node = ctx.result.get(i);
                standard_process(ctx);
                ctx.left = ctx.result.get(i);
                if(ctx.flag) { //This is the return/break process
                    endline();
                    return true;
                }
                i++;
            }
            endline();
            return false;
        }
    };

    inline uint32_t& Node::type()                   {return *(uint32_t*)unit->types[unit->node_type_id][unit->type_col][sidx];}
    inline void      Node::type(uint32_t t)         {unit->types[unit->node_type_id][unit->type_col].set(sidx,(void*)&t);}
    inline uint32_t& Node::sub_type()               {return *(uint32_t*)unit->types[unit->node_type_id][unit->sub_type_col][sidx];}
    inline void      Node::sub_type(uint32_t st)    {unit->types[unit->node_type_id][unit->sub_type_col].set(sidx,(void*)&st);}
    
    inline Ptr&      Node::name_ptr()               {return *(Ptr*)unit->types[unit->node_type_id][unit->name_col][sidx];}
    inline Col&      Node::name_col()               {Ptr& p = name_ptr(); return unit->types[p.pool][p.idx];}
    inline string    Node::name()                   {return string(name_col());}
    inline void      Node::name(std::string s)      {name() = s;}
    
    inline float&    Node::x()                      {return *(float*)unit->types[unit->node_type_id][unit->x_col][sidx];}
    inline float&    Node::y()                      {return *(float*)unit->types[unit->node_type_id][unit->y_col][sidx];}
    inline float&    Node::z()                      {return *(float*)unit->types[unit->node_type_id][unit->z_col][sidx];}
    
    inline Ptr&      Node::value()                  {return *(Ptr*)unit->types[unit->node_type_id][unit->value_col][sidx];}
    
    inline Ptr&      Node::children_ptr()           {return *(Ptr*)unit->types[unit->node_type_id][unit->children_col][sidx];}
    inline Col&      Node::children_col()           {Ptr& p = children_ptr(); return unit->types[p.pool][p.idx];}
    inline node_list Node::children()               {return node_list(children_ptr(),unit);}
    
    inline Ptr&      Node::quals_ptr()              {return *(Ptr*)unit->types[unit->node_type_id][unit->quals_col][sidx];}
    inline Col&      Node::quals_col()              {Ptr& p = quals_ptr(); return unit->types[p.pool][p.idx];}
    inline node_list Node::quals()                  {return node_list(quals_ptr(),unit);}
    
    inline Ptr&      Node::node_table_ptr()         {return *(Ptr*)unit->types[unit->node_type_id][unit->node_table_col][sidx];}
    inline Col&      Node::node_table_col()         {Ptr& p = node_table_ptr(); return unit->types[p.pool][p.idx];}
    inline ptr_table Node::node_table()             {return ptr_table(node_table_col());}
    
    inline Ptr&      Node::value_table_ptr()        {return *(Ptr*)unit->types[unit->node_type_id][unit->value_table_col][sidx];}
    inline Col&      Node::value_table_col()        {Ptr& p = value_table_ptr(); return unit->types[p.pool][p.idx];}
    inline ptr_table Node::value_table()            {return ptr_table(value_table_col());}
    
    inline Ptr&      Node::scopes_ptr()             {return *(Ptr*)unit->types[unit->node_type_id][unit->scopes_col][sidx];}
    inline Col&      Node::scopes_col()             {Ptr& p = scopes_ptr(); return unit->types[p.pool][p.idx];}
    inline node_list Node::scopes()                 {return node_list(scopes_ptr(),unit);}
    
    inline Ptr&      Node::parent()                 {return *(Ptr*)unit->types[unit->node_type_id][unit->parent_col][sidx];}
    inline Ptr&      Node::owner()                  {return *(Ptr*)unit->types[unit->node_type_id][unit->owner_col][sidx];}
    inline Ptr&      Node::in_scope()               {return *(Ptr*)unit->types[unit->node_type_id][unit->in_scope_col][sidx];}
    inline bool&     Node::is_scope()               {return *(bool*)unit->types[unit->node_type_id][unit->is_scope_col][sidx];}
    
    inline Ptr&      Node::opt_str_ptr()            {return *(Ptr*)unit->types[unit->node_type_id][unit->opt_str_col][sidx];}
    inline Col&      Node::opt_str_col()            {Ptr& p = opt_str_ptr(); return unit->types[p.pool][p.idx];}
    inline string    Node::opt_str()                {return string(opt_str_col());}
    
    inline bool&     Node::mute()                   {return *(bool*)unit->types[unit->node_type_id][unit->mute_col][sidx];}


    inline void Node::copy(Node o) {
        TypePool& t = unit->types[unit->node_type_id];
        for(int c = 0; c < t.columns.length(); c++) {
            t.columns[c].set(sidx, t.columns[c].get(o.sidx));
        }
    }

    Node make_node(Unit* unit) {
        opt_ptr obj = unit->types[unit->node_type_id].create();
        Node n;
        n.sidx = obj.sidx;
        n.pool = unit->node_type_id;
        n.unit = unit;
        return n;
    }

    inline Col& node_list::col() { return unit->types[col_ptr.pool][col_ptr.idx]; }

    inline Node node_list::get(uint32_t idx) {
        opt_ptr& p = *(opt_ptr*)col().get(idx);
        Node n;
        n.unit = unit;
        n.pool = p.pool;
        n.sidx = p.sidx;
        return n;
    }
    inline Node node_list::operator[](uint32_t idx) {
        return get(idx);
    }
    inline Node node_list::last() {
        return get(length()-1);
    }

    inline void node_list::removeAt(uint32_t idx) {
        col().removeAt(idx);
    }
    inline Node node_list::take(uint32_t idx) {
        Node val = get(idx);
        removeAt(idx);
        return val;
    }
    inline Node node_list::pop() {
        opt_ptr p;
        col().pop(&p);
        Node n;
        n.unit = unit;
        n.pool = p.pool;
        n.sidx = p.sidx;
        return n;
    }

    inline void node_list::push(Node n) {
        opt_ptr p{n.pool, n.sidx};
        col().push(&p);
    }
    inline void node_list::operator<<(Node n) {
        push(n);
    }
}