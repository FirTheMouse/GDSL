#pragma once

#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;
    struct Value;

    map<uint32_t,std::string> labels;

    template<typename T>
    struct TCol : Col {
        TCol() : Col(sizeof(T)) {}
        T& get(uint32_t idx) {return *(T*)Col::sget(idx);}
        void set(uint32_t idx, T val) {Col::set(idx,(void*)&val);}
        T& operator[](uint32_t idx) {return *(T*)Col::sget(idx);}
        void push(T t){Col::push((void*)&t);}
    };
    
    using TypeCol     = TCol<Col>;
    using TypeTypeCol = TCol<TypeCol>;
    TypeTypeCol types;

    std::string tag_to_str(uint32_t tag, void* data);

    list<Ptr> marked_ptrs;
    map<uint64_t,std::function<void(std::string&)>> ptr_colors;
    inline uint64_t Ptr_to_key(Ptr p) {
        return ((uint64_t)p.pool << 32) | (uint64_t)p.idx;
    }
    inline Ptr key_to_Ptr(uint64_t key) {
        return Ptr{(uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF), 0};
    }
    uint64_t make_overload_key(uint32_t root, uint32_t right) {
        return ((uint64_t)root << 32) | right;
    }
    inline std::pair<uint32_t,uint32_t> decode_key(uint64_t key) {
        return std::make_pair<uint32_t,uint32_t>((uint32_t)(key >> 32), (uint32_t)(key & 0xFFFFFFFF));
    }

    void mark_error(Ptr ptr) {marked_ptrs << ptr;}

    struct type_and_value {
        uint32_t type;
        Ptr value;
    };

    struct _layout {
        list<uint32_t> offsets;
        list<uint32_t> tags;
        list<uint32_t> sizes;
        list<std::string> labels;

        list<uint32_t> subtags;
        list<uint32_t> subsizes;
        list<Ptr> ptrs;

        map<std::string,uint32_t> label_to_index;
        map<uint64_t,type_and_value> overload;

        uint32_t total_size = 0;
        uint32_t add_prop(uint32_t tag, uint32_t size, const std::string& label, uint32_t subtag = 0, uint32_t subsize = 0, Ptr ptr = {0,0,0}) {
            label_to_index.put(label,offsets.length());
            offsets << total_size;
            tags << tag;
            sizes << size;
            labels << label;
            subtags << subtag;
            subsizes << subsize;
            ptrs << ptr;
            uint32_t old_size = total_size;
            total_size += size;
            return old_size;
        }
        void print_out() { //Make to_string later
            for(int i=0;i<offsets.length();i++) {
                print(i,": ",labels[i],": ",offsets[i],", ",Acorn::labels[tags[i]],", ",Acorn::labels[subtags[i]],"[",sizes[i],"]");
            }
            for(auto e : overload.entrySet()) {
                auto keyl = decode_key(e.key);
                print(Acorn::labels[keyl.first]," ",Acorn::labels[keyl.second],"(",keyl.second,"): ",Acorn::labels[e.value.type]);
            }
        }
    };
    map<uint32_t,_layout> layouts;

    void make_wrapper_for_layout(_layout& l, const std::string& name,  const std::string& output_path) {
        std::string s = "";
        std::string pad = "     ";
        s+="struct "+name+" : Ptr {\n";
        s += pad+name+"() {}\n";
        s += pad+name+"(uint32_t p, uint32_t i, uint32_t s) { pool = p; idx = i; sidx = s; }\n";
        s += pad+name+"(Ptr p) { pool = p.pool; idx = p.idx; sidx = p.sidx; }\n";
        for(int i=0;i<l.offsets.length();i++) {
            s+="\n";
            std::string type = labels[l.tags[i]];
            std::string label = l.labels[i];
            uint32_t offset = l.offsets[i];
            uint32_t size = l.sizes[i];

            bool is_compound = layouts.hasKey(l.tags[i]);

            if(is_compound) {
                s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return {pool, idx, sidx+"+std::to_string(offset)+"}; }\n";
                s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{types[pool][idx].qset(sidx+"+std::to_string(offset)+", types[t.pool][t.idx].qget(t.sidx), "+std::to_string(size)+"); }\n";
            } else {
                s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return *("+type+"*)types[pool][idx].qget(sidx+"+std::to_string(offset)+"); }\n";
                s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{types[pool][idx].qset(sidx+"+std::to_string(offset)+", (void*)&t, "+std::to_string(size)+"); }\n";
            }
        }
        s+="};";
        editTextFile(output_path,[s](std::string& source){
            source += (source.empty()?"":"\n\n")+s;
        });
    }

    bool is_live(Ptr p) {return (p.pool!=0||p.idx!=0)&&p.pool<types.length();}

    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2;



    uint32_t add_type() {
        uint32_t at = types.length();
        TypeCol to_return;
        types.push(to_return);
        return at;
    }

    Ptr add_type_for_handle() {
        Ptr to_return{add_type(),0,0};
        return to_return;
    }

    uint32_t init_handler_type() {
        TypeCol t;
        uint32_t at = types.length();
        note_value(t,"UNDEFINED",0,0);
        t.get(0).push_default(); //UNDEFINED cell
        note_value(t,"stages",sizeof(Ptr),ptr_id);
        note_value(t,"ptr",sizeof(Ptr),ptr_id);
        types.push(t);
        return at;
    }

    uint32_t handler_type_id = init_handler_type(); 

    uint32_t reg_id(const std::string& label) {
        uint32_t at = types[handler_type_id].length();
        note_value(types[handler_type_id],label,sizeof(Ptr),ptr_id);
        labels.put(at,label);
        return at;
    }

    uint32_t prefix_ptr_id = reg_id("prefix_ptr"); uint32_t suffix_ptr_id = reg_id("suffix_ptr");
    uint32_t float_id = reg_id("float"); uint32_t prefix_float_id = reg_id("prefix_float"); uint32_t suffix_float_id = reg_id("suffix_float");
    uint32_t int_id = reg_id("int"); uint32_t prefix_int_id = reg_id("prefix_int"); uint32_t suffix_int_id = reg_id("suffix_int");
    uint32_t bool_id = reg_id("bool"); uint32_t prefix_bool_id = reg_id("prefix_bool"); uint32_t suffix_bool_id = reg_id("suffix_bool");
    uint32_t string_id = reg_id("string"); uint32_t prefix_string_id = reg_id("prefix_string"); uint32_t suffix_string_id = reg_id("suffix_string");
    uint32_t char_id = reg_id("char"); uint32_t prefix_char_id = reg_id("prefix_char"); uint32_t suffix_char_id = reg_id("suffix_char");
    size_t list_id = reg_id("list");
    size_t map_id = reg_id("map");
    size_t weakptr_id = reg_id("weakptr");
    size_t col_id = reg_id("col");
    
    uint32_t silenced_id = reg_id("SILENCED");
    uint32_t any_id = reg_id("any");
    uint32_t null_id = reg_id("null");
    size_t identifier_id = reg_id("IDENTIFIER");
    size_t object_id = reg_id("OBJECT");
    size_t literal_id = reg_id("LITERAL");
    
    size_t node_id = reg_id("node"); size_t prefix_node_id = reg_id("prefix_node"); size_t suffix_node_id = reg_id("suffix_node");
    size_t value_id = reg_id("value"); size_t prefix_value_id = reg_id("prefix_value"); size_t suffix_value_id = reg_id("suffix_value");
    size_t context_id = reg_id("context"); size_t prefix_context_id = reg_id("prefix_context"); size_t suffix_context_id = reg_id("suffix_context");

    size_t var_decl_id = reg_id("VAR_DECL");
    size_t func_call_id = reg_id("FUNC_CALL");
    size_t method_call_id = reg_id("METHOD_CALL");
    size_t function_id = reg_id("FUNCTION");
    size_t method_id = reg_id("METHOD");
    size_t func_decl_id = reg_id("FUNC_DECL");
    size_t type_decl_id = reg_id("TYPE_DECL");

    size_t tombstone_col = 0; 
    size_t refs_col = 0;

    uint32_t make_store_type() {
        uint32_t at = add_type();
        return at;
    }
    
    uint32_t node_type_offset = 0;
    uint32_t node_sub_type_offset = 0;
    uint32_t node_name_offset = 0;
    uint32_t x_offset = 0;
    uint32_t y_offset = 0;
    uint32_t z_offset = 0;
    uint32_t node_value_offset = 0;
    uint32_t node_children_offset = 0;
    uint32_t node_quals_offset = 0;
    uint32_t node_node_table_offset = 0;
    uint32_t node_value_table_offset = 0;
    uint32_t node_scopes_offset = 0;
    uint32_t parent_offset = 0;
    uint32_t owner_offset = 0;
    uint32_t in_scope_offset = 0;
    uint32_t resolved_offset = 0;
    uint32_t node_opt_str_offset = 0;
    uint32_t mute_offset = 0;

    uint32_t value_type_offset = 0;
    uint32_t value_sub_type_offset = 0;
    uint32_t value_data_offset = 0;
    uint32_t address_offset = 0;
    uint32_t reg_offset = 0;
    uint32_t loc_offset = 0;
    uint32_t size_offset = 0;
    uint32_t sub_size_offset = 0;
    uint32_t value_quals_offset = 0;
    uint32_t value_sub_values_offset = 0;
    uint32_t type_scope_offset = 0;
    uint32_t store_offset = 0;

    uint32_t context_node_offset = 0;
    uint32_t context_qual_offset = 0;
    uint32_t context_left_offset = 0;
    uint32_t context_out_offset = 0;
    uint32_t context_root_offset = 0;
    uint32_t context_result_offset = 0;
    uint32_t context_value_offset = 0;
    uint32_t context_index_offset = 0;
    uint32_t context_state_offset = 0;
    uint32_t context_flag_offset = 0;
    uint32_t context_sub_offset = 0;
    uint32_t context_source_offset = 0;

    uint32_t init_node_type();
    uint32_t init_value_type();
    uint32_t init_context_type();

    uint32_t node_type_id = init_node_type();
    uint32_t value_type_id = init_value_type();
    uint32_t context_type_id = init_context_type();
    uint32_t name_store_id = make_store_type();
    uint32_t children_store_id = make_store_type();
    uint32_t quals_store_id = make_store_type();
    uint32_t node_table_store_id = make_store_type(); 
    uint32_t value_table_store_id = make_store_type(); 
    uint32_t scopes_store_id = make_store_type(); 
    uint32_t opt_str_store_id = make_store_type();

    uint32_t data_store_id = make_store_type();
    uint32_t sub_value_store_id = make_store_type();

    _layout& add_template(uint32_t for_type) {
        _layout temp;
        layouts[for_type] = temp;
        return layouts.get(for_type);
    }

    uint32_t init_node_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];

        _layout& ntemp = add_template(node_id); //Node template
        node_type_offset = ntemp.add_prop(int_id,4,"type");
        node_sub_type_offset = ntemp.add_prop(int_id,4,"sub_type");
        node_name_offset = ntemp.add_prop(string_id,sizeof(Ptr),"name",char_id,1);
        x_offset = ntemp.add_prop(float_id,4,"x");
        y_offset = ntemp.add_prop(float_id,4,"y");
        z_offset = ntemp.add_prop(float_id,4,"z");
        node_value_offset = ntemp.add_prop(value_id,sizeof(Ptr),"value");
        node_children_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"children",node_id,sizeof(Ptr));
        node_quals_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        node_node_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"node_table",node_id,sizeof(Ptr));
        node_value_table_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"value_table",value_id,sizeof(Ptr));
        node_scopes_offset = ntemp.add_prop(ptr_id,sizeof(Ptr),"scopes",node_id,sizeof(Ptr));
        parent_offset = ntemp.add_prop(node_id,sizeof(Ptr),"parent");
        owner_offset = ntemp.add_prop(node_id,sizeof(Ptr),"owner");
        in_scope_offset = ntemp.add_prop(node_id,sizeof(Ptr),"in_scope");
        resolved_offset = ntemp.add_prop(bool_id,1,"resolved");
        node_opt_str_offset = ntemp.add_prop(string_id,sizeof(Ptr),"opt_str");
        mute_offset = ntemp.add_prop(bool_id,1,"mute");

        return at;
    }

    uint32_t init_value_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];

        _layout& vtemp = add_template(value_id); //Value template
        value_type_offset = vtemp.add_prop(int_id,4,"type");
        value_sub_type_offset = vtemp.add_prop(int_id,4,"sub_type");
        value_data_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"data");
        address_offset = vtemp.add_prop(int_id,4,"address");
        reg_offset = vtemp.add_prop(int_id,4,"reg");
        loc_offset = vtemp.add_prop(int_id,4,"loc");
        size_offset = vtemp.add_prop(int_id,4,"size");
        sub_size_offset = vtemp.add_prop(int_id,4,"sub_size");
        value_quals_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"quals",node_id,sizeof(Ptr));
        value_sub_values_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"sub_values",value_id,sizeof(Ptr));
        type_scope_offset = vtemp.add_prop(node_id,sizeof(Ptr),"type_scope");
        store_offset = vtemp.add_prop(ptr_id,sizeof(Ptr),"store");

        return at;
    }

    uint32_t init_context_type() {
        uint32_t at = add_type();
        TypeCol& t = types[at];
        _layout& ctemp = add_template(context_id); //Context template
        context_node_offset = ctemp.add_prop(node_id,sizeof(Ptr),"node");
        context_qual_offset = ctemp.add_prop(node_id,sizeof(Ptr),"qual");
        context_left_offset = ctemp.add_prop(node_id,sizeof(Ptr),"left");
        context_out_offset = ctemp.add_prop(node_id,sizeof(Ptr),"out");
        context_root_offset = ctemp.add_prop(node_id,sizeof(Ptr),"root");
        context_result_offset = ctemp.add_prop(ptr_id,sizeof(Ptr),"result",node_id,sizeof(Ptr));
        context_value_offset = ctemp.add_prop(value_id,sizeof(Ptr),"value");
        context_index_offset = ctemp.add_prop(int_id,4,"index");
        context_state_offset = ctemp.add_prop(int_id,4,"state");
        context_flag_offset = ctemp.add_prop(bool_id,1,"flag");
        context_sub_offset = ctemp.add_prop(context_id,sizeof(Ptr),"sub");
        context_source_offset = ctemp.add_prop(string_id,sizeof(Ptr),"source",char_id,1);
        return at;
    }

    inline void* resolve_ptr(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx].get(ptr.sidx);
    }

    inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {
        return types[ptr.pool][idx].get(ptr.sidx);
    }

    inline Ptr& resolve_to_ptr(const Ptr& ptr) {
        return *(Ptr*)types[ptr.pool][ptr.idx].get(ptr.sidx);
    }

    inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {
        return *(Ptr*)types[ptr.pool][idx].get(ptr.sidx);
    }

    inline Col& resolve_to_col(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx];
    }

    inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {
        return types[ptr.pool][idx];
    }

    inline Col& to_col(const Ptr& ptr) {
        return types[ptr.pool][ptr.idx];
    }

    inline Ptr get_ticket(uint32_t type_id, uint32_t size, uint32_t tag) {
        Ptr ticket{type_id,create_column(types[type_id],size,tag),0};
        return ticket;
    }

    struct string : Ptr {
        string() {}
        string(Ptr p) {pool=p.pool;idx=p.idx;sidx=p.sidx;}
        inline Col& col() {DEBUG_ONLY(if(idx>=types[pool].length()) {throw_error("invalid string col: ",idx," for type ",pool);}) return types[pool][idx]; }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col().push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {Col& c = col(); DEBUG_ONLY(if(ERROR_FLAG) {return ERROR_MSG;}); return std::string((char*)c.storage, length());}
        inline int find(string look_for, int start_at, int nth_of = 1) {
            int found_at = -1;
            for(int i=start_at;i<col().length();i++) {
                bool match = true;
                for(int s=0;s<look_for.length();s++) {
                    if(*(char*)col()[i+s]!=look_for[s]) {
                        match = false;
                        break;
                    }
                }
                if(match) {
                    if(--nth_of<=0) {
                        found_at = i; break;
                    }
                }
            }
            return found_at;
        }
    };

    #define NAMED_PTRS 1

    std::string Ptr_as_string(Ptr p) {
        if(ERROR_FLAG||(p.pool>=types.length()||p.idx>=types[p.pool].length()||marked_ptrs.has(p))) {
            return red(std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx));
        }

        #if NAMED_PTRS
            std::string plabel = types[p.pool].label.empty()?std::to_string(p.pool):types[p.pool].label.to_std();
            std::string pidx = types[p.pool][p.idx].label.empty()?std::to_string(p.idx):types[p.pool][p.idx].label.to_std();
            std::string pstring = ""+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
            uint64_t key = Ptr_to_key(p);
        
            if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
            return pstring;
        #else
            return ""+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx)+"";
        #endif
    }

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }

    template<typename T>
    struct col_Ptr : Ptr {
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but col_ptr was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Col& col()                    {DEBUG_ONLY(if(safety_check("col_ptr:col")){static Col d; return d;}) return types[pool][idx];}
        inline uint32_t length()             {DEBUG_ONLY(if(safety_check("col_ptr:length")){return 0;}) return col().length();}
        inline bool empty()                  {DEBUG_ONLY(if(safety_check("col_ptr:empty")){return true;}) return col().empty();}
        inline void removeAt(uint32_t idx)   {DEBUG_ONLY(if(safety_check("col_ptr:removeAt")){return;}) col().removeAt(idx);}
        inline void clear()                  {DEBUG_ONLY(if(safety_check("col_ptr:clear")){return;}) col().clear();}
    
        inline T get(uint32_t idx)           {DEBUG_ONLY(if(safety_check("col_ptr:get")){return T(deadptr);}) void* ptr = col().get(idx); DEBUG_ONLY(if(safety_check("col_ptr:get:cast")){return T(deadptr);}) return T(*(Ptr*)ptr);}
        inline T operator[](uint32_t idx)    {return get(idx);}
        inline T last()                      {DEBUG_ONLY(if(safety_check("col_ptr:last")){return T(deadptr);}) return get(length()-1);}
    
        inline T take(uint32_t idx)          {DEBUG_ONLY(if(safety_check("col_ptr:take")){return T(deadptr);}) T val = get(idx); removeAt(idx); return val;}
        inline T pop()                       {DEBUG_ONLY(if(safety_check("col_ptr:pop")){return T(deadptr);}) Ptr p; col().pop(&p); return T(p);}
        inline void push(T t)                {DEBUG_ONLY(if(safety_check("col_ptr:push")){return;}) Ptr p(t); col().push(&p);}
        inline void operator<<(T t)          {push(t);}
    
        inline bool hasKey(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:hasKey")){return false;}) return col().hasKey(key);}
        inline T get(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:get_key")){return T(deadptr);}) return T(*(Ptr*)col().get(key));}
        inline T operator[](const std::string& key) {return get(key);}
        inline void put(const std::string& key, T t) {DEBUG_ONLY(if(safety_check("col_ptr:put")){return;}) col().put(key, (void*)&t);}
    };
    using node_col  = col_Ptr<Node>;
    using value_col = col_Ptr<Value>;

    struct Value : public Ptr {
        Value() {}
        Value(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but value was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("value:type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(value_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("value:type:set")){return;}) types[pool][idx].qset(value_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("value:sub_type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(value_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("value:sub_type:set")){return;}) types[pool][idx].qset(value_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      data_ptr()            {DEBUG_ONLY(if(safety_check("value:data_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_data_offset);}
        inline void      data_ptr(Ptr ptr)     {DEBUG_ONLY(if(safety_check("value:data_ptr:set")){return;}) resolve_to_col(*this).qset(value_data_offset,(void*)&ptr,sizeof(Ptr));}
        inline Col&      data_col()            {Ptr p = data_ptr(); return types[p.pool][p.idx];}
        
        inline uint32_t  address()             {DEBUG_ONLY(if(safety_check("value:address:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(address_offset);}
        inline void      address(uint32_t v)   {DEBUG_ONLY(if(safety_check("value:address:set")){return;}) types[pool][idx].qset(address_offset,(void*)&v,4);}
        inline int       reg()                 {DEBUG_ONLY(if(safety_check("value:reg:get")){return -1;}) return *(int*)types[pool][idx].qget(reg_offset);}
        inline void      reg(int i)            {DEBUG_ONLY(if(safety_check("value:reg:set")){return;}) types[pool][idx].qset(reg_offset,(void*)&i,4);}
        inline int       loc()                 {DEBUG_ONLY(if(safety_check("value:loc:get")){return -1;}) return *(int*)types[pool][idx].qget(loc_offset);}
        inline void      loc(int i)            {DEBUG_ONLY(if(safety_check("value:loc:set")){return;}) types[pool][idx].qset(loc_offset,(void*)&i,4);}
        
        inline uint32_t  size()                {DEBUG_ONLY(if(safety_check("value:size:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(size_offset);}
        inline void      size(uint32_t s)      {DEBUG_ONLY(if(safety_check("value:size:set")){return;}) types[pool][idx].qset(size_offset,(void*)&s,4);}
        inline uint32_t  sub_size()            {DEBUG_ONLY(if(safety_check("value:sub_size:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(sub_size_offset);}
        inline void      sub_size(uint32_t s)  {DEBUG_ONLY(if(safety_check("value:sub_size:set")){return;}) types[pool][idx].qset(sub_size_offset,(void*)&s,4);}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("value:quals_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
        
        inline Ptr&       sub_values_ptr()     {DEBUG_ONLY(if(safety_check("value:sub_values_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(value_sub_values_offset);}
        inline Col&       sub_values_col()     {Ptr& p = sub_values_ptr(); return types[p.pool][p.idx];}
        inline value_col  sub_values()         {return (value_col&)sub_values_ptr();}
        
        inline Node      type_scope();
        inline void      type_scope(Ptr o)     {DEBUG_ONLY(if(safety_check("value:type_scope:set")){return;}) types[pool][idx].qset(type_scope_offset,(void*)&o,sizeof(Ptr));}
        inline Ptr&      store()               {DEBUG_ONLY(if(safety_check("value:store:get")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(store_offset);}
        inline void      store(Ptr p)          {DEBUG_ONLY(if(safety_check("value:store:set")){return;}) types[pool][idx].qset(store_offset,(void*)&p,sizeof(Ptr));}
    
        inline void setup(uint32_t _type, uint32_t _size, uint32_t _address = 0) {
            type(_type); size(_size); address(_address);
        }
    
        inline Ptr init_data() {
            Ptr dataptr{data_store_id, create_column(types[data_store_id], size(), type()), 0};
            types[data_store_id][dataptr.idx].push_default();
            types[pool][idx].qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
            return dataptr;
        }
    
        inline void set(void* data) {
            DEBUG_ONLY(if(safety_check("value:set")){return;})
            Ptr dataptr = data_ptr();
            if(!is_live(dataptr)) {
                dataptr = init_data();
            }
            // print("SET AT: ",Ptr_as_string(dataptr));
            // print("FROM: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,types[dataptr.pool][dataptr.idx].get(dataptr.sidx)));
            // print("TO: ",tag_to_str(types[dataptr.pool][dataptr.idx].tag,data));
            types[dataptr.pool][dataptr.idx].set(dataptr.sidx, data);
        }
    
        inline void* get() {
            DEBUG_ONLY(if(safety_check("value:get")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].get(dataptr.sidx);
        }

        inline void* sget() {
            DEBUG_ONLY(if(safety_check("value:sget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].sget(dataptr.sidx);
        }
        
        inline void* qget() {
            DEBUG_ONLY(if(safety_check("value:qget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx].qget(dataptr.sidx);
        }

        inline void copy(Value o, bool is_deep) {
            Col& src = types[o.pool][o.idx];
            Col& dst = types[pool][idx];
            memcpy(dst.storage, src.storage, layouts[value_id].total_size);
            if(is_deep) {
                if(is_live(o.data_ptr())) {
                    init_data();
                    set(o.get());
                }

                Ptr qualsptr = get_ticket(quals_store_id, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();
                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                types[pool][idx].qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
        
                Ptr subvalsptr = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);
                Col& new_subvals = resolve_to_col(subvalsptr);
                Col& old_subvals = o.sub_values_col();
                new_subvals.reserve(old_subvals.size);
                memcpy(new_subvals.storage, old_subvals.storage, old_subvals.size);
                new_subvals.size = old_subvals.size;
                types[pool][idx].qset(value_sub_values_offset,(void*)&subvalsptr,sizeof(Ptr));
            }
        }

        int find_qual(uint32_t q_id);
        Node get_qual(uint32_t q_id);
    
        bool has_qual(uint32_t q_id) {
            return find_qual(q_id)!=-1;
        }
    };
    
    struct QNode : public Col {
        inline uint32_t& type()                   {return *(uint32_t*)&storage[node_type_offset];}
        inline void      type(uint32_t t)         {memcpy(&storage[node_type_offset],(void*)&t,4);}
        inline uint32_t& sub_type()               {return *(uint32_t*)&storage[node_sub_type_offset];}
        inline void      sub_type(uint32_t st)    {memcpy(&storage[node_sub_type_offset],(void*)&st,4);}
    };

    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}
    
        inline QNode& toQ() {return (QNode&)types[pool][idx];}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but node was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("node:type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(node_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("node:type:set")){return;}) types[pool][idx].qset(node_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("node:sub_type:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(node_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("node:sub_type:set")){return;}) types[pool][idx].qset(node_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      name_ptr()            {DEBUG_ONLY(if(safety_check("node:name_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_name_offset);}
        inline Col&      name_col()            {Ptr& p = name_ptr(); return types[p.pool][p.idx];}
        inline string    name()                {return string(name_ptr());}
        inline void      name(std::string s)   {DEBUG_ONLY(if(safety_check("node:name:set")){return;}) name() = s;}
        
        inline float     x()                   {DEBUG_ONLY(if(safety_check("node:x:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(x_offset);}
        inline void      x(float v)            {DEBUG_ONLY(if(safety_check("node:x:set")){return;}) types[pool][idx].qset(x_offset,(void*)&v,4);}
        inline float     y()                   {DEBUG_ONLY(if(safety_check("node:y:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(y_offset);}
        inline void      y(float v)            {DEBUG_ONLY(if(safety_check("node:y:set")){return;}) types[pool][idx].qset(y_offset,(void*)&v,4);}
        inline float     z()                   {DEBUG_ONLY(if(safety_check("node:z:get")){return -1.0f;}) return *(float*)types[pool][idx].qget(z_offset);}
        inline void      z(float v)            {DEBUG_ONLY(if(safety_check("node:z:set")){return;}) types[pool][idx].qset(z_offset,(void*)&v,4);}
        
        inline Ptr       value_ptr()           {DEBUG_ONLY(if(safety_check("node:value_ptr")){return deadptr;}) return *(Ptr*)types[pool][idx].qget(node_value_offset);}
        inline Value     value()               {return Value(value_ptr());}
        inline void      value(Ptr ptr)        {DEBUG_ONLY(if(safety_check("node:value:set")){return;}) types[pool][idx].qset(node_value_offset,(void*)&ptr,sizeof(Ptr));}
        
        inline Ptr&      children_ptr()        {DEBUG_ONLY(if(safety_check("node:children_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_children_offset);}
        inline Col&      children_col()        {Ptr& p = children_ptr(); return types[p.pool][p.idx];}
        inline node_col  children()            {return (node_col&)children_ptr();}
        inline void      children(node_col l)  {DEBUG_ONLY(if(safety_check("node:children:set")){return;}) types[pool][idx].qset(node_children_offset,(void*)&l,sizeof(Ptr));}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("node:quals_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
    
        inline Ptr&        node_table_ptr()    {DEBUG_ONLY(if(safety_check("node:node_table_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_node_table_offset);}
        inline Col&        node_table_col()    {Ptr& p = node_table_ptr(); return types[p.pool][p.idx];}
        inline node_col    node_table()        {return (node_col&)node_table_ptr();}
        
        inline Ptr&        value_table_ptr()   {DEBUG_ONLY(if(safety_check("node:value_table_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_value_table_offset);}
        inline Col&        value_table_col()   {Ptr& p = value_table_ptr(); return types[p.pool][p.idx];}
        inline value_col   value_table()       {return (value_col&)value_table_ptr();}
        
        inline Ptr&      scopes_ptr()          {DEBUG_ONLY(if(safety_check("node:scopes_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_scopes_offset);}
        inline Col&      scopes_col()          {Ptr& p = scopes_ptr(); return types[p.pool][p.idx];}
        inline node_col  scopes()              {return (node_col&)scopes_ptr();}
        
        inline Ptr&  parent_ptr()              {DEBUG_ONLY(if(safety_check("node:parent_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(parent_offset);}
        inline Node  parent()                  {return Node(parent_ptr());}
        inline void  parent(Ptr p)             {DEBUG_ONLY(if(safety_check("node:parent:set")){return;}) types[pool][idx].qset(parent_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  owner_ptr()               {DEBUG_ONLY(if(safety_check("node:owner_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(owner_offset);}
        inline Node  owner()                   {return Node(owner_ptr());}
        inline void  owner(Ptr p)              {DEBUG_ONLY(if(safety_check("node:owner:set")){return;}) types[pool][idx].qset(owner_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  in_scope_ptr()            {DEBUG_ONLY(if(safety_check("node:in_scope_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(in_scope_offset);}
        inline Node  in_scope()                {return Node(in_scope_ptr());}
        inline void  in_scope(Ptr p)           {DEBUG_ONLY(if(safety_check("node:in_scope:set")){return;}) types[pool][idx].qset(in_scope_offset,(void*)&p,sizeof(Ptr));}
                
        inline Ptr&   opt_str_ptr()            {DEBUG_ONLY(if(safety_check("node:opt_str_ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(node_opt_str_offset);}
        inline Col&   opt_str_col()            {Ptr& p = opt_str_ptr(); return types[p.pool][p.idx];}
        inline string opt_str()                {return string(opt_str_ptr());}
        
        inline bool  mute()                    {DEBUG_ONLY(if(safety_check("node:mute:get")){return false;}) return *(bool*)types[pool][idx].qget(mute_offset);}
        inline void  mute(bool b)              {DEBUG_ONLY(if(safety_check("node:mute:set")){return;}) types[pool][idx].qset(mute_offset,(void*)&b,1);}
    
        inline bool  resolved()                {DEBUG_ONLY(if(safety_check("node:resolved:get")){return false;}) return *(bool*)types[pool][idx].qget(resolved_offset);}
        inline void  resolved(bool b)          {DEBUG_ONLY(if(safety_check("node:resolved:set")){return;}) types[pool][idx].qset(resolved_offset,(void*)&b,1);}
    
        inline void copy(Node o) {
            Col& src = types[o.pool][o.idx];
            Col& dst = types[pool][idx];
            memcpy(dst.storage, src.storage, layouts[node_id].total_size);
        }

        int find_qual(uint32_t q_id) {
            for(int i=0;i<quals().length();i++){ 
                if(quals()[i].type()==q_id) {return i;}
            }
            return -1;
        } 
    
        int find_qual_in_value(uint32_t q_id) {
            if(is_live(value()))
                return value().find_qual(q_id);
            return -1;
        }
        
        bool has_qual(size_t q_id, bool check_value = true) {
            if(check_value&&find_qual_in_value(q_id)!=-1) return true;
            if(find_qual(q_id)!=-1) return true;
            return false;
        }
    };

    inline Node Value::type_scope() {return Node(*(Ptr*)types[value_type_id][idx].qget(type_scope_offset));}
    int Value::find_qual(uint32_t q_id) {
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) {return i;}
        }
        return -1;
    } 
    Node Value::get_qual(uint32_t q_id) {
        int q_at = find_qual(q_id);
        if(q_at!=-1) {
            return quals()[q_at];
        }
        return deadptr;
    } 

    Node make_node(uint32_t type = 0, uint32_t sub_type = 0, std::string name = "", float x = -1.0f, float y = -1.0f, float z = -1.0f,
        Value value = deadptr, Ptr childrenptr = deadptr, Ptr qualsptr = deadptr, Ptr nodetableptr = deadptr, 
        Ptr valuetableptr = deadptr, Ptr scopesptr = deadptr, Ptr parent = deadptr, Ptr owner = deadptr, 
        Ptr in_scope = deadptr, std::string opt_str = "", bool mute = false, bool resolved = false) 
    {
        Node n;
        n.pool = node_type_id;
        n.idx = push_column(types[node_type_id], layouts[node_id].total_size,node_id);
        n.sidx = 0;
        Col& col = types[node_type_id][n.idx];
        col.heterogenous = true;

        col.qset(node_type_offset,(void*)&type,4);
        col.qset(node_sub_type_offset,(void*)&sub_type,4);

        Ptr nameptr = get_ticket(name_store_id,sizeof(char),char_id);
        col.qset(node_name_offset, (void*)&nameptr,sizeof(Ptr));
        for(auto c : name) types[nameptr.pool][nameptr.idx].push((void*)&c);

        col.qset(x_offset, (void*)&x,4);
        col.qset(y_offset, (void*)&y,4);
        col.qset(z_offset, (void*)&z,4);

        col.qset(node_value_offset, (void*)&value,sizeof(Ptr));
    
        if(!is_live(childrenptr)) childrenptr = get_ticket(children_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_children_offset, (void*)&childrenptr,sizeof(Ptr));
    
        if(!is_live(qualsptr)) qualsptr = get_ticket(quals_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_quals_offset, (void*)&qualsptr,sizeof(Ptr));
        
        if(!is_live(nodetableptr)) nodetableptr = get_ticket(node_table_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_node_table_offset, (void*)&nodetableptr,sizeof(Ptr));
    
        if(!is_live(valuetableptr)) valuetableptr = get_ticket(value_table_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_value_table_offset, (void*)&valuetableptr,sizeof(Ptr));

        if(!is_live(scopesptr)) scopesptr = get_ticket(scopes_store_id,sizeof(Ptr),ptr_id);
        col.qset(node_scopes_offset, (void*)&scopesptr,sizeof(Ptr));
        
        col.qset(parent_offset, (void*)&parent,sizeof(Ptr));
        col.qset(owner_offset, (void*)&owner,sizeof(Ptr));
        col.qset(in_scope_offset, (void*)&in_scope,sizeof(Ptr));

        Ptr optstrptr = get_ticket(opt_str_store_id,sizeof(char),char_id);
        col.qset(node_opt_str_offset, (void*)&optstrptr,sizeof(Ptr));
        for(auto c : opt_str) types[optstrptr.pool][optstrptr.idx].push((void*)&c);

        col.qset(mute_offset, (void*)&mute,1);
        col.qset(resolved_offset, (void*)&resolved,1);

        return n;
    }

    void recycle_column(Ptr p) {
        recycle_column(types[p.pool], p.idx);
    }

    void recycle_node(Node n);

    void recycle_value(Value v) {
        if(is_live(v)&&resolve_to_col(v).live) {
            for(int i=0;i<v.quals().length();i++) {
                recycle_node(v.quals()[i]);
            }
            recycle_column(v.quals_ptr());

            for(int i=0;i<v.sub_values().length();i++) {
                recycle_value(v.sub_values()[i]);
            }
            recycle_column(v.sub_values_ptr());

            recycle_column(v.data_ptr());
        }
    }

    //Recycles everything
    void recycle_node(Node n) {
        if(is_live(n)&&resolve_to_col(n).live) {
            for(int i=0;i<n.children().length();i++) {
                recycle_node(n.children()[i]);
            }
            recycle_column(n.children_ptr());

            for(int i=0;i<n.scopes().length();i++) {
                recycle_node(n.scopes()[i]);
            }
            recycle_column(n.scopes_ptr());

            for(int i=0;i<n.quals().length();i++) {
                recycle_node(n.quals()[i]);
            }
            recycle_column(n.quals_ptr());

            recycle_column(n.name_ptr());
            recycle_value(n.value());
            recycle_column(n.node_table_ptr());
            recycle_column(n.value_table_ptr());
            recycle_column(n.opt_str_ptr());
            recycle_column(n);
        }
    }

    //Doesn't recycle the value or children or scopes or quals
    void soft_recycle_node(Node n) {
        recycle_column(n.name_ptr());
        recycle_column(n.children_ptr());
        recycle_column(n.quals_ptr());
        recycle_column(n.node_table_ptr());
        recycle_column(n.value_table_ptr());
        recycle_column(n.scopes_ptr());
        recycle_column(n.opt_str_ptr());
        recycle_column(n);
    }

    Value make_value(uint32_t type = 0, uint32_t size = 0, uint32_t address = 0, uint32_t sub_type = 0, 
        uint32_t sub_size = 0, Ptr type_scope = deadptr, Ptr data = deadptr, Ptr quals = deadptr, 
        Ptr sub_values = deadptr, Ptr store = deadptr, int reg = -1, int loc = -1) 
    {
        Value v;
        v.pool = value_type_id;
        v.idx = push_column(types[value_type_id], layouts[value_id].total_size, value_id);
        v.sidx = 0;
        Col& col = types[value_type_id][v.idx];
        col.heterogenous = true;
    
        col.qset(value_type_offset, (void*)&type, 4);
        col.qset(value_sub_type_offset, (void*)&sub_type, 4);
        col.qset(size_offset, (void*)&size, 4);
        col.qset(sub_size_offset, (void*)&sub_size, 4);
        col.qset(address_offset, (void*)&address, 4);
        col.qset(reg_offset, (void*)&reg, 4);
        col.qset(loc_offset, (void*)&loc, 4);
    
        col.qset(value_data_offset, (void*)&data, sizeof(Ptr));
        col.qset(type_scope_offset, (void*)&type_scope, sizeof(Ptr));
        col.qset(store_offset, (void*)&store, sizeof(Ptr));
    
        if(!is_live(quals)) quals = get_ticket(quals_store_id, sizeof(Ptr), ptr_id);
        col.qset(value_quals_offset, (void*)&quals, sizeof(Ptr));
    
        if(!is_live(sub_values)) sub_values = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);
        col.qset(value_sub_values_offset, (void*)&sub_values, sizeof(Ptr));
    
        return v;
    }



    struct Context : public Ptr {
        Context() {}
        Context(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx; }
    
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but context was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Ptr&     node_ptr()           {DEBUG_ONLY(if(safety_check("context:node:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_node_offset);}
        inline Node     node()               {return Node(node_ptr());}
        inline void     node(Ptr p)          {DEBUG_ONLY(if(safety_check("context:node:set")){return;}) types[pool][idx].qset(context_node_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     qual_ptr()           {DEBUG_ONLY(if(safety_check("context:qual:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_qual_offset);}
        inline Node     qual()               {return Node(qual_ptr());}
        inline void     qual(Ptr p)          {DEBUG_ONLY(if(safety_check("context:qual:set")){return;}) types[pool][idx].qset(context_qual_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     left_ptr()           {DEBUG_ONLY(if(safety_check("context:left:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_left_offset);}
        inline Node     left()               {return Node(left_ptr());}
        inline void     left(Ptr p)          {DEBUG_ONLY(if(safety_check("context:left:set")){return;}) types[pool][idx].qset(context_left_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     out_ptr()            {DEBUG_ONLY(if(safety_check("context:out:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_out_offset);}
        inline Node     out()                {return Node(out_ptr());}
        inline void     out(Ptr p)           {DEBUG_ONLY(if(safety_check("context:out:set")){return;}) types[pool][idx].qset(context_out_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     root_ptr()           {DEBUG_ONLY(if(safety_check("context:root:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_root_offset);}
        inline Node     root()               {return Node(root_ptr());}
        inline void     root(Ptr p)          {DEBUG_ONLY(if(safety_check("context:root:set")){return;}) types[pool][idx].qset(context_root_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     value_ptr()          {DEBUG_ONLY(if(safety_check("context:value:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_value_offset);}
        inline Value    value()              {return Value(value_ptr());}
        inline void     value(Ptr p)         {DEBUG_ONLY(if(safety_check("context:value:set")){return;}) types[pool][idx].qset(context_value_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     result_ptr()         {DEBUG_ONLY(if(safety_check("context:result:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_result_offset);}
        inline Col&     result_col()         {Ptr& p = result_ptr(); return types[p.pool][p.idx];}
        inline node_col result()             {return (node_col&)result_ptr();}
        inline void     result(Ptr p)        {DEBUG_ONLY(if(safety_check("context:result:set")){return;}) types[pool][idx].qset(context_result_offset,(void*)&p,sizeof(Ptr));}
    
        inline int&     index()              {DEBUG_ONLY(if(safety_check("context:index:get")){return _ctx_dummy_index;}) return *(int*)types[pool][idx].qget(context_index_offset);}
        inline void     index(int i)         {DEBUG_ONLY(if(safety_check("context:index:set")){return;}) types[pool][idx].qset(context_index_offset,(void*)&i,4);}
    
        inline Ptr&     sub_ptr()            {DEBUG_ONLY(if(safety_check("context:sub:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_sub_offset);}
        inline Context  sub()                {return Context(sub_ptr());}
        inline void     sub(Ptr p)           {DEBUG_ONLY(if(safety_check("context:sub:set")){return;}) types[pool][idx].qset(context_sub_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     source_ptr()         {DEBUG_ONLY(if(safety_check("context:source:ptr")){return dead_ref;}) return *(Ptr*)types[pool][idx].qget(context_source_offset);}
        inline Col&     source_col()         {Ptr& p = source_ptr(); return types[p.pool][p.idx];}
        inline string   source()             {return string(source_ptr());}
        inline void     source(Ptr p)        {DEBUG_ONLY(if(safety_check("context:source:set")){return;}) types[pool][idx].qset(context_source_offset,(void*)&p,sizeof(Ptr));}
        inline void     source(std::string s){DEBUG_ONLY(if(safety_check("context:source:set")){return;}) source() = s;}
    
        inline uint32_t state()              {DEBUG_ONLY(if(safety_check("context:state:get")){return 0;}) return *(uint32_t*)types[pool][idx].qget(context_state_offset);}
        inline void     state(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:state:set")){return;}) types[pool][idx].qset(context_state_offset,(void*)&s,4);}
    
        inline bool     flag()               {DEBUG_ONLY(if(safety_check("context:flag:get")){return false;}) return *(bool*)types[pool][idx].qget(context_flag_offset);}
        inline void     flag(bool b)         {DEBUG_ONLY(if(safety_check("context:flag:set")){return;}) types[pool][idx].qset(context_flag_offset,(void*)&b,1);}
    };


    Context make_context(Ptr result = deadptr, Ptr source = deadptr) {
        Context c;
        c.pool = context_type_id;
        c.idx = push_column(types[context_type_id], layouts[context_id].total_size, context_id);
        c.sidx = 0;
        Col& col = types[context_type_id][c.idx];
        col.heterogenous = true;
    
        Ptr dead_node = deadptr;
        Ptr dead_value = deadptr;
    
        col.qset(context_node_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_qual_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_left_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_out_offset,    (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_root_offset,   (void*)&dead_node,  sizeof(Ptr));
        col.qset(context_value_offset,  (void*)&dead_value, sizeof(Ptr));
        col.qset(context_sub_offset,    (void*)&dead_node,  sizeof(Ptr));
    
        if(!is_live(result)) result = get_ticket(children_store_id, sizeof(Ptr), ptr_id);
        col.qset(context_result_offset, (void*)&result, sizeof(Ptr));
        
        if(!is_live(source)) source = get_ticket(name_store_id, sizeof(char), char_id);
        col.qset(context_source_offset, (void*)&source, sizeof(Ptr));
    
        uint32_t zero = 0; bool f = false;
        col.qset(context_index_offset,  (void*)&zero, 4);
        col.qset(context_state_offset,  (void*)&zero, 4);
        col.qset(context_flag_offset,   (void*)&f,    1);
    
        return c;
    }

    void recycle_context(Context ctx) {
        recycle_column(ctx);
    }
    void deep_recycle_context(Context ctx) {
        // recycle_column(ctx.result_ptr()); //Because this is often somebodies children
        // recycle_column(ctx.source_ptr());
        recycle_column(ctx);
    }
    
    using Handler = std::function<void(Context&)>;

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

    struct Watcher {
        Watcher(){};
        Watcher(std::string _label) : label(_label) {};
        std::string label = "";
        Handler stagestart = nullptr;
        Handler prefix = nullptr;
        Handler suffix = nullptr;
        Handler stagend = nullptr;
        Handler logger = nullptr;
    };  

    std::string tag_to_str(uint32_t tag, void* data) {
        DEBUG_ONLY(if(ERROR_FLAG) {return "ERROR";})
        if(tag==int_id) {
            return std::to_string(*(int*)data);
        } else if(tag==float_id) {
            return std::to_string(*(float*)data);
        } else if(tag==bool_id) {
            return (*(bool*)data?"true":"false");
        } else if(tag==char_id) {
            return std::string(1,*(char*)data);
        } else if(tag==string_id) {
            Ptr ptr = *(Ptr*)data;
            if(ptr.pool>=types.length()) {
                return red("STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx));
            }
            std::string content = string(ptr).to_std();
            return Ptr_as_string(ptr)+"> \""+escape_string(content)+"\"";
        } else if(tag==ptr_id) {
            return Ptr_as_string(*(Ptr*)data);
        } else if(tag==ptr_id||tag==node_id||tag==value_id||tag==context_id) {
            return Ptr_as_string(*(Ptr*)data);
        } else if(tag==list_id||tag==col_id) {
            Ptr ptr = *(Ptr*)data;
            std::string s = Ptr_as_string(ptr)+"> [";
            Col& col = resolve_to_col(ptr);
            for(int i=0;i<col.length();i++) {
                s+=tag_to_str(col.tag,col[i]);
                if(i<col.length()-1) {s+=", ";}
            }
            s+="]";
            return s;
        } else if(tag==map_id) {
            Ptr ptr = *(Ptr*)data;
            std::string s = Ptr_as_string(ptr)+"> [";
            Col& col = resolve_to_col(ptr);
            // for(auto e : col.cells.entrySet()) {
            //     s+="{"+e.key+": "+tag_to_str(col.tag,col[e.value])+"}";
            // }
            s+="]";
            return s;
        } else {
            return labels[tag]+"?";
        }
    }
    
    std::string disassemble(uint32_t instr) {
        if(instr==0) return "NULL";
        if(instr==0xD65F03C0) return "ret";

        uint32_t op9 = (instr >> 23) & 0b111111111;
        uint32_t op7 = (instr >> 24) & 0b11111110;

        if(op9 == 0b010100101) { // MOVZ sf=0
            int rd    = instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 0";
        }
        if(op9 == 0b110100101) { // MOVZ sf=1
            int rd    = instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            return "movz " + std::to_string(rd) + " " + std::to_string(imm16) + " 1";
        }


        if((instr & 0b01111111100000000000000000000000) == 0b01110010100000000000000000000000) {
            int rd    =  instr & 0b11111;
            int imm16 = (instr >> 5) & 0b1111111111111111;
            int hw    = (instr >> 21) & 0b11;
            int sf    = (instr >> 31) & 1;
            return "movk " + std::to_string(rd) + " " + std::to_string(imm16) + " " + std::to_string(hw*16) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111001000000000001111100000) == 0b00101010000000000000001111100000) {
            int rd = instr & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "mov " + std::to_string(rd) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111111000000000000000000000) == 0b00001011000000000000000000000000) {
            int rd = instr & 0b11111;
            int rn = (instr >> 5) & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "add " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b01111111111000000000000000000000) == 0b00011011000000000000000000000000) {
            int rd = instr & 0b11111;
            int rn = (instr >> 5) & 0b11111;
            int rm = (instr >> 16) & 0b11111;
            int sf = (instr >> 31) & 1;
            return "mul " + std::to_string(rd) + " " + std::to_string(rn) + " " + std::to_string(rm) + " " + std::to_string(sf);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b10111001010000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "ldr32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b11111001010000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 8;
            return "ldr " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b10111001000000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "str32 " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }

        if((instr & 0b11111111110000000000000000000000) == 0b11111001000000000000000000000000) {
            int rt     = instr & 0b11111;
            int rn     = (instr >> 5) & 0b11111;
            int offset = ((instr >> 10) & 0b111111111111) * 4;
            return "str " + std::to_string(rt) + " " + std::to_string(rn) + " " + std::to_string(offset);
        }



        return "   ?   ";
    }

    std::string print_columnar_table(list<list<std::string>> lines) {
        list<uint32_t> widths;
        uint32_t longest_row = 0;
        for(int l=0;l<lines.length();l++) {
            list<std::string>& line = lines[l];
            uint32_t widest_row = 0;
            for(int i=0;i<line.length();i++) {
                if(i==0&&line[i].empty()) {line[i] = std::to_string(l);}
                if(line[i].length()>widest_row) {widest_row = line[i].length();}
            }
            widths << widest_row;
            if(line.length()>longest_row) {longest_row = line.length();}
        }

        std::string to_return = "";
        int lpadlen = digit_count(longest_row)+1;
        for(int r=0;r<longest_row;r++) {
            if(r==1) {
                for(int l=0;l<lines.length();l++) {
                    to_return+=std::string(widths[l]+lpadlen+3,'-')+"<|>";
                }
                to_return+="\n";
            }

            for(int l=0;l<lines.length();l++) {
                std::string line = "";
                if(lines[l].length()>r) {line = lines[l][r];}
                std::string rownum = std::to_string(r-1); //Minus 1 because indexes start at 0                    
                if(r==0) { //If a header
                    std::string column = std::to_string(l);
                    if(line==column) {
                        to_return += center_pad(line, widths[l]+lpadlen+3) + " | ";
                    } else {
                        to_return+=std::string(lpadlen-column.length(),' ')+column+" : ";
                        to_return += center_pad(line, widths[l]) + " | ";
                    }
                } else {
                    if(!line.empty()) {
                        to_return+=std::string(lpadlen-rownum.length(),' ')+rownum+" : ";
                        std::string padding(widths[l]-line.length(),' ');
                        to_return += line+padding+" | ";
                    } else {
                        to_return += center_pad("X",widths[l]+lpadlen+3)+" | ";
                    }
                }

            }
            to_return+="\n";

            if(longest_row>1&&r==longest_row-1) {
                for(int l=0;l<lines.length();l++) {
                    to_return+=std::string(widths[l]+lpadlen+3,'=')+"/ \\";
                }
            }
        }
        return to_return;
    }

    list<list<std::string>> type_to_lines(TypeCol& t) {
        list<list<std::string>> lines;
        list<uint32_t> dtypes;
        for(int c=0;c<t.length();c++) {
            Col& col = t[c];
            list<std::string> subline;
            subline << col.label.to_std()+(col.live?"":" [FREE]");
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int o=0;o<l.offsets.length();o++) {
                        std::string line = "";
                        line+=pad_str(l.labels[o]+": ",12);
                        line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                        subline << line;
                    }
                } else {
                    print(red("core::type_to_string unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                }
            } else {
                for(int r=0;r<col.length();r++) {
                    std::string line = "";
                    if(col.label=="type"||col.label=="sub_type") {
                        if(col.label=="type"){dtypes << *(int*)col[r];} //Passing info down so values can print themselves out
                        line+=labels[*(int*)col[r]];
                    } else if(col.label=="stages") { //From the handler type
                        std::string cell_label = ""; //col.get_cell_label(r);
                        if(!cell_label.empty()) line+=cell_label;
                        else line+="UNAMED STAGE";
                    } else if(!dtypes.empty()&&dtypes[r]!=0&&col.label=="data") {
                        Ptr p = *(Ptr*)col[r];
                        line+=Ptr_as_string(p)+"> "+tag_to_str(dtypes[r],resolve_ptr(p));
                    } else {
                        line+=tag_to_str(col.tag,col[r]);
                    }
                    subline << line;
                }
            }
            lines << subline;
        }
        return lines;
    }

    std::string type_to_string(TypeCol& t) {
        return print_columnar_table(type_to_lines(t));
    }

    void print_column(Col& col) {
        print("COL: ",col.label," TAG: ",labels[col.tag]," [",std::to_string(col.length()),"]");
        if(col.heterogenous) {
            if(layouts.hasKey(col.tag)) {
                _layout& l = layouts.get(col.tag);
                for(int o=0;o<l.offsets.length();o++) {
                    std::string line = "";
                    line+=pad_str(l.labels[o]+": ",12);
                    line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                    print(line);
                }
            } else {
                print(red("core::print_column unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
            }
        } else {
            for(int i=0;i<col.length();i++) {
                print(i,": ",tag_to_str(col.tag,col[i]));
            }
        }
    }

    void dump_unit(bool clear_dump) {
        if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");

        for(int t=0;t<types.length();t++) {
            std::string to_print = "";
            to_print += "TYPE "+std::to_string(t)+" "+types[t].label.to_std()+":\n";
            to_print += type_to_string(types[t]);
            to_print += "\n\n\n";

            editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                source+=to_print;
            });
        }
    }

    std::string value_info(Value value) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" while another error was active")); return "";})

        std::string to_return = "";
        to_return += cyan("["+Ptr_as_string(value)+"]")+
        + "(type: " + green(labels[value.type()])
        + (value.reg()!=-1?", reg: "+std::to_string(value.reg()):"");
        if(is_live(value.data_ptr())) { //For post-mortems we want to see the adress, so it needs to be computed first, before the error
            std::string ptr_addr = Ptr_as_string(value.data_ptr());
            if(resolve_to_col(value.data_ptr()).empty()) {
                to_return += ", value: "+gray("empty")+" @"+ptr_addr;
            } else {
                to_return += ", value: "+gray(tag_to_str(value.type(),value.get()))+" @"+ptr_addr;
            }
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" but the value was invalid")); return to_return;})
        }
        to_return += (value.sub_type()!=0?", sub_type: "+labels[value.sub_type()]:"")
        + (is_live(value.type_scope())?", type_scope: "+blue(Ptr_as_string(value.type_scope())):"")
        + (value.size()!=0?", size: "+std::to_string(value.size()):"")
        + (value.sub_size()!=0?", sub_size: "+std::to_string(value.sub_size()):"")
        + (value.address()!=0?", address: "+std::to_string(value.address()):"")
        + (value.loc()!=-1?", loc: "+std::to_string(value.loc()):"")
        + (is_live(value.store())?", store: "+Ptr_as_string(value.store()):"")
        + (!value.sub_values().empty()?", sub: "+std::to_string(value.sub_values().length()):"");
        if(!value.quals().empty()) {
            to_return += ", Quals: ";
            for(int i=0;i<value.quals().length();i++) {
                to_return += labels[value.quals()[i].type()]+(i!=value.quals().length()-1?", ":"");
            }
        }
        to_return += ")";
        return to_return;
    }
    
    std::string node_info(Node node) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})

        std::string to_return = "";
        to_return += blue(Ptr_as_string(node)+" ")
        + labels[node.type()]
        + (node.sub_type()==0?"":":"+labels[node.sub_type()])
        + (node.name().length()==0?"":" "+green(escape_string(node.name().to_std()))+" ") 
        + (is_live(node.value())?value_info(node.value()):"")
        + (node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"")
        + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"")
        + (!node.scopes().empty()?"[S:"+std::to_string(node.scopes().length())+"]":"")
        + (is_live(node.owner())?"[O:"+blue(Ptr_as_string(node.owner()))+"]":"")
        + (is_live(node.in_scope())?"{"+node.in_scope().name().to_std()+"}":"");
        if(!node.quals().empty()) {
            to_return += "[Q: ";
            for(int i=0;i<node.quals().length();i++) {
                if(node.quals()[i].mute()) {
                    to_return += italic_str(Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()]);
                } else {
                    to_return += Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()];
                }
                to_return+=(i!=node.quals().length()-1?", ":"");
            }
            to_return += "]";
        }
        return to_return;
    }

    #define ACORN_MUTE_TABLES 1

    std::string node_to_string(Node node, int depth = 0, int index = 0, bool print_sub_scopes = false, std::string sigil = "") {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + sigil + std::to_string(index) + ": " + node_info(node);
    
        #if !ACORN_MUTE_TABLES 
        if(node.value_table().length()>0) {
            to_return += "\n" + indent + "   Value table:";
            QCellCol cells = node.value_table().col().cells;
            for(int i=0;i<cells.length();i++) {
                to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+value_info(node.value_table().get(cells.get(i).index));
            }
        }
        if(node.node_table().length()>0) {
            to_return += "\n" + indent + "   Node table:";
            QCellCol cells = node.node_table().col().cells;
            for(int i=0;i<cells.length();i++) {
                to_return += "\n" + indent + "     Key: "+std::to_string(cells.get(i).hash)+" | "+node_info(node.node_table().get(cells.get(i).index));
            }
        }
        #endif
    


        // if(!node->opt_str.empty()) {
        //     to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
        // }

        if(!node.children().empty()) {
            for(int i=0;i<node.children().length();i++) {
                if(is_live(node.children()[i])) {
                    if(node.children()[i].idx==node.idx) {
                        to_return+="\n "+indent+red("  self refrence");
                    } else {
                        to_return += "\n " + node_to_string(node.children()[i], depth + 1, i, print_sub_scopes,"c");
                    }
                }
                else {
                    to_return += "\n" + indent + "[NULL CHILD] "+node_info(node.children()[i]);
                }
            }
        }

        if(!node.scopes().empty()) {
            //to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node.scopes().length());
            int i = 0;
            for(int s=0;s<node.scopes().length();s++) {
                Node scope = node.scopes()[s];
                if(scope.owner().idx==node.idx) {
                    to_return += "\n " + node_to_string(scope, depth + 1, s, print_sub_scopes,"s");
                }
                else {
                    to_return += "\n"+indent+"  s"+std::to_string(s)+": "+node_info(scope);
                }
            }
        }
    
        return to_return;
    }



    void init_type_pool() {
        labels[undefined_id] = "UDEFINED";
        labels[ptr_id] = "Ptr";

        types[handler_type_id].label = "handlers";
        types[node_type_id].label = "nodes";
        types[value_type_id].label = "values";
        types[context_type_id].label = "contexts";
        types[name_store_id].label = "names";
        types[children_store_id].label = "children";
        types[quals_store_id].label = "quals";
        types[node_table_store_id].label = "node table";
        types[value_table_store_id].label = "value table";
        types[scopes_store_id].label = "scopes";
        types[opt_str_store_id].label = "opt_str";
        types[data_store_id].label = "data";
        types[sub_value_store_id].label = "sub_value";

        add_template(ptr_id);
        add_template(string_id); 

        // writeFile("mixos-acorn/tests/printout2.txt","");
        // make_wrapper_for_layout(layouts[node_id],"Node","mixos-acorn/tests/printout2.txt");
    }

    Node scan_for_node(const std::string& label, Node from) {
        for(int i=0;i<from.children().length();i++) {
            //print("CHECKING: ",from.children()[i].name().to_std());
            if(from.children()[i].name().to_std()==label) {
                return from.children()[i];
            }
            Node found = scan_for_node(label,from.children()[i]);
            if(is_live(found)) {
                return found;
            }
        }
        for(int i=0;i<from.scopes().length();i++) {
            Node found = scan_for_node(label,from.scopes()[i]);
            if(is_live(found)) {
                return found;
            }
        }
        return deadptr;
    }

    void test_acorn() {
        Node n = make_node();
        Node m = make_node();
        n.name("NODE N"); m.name("NODE M");
        print("NODES WITH 2 NODES: ",types[node_type_id].length());
        print("NAMES WITH 2 NODES: ",types[name_store_id].length());
        print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
        recycle_node(n);
        print("NODES AFTER RECYCLE: ",types[node_type_id].length());
        print("NAMES AFTER RECYCLE: ",types[name_store_id].length());
        print("CHILDREN AFTER RECYCLE: ",types[children_store_id].length());
        Node c = make_node();
        print("NODES WITH 2 NODES: ",types[node_type_id].length());
        print("NAMES WITH 2 NODES: ",types[name_store_id].length());
        print("CHILDREN WITH 2 NODES: ",types[children_store_id].length());
        c.name("C");
        print(c.name().to_std());
    }

    void deep_copy_node(Node n, Node o, map<uint32_t,Value>& value_alias_table, map<uint32_t,Node>& node_alias_table) {
        n.type(o.type());
        n.sub_type(o.sub_type());
        n.name(o.name().to_std());
        n.x(o.x());
        n.y(o.y());
        n.z(o.z());
        n.mute(o.mute());
        n.resolved(o.resolved());
    
        n.children().clear();
        for(int i = 0; i < o.children().length(); i++) {
            Node newc = make_node();
            deep_copy_node(newc, o.children()[i], value_alias_table, node_alias_table);
            n.children() << newc;
        }
    
        n.quals().clear();
        for(int i = 0; i < o.quals().length(); i++) {
            Node newq = make_node();
            deep_copy_node(newq, o.quals()[i], value_alias_table, node_alias_table);
            n.quals() << newq;
        }
    
        n.scopes().clear();
        for(int i = 0; i < o.scopes().length(); i++) {
            Node news = make_node();
            deep_copy_node(news, o.scopes()[i], value_alias_table, node_alias_table);
            n.scopes() << news;
        }
    
        if(value_alias_table.hasKey(o.value().idx)) {
            Value aliased = value_alias_table.get(o.value().idx);
            n.value(aliased);
            if(is_live(aliased.type_scope())) {
                if(!n.scopes().empty()) {
                    Node ntyscope = aliased.type_scope();
                    n.scopes().col().set(0,(void*)&ntyscope);
                }
                else
                    n.scopes() << aliased.type_scope();
            }
        } else {
            if(is_live(o.value())) {
                if(!is_live(n.value())) {
                    n.value(make_value());
                }
                n.value().copy(o.value(),true);
            }
        }
    
        types[n.pool][n.idx].qset(node_value_table_offset,
            types[o.pool][o.idx].qget(node_value_table_offset), sizeof(Ptr));
        types[n.pool][n.idx].qset(node_node_table_offset,
            types[o.pool][o.idx].qget(node_node_table_offset), sizeof(Ptr));
    
        n.parent(o.parent_ptr());
        n.owner(o.owner_ptr());
        n.in_scope(o.in_scope_ptr());
        n.opt_str() = o.opt_str().to_std();
    }

    Node copy_as_token(Node node) {
        Node copy = make_node(node.type(),0,node.name().to_std(),node.x(),node.y(),node.z());
        copy.mute(true);
        for(int i=0;i<node.quals().length();i++) {
            Node q = node.quals()[i];
            if(q.mute()) {copy.quals() << q;}
        }
        return copy;
    }

    Node turn_into_token(Node node) {
        node.mute(true);
        return node;
    }


    void walk_nodenet(Node root, std::function<void(Node)> func) {
        func(root);
        for(int i=0;i<root.children().length();i++) walk_nodenet(root.children()[i],func);
        for(int i=0;i<root.quals().length();i++) walk_nodenet(root.quals()[i],func);
        for(int i=0;i<root.scopes().length();i++) if(root.scopes()[i].owner()==root) walk_nodenet(root.scopes()[i],func);
        if(is_live(root.value())) {
            for(int i=0;i<root.value().quals().length();i++) walk_nodenet(root.value().quals()[i],func);
        }
    }
        
    struct Unit : public q_object {
        Unit() {init();}

        map<std::string,g_ptr<Stage>> stages;
        Node unit_root = deadptr;
        std::string unit_label = "";
    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);

            Ptr p(0,0,false); //Just a dead pointer
            types[handler_type_id][stages_id].put(label,(void*)&p);

            return *new_stage.getPtr();
        }

        Stage& print_handlers = reg_stage("printing");


        std::string value_as_string(Value v) {
            Context ctx = make_context(); ctx.value(v);
            print_handlers.run(v.type())(ctx);
            deep_recycle_context(ctx);
            return ctx.source().to_std();
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
        list<Watcher> watchers;
        std::string GLOBAL_MSG = "";
        void log_to_watcher(Context& ctx, const std::string& msg) {
            GLOBAL_MSG = msg;
            for(auto& w : watchers) {if(w.logger) w.logger(ctx);}
            GLOBAL_MSG = "";
        }
    
        void start_logged_stage(Stage& stage) {
            active_stage = &stage;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagestart) w.stagestart((Context&)dead_ref);})
        }
        void end_logged_stage() {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.stagend) w.stagend((Context&)dead_ref);})
        }

        void start_stage(Stage& stage) {
            active_stage = &stage;
        }
    
        void start_stage(g_ptr<Stage> stage_ptr) {
            start_stage(*stage_ptr.getPtr());
        }

        void start_stage(Stage* stage_ptr) {
            start_stage(*stage_ptr);
        }

        virtual void init() {
            DEBUG_ONLY(
                Watcher def("core");
                def.stagestart = [this](Context& ctx){
                    if(active_stage) {
                        newline(active_stage->label);
                    }
                };
                def.prefix = [this](Context& ctx){
                    newline(active_stage->label+": "+node_info(ctx.node()));
                };
                def.suffix = [this](Context& ctx){
                    if(ERROR_FLAG) {log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
                    log(green("After: "),node_info(ctx.node()));
                    endline();
                };
                def.stagend = [this](Context& ctx){
                    endline();
                };
                watchers << def;
            )
        }
    
        virtual Node process(std::string path) {
            return deadptr;
        }
    
        virtual void run(Node root) {
            
        }
    
        bool standard_travel_pass(Node root, Context sub = deadptr);

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            active_stage->run(type)(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node().type());
        }
    
        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
        }
    
        void process_node(Context& ctx, Node node, Node left) {
            Node saved_node = ctx.node();
            Node saved_left = ctx.left();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            ctx.left(left);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.left(saved_left);
            ctx.sub(saved_sub);
        }
    
        void process_node(Node node, Node left) {
            Context ctx = make_context();
            process_node(ctx,node,left);
            deep_recycle_context(ctx);
        }
    
        void standard_sub_process_node(Node root) {
            Context ctx = make_context();
            ctx.node(root);
            standard_sub_process(ctx);
            deep_recycle_context(ctx);
        }

        void standard_sub_process(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());

            int& i = sub_ctx.index();
            while(i < sub_ctx.result().length()) {
                if(i==0) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i-1));
                }

                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})

                i++;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        void backwards_sub_process(Context& ctx) { 
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr());
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());
            int& i = sub_ctx.index();
            i = children.length()-1;
            while(i >= 0) {
                if(i==children.length()-1) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i+1));
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i--;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        //resolve_to_col(sub_ctx).qset(context_source_offset,(void*)&ctx.source_ptr(),sizeof(Ptr));

        void fire_quals(Context& ctx, Value value) {
            Value saved_value = ctx.value();
            ctx.value(value);
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+1))
                    (*active_stage)[qual.type()+1](ctx);
            }
            ctx.value(saved_value);
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            ctx.node(node);
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                if(active_stage->has(qual.type()+2))
                    (*active_stage)[qual.type()+2](ctx);
            }
            ctx.node(saved_node);
        }

        void standard_qual_process(Context& ctx) {
            for(int n=0;n<2;n++) {
                Context sub_ctx = make_context(n==0?ctx.node().quals():ctx.node().value().quals());
                int& i = sub_ctx.index();
                sub_ctx.root(ctx.node());
                sub_ctx.sub(ctx.sub());
                while(i < sub_ctx.result().length()) {
                    sub_ctx.qual(sub_ctx.result().get(i));
                    if(n==0) {
                        sub_ctx.node(ctx.node());
                    } else {
                        sub_ctx.value(ctx.node().value());
                    }
                    standard_process(sub_ctx,sub_ctx.qual().type());
                    sub_ctx.left(sub_ctx.result().get(i));
                    i++;
                }
                recycle_context(sub_ctx);
            }
        }
    
        void standard_direct_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a direct pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                ctx.left(ctx.result().get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
    
            node_col scopes = root.scopes();
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(scopes.get(i));
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_resolving_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a resolving pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Resolving pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            while(i < ctx.result().length()) {    //Process all nodes with scopes first (like any declerations)
                if(!ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then process nodes without scopes
                if(ctx.result()[i].scopes().empty()) {
                    ctx.node(ctx.result()[i]);
                    standard_process(ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    ctx.left(ctx.result()[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then the children of nodes with scopes
                if(!ctx.result()[i].scopes().empty()) {
                    standard_sub_process_node(ctx.result()[i]);
                }
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                i++;
            }
            i = 0;
            while(i < ctx.result().length()) {    //Then finnaly the subscopes
                if(!ctx.result()[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result()[i].scopes().length();s++) {
                        if(ctx.result()[i].scopes()[s].owner().idx==ctx.result()[i].idx) {
                            standard_resolving_pass(ctx.result()[i].scopes()[s]);
                            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                        }
                    }
                }
                i++;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void standard_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }

        void memory_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a memory backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children);
            int& i = ctx.index();
            ctx.root(root);
            i = ctx.result().length()-1;
            while(i >= 0) {
                ctx.node(ctx.result().get(i));
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner().idx==ctx.result().get(i).idx) {
                        memory_backwards_pass(scopes.get(s));
                        DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                    }
                }
                standard_process(ctx);
                DEBUG_ONLY(if(ERROR_FLAG) {endline(); return;})
                ctx.left(ctx.result().get(i));
                i--;
            }
            endline();
            deep_recycle_context(ctx);
        }
    };

    //Returns true if flagged for a return/break
    bool Unit::standard_travel_pass(Node root, Context sub) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a travel pass while an error was flagged")); return true;})
        node_col children = root.children();
        newline("Travel pass over "+std::to_string(children.length())+" nodes");
        Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr);
        int& i = ctx.index();
        ctx.root(root);
        ctx.sub(sub);
        while(i < ctx.result().length()) {
            ctx.node(ctx.result().get(i));
            standard_process(ctx);
            ctx.left(ctx.result().get(i));
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(ctx.flag()) { //This is the return/break process
                endline();
                deep_recycle_context(ctx);
                return true;
            }
            i++;
        }
        endline();
        deep_recycle_context(ctx);
        return false;
    }
}