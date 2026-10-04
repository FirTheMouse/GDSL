#pragma once
#include "../mixos-acorn/Acorn-Core.hpp"

namespace Acorn {
    struct Node;
    struct Context;
    struct Value;


    inline thread_local bool SHUTDOWN_FLAG = false;

    #ifdef _WIN32
        void setup_signals() {}
    #else
        inline void signal_handler(int signal) {
            print("\nRECIVED SIGNAL: ",signal);
            if(ERROR_FLAG) {
                std::abort();
            }
            ERROR_FLAG = true;
            ERROR_MSG = "Console interrupt";
        }

        inline void setup_signals() {
            struct sigaction sa;
            sa.sa_handler = signal_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, nullptr);
        }
    #endif


    using Handler = std::function<void(Context&)>;
    struct Stage : q_object {
        Stage() {}

        map<uint32_t,Handler> handlers;
        Handler default_function = nullptr;

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
        Handler passstart = nullptr;
    };  


    inline uint32_t node_type_offset = 0;
    inline uint32_t node_sub_type_offset = 0;
    inline uint32_t node_name_offset = 0;
    inline uint32_t x_offset = 0;
    inline uint32_t y_offset = 0;
    inline uint32_t z_offset = 0;
    inline uint32_t node_value_offset = 0;
    inline uint32_t node_children_offset = 0;
    inline uint32_t node_quals_offset = 0;
    inline uint32_t node_node_table_offset = 0;
    inline uint32_t node_value_table_offset = 0;
    inline uint32_t node_scopes_offset = 0;
    inline uint32_t parent_offset = 0;
    inline uint32_t owner_offset = 0;
    inline uint32_t in_scope_offset = 0;
    inline uint32_t resolved_offset = 0;
    inline uint32_t node_opt_str_offset = 0;
    inline uint32_t mute_offset = 0;

    inline uint32_t value_type_offset = 0;
    inline uint32_t value_sub_type_offset = 0;
    inline uint32_t value_data_offset = 0;
    inline uint32_t address_offset = 0;
    inline uint32_t reg_offset = 0;
    inline uint32_t loc_offset = 0;
    inline uint32_t size_offset = 0;
    inline uint32_t sub_size_offset = 0;
    inline uint32_t value_quals_offset = 0;
    inline uint32_t value_sub_values_offset = 0;
    inline uint32_t type_scope_offset = 0;
    inline uint32_t store_offset = 0;

    inline uint32_t context_node_offset = 0;
    inline uint32_t context_qual_offset = 0;
    inline uint32_t context_left_offset = 0;
    inline uint32_t context_out_offset = 0;
    inline uint32_t context_root_offset = 0;
    inline uint32_t context_result_offset = 0;
    inline uint32_t context_value_offset = 0;
    inline uint32_t context_index_offset = 0;
    inline uint32_t context_state_offset = 0;
    inline uint32_t context_flag_offset = 0;
    inline uint32_t context_sub_offset = 0;
    inline uint32_t context_source_offset = 0;
    inline uint32_t context_pass_offset = 0;
    inline uint32_t context_parent_offset = 0;

    inline uint32_t node_total_size = 0;
    inline uint32_t value_total_size = 0;
    inline uint32_t context_total_size = 0;


    using node_col  = col_Ptr<Node>;
    using value_col = col_Ptr<Value>;

    struct Value : public Ptr {
        Value() {}
        Value(Ptr p) : Ptr(p) {}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but value was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("value:type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(value_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("value:type:set")){return;}) resolve_to_col(*this).qset(value_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("value:sub_type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(value_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("value:sub_type:set")){return;}) resolve_to_col(*this).qset(value_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      data_ptr()            {DEBUG_ONLY(if(safety_check("value:data_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_data_offset);}
        inline void      data_ptr(Ptr ptr)     {DEBUG_ONLY(if(safety_check("value:data_ptr:set")){return;}) resolve_to_col(*this).qset(value_data_offset,(void*)&ptr,sizeof(Ptr));}
        inline Col&      data_col()            {Ptr p = data_ptr(); return resolve_to_col(p);}
        
        inline uint32_t  address()             {DEBUG_ONLY(if(safety_check("value:address:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(address_offset);}
        inline void      address(uint32_t v)   {DEBUG_ONLY(if(safety_check("value:address:set")){return;}) resolve_to_col(*this).qset(address_offset,(void*)&v,4);}
        inline int       reg()                 {DEBUG_ONLY(if(safety_check("value:reg:get")){return -1;}) return *(int*)resolve_to_col(*this).qget(reg_offset);}
        inline void      reg(int i)            {DEBUG_ONLY(if(safety_check("value:reg:set")){return;}) resolve_to_col(*this).qset(reg_offset,(void*)&i,4);}
        inline int       loc()                 {DEBUG_ONLY(if(safety_check("value:loc:get")){return -1;}) return *(int*)resolve_to_col(*this).qget(loc_offset);}
        inline void      loc(int i)            {DEBUG_ONLY(if(safety_check("value:loc:set")){return;}) resolve_to_col(*this).qset(loc_offset,(void*)&i,4);}
        
        inline uint32_t  size()                {DEBUG_ONLY(if(safety_check("value:size:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(size_offset);}
        inline void      size(uint32_t s)      {DEBUG_ONLY(if(safety_check("value:size:set")){return;}) resolve_to_col(*this).qset(size_offset,(void*)&s,4);}
        inline uint32_t  sub_size()            {DEBUG_ONLY(if(safety_check("value:sub_size:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(sub_size_offset);}
        inline void      sub_size(uint32_t s)  {DEBUG_ONLY(if(safety_check("value:sub_size:set")){return;}) resolve_to_col(*this).qset(sub_size_offset,(void*)&s,4);}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("value:quals_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return resolve_to_col(p);}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
        
        inline Ptr&       sub_values_ptr()     {DEBUG_ONLY(if(safety_check("value:sub_values_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(value_sub_values_offset);}
        inline Col&       sub_values_col()     {Ptr& p = sub_values_ptr(); return resolve_to_col(p);}
        inline value_col  sub_values()         {return (value_col&)sub_values_ptr();}
        
        inline Node      type_scope();
        inline void      type_scope(Ptr o)     {DEBUG_ONLY(if(safety_check("value:type_scope:set")){return;}) resolve_to_col(*this).qset(type_scope_offset,(void*)&o,sizeof(Ptr));}

        inline Ptr&      store_ptr()           {DEBUG_ONLY(if(safety_check("value:store:get")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(store_offset);}
        inline Col&      store_col()           {Ptr& p = store_ptr(); return resolve_to_col(p);}
        inline ColCol&   store_pool()          {Ptr& p = store_ptr(); return resolve_to_pool(p);}
        inline ColColCol& store_unit()         {Ptr& p = store_ptr(); return resolve_to_subunit(p);}
        inline void      store(Ptr p)          {DEBUG_ONLY(if(safety_check("value:store:set")){return;}) resolve_to_col(*this).qset(store_offset,(void*)&p,sizeof(Ptr));}
    
        inline void setup(uint32_t _type, uint32_t _size, uint32_t _address = 0) {
            type(_type); size(_size); address(_address);
        }
    
        inline Ptr init_data() {
            Ptr dataptr = get_ticket_from_unit(*this,find_pool_tag_in_unit(*this,stackpool_id),size(),type());
            resolve_to_col(dataptr).push_default();
            resolve_to_col(*this).qset(value_data_offset,(void*)&dataptr,sizeof(Ptr));
            return dataptr;
        }
    
        inline void set(void* data) {
            DEBUG_ONLY(if(safety_check("value:set")){return;})
            Ptr dataptr = data_ptr();
            if(!is_live(dataptr)) {
                dataptr = init_data();
            }
            if(resolve_to_col(dataptr).heterogenous) {
                resolve_to_col(dataptr).qset(dataptr.sidx, data, size());
            } else {
                resolve_to_col(dataptr).set(dataptr.sidx, data);
            }
        }

        inline void retype(uint32_t tag, uint32_t e_size) {
            if(is_live(data_ptr())) {
                data_col().tag = tag; data_col().element_size = e_size;
                if(data_col().empty()) {
                    data_col().push_default();
                }
            }
            type(tag); size(e_size);   
        }
    
        inline void* get() {
            DEBUG_ONLY(if(safety_check("value:get")){return nullptr;})
            Ptr dataptr = data_ptr();
            if(is_live(dataptr)) {
                return resolve_ptr(dataptr);
            } else {
                throw_error("core:value:get this value has no dataptr");
                return nullptr;
            }
        }

        inline void* sget() {
            DEBUG_ONLY(if(safety_check("value:sget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return resolve_to_col(dataptr).sget(dataptr.sidx);
        }
        
        inline void* qget() {
            DEBUG_ONLY(if(safety_check("value:qget")){return nullptr;})
            Ptr dataptr = data_ptr();
            return resolve_to_col(dataptr).qget(dataptr.sidx);
        }

        inline void copy(Value o, bool is_deep) { //Do we make the deep copying happen here or in acorn-compiler?
            Col& src = resolve_to_col(o);
            Col& dst = resolve_to_col(*this);
            memcpy(dst.storage, src.storage, value_total_size);
            if(is_deep) {
                if(is_live(o.data_ptr())&&!resolve_to_col(o.data_ptr()).empty()) {
                    init_data();
                    set(o.get());
                }
                Ptr qualsptr = get_ticket_from_unit(*this, o.quals_ptr().pool, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();
                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                resolve_to_col(*this).qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
                Ptr subvalsptr = get_ticket_from_unit(*this, o.sub_values_ptr().pool, sizeof(Ptr), ptr_id);
                Col& new_subvals = resolve_to_col(subvalsptr);
                Col& old_subvals = o.sub_values_col();
                new_subvals.reserve(old_subvals.size);
                memcpy(new_subvals.storage, old_subvals.storage, old_subvals.size);
                new_subvals.size = old_subvals.size;
                resolve_to_col(*this).qset(value_sub_values_offset,(void*)&subvalsptr,sizeof(Ptr));
            }
        }

        int find_qual(uint32_t q_id);
        Node get_qual(uint32_t q_id);
    
        bool has_qual(uint32_t q_id) {
            return find_qual(q_id)!=-1;
        }

        uint32_t count_qual(uint32_t q_id);
    };

    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) : Ptr(p) {}
    
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)||resolve_to_col(*this).empty()) {throw_error("Attempted ",log_msg," but node "+Ptr_to_string(*this)+" was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline uint32_t  type()                {DEBUG_ONLY(if(safety_check("node:type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(node_type_offset);}
        inline void      type(uint32_t t)      {DEBUG_ONLY(if(safety_check("node:type:set")){return;}) resolve_to_col(*this).qset(node_type_offset,(void*)&t,4);}
        inline uint32_t  sub_type()            {DEBUG_ONLY(if(safety_check("node:sub_type:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(node_sub_type_offset);}
        inline void      sub_type(uint32_t st) {DEBUG_ONLY(if(safety_check("node:sub_type:set")){return;}) resolve_to_col(*this).qset(node_sub_type_offset,(void*)&st,4);}
        
        inline Ptr&      name_ptr()            {DEBUG_ONLY(if(safety_check("node:name_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_name_offset);}
        inline Col&      name_col()            {Ptr& p = name_ptr(); return resolve_to_col(p);}
        inline string    name()                {return string(name_ptr());}
        inline void      name(std::string s)   {DEBUG_ONLY(if(safety_check("node:name:set")){return;}) name() = s;}
        
        inline float     x()                   {DEBUG_ONLY(if(safety_check("node:x:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(x_offset);}
        inline void      x(float v)            {DEBUG_ONLY(if(safety_check("node:x:set")){return;}) resolve_to_col(*this).qset(x_offset,(void*)&v,4);}
        inline float     y()                   {DEBUG_ONLY(if(safety_check("node:y:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(y_offset);}
        inline void      y(float v)            {DEBUG_ONLY(if(safety_check("node:y:set")){return;}) resolve_to_col(*this).qset(y_offset,(void*)&v,4);}
        inline float     z()                   {DEBUG_ONLY(if(safety_check("node:z:get")){return -1.0f;}) return *(float*)resolve_to_col(*this).qget(z_offset);}
        inline void      z(float v)            {DEBUG_ONLY(if(safety_check("node:z:set")){return;}) resolve_to_col(*this).qset(z_offset,(void*)&v,4);}
        
        inline Ptr       value_ptr()           {DEBUG_ONLY(if(safety_check("node:value_ptr")){return deadptr;}) return *(Ptr*)resolve_to_col(*this).qget(node_value_offset);}
        inline Value     value()               {return Value(value_ptr());}
        inline void      value(Ptr ptr)        {DEBUG_ONLY(if(safety_check("node:value:set")){return;}) resolve_to_col(*this).qset(node_value_offset,(void*)&ptr,sizeof(Ptr));}
        
        inline Ptr&      children_ptr()        {DEBUG_ONLY(if(safety_check("node:children_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_children_offset);}
        inline Col&      children_col()        {Ptr& p = children_ptr(); return resolve_to_col(p);}
        inline node_col  children()            {return (node_col&)children_ptr();}
        inline void      children(node_col l)  {DEBUG_ONLY(if(safety_check("node:children:set")){return;}) resolve_to_col(*this).qset(node_children_offset,(void*)&l,sizeof(Ptr));}
        
        inline Ptr&      quals_ptr()           {DEBUG_ONLY(if(safety_check("node:quals_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_quals_offset);}
        inline Col&      quals_col()           {Ptr& p = quals_ptr(); return resolve_to_col(p);}
        inline node_col  quals()               {return (node_col&)quals_ptr();}
    
        inline Ptr&        node_table_ptr()    {DEBUG_ONLY(if(safety_check("node:node_table_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_node_table_offset);}
        inline Col&        node_table_col()    {Ptr& p = node_table_ptr(); return resolve_to_col(p);}
        inline node_col    node_table()        {return (node_col&)node_table_ptr();}
        
        inline Ptr&        value_table_ptr()   {DEBUG_ONLY(if(safety_check("node:value_table_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_value_table_offset);}
        inline Col&        value_table_col()   {Ptr& p = value_table_ptr(); return resolve_to_col(p);}
        inline value_col   value_table()       {return (value_col&)value_table_ptr();}
        
        inline Ptr&      scopes_ptr()          {DEBUG_ONLY(if(safety_check("node:scopes_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_scopes_offset);}
        inline Col&      scopes_col()          {Ptr& p = scopes_ptr(); return resolve_to_col(p);}
        inline node_col  scopes()              {return (node_col&)scopes_ptr();}
        
        inline Ptr&  parent_ptr()              {DEBUG_ONLY(if(safety_check("node:parent_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(parent_offset);}
        inline Node  parent()                  {return Node(parent_ptr());}
        inline void  parent(Ptr p)             {DEBUG_ONLY(if(safety_check("node:parent:set")){return;}) resolve_to_col(*this).qset(parent_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  owner_ptr()               {DEBUG_ONLY(if(safety_check("node:owner_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(owner_offset);}
        inline Node  owner()                   {return Node(owner_ptr());}
        inline void  owner(Ptr p)              {DEBUG_ONLY(if(safety_check("node:owner:set")){return;}) resolve_to_col(*this).qset(owner_offset,(void*)&p,sizeof(Ptr));}
        
        inline Ptr&  in_scope_ptr()            {DEBUG_ONLY(if(safety_check("node:in_scope_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(in_scope_offset);}
        inline Node  in_scope()                {return Node(in_scope_ptr());}
        inline void  in_scope(Ptr p)           {DEBUG_ONLY(if(safety_check("node:in_scope:set")){return;}) resolve_to_col(*this).qset(in_scope_offset,(void*)&p,sizeof(Ptr));}
                
        inline Ptr&   opt_str_ptr()            {DEBUG_ONLY(if(safety_check("node:opt_str_ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(node_opt_str_offset);}
        inline Col&   opt_str_col()            {Ptr& p = opt_str_ptr(); return resolve_to_col(p);}
        inline string opt_str()                {return string(opt_str_ptr());}
        
        inline bool  mute()                    {DEBUG_ONLY(if(safety_check("node:mute:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(mute_offset);}
        inline void  mute(bool b)              {DEBUG_ONLY(if(safety_check("node:mute:set")){return;}) resolve_to_col(*this).qset(mute_offset,(void*)&b,1);}
    
        inline bool  resolved()                {DEBUG_ONLY(if(safety_check("node:resolved:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(resolved_offset);}
        inline void  resolved(bool b)          {DEBUG_ONLY(if(safety_check("node:resolved:set")){return;}) resolve_to_col(*this).qset(resolved_offset,(void*)&b,1);}
    
        inline Node left()                     {DEBUG_ONLY(if(safety_check("node:left")){return deadptr;} if(children().empty()){throw_error("Attempted to get left (first child) but node "+Ptr_to_string(*this)+"'s children was empty"); return deadptr;}) return children()[0];}
        inline Node right()                    {DEBUG_ONLY(if(safety_check("node:right")){return deadptr;} if(children().length()<2){throw_error("Attempted to get right (second child) but node "+Ptr_to_string(*this)+" did not have 2 children"); return deadptr;}) return children()[1];}
        inline Node scope()                    {DEBUG_ONLY(if(safety_check("node:scope:get")){return deadptr;}) if(scopes().empty()) {return deadptr;} else {return scopes()[0];}}
        inline void scope(Node n)              {DEBUG_ONLY(if(safety_check("node:scope:set")){return;}) if(scopes().empty()) {scopes() << n;} else {scopes().col().set(0,(void*)&n);}}
        inline Node scope_owner()              {DEBUG_ONLY(if(safety_check("node:scope_owner")){return deadptr;}) return scope().owner();}
        inline Node climb()                    {DEBUG_ONLY(if(safety_check("node:climb")){return deadptr;} if(!is_live(in_scope())){throw_error("Attempted to climb but node "+Ptr_to_string(*this)+" is not in a live scope"); return deadptr;}) return in_scope().owner();}
        inline Node c0()                       {DEBUG_ONLY(if(safety_check("node:c0")){return deadptr;} if(children().empty()){throw_error("Attempted to first child but node "+Ptr_to_string(*this)+"'s children was empty"); return deadptr;}) return children()[0];}
        inline Node c1()                       {DEBUG_ONLY(if(safety_check("node:c1")){return deadptr;} if(children().length()<2){throw_error("Attempted to get second child but node "+Ptr_to_string(*this)+" did not have 2 children"); return deadptr;}) return children()[1];}
        
        inline void* get()                     {DEBUG_ONLY(if(safety_check("node:get")){return nullptr;} if(!is_live(value())){throw_error("Attempted to get value but node "+Ptr_to_string(*this)+" does not have a live value"); return nullptr;}) return value().get();}
        inline void* get(uint32_t i)           {DEBUG_ONLY(if(safety_check("node:get:with_arg")){return nullptr;} if(i>=children().length()){throw_error("Attempted to get value of child ",i," but node "+Ptr_to_string(*this)+" only has ",children().length()," children"); return nullptr;}) return children()[i].get();}
        inline void set(void* v)               {DEBUG_ONLY(if(safety_check("node:set")){return;} if(!is_live(value())){throw_error("Attempted to set value but node "+Ptr_to_string(*this)+" does not have a live value"); return;}) value().set(v);}
        inline void set(uint32_t i, void* v)   {DEBUG_ONLY(if(safety_check("node:set:with_arg")){return;} if(i>=children().length()){throw_error("Attempted to set value of child ",i," but node "+Ptr_to_string(*this)+" only has ",children().length()," children"); return;}) return children()[i].set(v);}
        inline int&    getInt()                {void* p = get(); DEBUG_ONLY(if(safety_check("node:getInt")){return _ctx_dummy_index;})              return *(int*)p;}    inline int&    getInt(uint32_t i)    {void* p = get(i); DEBUG_ONLY(if(safety_check("node:getInt:i")){return _ctx_dummy_index;})              return *(int*)p;}
        inline float&  getFloat()              {void* p = get(); DEBUG_ONLY(if(safety_check("node:getFloat")){return *(float*)&_ctx_dummy_index;})  return *(float*)p;}  inline float&  getFloat(uint32_t i)  {void* p = get(i); DEBUG_ONLY(if(safety_check("node:getFloat:i")){return *(float*)&_ctx_dummy_index;})  return *(float*)p;}
        inline bool&   getBool()               {void* p = get(); DEBUG_ONLY(if(safety_check("node:getBool")){return *(bool*)&_ctx_dummy_index;})    return *(bool*)p;}   inline bool&   getBool(uint32_t i)   {void* p = get(i); DEBUG_ONLY(if(safety_check("node:getBool:i")){return *(bool*)&_ctx_dummy_index;})    return *(bool*)p;}
        inline Ptr&    getPtr()                {void* p = get(); DEBUG_ONLY(if(safety_check("node:getPtr")){return dead_ref;})                      return *(Ptr*)p;}    inline Ptr&    getPtr(uint32_t i)    {void* p = get(i); DEBUG_ONLY(if(safety_check("node:getPtr:i")){return dead_ref;})                      return *(Ptr*)p;}
        inline string  getString()             {DEBUG_ONLY(if(safety_check("node:getString")){return deadptr;}) return getPtr();}                           inline string  getString(uint32_t i) {DEBUG_ONLY(if(safety_check("node:getString:i")){return deadptr;}) return getPtr(i);}
        inline Node    getNode()               {DEBUG_ONLY(if(safety_check("node:getNode")){return deadptr;}) return getPtr();}                             inline Node    getNode(uint32_t i)   {DEBUG_ONLY(if(safety_check("node:getNode:i")){return deadptr;}) return getPtr(i);}
        inline Value   getValue()              {DEBUG_ONLY(if(safety_check("node:getValue")){return deadptr;}) return getPtr();}                            inline Value   getValue(uint32_t i)  {DEBUG_ONLY(if(safety_check("node:getValue:i")){return deadptr;}) return getPtr(i);}

        inline Context getContext(); inline Context getContext(uint32_t i);

        inline void copy(Node o) {
            Col& src = resolve_to_col(o);
            Col& dst = resolve_to_col(*this);
            memcpy(dst.storage, src.storage, node_total_size);
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

        uint32_t count_qual(size_t q_id, bool check_value = true) {
            uint32_t count = 0;
            if(check_value&&is_live(value())) {
                count += value().count_qual(q_id);
            }
            for(int i=0;i<quals().length();i++){ 
                if(quals()[i].type()==q_id) count++;
            }
            return count;
        }
    };

    inline Node Value::type_scope() {return Node(*(Ptr*) resolve_to_col(*this).qget(type_scope_offset));}
    inline int Value::find_qual(uint32_t q_id) {
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) {return i;}
        }
        return -1;
    } 
    inline Node Value::get_qual(uint32_t q_id) {
        int q_at = find_qual(q_id);
        if(q_at!=-1) {
            return quals()[q_at];
        }
        return deadptr;
    } 
    inline uint32_t Value::count_qual(uint32_t q_id) {
        uint32_t count = 0;
        for(int i=0;i<quals().length();i++){ 
            if(quals()[i].type()==q_id) count++;
        }
        return count;
    }

    struct Context : public Ptr {
        Context() {}
        Context(Ptr p) : Ptr(p) {}
    
        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but context was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Ptr&     node_ptr()           {DEBUG_ONLY(if(safety_check("context:node:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_node_offset);}
        inline Node     node()               {return Node(node_ptr());}
        inline void     node(Ptr p)          {DEBUG_ONLY(if(safety_check("context:node:set")){return;}) resolve_to_col(*this).qset(context_node_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     qual_ptr()           {DEBUG_ONLY(if(safety_check("context:qual:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_qual_offset);}
        inline Node     qual()               {return Node(qual_ptr());}
        inline void     qual(Ptr p)          {DEBUG_ONLY(if(safety_check("context:qual:set")){return;}) resolve_to_col(*this).qset(context_qual_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     left_ptr()           {DEBUG_ONLY(if(safety_check("context:left:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_left_offset);}
        inline Node     left()               {return Node(left_ptr());}
        inline void     left(Ptr p)          {DEBUG_ONLY(if(safety_check("context:left:set")){return;}) resolve_to_col(*this).qset(context_left_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     out_ptr()            {DEBUG_ONLY(if(safety_check("context:out:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_out_offset);}
        inline Node     out()                {return Node(out_ptr());}
        inline void     out(Ptr p)           {DEBUG_ONLY(if(safety_check("context:out:set")){return;}) resolve_to_col(*this).qset(context_out_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     root_ptr()           {DEBUG_ONLY(if(safety_check("context:root:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_root_offset);}
        inline Node     root()               {return Node(root_ptr());}
        inline void     root(Ptr p)          {DEBUG_ONLY(if(safety_check("context:root:set")){return;}) resolve_to_col(*this).qset(context_root_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     value_ptr()          {DEBUG_ONLY(if(safety_check("context:value:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_value_offset);}
        inline Value    value()              {return Value(value_ptr());}
        inline void     value(Ptr p)         {DEBUG_ONLY(if(safety_check("context:value:set")){return;}) resolve_to_col(*this).qset(context_value_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     result_ptr()         {DEBUG_ONLY(if(safety_check("context:result:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_result_offset);}
        inline Col&     result_col()         {Ptr& p = result_ptr(); return resolve_to_col(p);}
        inline node_col result()             {return (node_col&)result_ptr();}
        inline void     result(Ptr p)        {DEBUG_ONLY(if(safety_check("context:result:set")){return;}) resolve_to_col(*this).qset(context_result_offset,(void*)&p,sizeof(Ptr));}
    
        inline int&     index()              {DEBUG_ONLY(if(safety_check("context:index:get")){return _ctx_dummy_index;}) return *(int*)resolve_to_col(*this).qget(context_index_offset);}
        inline void     index(int i)         {DEBUG_ONLY(if(safety_check("context:index:set")){return;}) resolve_to_col(*this).qset(context_index_offset,(void*)&i,4);}
    
        inline Ptr&     sub_ptr()            {DEBUG_ONLY(if(safety_check("context:sub:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_sub_offset);}
        inline Context  sub()                {return Context(sub_ptr());}
        inline void     sub(Ptr p)           {DEBUG_ONLY(if(safety_check("context:sub:set")){return;}) resolve_to_col(*this).qset(context_sub_offset,(void*)&p,sizeof(Ptr));}

        inline Ptr&     parent_ptr()         {DEBUG_ONLY(if(safety_check("context:parent:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_parent_offset);}
        inline Context  parent()             {return Context(parent_ptr());}
        inline void     parent(Ptr p)        {DEBUG_ONLY(if(safety_check("context:parent:set")){return;}) resolve_to_col(*this).qset(context_parent_offset,(void*)&p,sizeof(Ptr));}
    
        inline Ptr&     source_ptr()         {DEBUG_ONLY(if(safety_check("context:source:ptr")){return dead_ref;}) return *(Ptr*)resolve_to_col(*this).qget(context_source_offset);}
        inline Col&     source_col()         {Ptr& p = source_ptr(); return resolve_to_col(p);}
        inline string   source()             {return string(source_ptr());}
        inline void     source(Ptr p)        {DEBUG_ONLY(if(safety_check("context:source:set")){return;}) resolve_to_col(*this).qset(context_source_offset,(void*)&p,sizeof(Ptr));}
        inline void     source(std::string s){DEBUG_ONLY(if(safety_check("context:source:set")){return;}) source() = s;}
    
        inline uint32_t state()              {DEBUG_ONLY(if(safety_check("context:state:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(context_state_offset);}
        inline void     state(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:state:set")){return;}) resolve_to_col(*this).qset(context_state_offset,(void*)&s,4);}

        inline uint32_t pass()              {DEBUG_ONLY(if(safety_check("context:pass:get")){return 0;}) return *(uint32_t*)resolve_to_col(*this).qget(context_pass_offset);}
        inline void     pass(uint32_t s)    {DEBUG_ONLY(if(safety_check("context:pass:set")){return;}) resolve_to_col(*this).qset(context_pass_offset,(void*)&s,4);}
    
        inline bool     flag()               {DEBUG_ONLY(if(safety_check("context:flag:get")){return false;}) return *(bool*)resolve_to_col(*this).qget(context_flag_offset);}
        inline void     flag(bool b)         {DEBUG_ONLY(if(safety_check("context:flag:set")){return;}) resolve_to_col(*this).qset(context_flag_offset,(void*)&b,1);}
    };

    inline Context Node::getContext() {DEBUG_ONLY(if(safety_check("node:getContext")){return deadptr;}) return getPtr();}  inline Context Node::getContext(uint32_t i) {DEBUG_ONLY(if(safety_check("node:getContext:i")){return deadptr;}) return getPtr(i);}

    inline bool init_type_pool() {
        global[handler_type_id].label = "handlers";
        global[layout_type_id].label = "layouts";
        // global[node_type_id].label = "nodes";
        // global[value_type_id].label = "values";
        // global[context_type_id].label = "contexts";
        global[name_store_id].label = "names";
        // global[children_store_id].label = "children";
        // global[quals_store_id].label = "quals";
        // global[node_table_store_id].label = "node table";
        // global[value_table_store_id].label = "value table";
        // global[scopes_store_id].label = "scopes";
        // global[opt_str_store_id].label = "opt_str";
        // global[data_store_id].label = "data";
        // global[sub_value_store_id].label = "sub_value";
        return true;
    }
    inline bool type_pool_intilized = init_type_pool();

    struct Nodenet_Unit : public virtual Unit {
        Nodenet_Unit(uint16_t _uid) : Unit(_uid) {init();}
        Nodenet_Unit() {init();}
        

        float at_x = 0.0f;
        float at_y = 0.0f;
        float at_z = 0.0f; float last_z = 0.0f;

        void init() override {
            setup_signals();
        }
        virtual Node process(std::string path) {return deadptr;}
        virtual void run(Node root) {}

        #define LOG_W(ctx, msg) DEBUG_ONLY(log_to_watcher(ctx, std::string(msg) + " [" + strip_path(__FILE__) + ":" + std::to_string(__LINE__) + "]"))

        list<Watcher> watchers;
        Stage* active_stage;
        bool log_all_process = false;
        Context unit_ctx = deadptr;
        map<std::string, g_ptr<Stage>> stages;
        Node unit_root = deadptr;

        void setup_standard_watchers() {
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
                log(green("After: "),node_info(ctx.node()));
                endline();
            };
            def.stagend = [this](Context& ctx){
                endline();
            };
            watchers << def;
        }

        void setup_uspan_standard_watchers() {
            Watcher def("uspan_core");
            def.stagestart = [this](Context& ctx){
                if(active_stage) {
                    uspan->newline(active_stage->label);
                }
            };
            def.passstart = [this](Context& ctx){
                if(ctx.pass()!=0) {
                    uspan->newline(labels[ctx.pass()]+" over "+std::to_string(ctx.result().length())+" nodes");
                }
            };
            def.prefix = [this](Context& ctx){
                if(is_live(ctx.qual())) {
                    uspan->newline(active_stage->label+": "+labels[ctx.qual().type()]+" in "+ctx.node().name().to_std());
                } else {
                    uspan->newline(active_stage->label+": "+node_basic_info_with_position(ctx.node()));
                }
            };
            def.suffix = [this](Context& ctx){
                uspan->endline();
            };
            def.stagend = [this](Context& ctx){
                uspan->endline();
            };
            watchers << def;
        }

        void setup_crash_watchers() {
            Watcher def("crash");
            def.stagestart = [this](Context& ctx){
                if(active_stage) {
                    print("STARTING STAGE: ",active_stage->label);
                }
            };
            def.passstart = [this](Context& ctx){
                if(ctx.pass()!=0) {
                    print("  ",labels[ctx.pass()]+" over "+std::to_string(ctx.result().length())+" nodes");
                }
            };
            def.prefix = [this](Context& ctx){
                if(is_live(ctx.qual())) {
                    print("    "+active_stage->label+": "+labels[ctx.qual().type()]+" in "+ctx.node().name().to_std());
                } else {
                    print("    "+active_stage->label+": "+node_basic_info_with_children_and_position(ctx.node()));
                }
            };
            watchers << def;
        }


        void stamp_onto_page(Node node, list<std::string>& lines) {
            if(node.x()>=0.0f&&node.y()>=0.0f) {
                // print("STAMPING: ",node_info(node));
                int x = (int)node.x();
                int y = (int)node.y();
                while(y>=lines.length()) {lines << "";}
                while((x+node.name().length())>=lines[y].length()) lines[y]+=" ";
                for(char c : node.name().to_std()) lines[y][x++] = c;
                // for(auto l : lines) {
                //     print(escape_string(l,false));
                // }
            }
            for(int i=0;i<node.children().length();i++) stamp_onto_page(node.children()[i],lines);
            for(int i=0;i<node.quals().length();i++) stamp_onto_page(node.quals()[i],lines);
            for(int i=0;i<node.scopes().length();i++) stamp_onto_page(node.scopes()[i],lines);
            if(is_live(node.value())) {
                for(int i=0;i<node.value().quals().length();i++) stamp_onto_page(node.value().quals()[i],lines);
            }
        }
        std::string nodenet_to_string(Node root) {
            list<std::string> lines;
            stamp_onto_page(root,lines);
            std::string out = "";
            for(auto l : lines) {
                out+=l+"\n";
            }
            return out;
        }

        std::string idx_to_color(const std::string& num, int idx) {
            float hue = fmod(idx * 137.508f, 360.0f);
            float s = 0.8f, v = 0.9f;
            
            float c = v * s;
            float x = c * (1.0f - fabs(fmod(hue / 60.0f, 2.0f) - 1.0f));
            float m = v - c;
            float r,g,b;
            if(hue<60)       {r=c;g=x;b=0;}
            else if(hue<120) {r=x;g=c;b=0;}
            else if(hue<180) {r=0;g=c;b=x;}
            else if(hue<240) {r=0;g=x;b=c;}
            else if(hue<300) {r=x;g=0;b=c;}
            else             {r=c;g=0;b=x;}
            return rgb(num, (int)((r+m)*255), (int)((g+m)*255), (int)((b+m)*255));
        }



        uint32_t node_type_id = init_node_type();
        uint32_t value_type_id = init_value_type();
        uint32_t context_type_id = init_context_type();

        inline uint32_t init_node_type() {
            uint32_t at = types.add_idx();
            ColCol& t = types[at];
            _layout ntemp(add_template(node_id)); //Node template
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
            node_total_size = ntemp.total_size;
            layouts.put(node_id,ntemp);
            return at;
        }

        inline uint32_t init_value_type() {
            uint32_t at = types.add_idx();
            ColCol& t = types[at];

            _layout vtemp(add_template(value_id)); //Value template
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
            value_total_size = vtemp.total_size;
            layouts.put(value_id,vtemp);
            return at;
        }

        inline uint32_t init_context_type() {
            uint32_t at = types.add_idx();
            ColCol& t = types[at];
            _layout ctemp(add_template(context_id)); //Context template
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
            context_pass_offset = ctemp.add_prop(int_id,4,"pass");
            context_parent_offset = ctemp.add_prop(context_id,sizeof(Ptr),"parent");
            context_total_size = ctemp.total_size;
            layouts.put(context_id,ctemp);
            return at;
        }

        uint32_t children_store_id = types.add_idx();
        uint32_t quals_store_id = types.add_idx();
        uint32_t node_table_store_id = types.add_idx(); 
        uint32_t value_table_store_id = types.add_idx(); 
        uint32_t scopes_store_id = types.add_idx(); 
        uint32_t opt_str_store_id = types.add_idx();
        uint32_t data_store_id = types.add_idx();
        uint32_t sub_value_store_id = types.add_idx();

        bool further_pool_init() {
            types[data_store_id].tag = stackpool_id;
            types[children_store_id].label = "Children";
            types[quals_store_id].label = "Quals";
            types[node_table_store_id].label = "Node tables";
            types[value_table_store_id].label = "Value tables";
            types[scopes_store_id].label = "Scopes";
            types[opt_str_store_id].label = "Opt strings";
            types[data_store_id].label = "Data";
            types[sub_value_store_id].label = "Sub values";

            types[node_type_id].label = "Nodes";
            types[value_type_id].label = "Values";
            types[context_type_id].label = "Contexts";
            return true;
        }
        bool furhter_pool_inits = further_pool_init();

        Node make_node(uint32_t type = 0, uint32_t sub_type = 0, std::string name = "", float x = -1.0f, float y = -1.0f, float z = -1.0f,
            Value value = deadptr, Ptr childrenptr = deadptr, Ptr qualsptr = deadptr, Ptr nodetableptr = deadptr, 
            Ptr valuetableptr = deadptr, Ptr scopesptr = deadptr, Ptr parent = deadptr, Ptr owner = deadptr, 
            Ptr in_scope = deadptr, std::string opt_str = "", bool mute = false, bool resolved = false) 
        {
            Node n;
            n.pool = node_type_id;
            n.idx = push_column(types[node_type_id], node_total_size, node_id);
            n.sidx = 0;
            n.subunit = 0;
            n.unit = uid;
            n.cache = &types;
            n.cachelevel = 3;
            Col& col = types[node_type_id][n.idx];
            col.heterogenous = true;
            n.gen = col.gen;
    
            col.qset(node_type_offset,(void*)&type,4);
            col.qset(node_sub_type_offset,(void*)&sub_type,4);
    
            Ptr nameptr = get_ticket(name_store_id,sizeof(char),char_id);
            col.qset(node_name_offset, (void*)&nameptr,sizeof(Ptr));
            for(auto c : name) types[nameptr.pool][nameptr.idx].push((void*)&c);
    
            col.qset(x_offset, (void*)&x,4);
            col.qset(y_offset, (void*)&y,4);
            col.qset(z_offset, (void*)&z,4);
    
            col.qset(node_value_offset, (void*)&value,sizeof(Ptr));
        
            if(!is_live(childrenptr)) {childrenptr = get_ticket(children_store_id,sizeof(Ptr),node_id);}
            col.qset(node_children_offset, (void*)&childrenptr,sizeof(Ptr));

        
            if(!is_live(qualsptr)) qualsptr = get_ticket(quals_store_id,sizeof(Ptr),ptr_id);
            col.qset(node_quals_offset, (void*)&qualsptr,sizeof(Ptr));
            
            if(!is_live(nodetableptr)) nodetableptr = get_ticket(node_table_store_id,sizeof(Ptr),node_id);
            col.qset(node_node_table_offset, (void*)&nodetableptr,sizeof(Ptr));
        
            if(!is_live(valuetableptr)) valuetableptr = get_ticket(value_table_store_id,sizeof(Ptr),value_id);
            col.qset(node_value_table_offset, (void*)&valuetableptr,sizeof(Ptr));
    
            if(!is_live(scopesptr)) scopesptr = get_ticket(scopes_store_id,sizeof(Ptr),node_id);
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
    
        Node make_node(uint32_t type, std::string name, Value value, Ptr in_scope) {
            return make_node(type,0,name,-1.0f,-1.0f,-1.0f,value,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,in_scope);
        }
    
        void recycle_value(Value v, bool recycle_data = true) {
            if(is_live(v)&&resolve_to_col(v).live) {
                CHECK_ERROR("Attempted to recycle value at ",Ptr_to_string(v,v.cachelevel)," while an error was active");
                //print("QUALS");
                for(int i=0;i<v.quals().length();i++) {
                    recycle_node(v.quals()[i]);
                }
                //print("QUALS PTR: ",Ptr_to_string(v.quals_ptr()));
                recycle_column(v.quals_ptr());
                
                //print("SUB VALUES");
                for(int i=0;i<v.sub_values().length();i++) {
                    recycle_value(v.sub_values()[i]);
                }
                //print("SUB VALUES PTR: ",Ptr_to_string(v.sub_values_ptr()));
                recycle_column(v.sub_values_ptr());
                
                if(recycle_data) {
                    //print("DATA PTR: ",Ptr_to_string(v.data_ptr()));
                    recycle_column(v.data_ptr());
                }
                recycle_column(v);
            }
        }
    
        //Recycles everything
        void recycle_node(Node n) {
            //print("Recycling: ",node_info(n));
            if(is_live(n)&&resolve_to_col(n).live) {
                CHECK_ERROR("Attempted to recycle node at ",Ptr_to_string(n,n.cachelevel)," while an error was active");
                //print("CHILDREN");
                for(int i=0;i<n.children().length();i++) {
                    recycle_node(n.children()[i]);
                }
                //print("CHILDREN PTR: ",Ptr_to_string(n.children_ptr()));
                recycle_column(n.children_ptr());
                //print("SCOPES");
                for(int i=0;i<n.scopes().length();i++) {
                    recycle_node(n.scopes()[i]);
                }
                //print("SCOPES PTR: ",Ptr_to_string(n.scopes_ptr()));
                recycle_column(n.scopes_ptr());
                //print("QUALS");
                for(int i=0;i<n.quals().length();i++) {
                    recycle_node(n.quals()[i]);
                }
                //print("QUALS PTR: ",Ptr_to_string(n.quals_ptr()));
                recycle_column(n.quals_ptr());
                //print("NAME PTR ",Ptr_to_string(n.name_ptr()));
                recycle_column(n.name_ptr());
                //print("VALUE ",Ptr_to_string(n.value()));
                recycle_value(n.value());
                //print("VALUE TABLE PTR ",Ptr_to_string(n.value_table_ptr()));
                recycle_column(n.node_table_ptr());
                //print("NODE TABLE PTR ",Ptr_to_string(n.node_table_ptr()));
                recycle_column(n.value_table_ptr());
                //print("OPT STR PTR ",Ptr_to_string(n.opt_str_ptr()));
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
            v.idx = push_column(types[value_type_id], value_total_size, value_id);
            v.sidx = 0;
            v.subunit = 0;
            v.unit = uid;
            v.cache = &types;
            v.cachelevel = 3;
            Col& col = types[value_type_id][v.idx];
            col.heterogenous = true;
            v.gen = col.gen;
        
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
        
            if(!is_live(sub_values)) {sub_values = get_ticket(sub_value_store_id, sizeof(Ptr), ptr_id);}
            col.qset(value_sub_values_offset, (void*)&sub_values, sizeof(Ptr));
        
            return v;
        }
        Context make_context(Ptr result = deadptr, Ptr source = deadptr, uint32_t pass = 0, Context parent = deadptr) {
            Context c;
            c.pool = context_type_id;
            c.idx = push_column(types[context_type_id], context_total_size, context_id);
            c.sidx = 0;
            c.unit = uid;
            c.subunit = 0;
            c.cache = &types;
            c.cachelevel = 3;
            Col& col = types[context_type_id][c.idx];
            col.heterogenous = true;
            c.gen = col.gen;
        
            Ptr dead_node = deadptr;
            Ptr dead_value = deadptr;
        
            col.qset(context_node_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_qual_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_left_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_out_offset,    (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_root_offset,   (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_value_offset,  (void*)&dead_value, sizeof(Ptr));
            col.qset(context_sub_offset,    (void*)&dead_node,  sizeof(Ptr));
            col.qset(context_parent_offset, (void*)&parent,  sizeof(Ptr));
        
            if(!is_live(result)) result = get_ticket(children_store_id, sizeof(Ptr), ptr_id);
            col.qset(context_result_offset, (void*)&result, sizeof(Ptr));
            
            if(!is_live(source)) source = get_ticket(name_store_id, sizeof(char), char_id);
            col.qset(context_source_offset, (void*)&source, sizeof(Ptr));
        
            uint32_t zero = 0; bool f = false;
            col.qset(context_index_offset,  (void*)&zero, 4);
            col.qset(context_state_offset,  (void*)&zero, 4);
            col.qset(context_pass_offset,   (void*)&pass, 4);
            col.qset(context_flag_offset,   (void*)&f,    1);
        
            return c;
        }
    
        void recycle_context(Context ctx) {
            recycle_column(ctx);
        }
        void deep_recycle_context(Context ctx) {
            recycle_column(ctx.result_ptr());
            recycle_column(ctx.source_ptr());
            recycle_column(ctx);
        }



        std::string value_info(Value value, int verbosity = 0, std::string indent = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(value)),red(" while another error was active")); return "";})

            std::string to_return = "";
            to_return += cyan("["+Ptr_as_string(value)+"]")+
            + "("+ green(labels[value.type()]) + (value.size()!=0?green("["+std::to_string(value.size())+"]"):"")
            + (value.sub_type()==0?"":green(":"+labels[value.sub_type()])) + (value.sub_size()!=0?green("["+std::to_string(value.sub_size())+"]"):"");
            if(is_live(value.data_ptr())) { //For post-mortems we want to see the adress, so it needs to be computed first, before the error
                std::string ptr_addr = Ptr_as_string(value.data_ptr());
                Col& datacol = resolve_to_col(value.data_ptr());
                CHECK_ERROR_VAL(to_return," failed to resolve value's dataptr");
                if(datacol.empty()) {
                    to_return += " "+gray("empty")+" @"+ptr_addr;
                } else if(!datacol.heterogenous&&datacol.length()<=value.data_ptr().sidx) {
                    to_return += " "+gray("out of bounds")+" @"+ptr_addr;
                } else {
                    std::string tagstr = tag_to_str(value.type(),value.get());
                    if(value.type()==string_id) { //Just making it strings for now
                        if(tagstr.length()>50) tagstr = tagstr.substr(0,50); //Disable this to disable truncation of large values
                    }
                    to_return += " "+gray(tagstr)+" @"+ptr_addr;
                }
                CHECK_ERROR_VAL(to_return,"Attempted to print info of ",cyan(Ptr_to_string(value))," but the value was invalid");
            }
            to_return += (value.reg()!=-1?", reg: "+std::to_string(value.reg()):"")
            + (value.address()!=0?", address: "+std::to_string(value.address()):"")
            + (value.loc()!=-1?", loc: "+std::to_string(value.loc()):"")
            + (is_live(value.store_ptr())?", store: "+Ptr_as_string(value.store_ptr()):"")
            + (is_live(value.type_scope())?"{"+value.type_scope().name().to_std()+":"+blue(Ptr_as_string(value.type_scope()))+"}":"");

            if(verbosity>5) {
                if(!value.sub_values().empty()) {
                    to_return += "\n" + indent + "   Sub values:";
                    for(int i=0;i<value.sub_values().length();i++) {
                        to_return += "\n"+indent+"     "+std::to_string(i)+": "+value_info(value.sub_values()[i],verbosity,indent);
                    }
                }
            } else {
                to_return+=(!value.sub_values().empty()?", subvals: "+std::to_string(value.sub_values().length()):"");
            }
            if(verbosity>0) {
                if(!value.quals().empty()) {
                    to_return += ", Quals: ";
                    for(int i=0;i<value.quals().length();i++) {
                        to_return += labels[value.quals()[i].type()]+(i!=value.quals().length()-1?", ":"");
                    }
                }
            }
            to_return += ")";
            return to_return;
        }

        std::string node_basic_info(Node node) {
            std::string type = labels[node.type()];
            std::string name = (node.name().length()==0?"":node.name().to_std());
            std::string to_return = type+(name!=type?" "+name:"");
            return to_return;
        }

        std::string node_basic_info_with_children(Node node) {
            std::string to_return = node_basic_info(node);
            for(int c=0;c<node.children().length();c++) {
                if(c==0) {to_return+="[";}
                to_return+=node_basic_info(node.children()[c]);
                if(c==node.children().length()-1) {to_return+="]";}
                else {to_return+=", ";}
            }
            return to_return;
        }

        std::string node_basic_info_with_position(Node node) {
            std::string to_return = node_basic_info(node);
            to_return+=(node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"");
            return to_return;
        }

        std::string node_basic_info_with_children_and_position(Node node) {
            std::string to_return = node_basic_info_with_children(node);
            to_return+=(node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"");
            return to_return;
        }


        
        //Verbosity levels
        //0 = smallest print
        //1 = print quals on values
        //2 = longform quals on node
        //3 = table display on node to string, but no longform quals on node and no muted form either
        //4 = as 3 but with longform quals on nodes
        std::string node_info(Node node, int verbosity = 1, std::string indent = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})

            std::string to_return = "";
            to_return += blue(Ptr_as_string(node)+" ")
            + labels[node.type()]
            + (node.sub_type()==0?"":":"+labels[node.sub_type()])
            + (node.name().length()==0?"":" "+green(escape_string(node.name().to_std(),true))+" ") 
            + (is_live(node.value())?value_info(node.value(),verbosity,indent):"")
            + (node.x()!=-1.0f?"("+std::to_string((int)node.x())+","+std::to_string((int)node.y())+")":"")
            + (!node.children().empty()?"[C:"+std::to_string(node.children().length())+"]":"")
            + (!node.scopes().empty()?"[S:"+std::to_string(node.scopes().length())+"]":"")
            + (is_live(node.owner())?"[O:"+blue(Ptr_as_string(node.owner()))+"]":"")
            + (is_live(node.in_scope())?"{"+node.in_scope().name().to_std()+"}":"");
            if(!node.quals().empty()) {
                std::string qual_list = "";
                for(int i=0;i<node.quals().length();i++) {
                    if(verbosity==2||verbosity==4) {
                        to_return += "\n " + node_to_string(node.quals()[i], (indent.length()/2) + 1, i, verbosity,"q");
                    }
                    else {
                        if(node.quals()[i].mute()) {
                            if(verbosity==3) {continue;}
                            qual_list += italic_str(Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()]);
                        } else {
                            qual_list += Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()];
                        }
                        qual_list+=(i!=node.quals().length()-1?", ":"");
                    }
                }
                if(qual_list.length()>0) {
                    to_return += "[Q: "+qual_list+"]";
                }
            }
            return to_return;
        }

        std::string node_to_string(Node node, int depth = 0, int index = 0, int verbosity = 1, std::string sigil = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})
            std::string indent(depth * 2, ' ');
            std::string to_return = "";
            
            to_return += indent + sigil + std::to_string(index) + ": " + node_info(node,verbosity,indent);
        
            if(verbosity==3||verbosity==4) {
                if(is_live(node.value_table_ptr())&&node.value_table_col().length()>0) {
                    to_return += "\n" + indent + "   Value table:";
                    list<CCol*> value_cells = node.value_table().col().allCells();
                    for(int i=0;i<value_cells.length();i++) {
                        to_return += "\n" + indent + "     Key: "+((QString&)(*(value_cells[i]))).to_std()+" | "+value_info(node.value_table()[value_cells[i]->index],verbosity,indent+"     ");
                    }
                }
                if(is_live(node.node_table_ptr())&&node.node_table_col().length()>0) {
                    to_return += "\n" + indent + "   Node table:";
                    list<CCol*> node_cells = node.node_table().col().allCells();
                    for(int i=0;i<node_cells.length();i++) {
                        to_return += "\n" + indent + "     Key: "+((QString&)(*(node_cells[i]))).to_std()+" | "+node_info(node.node_table()[node_cells[i]->index],verbosity,indent+"     ");
                    }
                }
            }
        
            // if(!node->opt_str.empty()) {
            //     to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
            // }

            if(!node.children().empty()) {
                for(int i=0;i<node.children().length();i++) {
                    if(is_live(node.children()[i])) {
                        if(node.children()[i].idx==node.idx) {
                            to_return+="\n "+indent+red("  self refrence");
                        } else if(node.children()[i].type()==hide_block_id) {
                            to_return += "\n "+indent+"   c"+std::to_string(i)+": HIDDEN";
                        } else {
                            to_return += "\n " + node_to_string(node.children()[i], depth + 1, i, verbosity,"c");
                        }
                    }
                    else {
                        to_return += "\n" + indent + "[NULL CHILD] "+node_info(node.children()[i],verbosity,indent);
                    }
                }
            }

            if(!node.scopes().empty()) {
                //to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node.scopes().length());
                int i = 0;
                for(int s=0;s<node.scopes().length();s++) {
                    Node scope = node.scopes()[s];
                    if(scope.owner().idx==node.idx) {
                        to_return += "\n " + node_to_string(scope, depth + 1, s, verbosity,"s");
                    }
                    else {
                        to_return += "\n"+indent+"  s"+std::to_string(s)+": "+node_info(scope,verbosity,indent);
                    }
                }
            }
        
            return to_return;
        }


        list<Context> get_context_trace(Context ctx) {
            list<Context> to_return;
            Context onctx = ctx;
            while(is_live(onctx)) {
                to_return << onctx;
                onctx = onctx.parent();
            }
            return to_return;
        }

        std::string context_result_brick(node_col result, int index, std::string indent) {
            std::string to_return = indent;
            for(int i=0;i<result.length();i++) {
                // if(to_return.length()%100==0) { Fix later if needed
                //     to_return+=(i>0?"\n":"")+indent;
                // }
                if(i==index) {
                    to_return+=gray(">"+node_basic_info_with_children(result[i]))+" | ";
                } else {
                    to_return+=node_basic_info(result[i])+" | ";
                }
            }
            return to_return;
        }

        std::string context_basic_info(Context ctx) {
            std::string to_return = labels[ctx.pass()];
            return to_return;
        }
    
        std::string context_info(Context ctx, int verbosity = 0, std::string indent = "", bool folded = false) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(ctx)),red(" while another error was active")); return "";})
        
            std::string nl = folded?"":"\n"+indent+"  ";
            std::string to_return = "";
            to_return += navy(Ptr_as_string(ctx)+" ")
            + labels[ctx.pass()]+" "
            + (ctx.state()==0?"":"State: "+std::to_string(ctx.state())+" ")
            + (!is_live(ctx.source())?"":navy("Source at: ")+Ptr_as_string(ctx.source())+" ")+navy("Index: ")+std::to_string(ctx.index())+" "
            + (folded?"":"\n"+context_result_brick(ctx.result(),ctx.index(),indent))
            + (!is_live(ctx.node())?"":nl+navy("Node: ")+node_info(ctx.node(),verbosity,indent)+" ")
            + (!is_live(ctx.left())?"":nl+navy("Left: ")+node_info(ctx.left(),verbosity,indent)+" ")
            + (!is_live(ctx.root())?"":nl+navy("Root: ")+node_info(ctx.root(),verbosity,indent)+" ")
            + (!is_live(ctx.out())?"":nl+navy("Out: ")+node_info(ctx.out(),verbosity,indent)+" ")
            + (!is_live(ctx.qual())?"":nl+navy("Qual: ")+node_info(ctx.qual(),verbosity,indent)+" ")
            + (!is_live(ctx.value())?"":nl+navy("Value: ")+value_info(ctx.value(),verbosity,indent)+" ")
            + (!is_live(ctx.sub())?"":nl+navy("Sub: ")+context_info(ctx.sub(),verbosity,indent+"  "));
            return to_return;
        }

        std::string context_to_string(Context ctx, int depth = 0, int index = 0, int verbosity = 1, std::string sigil = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(ctx)),red(" while another error was active")); return "";})
            std::string indent(depth * 2, ' ');
            std::string head_handle(depth*2,'=');
            std::string to_return = "";

            to_return += navy(head_handle+sigil+std::to_string(index)+"> ")+context_info(ctx,verbosity,indent,true);

            if(!ctx.result().empty()) {
                for(int i=0;i<ctx.result().length();i++) {
                    if(is_live(ctx.result()[i])) {
                        to_return += (i==ctx.index()?"\n>":"\n ") + node_to_string(ctx.result()[i], depth + 2, i, verbosity,"n");
                    }
                    else {
                        to_return += (i==ctx.index()?"\n>":"\n ") + indent + "[NULL NODE] ";
                    }
                }
            }

            return to_return;
        }
        std::string context_trace_verbose(Context ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(ctx)),red(" while another error was active")); return "";})
            std::string to_return = "";
            list<Context> trace = get_context_trace(ctx);
            for(int i = 0; i<trace.length(); i++) {
                to_return += context_to_string(trace[i], i*3, i)+"\n";
            }
            return to_return;
        }
        std::string context_trace_to_string(Context ctx, int verbosity = 0) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(ctx)),red(" while another error was active")); return "";})
            std::string to_return = "";
            list<Context> trace = get_context_trace(ctx);
            for(int i = trace.length()-1; i>=0; i--) {
                std::string header = pad_str("["+std::to_string(trace.length()-(i+1))+"] ",5);
                to_return += header+context_info(trace[i],verbosity,std::string(header.length()+1,' '),false)+(i!=0?"\n":"");
            }
            return to_return;
        }


        Node copy_as_token(Node node, float x = -1.0f, float y = -1.0f, float z = -1.0f) {
            Node copy = make_node(node.type(),0,node.name().to_std(),(x<0?node.x():x),(y<0?node.y():y),(z<0?node.z():z));
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



    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);
            types[handler_type_id][stages_id].put(label,(void*)&deadptr);
            return *new_stage.getPtr();
        }

        map<uint32_t,Handler> value_printers; 

        std::string value_as_string(Value v) {
            if(is_live(v)) {
                Context ctx = make_context(); ctx.value(v);
                if(value_printers.hasKey(v.type())) {
                    value_printers[v.type()](ctx);
                } else {
                    return "(add value printer for "+labels[v.type()]+")";
                }
                std::string str = ctx.source().to_std();
                deep_recycle_context(ctx);
                return str;
            }
            return  "[DEAD VALUE]";
        }

        std::string value_as_string(Ptr dataptr) {
            if(!is_live(dataptr)) return "";
            Value v = make_value(resolve_to_col(dataptr).tag,0,0,0,0,deadptr,dataptr);
            std::string to_return = value_as_string(v);
            recycle_value(v,false);
            return to_return;

        }
    

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

        inline void invoke_in(Stage* stage, Context& ctx) {
            Stage* old_stage = active_stage;
            active_stage = stage;
            standard_process(ctx);
            active_stage = old_stage;
        }
        inline void invoke_in(Stage& stage, Context& ctx) {
            invoke_in(&stage,ctx);
        }

        bool node_source_position(Node node, float& x, float& y, int depth = 0) {
            if(!is_live(node) || depth > 16) return false;
        
            if(node.x() >= 0.0f && node.y() >= 0.0f) {
                x = node.x(); y = node.y();
                return true;
            }
        
            for(int i = 0; i < node.quals().length(); i++) {
                Node qual = node.quals()[i];
                if(qual.mute() && node_source_position(qual, x, y, depth + 1)) {
                    return true;
                }
            }
        
            for(int i = 0; i < node.quals().length(); i++) {
                Node qual = node.quals()[i];
                if(!qual.mute() && node_source_position(qual, x, y, depth + 1)) {
                    return true;
                }
            }
        
            return false;
        }

        std::string position_of_node(Node node) {
            std::string to_return = "";
            float x = -1.0f; float y = -1.0f;
            if(node_source_position(node, x, y)) {
                if(y>=0) {to_return+=("("+std::to_string((int)y + 1));}
                if(x>=0) {to_return+=(","+std::to_string((int)x + 1));} 
                if(!to_return.empty()){to_return+=")";}
            }
            return to_return;
        }

        std::string enrich_error_msg(Context& ctx, std::string& msg) {
            std::string to_return = "";
            //to_return += context_trace_to_string(ctx);
            to_return += context_info(ctx); //Less verbose form for when I'm working in TwigSnap
            to_return+=red("\nERROR IN "+labels[ctx.pass()]);
            float x = -1.0f; float y = -1.0f;
            if (node_source_position(ctx.node(), x, y)) {
                if(y>=0) {to_return+=red(" ON LINE " + std::to_string((int)y + 1));}
                else if(x>=0) {to_return+=red(" COLUMN " + std::to_string((int)x + 1));} 
                //^ This is intentional, most humans dont' just read column like this, it's included for languages that are just one contigious stream
            }

            if(msg.find("tag is")!=std::string::npos) {
                size_t at = msg.find("tag is") + 7;
                size_t start = at;
                while(at < msg.length() && std::isdigit(msg[at])) at++;
                if(at > start) {
                    uint32_t tag = std::stoul(msg.substr(start, at - start));
                    msg = msg.substr(0, start) + labels[tag] + msg.substr(at);
                }
            }
            if(msg.find("node ")!=std::string::npos) {
                size_t at = msg.find("node ") + 5;
                size_t start = at;
                while(at < msg.length() && msg[at] != ' ') at++;
                std::string stretch = msg.substr(start, at - start);
                if(stretch.find('|') != std::string::npos) {
                    Ptr p = string_to_Ptr(stretch);
                    p.cache = &types;
                    msg = msg.substr(0, start) + node_basic_info_with_children_and_position(Node(p)) + msg.substr(at);
                }
            }

            to_return+=red(": ")+msg;
            return to_return;
        }

        void catch_pass_error(Context& ctx, bool should_print = true) {
            UERROR_FLAG = true;

            ERROR_FLAG = false; 
            if(should_print) {
                print(enrich_error_msg(ctx,ERROR_MSG));
            }
            UERRORS << ERROR_MSG;
            ERROR_MSG = "";
        }
        void end_pass(Context& ctx) {
            endline();
            unit_ctx = ctx.parent();
            deep_recycle_context(ctx);
        }

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            while(!running) {
                has_stopped = true;
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            has_stopped = false;
            if(log_all_process) {
                print(node_info(ctx.node(),1));
            }
            active_stage->run(type)(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        }

        inline void standard_process(Context& ctx) {
            standard_process(ctx,ctx.node().type());
        }
    
        void process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            Context saved_sub = ctx.sub();
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
            ctx.qual(saved_qual);
        }

        //Sets the root as the previous ctx.node
        void sub_process_node(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            Context saved_sub = ctx.sub();
            Node saved_root = ctx.root();
            ctx.root(ctx.node());
            ctx.node(node);
            standard_process(ctx);
            ctx.node(saved_node);
            ctx.sub(saved_sub);
            ctx.root(saved_root);
            ctx.qual(saved_qual);
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
            Context ctx = make_context(deadptr,deadptr,process_node_pass_id,unit_ctx);
            unit_ctx = ctx;
            process_node(ctx,node,left);
            unit_ctx = ctx.parent();
            deep_recycle_context(ctx);
        }
    
        void standard_sub_process_node(Node root) {
            Context ctx = make_context(deadptr,deadptr,process_node_pass_id,unit_ctx);
            unit_ctx = ctx;
            ctx.node(root);
            standard_sub_process(ctx);
            unit_ctx = ctx.parent();

            deep_recycle_context(ctx);
        }

        void standard_sub_process(Context& ctx) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr(),sub_pass_id,ctx);
            sub_ctx.root(ctx.node());
            sub_ctx.sub(ctx.sub());
            int& i = sub_ctx.index();
            while(i < sub_ctx.result().length()) {
                Node node = sub_ctx.result().get(i);
                if(node.resolved()) {
                    node.resolved(false);
                } else {
                    if(i==0) {
                        process_node(sub_ctx, node);
                    } else {
                        process_node(sub_ctx, node, sub_ctx.result().get(i-1));
                    }
                }

                DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(sub_ctx); endline(); return;})

                i++;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        void backwards_sub_process(Context& ctx) { 
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards sub_process while an error was flagged")); return;})
            node_col children = ctx.node().children();
            Context sub_ctx = make_context(children,ctx.source_ptr(),sub_pass_id,ctx);
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
                DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(sub_ctx); endline(); return;})
                i--;
            }
            ctx.flag(sub_ctx.flag());
            recycle_context(sub_ctx);
        }

        //resolve_to_col(sub_ctx).qset(context_source_offset,(void*)&ctx.source_ptr(),sizeof(Ptr));

        void fire_quals(Context& ctx, Value value) {
            Value saved_value = ctx.value();
            Node saved_qual = ctx.qual();
            ctx.value(value);
            for(int q=0;q<value.quals().length();q++) {
                Node qual = value.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                //if(active_stage->has(qual.type()+1))
                standard_process(ctx,qual.type()+1);
            }
            ctx.value(saved_value);
            ctx.qual(saved_qual);
        }
        void fire_quals(Context& ctx, Node node) {
            Node saved_node = ctx.node();
            Node saved_qual = ctx.qual();
            ctx.node(node);
            for(int q=0;q<node.quals().length();q++) {
                Node qual = node.quals()[q];
                if(qual.mute()) continue;
                ctx.qual(qual);
                standard_process(ctx,qual.type()+2);
            }
            ctx.node(saved_node);
            ctx.qual(saved_qual);
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


        void standard_pass_child_scopes(Node node, std::function<void(Node)> behaviour) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    standard_sub_process_node(child);
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            behaviour(child.scopes()[s]);
                        }
                    }
                }
                standard_pass_child_scopes(child, behaviour);
            }
        }

        map<uint32_t,Handler> pass_handlers;
        Handler default_pass_handler = [this](Context& ctx){
            print(red("No pass handler found for type: "+labels[ctx.pass()]));
        };
        bool init_pass_handlers() {
            pass_handlers[undefined_id] = [this](Context& ctx){};
            pass_handlers[direct_pass_id] = [this](Context& ctx){
                while(unit_ctx.index()< unit_ctx.result().length()) {
                    unit_ctx.node(unit_ctx.result().get(unit_ctx.index()));
                    standard_process(unit_ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(unit_ctx); return;})
                    unit_ctx.left((unit_ctx.index()<0)?deadptr:unit_ctx.result().get(unit_ctx.index()));
                    unit_ctx.index()++;
                }
                node_col scopes = unit_ctx.root().scopes();
                for(int i = 0; i<scopes.length(); i++) {
                    standard_direct_pass(scopes.get(i));
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(unit_ctx); return;})
                }
            };

            pass_handlers[resolving_pass_id] = [this](Context& ctx){
                int& i = unit_ctx.index();
                while(i < unit_ctx.result().length()) {    //Process all nodes with scopes first (like any declerations)
                    if(!unit_ctx.result()[i].scopes().empty()) {
                        unit_ctx.node(unit_ctx.result()[i]);
                        standard_process(unit_ctx);
                        DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx);  return;})
                        if(i>=0) {unit_ctx.left(unit_ctx.result()[i]);} else {unit_ctx.left(deadptr);}
                    }
                    i++;
                }
                i = 0;
                while(i < unit_ctx.result().length()) {    //Then process nodes without scopes
                    if(unit_ctx.result()[i].scopes().empty()) {
                        unit_ctx.node(unit_ctx.result()[i]);
                        standard_process(unit_ctx);
                        DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                        if(i>=0) {unit_ctx.left(unit_ctx.result()[i]);} else {unit_ctx.left(deadptr);}
                    }
                    i++;
                }
                i = 0;
                while(i < unit_ctx.result().length()) {    //Then the children of nodes with scopes
                    if(!unit_ctx.result()[i].scopes().empty()) {
                        standard_sub_process_node(unit_ctx.result()[i]);
                    }
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                    i++;
                }
                i = 0;
                while(i < unit_ctx.result().length()) {    //Then finnaly the subscopes
                    standard_pass_child_scopes(unit_ctx.result()[i],[this](Node node){standard_resolving_pass(node);}); //Processing closures and such, any child containing it's own scopes
                    if(!unit_ctx.result()[i].scopes().empty()) {
                        for(int s = 0;s<unit_ctx.result()[i].scopes().length();s++) {
                            if(unit_ctx.result()[i].scopes()[s].owner()==unit_ctx.result()[i]) {
                                standard_resolving_pass(unit_ctx.result()[i].scopes()[s]);
                                DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                            }
                        }
                    }
                    i++;
                }
            };

            pass_handlers[travel_pass_id] = [this](Context& ctx){
                while(unit_ctx.index() < unit_ctx.result().length()) {
                    unit_ctx.node(unit_ctx.result().get(unit_ctx.index()));
                    standard_process(unit_ctx);
                    unit_ctx.left((unit_ctx.index()<0)?deadptr:unit_ctx.result().get(unit_ctx.index()));
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                    if(unit_ctx.state()>0) { //This is the return/break process
                        return;
                    }
                    unit_ctx.index()++;
                }
            };

            pass_handlers[backwards_pass_id] = [this](Context& ctx){
                int& i = unit_ctx.index();
                while(i >= 0) {
                    unit_ctx.node(unit_ctx.result().get(i));
                    standard_process(unit_ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                    node_col scopes = unit_ctx.result().get(i).scopes();
                    standard_pass_child_scopes(unit_ctx.result()[i],[this](Node node){standard_backwards_pass(node);});
                    for(int s = 0; s<scopes.length(); s++) {
                        if(is_live(scopes.get(s).owner())&&scopes.get(s).owner()==unit_ctx.result().get(i)) {
                            memory_backwards_pass(scopes.get(s));
                            DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                        }
                    }
                    unit_ctx.left(unit_ctx.result().get(i));
                    i--;
                }
            };

            pass_handlers[memory_backwards_pass_id] = [this](Context& ctx){
                int& i = unit_ctx.index();
                while(i >= 0) {
                    unit_ctx.node(unit_ctx.result().get(i));
                    standard_pass_child_scopes(unit_ctx.result()[i],[this](Node node){memory_backwards_pass(node);});
                    node_col scopes = unit_ctx.result().get(i).scopes();
                    for(int s = 0; s<scopes.length(); s++) {
                        if(is_live(scopes.get(s).owner())&&scopes.get(s).owner()==unit_ctx.result().get(i)) {
                            memory_backwards_pass(scopes.get(s));
                            DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                        }
                    }
                    standard_process(unit_ctx);
                    DEBUG_ONLY(if(ERROR_FLAG) {catch_pass_error(ctx); return;})
                    unit_ctx.left(unit_ctx.result().get(i));
                    i--;
                }
            };
            return true;
        }
        bool passes_initilized = init_pass_handlers();

        void run_pass(Context& ctx) {
            pass_handlers.getOrDefault(ctx.pass(),default_pass_handler)(ctx);
        }
    
        void standard_direct_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a direct pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Direct pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children,deadptr,direct_pass_id,unit_ctx);
            ctx.root(root);
            unit_ctx = ctx;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
                run_pass(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
            endline();
            unit_ctx = unit_ctx.parent();
            recycle_column(ctx.source());
            recycle_context(ctx);
        }

        void standard_resolving_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a resolving pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Resolving pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children,deadptr,resolving_pass_id,unit_ctx);
            ctx.root(root);
            unit_ctx = ctx;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
                run_pass(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
            endline();
            unit_ctx = unit_ctx.parent();
            recycle_column(ctx.source());
            recycle_context(ctx);
        }

        uint32_t standard_travel_pass(Node root, Context sub = deadptr) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a travel pass while an error was flagged")); return 0;})
            node_col children = root.children();
            newline("Travel pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr,travel_pass_id,unit_ctx);
            ctx.root(root);
            ctx.sub(sub);
            unit_ctx = ctx;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
                run_pass(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
            endline();
            uint32_t state = unit_ctx.state();
            unit_ctx = unit_ctx.parent();
            if(!is_live(sub)) {
                recycle_column(ctx.source());
            }
            recycle_context(ctx);
            return state;
        }

        void standard_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children,deadptr,backwards_pass_id,unit_ctx);
            ctx.root(root);
            ctx.index() = ctx.result().length()-1;
            unit_ctx = ctx;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
                run_pass(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
            endline();
            unit_ctx = unit_ctx.parent();
            recycle_column(ctx.source());
            recycle_context(ctx);
        }

        void memory_backwards_pass(Node root) {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a memory backwards pass while an error was flagged")); return;})
            node_col children = root.children();
            newline("Backwards pass over "+std::to_string(children.length())+" nodes");
            Context ctx = make_context(children,deadptr,memory_backwards_pass_id,unit_ctx);
            ctx.root(root);
            ctx.index() = ctx.result().length()-1;
            unit_ctx = ctx;
            DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
                run_pass(ctx);
            DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
            endline();
            unit_ctx = unit_ctx.parent();
            recycle_column(ctx.source());
            recycle_context(ctx);
        }   

    };
}