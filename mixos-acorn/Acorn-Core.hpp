#pragma once

#include <thread>
#include <mutex>
#include <filesystem>
#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"
#include "../ext/g_lib/core/thread.hpp"

#ifdef _WIN32

#else
    #include <termios.h>
    #include <unistd.h>
    #include <csignal>
#endif


// #include <mach/mach.h>
inline size_t current_memory_usage() {
    // #ifdef _WIN32
    //     return 0;
    // #else
    //     struct mach_task_basic_info info;
    //     mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    //     task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    //     return info.resident_size; // current RSS in bytes
    //     return 0;
    // #endif
    return 0;
}
inline size_t current_rss() {
    // #ifdef _WIN32
    //     return 0;
    // #else
    //     struct mach_task_basic_info info;
    //     mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    //     task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    //     return info.resident_size;
    // #endif
    return 0;
}

inline size_t current_vsz() {
    // #ifdef _WIN32
    //     return 0;
    // #else
    //     struct mach_task_basic_info info;
    //     mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    //     task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    //     return info.virtual_size;
    // #endif
    return 0;
}

#define NAMED_PTRS 0

namespace Acorn {   
    static inline int _ctx_dummy_index = 0;
    class Unit;

     //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
    inline uint32_t create_column(ColCol& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling&&!col.free.empty()) {
            //Lazy version that doesn't care about tags or sizes
            uint32_t idx = col.free.pop();
            Col& ncol = col[idx];
            ncol.erase(); ncol.element_size = size; ncol.tag = tag;  
            ncol.live = true;
            return idx; 
            
            // for(int i=0;i<col.free.length();i++) {
            //     Col& ncol = col[col.free[i]];
            //     if(!ncol.live&&ncol.element_size==size&&(!tag_sensitive||ncol.tag==tag)) {
            //         ncol.clear();
            //         ncol.live = true;
            //         return col.free[i];
            //     }
            // }
        }
        uint32_t index = col.add_idx();
        Col& c = col[index];
        c.element_size = size; c.tag = tag;
        return index;
    }
    //Creates a column from pool and intilizes it's memory if empty
    inline uint32_t push_column(ColCol& col, uint32_t size, uint32_t tag) {
        uint32_t at = create_column(col,size,tag);
        Col& ncol = *(Col*)col.sget(at);
        if(ncol.size<size) {
            ncol.resize(size);
        }
        return at;
    }
    static void recycle_column(ColCol& col, uint32_t id) {
        CHECK_ERROR("Tried to recycle ",id," while an error was active");
       Col* c = ((Col*)col.sget(id));
       if(c) {
        c->live = false;
        c->gen++;
        col.free.push(id);
       } else {
        throw_error("core:recycle_column unable to recycle ",id);
       }
    }
    
    inline ColColCol& cache_as_unit(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 3) throw_error("cache_as_unit called on Ptr with cachelevel ", p.cachelevel);)
        return *(ColColCol*)p.cache; 
    }
    inline ColCol& cache_as_pool(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 2) throw_error("cache_as_pool called on Ptr with cachelevel ", p.cachelevel);)
        return *(ColCol*)p.cache; 
    }
    inline Col& cache_as_col(const Ptr& p) { 
        DEBUG_ONLY(if(p.cachelevel != 1) throw_error("cache_as_col called on Ptr with cachelevel ", p.cachelevel);)
        return *(Col*)p.cache; 
    }

    inline  std::string Ptr_to_string(Ptr p, int print_level = 3) {
        if(p.cachelevel > 0 && print_level > p.cachelevel)  return "CANNOT PRINT LEVEL "+std::to_string(print_level)+" ON PTR WITH CACHELEVEL "+std::to_string(p.cachelevel);

        switch(print_level) {
            case 7: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.subunit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 6: return std::to_string(p.zone)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.subunit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 5: return std::to_string(p.unit)+"|"+std::to_string(p.subunit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 4: return std::to_string(p.subunit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 3: return std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 2: return std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 1: return std::to_string(p.sidx);
            case 0: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.subunit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            default: return "INVALID PRINT LEVEL FOR PTR_TO_STRING "+std::to_string(print_level);
        }
    }
    inline std::string capture_ptr(Ptr p) {
        return std::to_string((uint64_t)p.cache)+"."+std::to_string(p.gen)+"."+std::to_string(p.specialization)+"."+Ptr_to_string(p,p.cachelevel);
    }
    inline Ptr decode_ptr_string(const std::string& s) {
        auto l = split_str(s,'|');
        if(l.length()==2) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]));
            p.cachelevel = 2;
            return p;
        } else if(l.length()==3) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]));
            p.cachelevel = 3;
            return p;
        } else if(l.length()==4) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]),std::stoi(l[3]));
            p.cachelevel = 4;
            return p;
        } else if(l.length()==5) {
            Ptr p(std::stoi(l[0]),std::stoi(l[1]),std::stoi(l[2]),std::stoi(l[3]),std::stoi(l[4]));
            p.cachelevel = 5;
            return p;
        } else {
            print(red("Unable to convert "+s+" to a Ptr")); 
            return deadptr;
        }
    }
    inline Ptr string_to_Ptr(const std::string& s) {
        if(s.find('.') != std::string::npos) {
            auto parts = split_str(s, '.');
            Ptr p = decode_ptr_string(parts[3]);
            p.cache = (void*)std::stoull(parts[0]);
            p.gen = (uint16_t)std::stoul(parts[1]);
            p.specialization = (uint8_t)std::stoul(parts[2]);
            return p;
        } else {
            return decode_ptr_string(s);
        }
    }
    inline uint8_t string_to_cachelevel(const std::string& s) {
        uint8_t to_return = 0;
        for(auto& c : s) if(c=='|') to_return++;
        if(to_return!=0) to_return+=1;
        return to_return;
    }

    inline list<g_ptr<Unit>> units;
    static inline std::mutex units_mutex;

    inline ColColCol& init_first_unit();

    inline ColColCol& global = init_first_unit();


    inline uint32_t undefined_id = 0;
    inline uint32_t stages_id = 1;
    inline uint32_t ptr_id = 2; inline uint32_t prefix_ptr_id = 3; inline uint32_t suffix_ptr_id = 4;
    // inline uint32_t string_id = 5; inline uint32_t prefix_string_id = 6; inline uint32_t suffix_string_id = 7;

    static inline ColColCol col3_ref;
    static inline PtrColColCol pcol3_ref;
    inline PtrColColCol& resolve_to_unit(const Ptr& ptr);
    inline ColColCol& resolve_to_subunit(const Ptr& ptr);
    static inline ColCol col2_ref;
    inline ColCol& resolve_to_pool(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 4: case 3: {
                ColColCol& unit = resolve_to_subunit(ptr); 
                DEBUG_ONLY(if(ERROR_FLAG) {return col2_ref;});
                //CHECK_ERROR_VAL(col2_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to pool because it failed to resolve to a subunit"); 
                ColCol& col = unit[ptr.pool];
                DEBUG_ONLY(if(ERROR_FLAG) {return col2_ref;});
                //CHECK_ERROR_VAL(col2_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to pool because it was out of bounds");
                return col;
            }
            case 2: return *(ColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to pool because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col2_ref;
        }
    }

    inline Col& resolve_to_col(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 4: case 3: case 2: {
                ColCol& pool = resolve_to_pool(ptr); 
                DEBUG_ONLY(if(ERROR_FLAG) {return col1_ref;});
                //CHECK_ERROR_VAL(col1_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to col because it failed to resolve to a pool"); 
                Col& col = pool[ptr.idx];
                DEBUG_ONLY(if(ERROR_FLAG) {return col1_ref;});
                //CHECK_ERROR_VAL(col1_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to col because it was out of bounds");
                return col;
            }
            case 1: return *(Col*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to col because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col1_ref;
        }
    }
    inline void* resolve_ptr(Ptr& ptr) {
        return (uint8_t*)resolve_to_col(ptr)[ptr.sidx]+ptr.offset();
    }
    inline void* resolve_ptr(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_ptr(ptr);}
    inline Col& resolve_to_col(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_to_col(ptr);}

    inline Ptr makePtr(ColColCol& unit, uint32_t pool, uint32_t idx = 0, uint32_t sidx = 0) {return Ptr(&unit,pool,idx,sidx);}
    inline Ptr makePtr(ColColCol& unit, ColCol* pool) {return makePtr(unit,unit.indexof(pool));}
    inline Ptr makePtr(ColColCol& unit, ColCol& pool) {return makePtr(unit,&pool);}


    inline Ptr get_ticket_from_unit(Ptr p, uint32_t type_id, uint32_t size, uint32_t tag);
    inline uint32_t find_pool_tag_in_unit(Ptr p, uint32_t tag);
    inline uint32_t size_of_from_unit(Ptr p, uint32_t tag);




    struct string : Ptr {
        string() {}
        string(Ptr p) : Ptr(p) {}
        inline Col& col() {return resolve_to_col(*this); }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {col().QCol::push(s,len);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void push(string s) {push((const char*)s.col().storage, s.length()); }
        inline void insert(uint32_t idx, char c) { col().insert(idx, &c); }
        inline void insert(uint32_t idx, const char* s, uint32_t len) { for(uint32_t i = 0; i < len; i++) col().insert(idx+i, &s[i]); }
        inline void insert(uint32_t idx, const std::string& s) { insert(idx, s.data(), s.length()); }
        inline void insert(uint32_t idx, string s) { insert(idx, (const char*)s.col().storage, s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {Col& c = col(); CHECK_ERROR_VAL(red("to_std failed: ")+ERROR_MSG,"Bad col in string");  return std::string((char*)c.storage, length());}
        inline int find(string look_for, int start_at, int nth_of = 1) {
            int found_at = -1;
            for(int i=start_at;i<col().length();i++) {
                if(i+look_for.length() > col().length()) break; 
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
    struct RString : Col {
        inline void push(const std::string& s) {QCol::push(s.data(), (uint32_t)s.length()); }
        inline void push(string s) {QCol::push(s.col().storage, s.length()); }
    };
    inline string get_global_string_ticket();



    struct type_and_value {
        uint32_t type;
        Ptr value;
    };

    inline uint32_t offsets_col = 0;
    inline uint32_t tags_col = offsets_col + 1;
    inline uint32_t sizes_col = tags_col + 1;
    inline uint32_t labels_col = sizes_col + 1;
    inline uint32_t subtags_col = labels_col + 1;
    inline uint32_t subsizes_col = subtags_col + 1;
    inline uint32_t ptrs_col = subsizes_col + 1;

    inline uint32_t overloads_col = ptrs_col + 1;

    inline Col& global_resolve_to_col(const Ptr& ptr, const uint32_t& idx) {return global[ptr.pool][idx];}

    struct _layout {
        _layout() {}
        _layout(Ptr p) : impl(p) {}
        Ptr impl = deadptr;
        map<std::string,uint32_t> label_to_index;
        uint32_t total_size = 0;

        list<uint32_t> offsets;
        list<uint32_t> tags;
        list<uint32_t> sizes;
        list<std::string> labels;

        list<uint32_t> subtags;
        list<uint32_t> subsizes;
        list<Ptr> ptrs;

        Col overloads = Col(sizeof(type_and_value));

        void add_overload(uint64_t key, uint32_t type, Ptr value) {
            type_and_value tnv{type, value};
            overloads.put(key,(void*)&tnv);
            if(is_live(impl)) {
                resolve_to_col(impl,impl.idx+overloads_col).put(key,(void*)&tnv);
            } else {
                throw_error("Unable to add overload to layout because it's implmentation is dead");
            }
        }
        bool has_overload(uint64_t key) {
            return overloads.hasKey(key);
        }
        type_and_value get_overload(uint64_t key) {
            return *(type_and_value*)overloads.get(key);
        }

        void add_overload(std::string key, uint32_t type, Ptr value) {
            type_and_value tnv{type, value};
            overloads.put(key,(void*)&tnv);
            if(is_live(impl)) {
                resolve_to_col(impl,impl.idx+overloads_col).put(key,(void*)&tnv);
            } else {
                throw_error("Unable to add overload to layout because it's implmentation is dead");
            }
        }
        bool has_overload(std::string key) {
            return overloads.hasKey(key);
        }
        type_and_value get_overload(std::string key) {
            return *(type_and_value*)overloads.get(key);
        }


        uint32_t add_prop(uint32_t tag, uint32_t size, const std::string& label, uint32_t subtag = 0, uint32_t subsize = 0, Ptr ptr = deadptr) {
            label_to_index.put(label,offsets.length());
            offsets << total_size;
            tags << tag;
            sizes << size;
            labels << label;
            subtags << subtag;
            subsizes << subsize;
            ptrs << ptr;
            //print("My impl: ",Ptr_to_string(impl,impl.cachelevel));
            resolve_to_col(impl,impl.idx+offsets_col).push((void*)&total_size);
            // print("B");
            resolve_to_col(impl,impl.idx+tags_col).push((void*)&tag);
            resolve_to_col(impl,impl.idx+sizes_col).push((void*)&size);
            string label_ptr = get_global_string_ticket();
            label_ptr = label;
            resolve_to_col(impl,impl.idx+labels_col).push((void*)&label_ptr);
            resolve_to_col(impl,impl.idx+subtags_col).push((void*)&subtag);
            resolve_to_col(impl,impl.idx+subsizes_col).push((void*)&subsize);
            resolve_to_col(impl,impl.idx+ptrs_col).push((void*)&ptr);
            uint32_t old_size = total_size;
            total_size += size;
            return old_size;
        }
    };


    inline uint32_t add_type() {
        return global.add_idx();
    }

    inline uint32_t typedata_type_id = add_type();
    inline void global_add_typedata(Col& type, uint32_t tag, uint32_t size) {
        Ptr typedata(&global,typedata_type_id,create_column(global[typedata_type_id],size,tag),0);
        type.put("data",(void*)&typedata);
    }

    inline uint32_t init_handler_type() {
        uint32_t at = global.add_idx();
        ColCol& t = global[at];
        t.add("UNDEFINED",0,0);
        t.add("stages",sizeof(Ptr),ptr_id);
        t.add("ptr",sizeof(Ptr),ptr_id); t.add("prefix_ptr",sizeof(Ptr),ptr_id); t.add("suffix_ptr",sizeof(Ptr),ptr_id);
        global_add_typedata(t.get(2),ptr_id,sizeof(Ptr));
        return at;
    }

    inline uint32_t handler_type_id = init_handler_type();
    inline uint32_t layout_type_id = add_type(); 

    inline uint32_t size_of_from_subunit(ColColCol& subunit, uint32_t tag) {
        Col& handlers = subunit[handler_type_id][tag];
        if(handlers.hasKey("data")) {
            return resolve_to_col(*(Ptr*)handlers.get("data")).element_size;
        } else {
            throw_error("global:size_of_from_subunit Tag "+handlers.label.to_std()+" has no data handler");
            return 0;
        }
    }
    inline uint32_t global_size_of(uint32_t tag) {
        Col& handlers = global[handler_type_id][tag];
        return size_of_from_subunit(global,tag);
    }

    inline uint32_t global_reg_id(const std::string& label) {
        uint32_t at = global[handler_type_id].length();
        global[handler_type_id].add(label,sizeof(Ptr),ptr_id);
        return at;
    }
    inline uint32_t global_register_type_ids(const std::string& label) {
        uint32_t id = global_reg_id(label);
        global_reg_id("prefix_"+label);
        global_reg_id("suffix_"+label);
        return id;
    }
    //Qual handlers which act on the value
    inline size_t to_prefix_id(size_t id) {return id+1;}
    //Qual handlers which act on the node
    inline size_t to_suffix_id(size_t id) {return id+2;}

    inline uint32_t float_id  = global_register_type_ids("float");
    inline uint32_t int_id    = global_register_type_ids("int");
    inline uint32_t bool_id   = global_register_type_ids("bool");
    inline uint32_t string_id = global_register_type_ids("string");
    inline uint32_t char_id   = global_register_type_ids("char");
    inline uint32_t ptr4_id   = global_register_type_ids("ptr4");
    inline uint32_t duck_id   = global_register_type_ids("duck");
    inline uint32_t void_id   = global_register_type_ids("void");
    
    inline size_t col_id = global_register_type_ids("col");
    inline size_t colcol_id = global_register_type_ids("colcol");
    inline size_t colcolcol_id = global_register_type_ids("colcolcol");
    inline size_t subunit_id = global_register_type_ids("subunit");

    inline Col& global_setup_typedata_for(uint32_t tag, uint32_t size) {
        global_add_typedata(global[handler_type_id][tag],tag,size);
        return resolve_to_col(*(Ptr*)global[handler_type_id][tag].get("data"));
    }
    inline bool init_basic_typedatas() {
        Col& float_col = global_setup_typedata_for(float_id,4);
        Col& int_col = global_setup_typedata_for(int_id,4);
        Col& bool_col = global_setup_typedata_for(bool_id,1);
        Col& string_col = global_setup_typedata_for(string_id,sizeof(Ptr));
        Col& char_col = global_setup_typedata_for(char_id,1);
        Col& ptr4_col = global_setup_typedata_for(ptr4_id,sizeof(Ptr4));
        Col& duck_col = global_setup_typedata_for(duck_id,0);
        Col& void_col = global_setup_typedata_for(void_id,0);
        return true;
    }
    inline bool basic_typedata_initlized = init_basic_typedatas();


    inline size_t list_id = global_reg_id("list");
    inline size_t map_id = global_reg_id("map");
    inline size_t weakptr_id = global_reg_id("weakptr");
    
    inline uint32_t silenced_id = global_reg_id("SILENCED");
    inline uint32_t any_id = global_reg_id("any");
    inline uint32_t null_id = global_reg_id("null");
    inline size_t identifier_id = global_reg_id("IDENTIFIER");
    inline size_t literal_id = global_reg_id("LITERAL");
    inline uint32_t root_id = global_reg_id("ROOT");
    
    inline size_t node_id = global_reg_id("node"); inline size_t prefix_node_id = global_reg_id("prefix_node"); inline size_t suffix_node_id = global_reg_id("suffix_node");
    inline size_t value_id = global_reg_id("value"); inline size_t prefix_value_id = global_reg_id("prefix_value"); inline size_t suffix_value_id = global_reg_id("suffix_value");
    inline size_t context_id = global_reg_id("context"); inline size_t prefix_context_id = global_reg_id("prefix_context"); inline size_t suffix_context_id = global_reg_id("suffix_context");

    inline size_t var_decl_id = global_reg_id("VAR_DECL");
    inline size_t func_call_id = global_reg_id("FUNC_CALL");
    inline uint32_t lambda_call_id = global_reg_id("LAMBDA_CALL");
    inline size_t lambda_id = global_reg_id("LAMBDA");
    inline size_t function_id = global_reg_id("function"); inline size_t prefix_function_id = global_reg_id("prefix_function"); inline size_t suffix_function_id = global_reg_id("suffix_function");
    inline size_t method_call_id = global_reg_id("METHOD_CALL");
    inline size_t method_id = global_reg_id("METHOD");
    inline size_t func_decl_id = global_reg_id("FUNC_DECL");
    inline size_t type_decl_id = global_reg_id("TYPE_DECL");
    inline uint32_t hide_block_id = global_reg_id("HIDE_BLOCK");

    inline uint32_t sub_pass_id = global_reg_id("SUB_PASS");
    inline uint32_t process_node_pass_id = global_reg_id("PROCESS_NODE");
    inline uint32_t direct_pass_id = global_reg_id("DIRECT_PASS");
    inline uint32_t resolving_pass_id = global_reg_id("RESOLVING_PASS");
    inline uint32_t travel_pass_id = global_reg_id("TRAVEL_PASS");
    inline uint32_t backwards_pass_id = global_reg_id("BACKWARDS_PASS");
    inline uint32_t memory_backwards_pass_id = global_reg_id("MEMORY_BACKWARDS_PASS");

    inline uint32_t headerpool_id = global_reg_id("headerpool");  inline uint32_t header_id = global_register_type_ids("header");
    inline uint32_t messagepool_id = global_reg_id("messagepool");
    inline uint32_t footerpool_id = global_reg_id("footerpool");

    inline uint32_t stackpool_id = global_reg_id("stackpool");
    inline uint32_t heappool_id = global_reg_id("heappool");
    
    inline uint32_t name_store_id = add_type(); 

    inline size_t tombstone_col = 0; 
    inline size_t refs_col = 0;

    inline Ptr global_add_layout_to_col(uint32_t type) {
        Ptr p((uint32_t)0,layout_type_id,global[layout_type_id].add_idx(std::to_string(type)+" Offsets",4,int_id),0);
        global[layout_type_id].add("Tags",4,int_id);
        global[layout_type_id].add("Sizes",4,int_id);
        global[layout_type_id].add("Labels",sizeof(Ptr),string_id);
        global[layout_type_id].add("Subtags",4,int_id);
        global[layout_type_id].add("Subsizes",4,int_id);
        global[layout_type_id].add("Ptrs",sizeof(Ptr),ptr_id);
        global[layout_type_id].add("Overloads",sizeof(Ptr4),ptr4_id);
        global[handler_type_id][type].put("Layout",(void*)&p,string_id);
        return p;
    }

    inline uint32_t make_store_type() {
        uint32_t at = add_type();
        return at;
    }
    

    template<typename T>
    struct col_Ptr : Ptr {
        col_Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx)  : Ptr(_unit,_pool,_idx,0) {}

        inline bool safety_check(std::string log_msg) {if(ERROR_FLAG) {log(red("Attempted to call "),log_msg,red(" while another error was flagged")); return true;} if(!is_live(*this)) {throw_error("Attempted ",log_msg," but col_ptr was dead"); log(red("ERROR: "),ERROR_MSG); return true;} return false;}
    
        inline Col& col()                    {DEBUG_ONLY(if(safety_check("col_ptr:col")){static Col d; return d;}) return resolve_to_col(*this);}
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
        inline void insert( uint32_t idx, T t){DEBUG_ONLY(if(safety_check("col_ptr:insert")){return;}) Ptr p(t); col().insert(idx,&p);}
    
        inline bool hasKey(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:hasKey")){return false;}) return col().hasKey(key);}
        inline T get(const std::string& key) {DEBUG_ONLY(if(safety_check("col_ptr:get_key")){return T(deadptr);}) return T(*(Ptr*)col().get(key));}
        inline T operator[](const std::string& key) {return get(key);}
        inline void put(const std::string& key, T t) {DEBUG_ONLY(if(safety_check("col_ptr:put")){return;}) col().put(key, (void*)&t, string_id);}
    };
    
    inline uint32_t message_id = global_register_type_ids("message");
    inline uint32_t message_from_offset = 0;
    inline uint32_t message_to_offset = 0;
    inline uint32_t message_status_offset = 0;
    inline uint32_t message_total_size = 0;




    inline string get_global_string_ticket() {
        Ptr ticket((uint32_t)0,name_store_id,create_column(global[name_store_id],1,char_id),0);
        return ticket;
    }

    inline Ptr global_add_template(uint32_t for_type) {
        Ptr p = global_add_layout_to_col(for_type);
        return p;
    }


    inline bool init_message_type() {
        _layout mtemp(global_add_template(message_id)); //Message template
        message_from_offset = mtemp.add_prop(int_id, 4, "from");
        message_to_offset = mtemp.add_prop(int_id, 4, "to");
        message_status_offset = mtemp.add_prop(int_id, 4, "status");
        message_total_size = mtemp.total_size;
        return true;
    }
    inline bool message_type_ready = init_message_type();

    struct Message : public Ptr {
        Message() {}
        Message(Ptr p) : Ptr(p) {}

        uint32_t& from() {return *(uint32_t*)resolve_to_col(*this).qget(message_from_offset+(sidx*message_total_size));}
        uint32_t& to() {return *(uint32_t*)resolve_to_col(*this).qget(message_to_offset+(sidx*message_total_size));}
        uint32_t& status() {return *(uint32_t*)resolve_to_col(*this).qget(message_status_offset+(sidx*message_total_size));}
    };


    struct Header : Ptr {
        Header() {}
        Header(Ptr p) : Ptr(p) {}

        ColCol& col() {return resolve_to_pool(*this);}
        ColColCol& col3() {return resolve_to_subunit(*this);}

        uint32_t add_ribbon(std::string key = "") {
            uint32_t at = col().length();
            create_column(col(),sizeof(Ptr),ptr_id);
            if(!key.empty()) {
                col().get(at).label = key;
                CCol c; c.index = at; c.hash = hashBytes(key.data(),key.length()); c.tag = string_id; ((QString&)c) = key;
                col().cells.scan_for_slot(std::move(c));
            } else {
                col().get(at).label = "Ribbon";
            }
            return at;
        }

        Col& ribbon(std::string ribbon_label = "") {
            if(!col().empty()) {
                if(ribbon_label.empty()) {
                    return col().get(idx);
                } else {
                    void* data = col().Col::get(ribbon_label);
                    CHECK_ERROR_VAL(col1_ref,"No ribbon found with label ",ribbon_label);
                    return *(Col*)data;
                }
            } else {
                throw_error("Header:ribbon header is empty!");
                return col1_ref;
            }
        }
        void* get(const std::string& label, std::string ribbon_label = "") {
            Col& col = ribbon(ribbon_label);
            CHECK_ERROR_VAL(nullptr,"Invalid ribbon in get");
            void* data = col.get(label);
            CHECK_ERROR_VAL(nullptr,"Label ",label," wasn't found in get");
            Ptr p = *(Ptr*)data;
            if(p.cachelevel==2) p.cache = this;
            return resolve_ptr(p);
        }

        string getString(const std::string& label,std::string ribbon_label = "") {
            void* got = get(label,ribbon_label);
            CHECK_ERROR_VAL(deadptr,"Label ",label," wasn't found in getString");
            return (string&)*(Ptr*)got;
        }
        void putString(const std::string& label, const std::string& str, std::string ribbon_label = "") {
            Col& ribcol = ribbon(ribbon_label);
            uint32_t col_at = col().indexof(&ribcol);
            CHECK_ERROR("Invalid ribbon in putString");
            uint32_t header_at = col3().indexof(&col());
            Ptr str_ticket(&col3(),header_at,create_column(col3()[header_at],sizeof(Ptr),string_id,true),0);
            Ptr char_ticket(&col3(),header_at,create_column(col3()[header_at],1,char_id,true),0);
            resolve_to_col(str_ticket).push((void*)&char_ticket);
            ((string)char_ticket) = str;
            col().get(col_at).put(label,(void*)&str_ticket,string_id);
        }
    };



    inline uint64_t get_real_size_of_col(Col& col) {
        uint64_t result = sizeof(Col)+col.size;
        return result;
    }
    inline uint64_t get_real_size_of_colcol(ColCol& col) {
        uint64_t result = sizeof(ColCol);
        for(int i=0;i<col.length();i++) {
            result += get_real_size_of_col(col[i]);
        }
        return result;
    }
    inline uint64_t get_real_size_of_colcolcol(ColColCol& col) {
        uint64_t result = sizeof(ColColCol);
        for(int i=0;i<col.length();i++) {
            result += get_real_size_of_colcol(col[i]);
        }
        return result;
    }


    inline uint64_t get_real_real_size_of_col(Col& col) {
        uint64_t result = sizeof(Col)+col.size;
        for(uint32_t i=0;i<col.length();i++) {
            Col* subcol = nullptr;
            if(col.specialization==Spec::COL) {
                subcol = (Col*)col.get(i);
            } else if(col.specialization==Spec::PTR_COL) {
                subcol = *(Col**)col.get(i);
            }
            if(subcol) {
                result+=get_real_real_size_of_col(*subcol);
            }
        }
        return result;
    }
    inline uint64_t get_real_capacity_of_col(Col& col) {
        uint64_t result = sizeof(Col)+col.capacity;
        for(uint32_t i=0;i<col.length();i++) {
            Col* subcol = nullptr;
            if(col.specialization==Spec::COL) {
                subcol = (Col*)col.get(i);
            } else if(col.specialization==Spec::PTR_COL) {
                subcol = *(Col**)col.get(i);
            }
            if(subcol) {
                result+=get_real_capacity_of_col(*subcol);
            }
        }
        return result;
    }

    inline void clear_all_col_locks(Col& col) {
        for(uint32_t i=0;i<col.length();i++) {
            Col* subcol = nullptr;
            if(col.specialization==Spec::COL) {
                subcol = (Col*)col.get(i);
            } else if(col.specialization==Spec::PTR_COL) {
                subcol = *(Col**)col.get(i);
            }
            if(subcol) {
                clear_all_col_locks(*subcol);
            }
        }
        col.live.store(0);
    }

    class Unit : public q_object {
        public:
        g_ptr<Log::Span> uspan = make<Log::Span>();
        list<std::string> uargs;

        bool UERROR_FLAG = false;
        list<std::string> UERRORS;


        g_ptr<Thread> uthread = nullptr;
        void start_thread(std::function<void()> func) {
            if(uthread) {uthread->end();}
            uthread = make<Thread>();
            uthread->run_raw(func);
        }

        uint16_t derive_uid(bool init_layouts) {
            uid = (uint16_t)units.length();

            if(init_layouts) {
                ColCol& h = global[handler_type_id];
                for(int i = 0; i < h.length(); i++) {
                    Col& handler_col = h[i];
        
                    labels.put(i,handler_col.label.to_std());
                    labels_lookup.put(handler_col.label.to_std(),i);

                    if(handler_col.hasKey("Layout")) {
                        Ptr lptr = *(Ptr*)handler_col.get("Layout");
                        _layout l(lptr);
                
                        Col& offsets_c  = resolve_to_col(lptr, lptr.idx + offsets_col);
                        Col& tags_c     = resolve_to_col(lptr, lptr.idx + tags_col);
                        Col& sizes_c    = resolve_to_col(lptr, lptr.idx + sizes_col);
                        Col& labels_c   = resolve_to_col(lptr, lptr.idx + labels_col);
                        Col& subtags_c  = resolve_to_col(lptr, lptr.idx + subtags_col);
                        Col& subsizes_c = resolve_to_col(lptr, lptr.idx + subsizes_col);
                        Col& ptrs_c     = resolve_to_col(lptr, lptr.idx + ptrs_col);
                        //l.overloads     = resolve_to_col(lptr, lptr.idx + overloads_col);
                
                        uint32_t count = offsets_c.size / sizeof(uint32_t);
                        for(uint32_t f = 0; f < count; f++) {
                            uint32_t offset  = *(uint32_t*)offsets_c.sget(f);
                            uint32_t tag     = *(uint32_t*)tags_c.sget(f);
                            uint32_t size    = *(uint32_t*)sizes_c.sget(f);
                            Ptr      label_p = *(Ptr*)labels_c.sget(f);
                            uint32_t subtag  = *(uint32_t*)subtags_c.sget(f);
                            uint32_t subsize = *(uint32_t*)subsizes_c.sget(f);
                            Ptr      ptr     = *(Ptr*)ptrs_c.sget(f);
                            
                            std::string label_str = string(label_p).to_std();
                            l.label_to_index.put(label_str, l.offsets.length());
                            l.offsets  << offset;
                            l.tags     << tag;
                            l.sizes    << size;
                            l.labels   << label_str;
                            l.subtags  << subtag;
                            l.subsizes << subsize;
                            l.ptrs     << ptr;
                        }
                        l.total_size = l.offsets.length() > 0 
                            ? l.offsets.last() + l.sizes.last() 
                            : 0;
                        l.impl.unit = uid;
                        layouts.put(i,l);
                    }
                }
            }

            std::lock_guard<std::mutex> lock(units_mutex);
            units << this;
            return (uint16_t)units.length()-1;
        }

        Unit() : types(global), uid(derive_uid(true)) {
            ColColCol* types_ptr = &types; subunits.push(&types_ptr); 
            ColCol& h = types[handler_type_id];
                for(uint32_t i = 0; i < h.length(); i++) {
                    Col& handler_col = h[i];
                    if(handler_col.hasKey("data")) {
                        Ptr& typedata_ptr = *(Ptr*)handler_col.get("data");
                        typedata_ptr.cache = &types;
                    }
                }
            init();
        }
        Unit(const ColColCol& starter) : types(starter), uid(derive_uid(false)) {ColColCol* types_ptr = &types; subunits.push(&types_ptr); init();}
        Unit(bool do_not_init) {
            ColColCol* types_ptr = &types; subunits.push(&types_ptr); 
        }

        ~Unit() {
            memset(subunits.storage,0,sizeof(void*));
        }   

    
        map<uint32_t, std::string> labels;
        map<std::string,uint32_t> labels_lookup;
        map<uint32_t,_layout> layouts;
        uint32_t next_id = 0;
        uint16_t uid;

        std::string unit_label = "";

        ColColCol types;
        PtrColColCol subunits;
        inline ColColCol* get_subunit(uint32_t idx) {return &subunits[idx];}
        ColCol& operator[](uint16_t index) {return types[index];}

        // ColColCol setup_mailbox_subunit() {
        //     ColColCol mailbox;
        //     ColCol instr_plate; instr_plate.label = "Instructions";
        //     mailbox.push(instr_plate);
        //     mailbox.unlock();
        //     return mailbox;
        // }

        // ColColCol sendunit = setup_mailbox_subunit();
        // ColColCol recvunit = setup_mailbox_subunit();

        virtual void init() {
           
        }

        bool running = true; 
        void suspend() {running = false;}
        void resume() {running = true;}
        bool has_stopped = false;

        inline Ptr get_ticket(uint32_t type_id, uint32_t size, uint32_t tag, ColColCol* in = nullptr) {
            if(!in) in = &types;
            if(type_id>=in->length()) {throw_error("Unable to create a ticket: an invalid type id ",type_id," was given"); return deadptr;}
            Ptr ticket(in,type_id,create_column(in->get(type_id),size,tag,true),0);
            Col& col = resolve_to_col(ticket);
            CHECK_ERROR_VAL(ticket,"Bad ticket created: ",Ptr_as_string(ticket));
            ticket.gen = col.gen;
            return ticket;
        }

        inline Ptr get_ticket(ColCol* pool, uint32_t size, uint32_t tag) {
            Ptr ticket(pool,create_column(*pool,size,tag,true),0);
            ticket.gen = resolve_to_col(ticket).gen;
            return ticket;
        }

        inline Ptr get_ticket(Ptr storeptr, uint32_t size, uint32_t tag) {
            if(storeptr.cachelevel==0) {
                Ptr ticket(storeptr.unit,storeptr.subunit,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                ticket.gen = resolve_to_col(ticket).gen;
                return ticket;
            } else {
                Ptr ticket(storeptr.cache,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                ticket.gen = resolve_to_col(ticket).gen;
                return ticket;
            }
        }

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

        std::string Ptr_as_string(Ptr p) {
            if(p.specialization==_DEADSPEC) {
                return "x|x|x";
            } else if(p.cachelevel==1||p.cachelevel==2) {
                return Ptr_to_string(p,p.cachelevel);
            }

            if(ERROR_FLAG) {
                return red("ERROR_ACTIVE:"+Ptr_to_string(p,p.cachelevel));
            } else {
                if(p.cachelevel==0||p.cachelevel>5) {
                    if(p.unit>=units.length()) {
                        return red("UNIT_OUT_OF_BOUNDS:"+Ptr_to_string(p,p.cachelevel));
                    }
                } else {
                    if(!p.cache) return red("PTR_CACHE_MISSING");
                }
                //This crashes during static intilization, invesitgate later.
                // if(p.subunit>=resolve_to_unit(p).length()) {
                //     return red("SUBUNIT_OUT_OF_BOUNDS("+std::to_string(resolve_to_unit(p).length())+"):"+Ptr_to_string(p,p.cachelevel));
                // } else 
                
                if(p.pool>=resolve_to_subunit(p).length()) {
                    return red("POOL_OUT_OF_BOUNDS("+std::to_string(resolve_to_subunit(p).length())+"):"+Ptr_to_string(p,p.cachelevel));
                } else if(p.idx>=resolve_to_pool(p).length()) {
                    return red("IDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_pool(p).length())+"):"+Ptr_to_string(p,p.cachelevel));
                } else { //This keeps popping up on sidx 0 on accident
                    // if(resolve_to_col(p).heterogenous) {
                    //     if(p.sidx>=resolve_to_col(p).size) {
                    //         return red("SIDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_col(p).size)+"):"+Ptr_to_string(p));
                    //     }
                    // } else {
                    //     if(p.sidx>=resolve_to_col(p).length()) {
                    //         return red("SIDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_col(p).length())+"):"+Ptr_to_string(p));
                    //     }
                    // }
                } 
            }

            //ADD CACHE LEVELS HERE LATER!!!
            #if NAMED_PTRS
                std::string plabel = resolve_to_pool(p).label.empty()?std::to_string(p.pool):resolve_to_pool(p).label.to_std();
                std::string pidx = resolve_to_col(p).label.empty()?std::to_string(p.idx):resolve_to_col(p).label.to_std();
                std::string pstring = "";
                if(p.cachelevel==3) {
                    pstring = plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
                } else {
                    pstring = std::to_string(p.unit)+"|"+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
                }
                uint64_t key = Ptr_to_key(p);
            
                if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
                return pstring;
            #else
                //ptr_to_string(p.cache)+"|"
                return Ptr_to_string(p,p.cachelevel)+(p.gen>0?"|G"+std::to_string(p.gen):"");
            #endif
        }

        void print_layout(_layout& l) {
            for(int i=0;i<l.offsets.length();i++) {
                print(i,": ",labels[i],": ",l.offsets[i],", ",labels[l.tags[i]],", ",labels[l.subtags[i]],"[",l.sizes[i],"]");
            }
            // for(auto e : l.overload.entrySet()) {
            //     auto keyl = decode_key(e.key);
            //     print(labels[keyl.first]," ",labels[keyl.second],"(",keyl.second,"): ",labels[e.value.type]);
            // }
        }

        inline uint32_t size_of(uint32_t tag) {
            return size_of_from_subunit(types,tag);
        }


        uint32_t reg_id(const std::string& label) {
            uint32_t at = types[handler_type_id].length();
            types[handler_type_id].add(label,sizeof(Ptr),ptr_id);
            labels[at] = label;
            labels_lookup[label] = at;
            return at;
        }
        Ptr add_layout_to_col(uint32_t type) {
            Ptr p(uid,layout_type_id,types[layout_type_id].add_idx(std::to_string(type)+" Offsets",4,int_id),0);
            types[layout_type_id].add("Tags",4,int_id);
            types[layout_type_id].add("Sizes",4,int_id);
            types[layout_type_id].add("Labels",sizeof(Ptr),string_id);
            types[layout_type_id].add("Subtags",4,int_id);
            types[layout_type_id].add("Subsizes",4,int_id);
            types[layout_type_id].add("Ptrs",sizeof(Ptr),ptr_id);
            types[layout_type_id].add("Overloads",sizeof(Ptr4),ptr4_id);
            types[handler_type_id][type].put("Layout",(void*)&p,string_id);
            return p;
        }
        Ptr add_template(uint32_t for_type) {
            Ptr p = add_layout_to_col(for_type);
            return p;
        }
        
        void recycle_column(Ptr p) {
            if(is_live(p)) {
                Acorn::recycle_column(resolve_to_pool(p), p.idx);
            }
            CHECK_ERROR("Error while recycling ",Ptr_to_string(p,p.cachelevel),is_live(p)?"":"[X]");
        }
        

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
                if(ptr.pool>=resolve_to_subunit(ptr).length()||ptr.idx>=resolve_to_subunit(ptr)[ptr.pool].length()) {
                    return "STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx);
                }
                std::string content = string(ptr).to_std();
                return Ptr_as_string(ptr)+"> \""+escape_string(content,true)+"\"";
            } else if(is_ptr_alias(tag)||tag==function_id) {
                return Ptr_as_string(*(Ptr*)data);
            } else if(tag==ptr4_id) {
                Ptr4 p = *(Ptr4*)data;
                std::string s = labels[p.midx]+"> "+Ptr_as_string(p.ptr);
                return s;
            } else {
                if(layouts.hasKey(tag)) {
                    std::string to_return = "";
                    _layout& l = layouts.get(tag);
                    for(uint32_t o=0;o<l.offsets.length();o++) {
                        std::string line = "";
                        line+=l.labels[o]+": ";
                        line +=tag_to_str(l.tags[o], (uint8_t*)data + l.offsets[o]);
                        line+=" | ";
                        to_return+=line;
                    }
                    return to_return;
                } else {
                    return "(add tag_to_str for "+labels[tag]+")";
                }
            }
        }
        
        std::string print_columnar_table(list<list<std::string>> lines) {
            //print("Printing columar with ",lines.length()," lines ");
            list<int> widths;
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

            if(longest_row>50) longest_row = 50; //Truncation for large fields

            std::string to_return = "";
            int lpadlen = digit_count(longest_row)+1;
            lpadlen = std::max((int)digit_count(lines.length())+1,lpadlen);
            for(int r=0;r<longest_row;r++) {
                if(r==1) {
                    for(int l=0;l<lines.length();l++) {
                        to_return+=std::string(widths[l]+lpadlen+3,'-')+"<|>";
                    }
                    to_return+="\n";
                }

                for(int l=0;l<lines.length();l++) {
                    std::string line = "";
                    //print("On line ",l," row ",r);
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
            //print("Finished printing columar table");
            return to_return;
        }

        list<std::string> heterogenous_col_to_lines(Col& col) {
            list<std::string> to_return;
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int i=0;i<col.length();i++) {
                        for(int o=0;o<l.offsets.length();o++) {
                            std::string line = "";
                            line+=pad_str(l.labels[o]+": ",12);
                            if(l.labels[o]=="type"||l.labels[o]=="sub_type"||l.labels[o]=="pass") {
                                line+=labels[*(uint32_t*)col.qget(l.offsets[o]+(l.total_size*i))];
                            } else {
                                line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]+(l.total_size*i)));
                            }
                            to_return<<line;
                        }
                    }
                } else {
                    print(red("core::heterogenous_col_to_lines unable to convert heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                }
            }
            return to_return;
        }
        std::string heterogenous_col_to_string(Col& col) {
            std::string to_return = "";
            list<std::string> lines = heterogenous_col_to_lines(col);
            for(int i=0;i<lines.length();i++) {
                to_return+=lines[i]+(i==lines.length()-1?"":"\n");
            }
            return to_return;
        }

        list<list<std::string>> type_to_lines(ColCol& t) {
            list<list<std::string>> lines;
            list<uint32_t> dtypes;
            for(int c=0;c<t.length();c++) {
                Col& col = t[c];
                list<std::string> subline;
                subline << (col.label.empty()?"":col.label.to_std()+" ")+(labels[col.tag]+":"+std::to_string(col.element_size)+" G:"+std::to_string(col.gen))+(t.free.has(c)?" [FREE]":"");
                //print("Pushed label ",subline[0]);
                if(col.heterogenous) {
                    subline << heterogenous_col_to_lines(col);
                } else {
                    for(int r=0;r<col.length();r++) {
                        std::string line = "";
                        //print("Line ",lines.length()," Subline ",subline.length());
                        //print("Row ",r," Column ",c," Tag ",labels[col.tag],"(",col.tag,")");
                        CCol* cell = col.cells.find_cell(r);
                        if(cell) {
                            if(cell->tag==string_id) {
                                line += "["+((QString&)*cell).to_std()+"] ";
                            } else {
                                line += "["+tag_to_str(cell->tag,cell->storage)+"] ";
                            }
                        }
                        line += tag_to_str(col.tag,col[r]);
                        //print("Result: ",line);
                        subline << line;
                    }
                }
                lines << subline;
                //print("Pushed ",subline.length()," sublines");

            }
            //print("Returned ",lines.length()," lines");
            return lines;
        }

        std::string type_to_string(ColCol& t) {
            return print_columnar_table(type_to_lines(t));
        }

        std::string column_to_string(Col& col) {
            std::string to_return = "COL: "+col.label.to_std()+" TAG: "+labels[col.tag]+" ["+std::to_string(col.length())+"]\n";
            if(col.heterogenous) {
                to_return+=heterogenous_col_to_string(col);
            } else {
                for(int i=0;i<col.length();i++) {
                    std::string line = "";
                    CCol* cell = col.cells.find_cell(i);
                    if(cell) {
                        if(cell->tag==string_id) {
                            line += "["+((QString&)*cell).to_std()+"] ";
                        } else {
                            line += "["+tag_to_str(cell->tag,cell->storage)+"] ";
                        }
                    }
                    line += tag_to_str(col.tag,col[i]);
                    to_return+=std::to_string(i)+": "+line;
                    if(i<col.length()-1) to_return+="\n";
                }
            }
            return to_return;
        }

        void print_column(Col& col) {
            print(column_to_string(col));
        }

        std::string col_info(Col& col) {
            std::string to_return = "";
            uint64_t size = get_real_real_size_of_col(col);
            uint64_t capacity = get_real_capacity_of_col(col);
            to_return += "Mem: " + fmem(capacity);
            if(capacity > 0) {
                double pct = (double)size * 100.0 / (double)capacity;
                std::string pct_str = (size > 0 && pct < 1.0) ? "<1%" : std::to_string((int)(pct + 0.5)) + "%";
                to_return += " ("+pct_str+" used ["+fmem(size)+"])";
            }
            return to_return;
        }

        std::string pool_info(ColCol& pool) {
            uint32_t plen = pool.length();
            std::string to_return = "";
            if(!pool.label.empty()) {
                to_return+=pool.label.to_std();
            } else {
                to_return+="Unlabled pool";
            }
            if(pool.tag!=0) {
                to_return+="["+labels[pool.tag]+"]";
            }

            to_return+=" : "+col_info(pool)+", Len: " + add_commas(plen);
            return to_return;
        }
        std::string pool_info(uint32_t poolid) {
            return pool_info(types[poolid]);
        }

        std::string subunit_info(ColColCol& sub) {
            std::string to_return = col_info(sub)+"\n---------------\n";
            for(int i=0;i<sub.length();i++) {
                to_return+=pool_info(sub[i])+"\n";
            }
            return to_return;
        }
        std::string unit_info() {
            std::string to_return = "Unit "+std::to_string(uid)+" : "+col_info(subunits)+"\n===============\n";
            for(uint32_t i=0;i<subunits.length();i++) {
                to_return+="---------------\n";
                to_return+=subunits[i].label.empty()?"Subunit "+std::to_string(i):subunits[i].label.to_std();
                to_return+=" : ";
                to_return+=subunit_info(subunits[i]);
            }
            return to_return;
        }

        std::string format_col_label(ColCol& parent, uint32_t idx, Col& col, const std::string& fallback_label) {
            CCol* cell = parent.cells.find_cell(idx);
            std::string label = col.label.to_std();
            if(cell) {
                std::string key = (*(QString*)cell).to_std();
                if(key==label) {
                    label="[:] "+label;
                } else {
                    label=key+": "+label;
                }
            }
            if(col.tag>0) {
                label+=" ["+labels[col.tag]+"]";
            }

            if(label.empty()) {
                label = fallback_label+" "+std::to_string(idx);
            }
            if(col.live>0) {
                label+=" [LOCK:"+std::to_string((uint8_t)col.live.load())+"]";
            }
            return label;
        }

        void dump_pool(ColCol& pool, uint32_t index, bool clear_dump, std::string path = "printout.txt", std::string label = "") {
            if(clear_dump) writeFile(path,"");
            std::string to_print = "";
            if(label.empty()) {
                to_print += "TYPE "+std::to_string(index)+" "+pool.label.to_std()+(pool.tag!=0?" ["+labels[pool.tag]+"]":"")+":\n";
            } else {
                to_print+=label+"\n";
            }
            to_print += type_to_string(pool);
            to_print += "\n\n\n";
            editTextFile(path,[to_print](std::string& source){
                source+=to_print;
            });
        }

        void dump_subunit(ColColCol& subunit, bool clear_dump, std::string path = "printout.txt") {
            if(clear_dump) writeFile(path,"");
            for(int t=0;t<subunit.length();t++) {
                dump_pool(subunit[t],t,false,path,format_col_label((ColCol&)subunit,t,subunit[t],"POOL"));
            }
        }

        void dump_unit(bool clear_dump, std::string path = "printout.txt", uint32_t from = 0, uint32_t to = 0) {
            if(clear_dump) writeFile(path,"");

            if(!unit_label.empty()) {
                editTextFile(path,[this](std::string& source){
                    source+="UNIT: "+unit_label+"\n\n";
                });
            }
            for(int s=0;s<subunits.length();s++) {
                editTextFile(path,[this,s](std::string& source){
                    source+=format_col_label((ColCol&)subunits,s,*get_subunit(s),"SUBUNIT")+":\n\n";
                });
                dump_subunit(*get_subunit(s),false,path);
            }
            // for(int t=from;t<(to==0?types.length():to);t++) {
            //     std::string to_print = "";
            //     to_print += "TYPE "+std::to_string(t)+" "+types[t].label.to_std()+(types[t].tag!=0?" ["+labels[types[t].tag]+"]":"")+":\n";
            //     to_print += type_to_string(types[t]);
            //     to_print += "\n\n\n";
            //     editTextFile(path,[to_print](std::string& source){
            //         source+=to_print;
            //     });
            // }
            // editTextFile("printout.txt",[](std::string& source){
            //     source+="SENDUNIT:\n";
            // });
            // for(int p=0;p<sendunit.length();p++) {
            //     dump_pool(sendunit[p],p,false);
            // }
            // editTextFile("printout.txt",[](std::string& source){
            //     source+="RECVUNIT:\n";
            // });
            // for(int p=0;p<recvunit.length();p++) {
            //     dump_pool(recvunit[p],p,false);
            // }
        }


        map<uint32_t,bool> init_ptr_aliases() {
            map<uint32_t,bool> to_return;
            to_return.put(ptr_id,true); to_return.put(string_id,true); 
            to_return.put(node_id,true); to_return.put(value_id,true); to_return.put(context_id,true);
            to_return.put(col_id,true); to_return.put(colcol_id,true); to_return.put(colcolcol_id,true);
            to_return.put(header_id,true);
            return to_return;
        }
        map<uint32_t,bool> ptr_alias_lookup = init_ptr_aliases();
        inline void register_ptr_alias(uint32_t type) {ptr_alias_lookup.put(type, true);}
        inline bool is_ptr_alias(uint32_t type) {return ptr_alias_lookup.getOrDefault(type, false);}
    
        inline list<Col*> ColCol_to_group(ColCol& col) {list<Col*> grouping; for(int c=0;c<col.length();c++){grouping << &col[c];} return grouping;}
        inline list<ColCol*> ColColCol_to_group(ColColCol& col) {list<ColCol*> grouping; for(int c=0;c<col.length();c++){grouping << &col[c];} return grouping;}
    

        inline void adopt_ptrs(Col& col, ColColCol* into = nullptr) {
            if(!into) into = &types;

            if(col.heterogenous) {
                if(!layouts.hasKey(col.tag)) return;
                _layout& l = layouts.get(col.tag);
                for(uint32_t row = 0; row < col.length(); row++) {
                    for(uint32_t f = 0; f < l.offsets.length(); f++) {
                        if(!is_ptr_alias(l.tags[f])) continue;
                        Ptr& ptr = *(Ptr*)col.qget(row * l.total_size + l.offsets[f]);
                        if(is_live(ptr)) {
                            if(ptr.cachelevel==3) ptr.cache = into;
                            else if(ptr.cachelevel==0) ptr.unit = uid;
                        }
                    }
                }
            } else if(is_ptr_alias(col.tag)) {
                for(int r=0;r<col.length();r++) {
                    Ptr& ptr = *(Ptr*)col[r];
                    if(is_live(ptr)) {
                        if(ptr.cachelevel==3) {
                            ptr.cache=into;
                        } else if(ptr.cachelevel==0) {
                            ptr.unit = uid;
                        } else {
                            print(red("core:adopt_ptrs unable to adopt ptr "+Ptr_to_string(ptr,ptr.cachelevel)+" because it's cachelevel was too low or high"));
                        }
                    }
                }
            }
        }
        inline void adopt_ptrs(ColCol& pool, ColColCol* into = nullptr) {
            for(int i=0;i<pool.length();i++) {
                adopt_ptrs(pool[i],into);
            }
        }   
        inline void adopt_ptrs(ColColCol& col3, ColColCol* into = nullptr) {
            for(int i=0;i<col3.length();i++) {
                adopt_ptrs(col3[i],into);
            }
        }   

        inline void opperate_on_ptrs(Col& col,std::function<void(Ptr&)> opp) {
            if(col.heterogenous) {
                if(!layouts.hasKey(col.tag)) return;
                _layout& l = layouts.get(col.tag);
                for(uint32_t row = 0; row < col.length(); row++) {
                    for(uint32_t f = 0; f < l.offsets.length(); f++) {
                        if(!is_ptr_alias(l.tags[f])) continue;
                        Ptr& ptr = *(Ptr*)col.qget(row * l.total_size + l.offsets[f]);
                        opp(ptr);
                    }
                }
            } else if(is_ptr_alias(col.tag)) {
                for(int r=0;r<col.length();r++) {
                    Ptr& ptr = *(Ptr*)col[r];
                    opp(ptr);
                }
            }
        }

        inline void offset_field_ptrs(Col& col, int offset, uint32_t field, uint32_t greater_than_threshold = 0) {
            if(col.heterogenous) {
                if(!layouts.hasKey(col.tag)) return;
                _layout& l = layouts.get(col.tag);
                for(uint32_t row = 0; row < col.length(); row++) {
                    for(uint32_t f = 0; f < l.offsets.length(); f++) {
                        if(!is_ptr_alias(l.tags[f])) continue;
                        Ptr& ptr = *(Ptr*)col.qget(row * l.total_size + l.offsets[f]);
                        if(is_live(ptr)) {
                            uint32_t val = ptr[field];
                            if(offset < 0 && val >= greater_than_threshold && val < greater_than_threshold+(uint32_t)(-offset)) {
                                ptr = deadptr;
                            } else if(val >= greater_than_threshold) {
                                ptr[field] += offset;
                            }
                        }
                    }
                }
            } else if(is_ptr_alias(col.tag)) {
                for(int r=0;r<col.length();r++) {
                    Ptr& ptr = *(Ptr*)col[r];
                    if(is_live(ptr)) {
                        uint32_t val = ptr[field];
                        if(offset < 0 && val >= greater_than_threshold && val < greater_than_threshold+(uint32_t)(-offset)) {
                            ptr = deadptr;
                        } else if(val >= greater_than_threshold) {
                            ptr[field] += offset;
                        }
                    }
                }
            }
        }
    
        void offset_sidx_ptrs(Col& col, int offset, uint32_t greater_than_threshold = 0) {offset_field_ptrs(col,offset,1,greater_than_threshold);}
    
        void offset_idx_ptrs(list<Col*> cols, int offset, uint32_t greater_than_threshold = 0) {
            for(int c=0;c<cols.length();c++) {
                Col& col = *cols[c];
                offset_field_ptrs(col,offset,2,greater_than_threshold);
            }
        }
        void offset_idx_ptrs(ColCol& cols, int offset, uint32_t greater_than_threshold = 0) {offset_idx_ptrs(ColCol_to_group(cols),offset,greater_than_threshold);}
        
        void offset_pool_ptrs(list<ColCol*> pools, int offset, uint32_t greater_than_threshold = 0) {
            for(int p=0;p<pools.length();p++) {
                for(int c=0;c<pools[p]->length();c++) {
                    Col& col = pools[p]->get(c);
                    offset_field_ptrs(col,offset,3,greater_than_threshold);
                }
            }
        }
        void offset_pool_ptrs(ColColCol& cols, int offset, uint32_t greater_than_threshold = 0) {offset_pool_ptrs(ColColCol_to_group(cols),offset,greater_than_threshold);}
    
        void offset_subunit_ptrs(list<ColCol*> pools, int offset, uint32_t greater_than_threshold = 0) {
            for(int p=0;p<pools.length();p++) {
                for(int c=0;c<pools[p]->length();c++) {
                    Col& col = pools[p]->get(c);
                    offset_field_ptrs(col,offset,4,greater_than_threshold);
                }
            }
        }
        void offset_subunit_ptrs(ColColCol& cols, int offset, uint32_t greater_than_threshold = 0) {offset_subunit_ptrs(ColColCol_to_group(cols),offset,greater_than_threshold);}

        QCol take_elements(Col& col, uint32_t from, uint32_t to) {
            QCol to_return = col.take_range(from,to);
            offset_sidx_ptrs(col, -(int)(to - from), from);
            return to_return;
        }
        uint32_t find_pools_start(ColColCol& col3, uint32_t from, uint32_t start_tag) {
            if(from==0) return 0;
            while(col3[from].tag != start_tag) {
                if(from == 0) {
                    throw_error("core:find_pools_start unable to find the starting tag "+labels[start_tag]);
                    return 0;
                }
                from -= 1;
            }
            return from;
        }
        uint32_t find_pools_start(uint32_t from, uint32_t start_tag) {
            return find_pools_start(types,from,start_tag);
        }
        list<ColCol*> gather_pools_from(ColColCol& col3, uint32_t from, uint32_t start_tag, uint32_t end_tag) {
            list<ColCol*> to_return;
            if(from>=col3.length()) {print(red("core:gather_pools_from from "+std::to_string(from)+" out of bounds for col3 length "+std::to_string(col3.length()))); return to_return;}
            from = find_pools_start(from,start_tag);
            for(int p=from;p<col3.length();p++) {
                to_return << &col3[p];
                if(col3[p].tag==end_tag) {
                    break;
                }
            }
            return to_return;
        }
        list<ColCol*> gather_pools_from(uint32_t from, uint32_t start_tag, uint32_t end_tag) {
            return gather_pools_from(types,from,start_tag,end_tag);
        }

        list<ColCol*> gather_pools(ColColCol& col3, uint32_t from, uint32_t to) {
            list<ColCol*> to_return;
            if(to>col3.length()) {print(red("core:gather_pools to "+std::to_string(to)+" out of bounds for col3 length "+std::to_string(col3.length()))); return to_return;}
            for(int p=from;p<to;p++) {
                to_return << &col3[p];
            }
            return to_return;
        }
        list<ColCol*> gather_pools(uint32_t from, uint32_t to) {
            return gather_pools(types,from,to);
        }

        bool has_pool(list<ColCol*> pools, uint32_t tag, uint32_t nth = 0) {
            for(int i=0;i<pools.length();i++) {
                if(pools[i]->tag==tag) {
                    if(nth==0) {
                        return true;
                    } else nth-=1;
                }
            }
            return false;
        }
        uint32_t find_poolidx(list<ColCol*> pools, uint32_t tag, uint32_t nth = 0) {
            for(int i=0;i<pools.length();i++) {
                if(pools[i]->tag==tag) {
                    if(nth==0) {
                        return i;
                    } else nth-=1;
                }
            }
            print(red("core:find_poolidx could not find pool "+labels[tag]));
            return 0;
        }
        ColCol* find_pool(list<ColCol*> pools, uint32_t tag, uint32_t nth = 0) {
            uint32_t index = find_poolidx(pools,tag,nth);
            return pools[index];
        }
        ColCol* find_pool(ColColCol& pools, uint32_t tag, uint32_t nth = 0) {
            return find_pool(ColColCol_to_group(pools),tag,nth);
        }

        int getpool(ColColCol& group, uint32_t tag, uint32_t nth = 0) {
            for(int i=0;i<group.length();i++) {
                if(group[i].tag==tag) {
                    if(nth==0) {
                        return i;
                    } else nth-=1;
                }
            }
            return -1;
        }


        enum class SnapField : uint8_t {
            //QCol fields
            Size = 1, Data = 2,
            //CCol fields  
            Esize = 3, Tag = 4, Hash = 5, Index = 6, Cachelevel = 7, Live = 8, Gen = 9,
            //Col fields
            Hetero = 10, Label = 11, Cells = 12, Free = 13,
            //Structural
            Cols = 14, Spec = 15, End = 255,
        };

        template<typename T>
        inline void snapshot_field(std::ostream& out, SnapField field, T val) {
            write_raw<uint8_t>(out, (uint8_t)field); write_raw<uint32_t>(out, sizeof(T)); write_raw<T>(out, val);
        }
        inline void snapshot_end(std::ostream& out) {
            write_raw<uint8_t>(out, (uint8_t)SnapField::End); write_raw<uint32_t>(out, 0);
        }
        inline void snapshot_string(std::ostream& out, SnapField field, const std::string& s) {
            write_raw<uint8_t>(out, (uint8_t)field); write_raw<uint32_t>(out, s.size()); out.write(s.data(), s.size());
        }


        void snapshot_ccol(std::ostream& out, CCol& col) {
            snapshot_string(out, SnapField::Tag, labels[col.tag]);
            snapshot_field<uint32_t>(out, SnapField::Hash, col.hash);
            snapshot_field<uint32_t>(out, SnapField::Index, col.index);
            snapshot_field<uint32_t>(out, SnapField::Size, col.size);
            if(col.storage && col.size > 0) {
                write_raw<uint8_t>(out, (uint8_t)SnapField::Data);
                write_raw<uint32_t>(out, col.size);
                out.write((const char*)col.storage, col.size);
            }
            snapshot_end(out);
        }
        void load_snapshot_ccol(std::istream& in, CCol& col) {
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Tag: {
                        std::string s(len, '\0');
                        in.read(s.data(), len);
                        uint32_t zero = 0;
                        col.tag = labels_lookup.getOrDefault(s, zero);
                        break;
                    }
                    case SnapField::Hash:  col.hash  = read_raw<uint32_t>(in); break;
                    case SnapField::Index: col.index = read_raw<uint32_t>(in); break;
                    case SnapField::Size:  col.resize(read_raw<uint32_t>(in)); break;
                    case SnapField::Data:  in.read((char*)col.storage, len); col.size = len; break;
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
        }
        
        void snapshot_qcellcol(std::ostream& out, QCellCol& cells) {
            uint32_t count = 0;
            list<CCol*> to_save;
            for(uint32_t i = 0; i < cells.length(); i++) {
                if(cells.get(i).storage) {
                    count++;
                    to_save << &cells.get(i);
                }
            }
            write_raw<uint8_t>(out, (uint8_t)SnapField::Cols);
            write_raw<uint32_t>(out, count);
            for(uint32_t i = 0; i < to_save.length(); i++) snapshot_ccol(out, *to_save[i]);
            snapshot_end(out);
        }
        void load_snapshot_qcellcol(std::istream& in, QCellCol& cells) {
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Cols: {
                        uint32_t count = len;
                        for(uint32_t i = 0; i < count; i++) {
                            CCol c;
                            load_snapshot_ccol(in, c);
                            c.hash = hashBytes(c.storage, c.size);
                            cells.scan_for_slot(std::move(c));
                        }
                        break;
                    }
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
        }
        

        void snapshot_qcol(std::ostream& out, QCol& col, bool include_data) {
            snapshot_field<uint32_t>(out, SnapField::Size, col.size);
            if(include_data && col.storage) {
                write_raw<uint8_t>(out, (uint8_t)SnapField::Data);
                write_raw<uint32_t>(out, col.size);
                out.write((const char*)col.storage, col.size);
            }
            snapshot_end(out);
        }
        void load_snapshot_qcol(std::istream& in, QCol& col, bool include_data) {
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Size: col.resize(read_raw<uint32_t>(in)); break;
                    case SnapField::Data: if(include_data) {in.read((char*)col.storage, len);} else {in.seekg(len, std::ios::cur);} break;
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
        }

        void snapshot_tcol(std::ostream& out, TCol& col) {
            snapshot_field<uint32_t>(out, SnapField::Esize, col.element_size);
            snapshot_string(out, SnapField::Tag, labels[col.tag]);
            snapshot_field<uint32_t>(out, SnapField::Hash, col.hash);
            snapshot_field<uint32_t>(out, SnapField::Index, col.index);
            snapshot_field<uint16_t>(out, SnapField::Gen, col.gen);
            snapshot_field<uint8_t>(out, SnapField::Spec, (uint8_t)col.specialization);
            snapshot_end(out);

            bool should_include_data = true;
            if(col.specialization==Spec::COL||col.specialization==Spec::PTR_COL) {should_include_data = false;}
            snapshot_qcol(out, col, should_include_data);
        }
        void load_snapshot_tcol(std::istream& in, TCol& col) {
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Esize: col.element_size = read_raw<uint32_t>(in); break;
                    case SnapField::Tag: {
                        std::string s(len, '\0');
                        in.read(s.data(), len);
                        uint32_t zero = 0;
                        col.tag = labels_lookup.getOrDefault(s, zero);
                        break;
                    }
                    case SnapField::Hash:  col.hash  = read_raw<uint32_t>(in); break;
                    case SnapField::Index: col.index = read_raw<uint32_t>(in); break;
                    case SnapField::Gen:   col.gen   = read_raw<uint16_t>(in); break;
                    case SnapField::Spec:  col.specialization = (Spec)read_raw<uint8_t>(in); break;
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            bool should_include_data = true;
            if(col.specialization==Spec::COL||col.specialization==Spec::PTR_COL) {should_include_data = false;}
            load_snapshot_qcol(in, col, should_include_data);
        }
        
        void snapshot_col(std::ostream& out, Col& col) {
            snapshot_qcellcol(out, col.cells);
            snapshot_tcol(out, col);
            snapshot_field<bool>(out, SnapField::Hetero, col.heterogenous);
            snapshot_string(out, SnapField::Label, col.label.to_std());
            write_raw<uint8_t>(out, (uint8_t)SnapField::Free);
            write_raw<uint32_t>(out, col.free.length() * 4);
            for(uint32_t i = 0; i < col.free.length(); i++) write_raw<uint32_t>(out, col.free[i]);
            if(col.specialization == Spec::COL) {
                write_raw<uint8_t>(out, (uint8_t)SnapField::Cols);
                write_raw<uint32_t>(out, col.length());
                for(uint32_t i = 0; i < col.length(); i++) snapshot_col(out, *(Col*)col[i]);
            }
            snapshot_end(out);
        }
        void load_snapshot_col(std::istream& in, Col& col) {
            load_snapshot_qcellcol(in,col.cells);
            load_snapshot_tcol(in, col);
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Hetero: col.heterogenous = read_raw<bool>(in); break;
                    case SnapField::Label: {
                        std::string s(len, '\0');
                        in.read(s.data(), len);
                        col.label = s;
                        break;
                    }
                    case SnapField::Free: {
                        uint32_t count = len / 4;
                        for(uint32_t i = 0; i < count; i++) col.free << read_raw<uint32_t>(in);
                        break;
                    }
                    case SnapField::Cols: {
                        uint32_t count = len;
                        col.size = 0;
                        for(uint32_t i = 0; i < count; i++) {
                            load_snapshot_col(in, col.add());
                        }
                        break;
                    }
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
        }


        void save_subunit(ColColCol* subunit, bool snapshot = true, std::string otherpath = "") {
            auto out = openWriteStream(otherpath.empty()?subunit->label.to_std():otherpath);
            if(snapshot) {
                snapshot_col(out, *subunit);
            } else {
                write_col(out,*subunit);
            }
            out.close();
        }

        void load_subunit(ColColCol* subunit, bool overwrite_cache = false) {
            if(!subunit->empty()&&!overwrite_cache) return;
            if(subunit->try_lock_forever()) {
                if(!subunit->empty()&&!overwrite_cache) {subunit->unlock(); return;}
                if(subunit->label.empty()){
                    subunit->unlock();
                    throw_error("unit:load_subunit could not load subunit: ",subunit->label.to_std(),"  because it's label was not a valid filepath");
                    return;
                }
                std::string path = subunit->label.to_std();
                std::ifstream in;
                try {
                    in = openReadStream(path);
                } catch(std::exception& e) {
                    subunit->unlock();
                    throw_error("unit:load_subunit path not found: ", path, ": ", e.what());
                    return;
                }
                load_snapshot_col(in,*subunit);
                adopt_ptrs(*subunit,subunit);
                in.close();
                subunit->unlock();
            }
        }

        Ptr load_subunit(const std::string& path, bool overwrite_cache = false) {
            ColColCol* subunit = nullptr;
            uint32_t at = 0;
            std::ifstream in;
            try {
                in = openReadStream(path);
            } catch(std::exception& e) {
                throw_error("unit:load_subunit path not found: ", path, ": ", e.what());
                return deadptr;
            }
            if(!subunits.hasKey(path)) {
                at = subunits.add_idx();
                subunit = &subunits[at];
                load_snapshot_col(in,*subunit);
                subunits.addcell(at,path.data(),path.size(),string_id);
                adopt_ptrs(*subunit,subunit);
                subunit->unlock();
            } else {
                at = subunits.getidx(path.data(),path.size());
                subunit = &subunits[at];
                if((overwrite_cache||subunit->empty())&&subunit->try_lock_forever()) {
                    if((overwrite_cache||subunit->empty())) {
                        load_snapshot_col(in,*subunit);
                        adopt_ptrs(*subunit,subunit);
                    }
                    subunit->unlock();
                }
            }
            in.close();
            Ptr p((void*)subunit,0,0,0);
            p.unit = uid, p.subunit = at;
            return p;
        }

        bool acquire_subunit(ColColCol* subunit) {
            load_subunit(subunit);
            return subunit->try_lock_forever();
        }
        bool read_acquire_subunit(ColColCol* subunit) {
            load_subunit(subunit);
            return subunit->try_read_forever();
        }

        void bounce_subunit(ColColCol* subunit) {
            if(subunit->label.empty()){
                throw_error("unit:bounce_subunit could not bounce subunit: ",subunit->label.to_std(),"  because it's label was not a valid filepath");
                return;
            }
            if(subunit->try_lock_forever()) {
                save_subunit(subunit);
                for(uint32_t i = 0; i < subunit->length(); i++) {
                    subunit->get(i).~ColCol();
                }
                subunit->clear();
                subunit->gen++;
                subunit->unlock();
            }
        }

        uint32_t indexof_subunit(uint8_t* subunit_storage) {
            for(int i=0;i<subunits.length();i++) {
                if(subunits[i].storage==subunit_storage) {
                    return i;
                }
            }
            return 0;
        }
        void dump_text(std::string text) {
            editTextFile("printout.txt",[text](std::string& source){source+=text;});
        }

    
        inline uint32_t add_column(Col& col, size_t size = 0, uint32_t tag = 0) {
            Col ncol(size);
            ncol.tag = tag;
            col.push((void*)&ncol);
            return col.length()-1;
        }
    

        void test_unit() {
            print(size_of(int_id));
            print(size_of(string_id));

           
        }
    };

    template<typename T>
    inline g_ptr<T> make_unit() {
        g_ptr<T> u = make<T>();
        return u;
    }

    template<typename T>
    inline g_ptr<T> make_unit(const ColColCol& starter) {
        g_ptr<Unit> u = make<Unit>(starter);
        return u;
    }
    
    inline PtrColColCol& resolve_to_unit(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit]).subunits;}
            case 4: return *(PtrColColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to unit because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return pcol3_ref;
        }
    }
   
    inline ColColCol& resolve_to_subunit(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 4: {
                Col& unit = resolve_to_unit(ptr); 
                DEBUG_ONLY(if(ERROR_FLAG) {return col3_ref;});
                //CHECK_ERROR_VAL(col3_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to subunit because it failed to resolve to a subunit"); 
                ColColCol& col =  *(*(ColColCol**)unit[ptr.subunit]);
                DEBUG_ONLY(if(ERROR_FLAG) {return col3_ref;});
                //CHECK_ERROR_VAL(col3_ref,"Could not resolve Ptr ",Ptr_to_string(ptr,ptr.cachelevel)," to subunit because it was out of bounds");
                return col;
            }
            case 3: return *(ColColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to subunit because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col3_ref;
        }
    }

    inline Ptr get_ticket_from_unit(Ptr p, uint32_t type_id, uint32_t size, uint32_t tag) {
        if(p.cachelevel==3) {
            Ptr ticket(p.cache,type_id,create_column(resolve_to_subunit(p)[type_id],size,tag,true),0);
            ticket.gen = resolve_to_col(ticket).gen;
            return ticket;
        } else if(p.cachelevel==0) {
            std::lock_guard<std::mutex> lock(units_mutex);
            return (*units[p.unit]).get_ticket(type_id,size,tag);
        }
        return deadptr;
    }
    inline uint32_t find_pool_tag_in_unit(Ptr p, uint32_t tag) {
        if(p.cachelevel==3) {
            ColColCol& sub  = resolve_to_subunit(p);
            for(int i=0;i<sub.length();i++) {
                if(sub[i].tag==tag) return i;
            }
        }
        return 0;
    }
    


    inline uint32_t size_of_from_unit(Ptr p, uint32_t tag) {
        if(p.cachelevel==3) {
            return size_of_from_subunit(*(ColColCol*)p.cache,tag);
        } else if(p.cachelevel==0) {
            std::lock_guard<std::mutex> lock(units_mutex);
            return size_of_from_subunit(units[p.unit]->types, tag);
        } else {
            throw_error("global:size_of_from_unit Ptr cachelevel  "+std::to_string((int)p.cachelevel)+" is not valid for this operation");
            return 0;
        }
    }

    inline std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }

    
    inline ColColCol& init_first_unit() {
        g_ptr<Unit> u = make<Unit>(false);
        units << u;
        return *u->get_subunit(0);
    }
}