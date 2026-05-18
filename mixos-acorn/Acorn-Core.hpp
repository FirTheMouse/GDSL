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
    list<TypePool> types;

    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2;



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
        t.note_value("UNDEFINED",sizeof(Ptr),ptr_id);
        t.add_row(0); //The nullptr
        t.note_value("stages",sizeof(Ptr),ptr_id);
        t.note_value("ptr",sizeof(Ptr),ptr_id);
        types << t;
        return at;
    }

    uint32_t handler_type_id = init_handler_type(); 

    uint32_t reg_id(const std::string& label) {
        uint32_t at = types[handler_type_id].columns.length();
        types[handler_type_id].note_value(label,sizeof(Ptr),ptr_id);
        labels.put(at,label);
        return at;
    }
    uint32_t float_id = reg_id("float"); uint32_t prefix_float_id = reg_id("prefix_float"); uint32_t suffix_float_id = reg_id("suffix_float");
    uint32_t int_id = reg_id("int"); uint32_t prefix_int_id = reg_id("prefix_int"); uint32_t suffix_int_id = reg_id("suffix_int");
    uint32_t bool_id = reg_id("bool"); uint32_t prefix_bool_id = reg_id("prefix_bool"); uint32_t suffix_bool_id = reg_id("suffix_bool");
    uint32_t string_id = reg_id("string"); uint32_t prefix_string_id = reg_id("prefix_string"); uint32_t suffix_string_id = reg_id("suffix_string");
    uint32_t char_id = reg_id("char"); uint32_t prefix_char_id = reg_id("prefix_char"); uint32_t suffix_char_id = reg_id("suffix_char");
    size_t list_id = reg_id("list");
    size_t map_id = reg_id("map");
    size_t weakptr_id = reg_id("weakptr");
    size_t col_id = reg_id("col");
    
    size_t identifier_id = reg_id("IDENTIFIER");
    size_t object_id = reg_id("OBJECT");
    size_t literal_id = reg_id("LITERAL");
    
    size_t node_id = reg_id("node"); size_t prefix_node_id = reg_id("prefix_node"); size_t suffix_node_id = reg_id("suffix_node");
    size_t value_id = reg_id("value"); size_t prefix_value_id = reg_id("prefix_value"); size_t suffix_value_id = reg_id("suffix_value");

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
        tombstone_col = t.note_value("dead",1,bool_id);
        refs_col = t.note_value("alive",4,int_id);
        return at;
    }

    uint32_t make_store_type() {
        uint32_t at = add_type();
        TypePool& t = types[at];
        return at;
    }
    
    size_t node_type_col = 0;
    size_t node_sub_type_col = 0;
    size_t node_name_col = 0;
    size_t x_col = 0;
    size_t y_col = 0;
    size_t z_col = 0;
    size_t node_value_col = 0;
    size_t node_children_col = 0;
    size_t node_quals_col = 0;
    size_t node_node_table_col = 0;
    size_t node_value_table_col = 0;
    size_t node_scopes_col = 0;
    size_t parent_col = 0;
    size_t owner_col = 0;
    size_t in_scope_col = 0;
    size_t is_scope_col = 0;
    size_t node_opt_str_col = 0;
    size_t mute_col = 0;

    uint32_t value_type_col = 0;
    uint32_t value_sub_type_col = 0;
    uint32_t value_data_col = 0;
    uint32_t address_col = 0;
    uint32_t reg_col = 0;
    uint32_t loc_col = 0;
    uint32_t size_col = 0;
    uint32_t sub_size_col = 0;
    uint32_t value_quals_col = 0;
    uint32_t value_sub_values_col = 0;
    uint32_t type_scope_col = 0;
    uint32_t store_col = 0;

    uint32_t init_node_type();
    uint32_t init_value_type();

    uint32_t node_type_id = init_node_type();
    uint32_t value_type_id = init_value_type();
    uint32_t name_store_id = make_store_type();
    uint32_t children_store_id = make_store_type();
    uint32_t quals_store_id = make_store_type();
    uint32_t node_table_store_id = make_store_type(); 
    uint32_t value_table_store_id = make_store_type(); 
    uint32_t scopes_store_id = make_store_type(); 
    uint32_t opt_str_store_id = make_store_type();

    uint32_t data_store_id = make_store_type();
    uint32_t sub_value_store_id = make_store_type();

    uint32_t init_node_type() {
        uint32_t at = make_object_type();
        TypePool& t = types[at];
        node_type_col = t.note_value("type",4,int_id);
        node_sub_type_col = t.note_value("sub_type",4,int_id);
        node_name_col = t.note_value("name",sizeof(Ptr),string_id);
        x_col = t.note_value("x",4,float_id);
        y_col = t.note_value("y",4,float_id);
        z_col = t.note_value("z",4,float_id);
        node_value_col = t.note_value("value",sizeof(Ptr),ptr_id);
        node_children_col = t.note_value("children",sizeof(Ptr),list_id);
        node_quals_col = t.note_value("quals",sizeof(Ptr),list_id);
        node_node_table_col = t.note_value("node_table",sizeof(Ptr),map_id);
        node_value_table_col = t.note_value("value_table",sizeof(Ptr),map_id);
        node_scopes_col = t.note_value("scopes",sizeof(Ptr),list_id);
        parent_col = t.note_value("parent",sizeof(Ptr),ptr_id);
        owner_col = t.note_value("owner",sizeof(Ptr),ptr_id);
        in_scope_col = t.note_value("in_scope",sizeof(Ptr),ptr_id);
        is_scope_col = t.note_value("is_scope",1,bool_id);
        node_opt_str_col = t.note_value("opt_str",sizeof(Ptr),string_id);
        mute_col = t.note_value("mute",1,bool_id);

        t.init_funcs << [at](Ptr& optr) {
            TypePool& t = types[at];
            optr.pool = at;
            Ptr nameptr{name_store_id, (uint32_t)types[name_store_id].note_value("",sizeof(char),char_id), 0};
            t.set(node_name_col, optr.sidx, (void*)&nameptr);
        
            Ptr childrenptr{children_store_id, (uint32_t)types[children_store_id].note_value("",sizeof(Ptr),ptr_id), 0};
            t.set(node_children_col, optr.sidx, (void*)&childrenptr);
        
            Ptr qualsptr{quals_store_id, (uint32_t)types[quals_store_id].note_value("",sizeof(Ptr),ptr_id), 0};
            t.set(node_quals_col, optr.sidx, (void*)&qualsptr);
        
            Ptr scopesptr{scopes_store_id, (uint32_t)types[scopes_store_id].note_value("",sizeof(Ptr),ptr_id), 0};
            t.set(node_scopes_col, optr.sidx, (void*)&scopesptr);
        
            Ptr nodetableptr{node_table_store_id, (uint32_t)types[node_table_store_id].note_value("",sizeof(Ptr),ptr_id), 0};
            t.set(node_node_table_col, optr.sidx, (void*)&nodetableptr);
        
            Ptr valuetableptr{value_table_store_id, (uint32_t)types[value_table_store_id].note_value("",sizeof(Ptr),ptr_id), 0};
            t.set(node_value_table_col, optr.sidx, (void*)&valuetableptr);
        
            Ptr optstrptr{opt_str_store_id, (uint32_t)types[opt_str_store_id].add_column(sizeof(char)), 0};
            t.set(node_opt_str_col, optr.sidx, (void*)&optstrptr);
            
            Ptr deadptr; deadptr.idx = 0;
            t.set(parent_col, optr.sidx, (void*)&deadptr);
            t.set(owner_col, optr.sidx, (void*)&deadptr);
            t.set(in_scope_col, optr.sidx, (void*)&deadptr);
            t.set(node_value_col, optr.sidx, (void*)&deadptr);
    
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
        TypePool& t = types[at];

        value_type_col = t.note_value("type",4,int_id);
        value_sub_type_col = t.note_value("sub_type",4,int_id);
        value_data_col = t.note_value("data",sizeof(Ptr),ptr_id);
        address_col = t.note_value("address",4,int_id);
        reg_col = t.note_value("reg",4,int_id);
        loc_col = t.note_value("loc",4,int_id);
        size_col = t.note_value("size",4,int_id);
        sub_size_col = t.note_value("sub_size",4,int_id);
        value_quals_col = t.note_value("quals",sizeof(Ptr),ptr_id);
        value_sub_values_col = t.note_value("sub_values",sizeof(Ptr),ptr_id);
        type_scope_col = t.note_value("type_scope",sizeof(Ptr),ptr_id);
        store_col = t.note_value("store",sizeof(Ptr),ptr_id);

        t.init_funcs << [at](Ptr& optr) {
            TypePool& t = types[at];
            optr.pool = at;                       
            Ptr qualsptr{quals_store_id, (uint32_t)types[quals_store_id].add_column(sizeof(Ptr)), 0};
            t.set(value_quals_col, optr.sidx, (void*)&qualsptr);
        
            Ptr subvalueptr{sub_value_store_id, (uint32_t)types[sub_value_store_id].add_column(sizeof(Ptr)), 0};
            t.set(value_sub_values_col, optr.sidx, (void*)&subvalueptr);
        
            Ptr deadptr; deadptr.idx = 0;
            t.set(type_scope_col, optr.sidx, (void*)&deadptr);

            Ptr noptr{0, 0, 0};
            t.set(store_col, optr.sidx, (void*)&noptr);
            t.set(value_data_col, optr.sidx, (void*)&noptr);

            int neg_one = -1;
            t.set(reg_col, optr.sidx, (void*)&neg_one);
        };

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


    struct string {
        string() {}
        string(Ptr ptr) : col_ptr(ptr) {}
        Ptr col_ptr;
        inline Col& col() {return types[col_ptr.pool][col_ptr.idx]; }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col().push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {return std::string((char*)col().storage, length());}
    };

    #define NAMED_PTRS 1

    std::string Ptr_as_string(Ptr p) {
        #if NAMED_PTRS
            std::string plabel = types[p.pool].type_name=="bullets"?std::to_string(p.pool):types[p.pool].type_name;
            std::string pidx = types[p.pool][p.idx].label.empty()?std::to_string(p.idx):types[p.pool][p.idx].label;
            return ""+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
        #else
            return ""+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx)+"";
        #endif
    }

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }


    struct value_table {
        Ptr col_ptr;

        value_table() {}
        value_table(Ptr _col_ptr) : col_ptr(_col_ptr) {}

        inline Col& col() {return types[col_ptr.pool][col_ptr.idx]; }
        inline bool hasKey(const std::string& key) {return col().hasKey(key);}
        inline Value get(const std::string& key);
        inline Value operator[](const std::string& key);
        inline void put(const std::string& key, Value value);
    };

    struct node_table {
        Ptr col_ptr;

        node_table() {}
        node_table(Ptr _col_ptr) : col_ptr(_col_ptr) {}
        inline Col& col() {return types[col_ptr.pool][col_ptr.idx]; }
        inline bool hasKey(const std::string& key) {return col().hasKey(key);}
        inline Node get(const std::string& key);
        inline Node operator[](const std::string& key);
        inline void put(const std::string& key, Node node);
    };

    struct node_list {
        Ptr col_ptr;
        
        node_list(Ptr _col_ptr) : col_ptr(_col_ptr) {}
        node_list() {};
        
        inline Col& col()                         {return types[col_ptr.pool][col_ptr.idx]; }

        inline uint32_t length() { return col().length(); }
        inline bool empty() { return col().empty(); }
        
        inline Node get(uint32_t idx);
        inline Node operator[](uint32_t idx);
        inline Node last();
        inline void removeAt(uint32_t idx)        {col().removeAt(idx);}
        inline Node take(uint32_t idx);
        inline Node pop();
        inline void clear() {col().clear();}
        inline void push(Node n);
        inline void operator<<(Node n);
    };

    struct value_list {
        Ptr col_ptr;
        
        value_list(Ptr _col_ptr) : col_ptr(_col_ptr) {}
        value_list() {};
        
        inline Col& col()                         {return types[col_ptr.pool][col_ptr.idx]; }

        inline uint32_t length() { return col().length(); }
        inline bool empty() { return col().empty(); }
        
        inline Value get(uint32_t idx);
        inline Value operator[](uint32_t idx);
        inline Value last();
        inline void removeAt(uint32_t idx)        {col().removeAt(idx);}
        inline Value take(uint32_t idx);
        inline Value pop();
        inline void clear() {col().clear();}
        inline void push(Value n);
        inline void operator<<(Value n);
    };

    struct Value : public Ptr {

        Value() {}
        Value(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}
    
        inline uint32_t& type()                 {return *(uint32_t*)types[value_type_id][value_type_col][sidx];}
        inline void      type(uint32_t t)       {types[value_type_id][value_type_col].set(sidx,(void*)&t);}
        inline uint32_t& sub_type()             {return *(uint32_t*)types[value_type_id][value_sub_type_col][sidx];}
        inline void      sub_type(uint32_t st)  {types[value_type_id][value_sub_type_col].set(sidx,(void*)&st);}
    
        inline Ptr&      data_ptr()             {return *(Ptr*)types[value_type_id][value_data_col][sidx];}
        inline Col&      data_col()             {Ptr& p = data_ptr(); return types[p.pool][p.idx];}
    
        inline uint32_t& address()              {return *(uint32_t*)types[value_type_id][address_col][sidx];}
        inline void      address(uint32_t v)    {return types[value_type_id][address_col].set(sidx,(void*)&v);}
        inline int&      reg()                  {return *(int*)types[value_type_id][reg_col][sidx];}
        inline void      reg(int i)             {types[value_type_id][reg_col].set(sidx,(void*)&i);}
        inline int&      loc()                  {return *(int*)types[value_type_id][loc_col][sidx];}
    
        inline uint32_t& size()                 {return *(uint32_t*)types[value_type_id][size_col][sidx];}
        inline void      size(uint32_t s)       {types[value_type_id][size_col].set(sidx,(void*)&s);}
        inline uint32_t& sub_size()             {return *(uint32_t*)types[value_type_id][sub_size_col][sidx];}
        inline void      sub_size(uint32_t s)   {types[value_type_id][sub_size_col].set(sidx,(void*)&s);}
    
        inline Ptr&      quals_ptr()            {return *(Ptr*)types[value_type_id][value_quals_col][sidx];}
        inline Col&      quals_col()            {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_list quals()                {return node_list(quals_ptr());}
    
        inline Ptr&       sub_values_ptr()      {return *(Ptr*)types[value_type_id][value_sub_values_col][sidx];}
        inline Col&       sub_values_col()      {Ptr& p = sub_values_ptr(); return types[p.pool][p.idx];}
        inline value_list sub_values()          {return value_list(sub_values_ptr());}
    
        inline Node      type_scope();
        inline void      type_scope(Ptr o)  {return types[value_type_id][type_scope_col].set(sidx,(void*)&o);}
        inline Ptr&      store()                {return *(Ptr*)types[value_type_id][store_col][sidx];}

        inline void setup(uint32_t _type, uint32_t _size, uint32_t _address = 0) {
            type(_type); size(_size); address(_address);
        }

        inline Ptr init_data() {
            Ptr dataptr = {data_store_id, types[data_store_id].note_value("",size(),type()), 0};
            types[data_store_id].add_row(dataptr.idx);
            types[pool][value_data_col].set(sidx,(void*)&dataptr);
            return dataptr;
        }

        inline void set(void* data) {
            Ptr dataptr = data_ptr();
            if(dataptr.pool==0) {
               dataptr = init_data();
            }
            types[dataptr.pool][dataptr.idx].set(dataptr.sidx,data);
        }
        inline void* get() {
            Ptr dataptr = data_ptr();
            return types[dataptr.pool][dataptr.idx][dataptr.sidx];
        }

        inline void copy(Value o) {
            TypePool& t = types[value_type_id];
            for(int c = 0; c < t.columns.length(); c++) {
                t.columns[c].set(sidx, t.columns[c].get(o.sidx));
            }
            idx = o.idx;
        }
    };
    
    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) { pool = p.pool; sidx = p.sidx; idx = p.idx;}
    
        inline uint32_t& type()                   {return *(uint32_t*)types[node_type_id][node_type_col][sidx];}
        inline void      type(uint32_t t)         {types[node_type_id][node_type_col].set(sidx,(void*)&t);}
        inline uint32_t& sub_type()               {return *(uint32_t*)types[node_type_id][node_sub_type_col][sidx];}
        inline void      sub_type(uint32_t st)    {types[node_type_id][node_sub_type_col].set(sidx,(void*)&st);}
        
        inline Ptr&      name_ptr()               {return *(Ptr*)types[node_type_id][node_name_col][sidx];}
        inline Col&      name_col()               {Ptr& p = name_ptr(); return types[p.pool][p.idx];}
        inline string    name()                   {return string(name_ptr());}
        inline void      name(std::string s)      {name() = s;}
        
        inline float&    x()                      {return *(float*)types[node_type_id][x_col][sidx];}
        inline float&    y()                      {return *(float*)types[node_type_id][y_col][sidx];}
        inline float&    z()                      {return *(float*)types[node_type_id][z_col][sidx];}
        
        inline Ptr value_ptr()                {return *(Ptr*)types[node_type_id][node_value_col][sidx];}
        inline Value   value()                    {return Value(value_ptr());}
        inline void    value(Ptr ptr)         {types[node_type_id][node_value_col].set(sidx,(void*)&ptr);}
        
        inline Ptr&      children_ptr()           {return *(Ptr*)types[node_type_id][node_children_col][sidx];}
        inline Col&      children_col()           {Ptr& p = children_ptr(); return types[p.pool][p.idx];}
        inline node_list children()               {return node_list(children_ptr());}
        inline void      children(node_list l)    {types[node_type_id][node_children_col].set(sidx,(void*)&l);}
        
        inline Ptr&      quals_ptr()              {return *(Ptr*)types[node_type_id][node_quals_col][sidx];}
        inline Col&      quals_col()              {Ptr& p = quals_ptr(); return types[p.pool][p.idx];}
        inline node_list quals()                  {return node_list(quals_ptr());}
        
        inline Ptr&        node_table_ptr()         {return *(Ptr*)types[node_type_id][node_node_table_col][sidx];}
        inline Col&        node_table_col()         {Ptr& p = node_table_ptr(); return types[p.pool][p.idx];}
        inline node_table  node_table()             {return Acorn::node_table(node_table_ptr());}
        
        inline Ptr&        value_table_ptr()        {return *(Ptr*)types[node_type_id][node_value_table_col][sidx];}
        inline Col&        value_table_col()        {Ptr& p = value_table_ptr(); return types[p.pool][p.idx];}
        inline value_table value_table()            {return Acorn::value_table(value_table_ptr());}
        
        inline Ptr&      scopes_ptr()             {return *(Ptr*)types[node_type_id][node_scopes_col][sidx];}
        inline Col&      scopes_col()             {Ptr& p = scopes_ptr(); return types[p.pool][p.idx];}
        inline node_list scopes()                 {return node_list(scopes_ptr());}
        
        inline Ptr&  parent_ptr()             {return *(Ptr*)types[node_type_id][parent_col][sidx];}
        inline Node      parent()                 {return Node(parent_ptr());}
        inline void      parent(Ptr p)        {types[node_type_id][parent_col].set(sidx,(void*)&p);}
        inline Ptr&  owner_ptr()              {return *(Ptr*)types[node_type_id][owner_col][sidx];}
        inline Node      owner()                  {return Node(owner_ptr());}
        inline void      owner(Ptr p)         {types[node_type_id][owner_col].set(sidx,(void*)&p);}
        inline Ptr&  in_scope_ptr()           {return *(Ptr*)types[node_type_id][in_scope_col][sidx];}
        inline Node      in_scope()               {return Node(in_scope_ptr());}
        inline void      in_scope(Ptr p)      {types[node_type_id][in_scope_col].set(sidx,(void*)&p);}
        inline bool&     is_scope()               {return *(bool*)types[node_type_id][is_scope_col][sidx];}
        
        inline Ptr&      opt_str_ptr()            {return *(Ptr*)types[node_type_id][node_opt_str_col][sidx];}
        inline Col&      opt_str_col()            {Ptr& p = opt_str_ptr(); return types[p.pool][p.idx];}
        inline string    opt_str()                {return string(opt_str_ptr());}
        
        inline bool&     mute()                   {return *(bool*)types[node_type_id][mute_col][sidx];}
    
    
        inline void copy(Node o) {
            TypePool& t = types[node_type_id];
            for(int c = 0; c < t.columns.length(); c++) {
                t.columns[c].set(sidx, t.columns[c].get(o.sidx));
            }
            idx = o.idx;
        }
    };

    inline Node Value::type_scope() {return Node(*(Ptr*)types[value_type_id][type_scope_col][sidx]);}


    inline Node node_table::get(const std::string& key) {return Node(*(Ptr*)col().get(key));}
    inline Node node_table::operator[](const std::string& key) {return get(key);}
    inline void node_table::put(const std::string& key, Node node) {
        Ptr p(node.pool,1,node.sidx);
        col().put(key, (void*)&p);
    }

    inline Value value_table::get(const std::string& key) {return Value(*(Ptr*)col().get(key));}
    inline Value value_table::operator[](const std::string& key) {return get(key);}
    inline void value_table::put(const std::string& key, Value value) {
        Ptr p(value.pool,1,value.sidx);
        col().put(key, (void*)&p);
    }

    inline Node node_list::get(uint32_t idx)             {return Node(*(Ptr*)col().get(idx));}
    inline Node node_list::operator[](uint32_t idx)      {return get(idx);}
    inline Node node_list::last()                        {return get(length()-1);}
    inline Node node_list::take(uint32_t idx) {
        Node val = get(idx);
        removeAt(idx);
        return val;
    }
    inline Node node_list::pop() {
        Ptr p;
        col().pop(&p);
        return Node(p);
    }
    inline void node_list::push(Node n) {
        Ptr p(n.pool, 1, n.sidx);
        col().push(&p);
    }
    inline void node_list::operator<<(Node n) {push(n);}


    inline Value value_list::get(uint32_t idx)             {return Value(*(Ptr*)col().get(idx));}
    inline Value value_list::operator[](uint32_t idx)      {return get(idx);}
    inline Value value_list::last()                        {return get(length()-1);}
    inline Value value_list::take(uint32_t idx) {
        Value val = get(idx);
        removeAt(idx);
        return val;
    }
    inline Value value_list::pop() {
        Ptr p;
        col().pop(&p);
        return Value(p);
    }
    inline void value_list::push(Value n) {
        Ptr p(n.pool, 1, n.sidx);
        col().push(&p);
    }
    inline void value_list::operator<<(Value n) {push(n);}

    Node make_node() {return Node(types[node_type_id].create());}
    Node make_node(uint32_t type) {
        Node n(types[node_type_id].create());
        n.type(type);
        return n;
    }

    Value make_value() {
        Ptr p = types[value_type_id].create();
        return Value(p);
    }
    Value make_value(uint32_t type, uint32_t size, uint32_t addr = 0, uint32_t subtype = 0, uint32_t subsize = 0) {
        Value v = make_value();
        v.size(size); v.type(type); v.address(addr); v.sub_type(subtype); v.sub_size(subsize);
        return v;
    }


    Ptr last_source_ptr = {0,0,0};

    struct Context {
        inline void allocate_source() {
            if(last_source_ptr.pool==0) {
                last_source_ptr = Ptr{name_store_id,types[name_store_id].note_value("sourcestore",1,char_id),0};
            } else {
                string(last_source_ptr).col().clear();
            }
            source.col_ptr = last_source_ptr;
        }

        Context() : index(_ctx_dummy_index) {
            allocate_source();
        }
        Context(int& _index) : index(_index) {
            allocate_source();
        }
        Context(node_list _result, int& _index) : result(_result), index(_index) {
            allocate_source();
        }
        
        Node node;
        Node qual;
        Node left;
        Node out;
        Node root;
        node_list result;
        list<Ptr> nodes;
        Value value;
        int& index;
        uint32_t state = 0;
        bool flag = false;
    
        Context* sub;
        list<uint32_t>* buffer;
    
        string source;
    
        Context duplicate() {
            return Context(result, index);
        }
    };

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


    std::string tag_to_str(uint32_t tag, void* data) {
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
            std::string content = string(ptr).to_std();
            content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());
            return Ptr_as_string(ptr)+"> \""+content+"\"";
        } else if(tag==ptr_id) {
            return Ptr_as_string(*(Ptr*)data);
        } else if(tag==ptr_id||tag==node_id||tag==value_id) {
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
            for(auto e : col.cells.entrySet()) {
                s+="{"+e.key+": "+tag_to_str(col.tag,col[e.value])+"}";
            }
            s+="]";
            return s;
        } else {
            return labels[tag]+"?";
        }
    }

    std::string pad_str(const std::string& s, uint32_t width) {
        std::string to_return = s;
        while(width>to_return.length()) to_return+=" ";
        return to_return;
    }

    std::string center_pad(const std::string& s, uint32_t width) {
        if(s.length() >= width) return s;
        uint32_t total_pad = width - s.length();
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }

    uint32_t digit_count(uint32_t n) {
        if(n == 0) return 1;
        uint32_t digits = 0;
        while(n > 0) { n /= 10; digits++; }
        return digits;
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

    std::string print_columnar_table(list<list<std::string>>& lines) {
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

    std::string type_to_string(TypePool& t) {
        list<list<std::string>> lines;
        list<uint32_t> dtypes;
        for(int c=0;c<t.column_count();c++) {
            Col& col = t.columns[c];
            list<std::string> subline;
            subline << col.label;
            for(int r=0;r<col.length();r++) {
                std::string line = "";
                if(col.label=="type"||col.label=="sub_type") {
                    if(col.label=="type"){dtypes << *(int*)col[r];} //Passing info down so values can print themselves out
                    line+=labels[*(int*)col[r]];
                } else if(col.label=="stages") { //From the handler type
                    std::string cell_label = col.get_cell_label(r);
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
            lines << subline;
        }
        // int grouplen = 5;
        // if(lines.length()>grouplen) {
        //     while(lines.length()>grouplen) {
        //         list<list<std::string>> group;
        //         for(int i=0;i<grouplen;i++) {if(i>=lines.length()) {break;} group << lines.take(i);}
        //         print(print_columnar_table(group));
        //     }
        // } 
        return print_columnar_table(lines);
    }

    std::string value_info(Value value) {
        std::string to_return = "";
        to_return += cyan("["+Ptr_as_string(value)+"]")+
        + "(type: " + green(labels[value.type()])
        + (value.reg()!=-1?", reg: "+std::to_string(value.reg()):"")
        + (value.data_ptr().pool!=0?", value: "+gray(tag_to_str(value.type(),resolve_ptr(value.data_ptr())))+" @"+Ptr_as_string(value.data_ptr()):"")
        + (value.sub_type()!=0?", sub_type: "+labels[value.sub_type()]:"")
        + (value.type_scope().idx!=0?", type_scope: "+blue(Ptr_as_string(value.type_scope())):"")
        + (value.size()!=0?", size: "+std::to_string(value.size()):"")
        + (value.address()!=0?", address: "+std::to_string(value.address()):"")
        + (value.store().pool!=0?", store: "+Ptr_as_string(value.store()):"")
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
        std::string to_return = "";
        
        to_return += blue(Ptr_as_string(node)+" ")
        + labels[node.type()]
        + (node.sub_type()==0?"":":"+labels[node.sub_type()])
        + (node.name().length()==0?"":" "+green(node.name().to_std())+" ") 
        + (node.value().idx!=0?value_info(node.value()):"")
        // + (node->x!=-1.0f?"("+std::to_string((int)node->x)+","+std::to_string((int)node->y)+")":"")
        + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"")
        + (!node.scopes().empty()?"[S:"+std::to_string(node.scopes().length())+"]":"")
        + (node.in_scope().idx!=0?"{"+node.in_scope().name().to_std()+"}":"");
        return to_return;
    }

    // #define MUTE_TABLES 1
    // #define MUTE_MUTED_QUALS 1

    std::string node_to_string(Node node, int depth = 0, int index = 0, bool print_sub_scopes = false, std::string sigil = "") {
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + sigil + std::to_string(index) + ": " + node_info(node);
        
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

        // if(node.owner().live) {
        //     to_return +=  "\n" + indent + "  Owner: " + node_info(node.owner());
        // }

        if(!node.children().empty()) {
            for(int i=0;i<node.children().length();i++) {
                if(node.children()[i].idx!=0) {
                    if(node.children()[i].sidx==node.sidx) {
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
                if(print_sub_scopes&&scope.owner().sidx==node.sidx) {
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
        labels[ptr_id] = "ptr";
        labels[ptr_id] = "Ptr";

        types[handler_type_id].type_name = "handlers";
        types[node_type_id].type_name = "nodes";
        types[value_type_id].type_name = "values";
        types[name_store_id].type_name = "names";
        types[children_store_id].type_name = "children";
        types[quals_store_id].type_name = "quals";
        types[node_table_store_id].type_name = "node table";
        types[value_table_store_id].type_name = "value table";
        types[scopes_store_id].type_name = "scopes";
        types[opt_str_store_id].type_name = "opt_str";
        types[data_store_id].type_name = "data";
        types[sub_value_store_id].type_name = "sub_value";
        
        value_table look;
        Node ntemp(types[node_type_id].create()); //The node template
        ntemp.name("Node template");
        ntemp.value_table_col().label = "node template";
        look = ntemp.value_table();
        look.put("type",make_value(int_id,4,node_type_col));
        look.put("name",make_value(string_id,sizeof(Ptr),node_name_col));
        look.put("children",make_value(col_id,sizeof(Ptr),node_children_col,node_id,sizeof(Ptr)));
        look.put("value",make_value(value_id,sizeof(Ptr),node_value_col));
        look.put("scopes",make_value(col_id,sizeof(Ptr),node_scopes_col,node_id,sizeof(Ptr)));
        look.put("quals",make_value(col_id,sizeof(Ptr),node_quals_col,node_id,sizeof(Ptr)));

        Node vtemp(types[node_type_id].create()); //The value template
        vtemp.name("Value template");
        vtemp.value_table_col().label = "value template";
        look = vtemp.value_table();
        look.put("type",make_value(int_id,4,value_type_col));
        look.put("size",make_value(int_id,4,size_col));
        look.put("data",make_value(ptr_id,sizeof(Ptr),value_data_col));
        look.put("quals",make_value(col_id,sizeof(Ptr),value_quals_col,node_id,sizeof(Ptr)));
    }

    Node scan_for_node(const std::string& label, Node from) {
        for(int i=0;i<from.children().length();i++) {
            if(from.children()[i].name().to_std()==label) {
                return from.children()[i];
            }
        }
        for(int i=0;i<from.scopes().length();i++) {
            Node found = scan_for_node(label,from.scopes()[i]);
            if(found.pool!=0) {
                return found;
            }
        }
        return Ptr(0,0,0);
}
        
    struct Unit : public q_object {
        Unit() {init();}

        map<std::string,g_ptr<Stage>> stages;
        Node unit_root;
    
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
            Context ctx; ctx.value = v;
            print_handlers.run(v.type())(ctx);
            return ctx.source.to_std();
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


        virtual void init() {

        }
    
        virtual Node process(std::string path) {
            Ptr obj = types[node_type_id].create();
            Node n;
            n.sidx = obj.sidx;
            n.pool = node_type_id;
            n.idx = 1;
            return n;
        }
    
        virtual void run(Node root) {
            
        }
    
        bool standard_travel_pass(Node root, Context* sub = nullptr);

                    // if(types[handler_type_id][type].length()>0) {
            //     uint32_t stage_id = types[handler_type_id][stages_id].cells.get(active_stage->label);
            //     Node nhandler = Node(*(Ptr*)types[handler_type_id][type][stage_id]);
            //     if(nhandler.idx!=0) {
            //         standard_travel_pass(nhandler,&ctx);
            //     } else {
            //         active_stage->run(type)(ctx);
            //     }
            // } else {
            //     active_stage->run(type)(ctx);
            // }

        inline void standard_process(Context& ctx, uint32_t type) {
            newline(active_stage->label+": "+node_info(ctx.node));
            active_stage->run(type)(ctx);
            endline();
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node.type());
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
            sub_ctx.source.col_ptr = ctx.source.col_ptr;
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

        void fire_quals(Context& ctx, Value value) {
            Value saved_value = ctx.value;
            ctx.value = value;
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual = qual;
                if(active_stage->has(qual.type()+1))
                    (*active_stage)[qual.type()+1](ctx);
            }
            ctx.value = saved_value;
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node;
            ctx.node = node;
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual = qual;
                if(active_stage->has(qual.type()+2))
                    (*active_stage)[qual.type()+2](ctx);
            }
            ctx.node = saved_node;
        }

        void standard_qual_process(Context& ctx) {
            for(int n=0;n<2;n++) {
                int i = 0;
                Context sub_ctx(n==0?ctx.node.quals():ctx.node.value().quals(),i);
                sub_ctx.root = ctx.node;
                sub_ctx.sub = ctx.sub;
                while(i < sub_ctx.result.length()) {
                    sub_ctx.qual = sub_ctx.result.get(i);
                    if(n==0) {
                        sub_ctx.node = ctx.node;
                    } else {
                        sub_ctx.value = ctx.node.value();
                    }
                    standard_process(sub_ctx,sub_ctx.qual.type());
                    sub_ctx.left = sub_ctx.result.get(i);
                    i++;
                }
            }
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
    
        void standard_direct_pass(Node root, list<uint32_t>* buffer = nullptr) {
            node_list children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            int i = 0;
            Context ctx(children,i);
            ctx.buffer = buffer;
            ctx.root = root;
            while(i < ctx.result.length()) {
                ctx.node = ctx.result.get(i);
                standard_process(ctx);
                ctx.left = ctx.result.get(i);
                i++;
            }
    
            node_list scopes = root.scopes();
            for(int i = 0; i<scopes.length(); i++) {
                standard_direct_pass(scopes.get(i),buffer);
            }
            endline();
        }
    
        void standard_resolving_pass(Node root) {
            node_list children = root.children();
            newline("Resolving pass over "+std::to_string(children.length())+" nodes");
            int i = 0;
            Context ctx(children,i);
            ctx.root = root;
            while(i < ctx.result.length()) {
                if(!ctx.result[i].scopes().empty()) {
                    ctx.node = ctx.result[i];
                    standard_process(ctx);
                    ctx.left = ctx.result[i];
                }
                i++;
            }
            i = 0;
            while(i < ctx.result.length()) {
                if(ctx.result[i].scopes().empty()) {
                    ctx.node = ctx.result[i];
                    standard_process(ctx);
                    ctx.left = ctx.result[i];
                }
                i++;
            }
            i = 0;
            while(i < ctx.result.length()) {
                if(!ctx.result[i].scopes().empty()) {
                    standard_sub_process_node(ctx.result[i]);
                }
                i++;
            }
            i = 0;
            while(i < ctx.result.length()) {
                if(!ctx.result[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result[i].scopes().length();s++) {
                        standard_resolving_pass(ctx.result[i].scopes()[s]);
                    }
                }
                i++;
            }

            // for(int s = 0;s<root.scopes().length();s++) {
            //     standard_resolving_pass(root.scopes()[s]);
            // }
            endline();
        }
    };

    //Returns true if flagged for a return/break
    bool Unit::standard_travel_pass(Node root, Context* sub) {
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
}