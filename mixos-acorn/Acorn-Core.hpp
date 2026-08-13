#pragma once

#include <thread>
#include <mutex>
#include <filesystem>
#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"
#include "../GDSL/ext/g_lib/core/thread.hpp"

#define NAMED_PTRS 0

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;
    struct Context;
    struct Value;

    struct ColCol : Col {
        ColCol() : Col(sizeof(Col)) {}
        ColCol(Col c) : Col(c) {}
        ColCol(const ColCol& o) : Col(sizeof(Col)) {
            element_size = o.element_size;
            tag = o.tag;
            gen = o.gen;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
            cells = o.cells;
            free = o.free;
            for(uint32_t i = 0; i < o.length(); i++) {
                Col copy(*(Col*)o.sget(i));
                push(copy);
            }
        }
        ColCol& operator=(ColCol&& o) {
            if(this == &o) return *this;
            if(storage && element_size != 0) {
                for(uint32_t i = 0; i < length(); i++) get(i).~Col();
            }
            Col::operator=(std::move(o));
            return *this;
        }
        ~ColCol() {
            if(!storage || element_size == 0) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~Col();
            }
        }

        // void clear() {
        //     for(uint32_t i = 0; i < length(); i++) {get(i).~Col();}
        //     Col::clear();
        //     ::free(storage);
        //     capacity = 4;
        //     storage = new uint8_t[capacity*sizeof(Col)];
        // }

        Col& get(uint32_t idx) {return *(Col*)Col::sget(idx);}
        void set(uint32_t idx, Col val) {
            get(idx).~Col();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.nullstorage();
        }
        Col& operator[](uint32_t idx) {return *(Col*)Col::sget(idx);}
        void push(Col t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.nullstorage();
        }
        void put(const std::string& key, Col t) {
            Col::put(key,(void*)&t); //This should probably have tag string id for display later, may require reordering how we register the ids
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.nullstorage();
        }
    };

     //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
     uint32_t create_column(ColCol& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling&&!col.free.empty()) {
            //Lazy version that doesn't care about tags or sizes
            uint32_t idx = col.free.pop();
            Col& ncol = col[idx];
            ncol.clear(); ncol.element_size = size; ncol.tag = tag;  
            ncol.gen++; //Gen only goes up to 65,535 it'll wraparound after that
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
        add_column(col,size,tag);
        return col.length()-1;
    }
    //Creates a column from pool and intilizes it's memory if empty
    uint32_t push_column(ColCol& col, uint32_t size, uint32_t tag) {
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
        col.free.push(id);
       } else {
        throw_error("core:recycle_column unable to recycle ",id);
       }
    }
    
    struct ColColCol : Col {
        ColColCol() : Col(sizeof(ColCol)) {}
        ColColCol(Col c) : Col(c) {}
        ColColCol(const ColColCol& o) : Col(sizeof(ColCol)) {
            element_size = o.element_size;
            tag = o.tag;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
            cells = o.cells;
            for(uint32_t i = 0; i < o.length(); i++) {
                ColCol copy(*(ColCol*)o.sget(i));
                push(copy);
            }
        }
        ColColCol& operator=(ColColCol&& o) {
            if(this == &o) return *this;
            if(storage && element_size != 0) {
                for(uint32_t i = 0; i < length(); i++) get(i).~ColCol();
            }
            Col::operator=(std::move(o));
            return *this;
        }
        ~ColColCol() {
            if(!storage || element_size == 0) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~ColCol();
            }
        }

        // void clear() {
        //     for(uint32_t i = 0; i < length(); i++) {get(i).~ColCol();}
        //     Col::clear();
        //     ::free(storage);
        //     capacity = 4;
        //     storage = new uint8_t[capacity*sizeof(ColCol)];
        // }

        ColCol& get(uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void set(uint32_t idx, ColCol val) {
            get(idx).~ColCol();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.nullstorage();
        }
        ColCol& operator[](uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void push(ColCol t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.nullstorage();
        }
        void insert(uint32_t index, ColCol t) {
            CCol::insert(index,(void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.nullstorage();
        }
    };

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

    std::string Ptr_to_string(Ptr p, int print_level = 3) {
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
    std::string capture_ptr(Ptr p) {
        return std::to_string((uint64_t)p.cache)+"."+std::to_string(p.gen)+"."+std::to_string(p.specialization)+"."+Ptr_to_string(p,p.cachelevel);
    }
    Ptr decode_ptr_string(const std::string& s) {
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
    Ptr string_to_Ptr(const std::string& s) {
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
    uint8_t string_to_cachelevel(const std::string& s) {
        uint8_t to_return = 0;
        for(auto& c : s) if(c=='|') to_return++;
        if(to_return!=0) to_return+=1;
        return to_return;
    }

    list<g_ptr<Unit>> units;
    static std::mutex units_mutex;

    ColColCol& init_first_unit();

    ColColCol& global = init_first_unit();


    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2; uint32_t prefix_ptr_id = 3; uint32_t suffix_ptr_id = 4;
    uint32_t subunit_id = 5; uint32_t prefix_subunit_id = 6; uint32_t suffix_subunit_id = 7;

    struct PtrColColCol : Col {
        PtrColColCol() : Col(sizeof(void*),subunit_id) {}
        PtrColColCol(Col c) : Col(c) {}
        ColColCol* get(uint32_t idx) {return *(ColColCol**)Col::sget(idx);}
        ColColCol* operator[](uint32_t idx) {return PtrColColCol::get(idx);}
        ColColCol* create(std::string path = "") {
            if(!free.empty()) {
                uint32_t idx = free.pop();
                ColColCol* c3 = get(idx);
                c3->clear();
                c3->gen++;
                if(!path.empty()) {
                    c3->label = path;
                    addcell(idx,path.data(),path.size(),0); //<- Make this string id later when I want to wrestle with intilization order
                }
                return c3;
            } else {
                ColColCol* c3 = new ColColCol();
                if(path.empty()) {
                    push((void*)&c3);
                } else {
                    c3->label = path;
                    qput((void*)&c3,path.data(),path.size(),0);
                }
                return c3;
            }
        }
        void recycle(uint32_t idx) {
            get(idx)->unlock();
            // cells.removeAt(cells.get()); <- add this later
            free.push(idx);
        }
    };

    static ColColCol col3_ref;
    static PtrColColCol pcol3_ref;
    inline PtrColColCol& resolve_to_unit(const Ptr& ptr);
    inline ColColCol& resolve_to_subunit(const Ptr& ptr);
    static ColCol col2_ref;
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
    static Col col1_ref;
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

    struct string : Ptr {
        string() {}
        string(Ptr p) : Ptr(p) {}
        inline Col& col() {return resolve_to_col(*this); }
        inline char at(uint32_t idx) {return *(char*)col()[idx];}
        inline uint32_t length() {return col().length();}
        inline void push(char c) { col().push(&c); }
        inline void push(const char* s, uint32_t len) {for(uint32_t i = 0; i < len; i++) col().push(&s[i]);}
        inline void push(const std::string& s) { push(s.data(), s.length()); }
        inline void insert(uint32_t idx, char c) { col().insert(idx, &c); }
        inline void insert(uint32_t idx, const char* s, uint32_t len) { for(uint32_t i = 0; i < len; i++) col().insert(idx+i, &s[i]); }
        inline void insert(uint32_t idx, const std::string& s) { insert(idx, s.data(), s.length()); }
        inline void insert(uint32_t idx, string s) { insert(idx, (const char*)s.col().storage, s.length()); }
        inline void operator=(const std::string& s){ col().clear(); push(s);}
        inline void operator=(string s){ col().clear(); push((const char*)s.col().storage, s.length());}
        inline void operator=(const char* s) { col().clear(); push(s, strlen(s)); }
        inline char& operator[](uint32_t idx) { return *(char*)col().get(idx); }
        std::string to_std() {Col& c = col(); DEBUG_ONLY(if(ERROR_FLAG) {return ERROR_MSG;}); return std::string((char*)c.storage, length());}
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
    string get_global_string_ticket();

    struct type_and_value {
        uint32_t type;
        Ptr value;
    };

    uint32_t offsets_col = 0;
    uint32_t tags_col = offsets_col + 1;
    uint32_t sizes_col = tags_col + 1;
    uint32_t labels_col = sizes_col + 1;
    uint32_t subtags_col = labels_col + 1;
    uint32_t subsizes_col = subtags_col + 1;
    uint32_t ptrs_col = subsizes_col + 1;

    uint32_t overloads_col = ptrs_col + 1;

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


    uint32_t add_type() {
        uint32_t at = global.length();
        ColCol to_return;
        global.push(to_return);
        return at;
    }

    uint32_t init_handler_type() {
        ColCol t;
        uint32_t at = global.length();
        note_value(t,"UNDEFINED",0,0);
        t.get(0).push_default(); //UNDEFINED cell
        note_value(t,"stages",sizeof(Ptr),ptr_id);
        t.get(1).put("Layouts",(void*)&deadptr); //Layouts label
        note_value(t,"ptr",sizeof(Ptr),ptr_id); note_value(t,"prefix_ptr",sizeof(Ptr),ptr_id); note_value(t,"suffix_ptr",sizeof(Ptr),ptr_id);
        t.get(2).push_default(); t.get(3).push_default(); t.get(4).push_default();
        note_value(t,"subunit",sizeof(Ptr),ptr_id); note_value(t,"prefix_subunit",sizeof(Ptr),ptr_id); note_value(t,"suffix_subunit",sizeof(Ptr),ptr_id);
        t.get(5).push_default(); t.get(6).push_default(); t.get(7).push_default();
        global.push(t);
        return at;
    }

    uint32_t handler_type_id = init_handler_type();
    
    uint32_t init_layout_type() {
        ColCol t;
        uint32_t at = global.length();
        global.push(t);
        return at;
    }
    uint32_t layout_type_id = init_layout_type(); 

    uint32_t global_reg_id(const std::string& label) {
        uint32_t at = global[handler_type_id].length();
        note_value(global[handler_type_id],label,sizeof(Ptr),ptr_id);
        global[handler_type_id][at].push_default();
        return at;
    }

    uint32_t global_register_type_ids(const std::string& label) {
        uint32_t id = global_reg_id(label);
        global_reg_id("prefix_"+label);
        global_reg_id("suffix_"+label);
        return id;
    }
    //Qual handlers which act on the value
    size_t to_prefix_id(size_t id) {return id+1;}
    //Qual handlers which act on the node
    size_t to_suffix_id(size_t id) {return id+2;}

    uint32_t float_id  = global_register_type_ids("float");
    uint32_t int_id    = global_register_type_ids("int");
    uint32_t bool_id   = global_register_type_ids("bool");
    uint32_t string_id = global_register_type_ids("string");
    uint32_t char_id   = global_register_type_ids("char");
    uint32_t ptr4_id   = global_register_type_ids("ptr4");
    uint32_t duck_id   = global_register_type_ids("duck");
    uint32_t void_id   = global_register_type_ids("void");

    size_t list_id = global_reg_id("list");
    size_t map_id = global_reg_id("map");
    size_t weakptr_id = global_reg_id("weakptr");
    size_t col_id = global_reg_id("col");
    size_t colcol_id = global_reg_id("colcol");
    size_t colcolcol_id = global_reg_id("colcolcol");
    
    uint32_t silenced_id = global_reg_id("SILENCED");
    uint32_t any_id = global_reg_id("any");
    uint32_t null_id = global_reg_id("null");
    size_t identifier_id = global_reg_id("IDENTIFIER");
    size_t object_id = global_reg_id("OBJECT");
    size_t literal_id = global_reg_id("LITERAL");
    
    size_t node_id = global_reg_id("node"); size_t prefix_node_id = global_reg_id("prefix_node"); size_t suffix_node_id = global_reg_id("suffix_node");
    size_t value_id = global_reg_id("value"); size_t prefix_value_id = global_reg_id("prefix_value"); size_t suffix_value_id = global_reg_id("suffix_value");
    size_t context_id = global_reg_id("context"); size_t prefix_context_id = global_reg_id("prefix_context"); size_t suffix_context_id = global_reg_id("suffix_context");

    size_t var_decl_id = global_reg_id("VAR_DECL");
    size_t func_call_id = global_reg_id("FUNC_CALL");
    size_t lambda_id = global_reg_id("LAMBDA");
    size_t function_id = global_reg_id("function"); size_t prefix_function_id = global_reg_id("prefix_function"); size_t suffix_function_id = global_reg_id("suffix_function");
    size_t method_call_id = global_reg_id("METHOD_CALL");
    size_t method_id = global_reg_id("METHOD");
    size_t func_decl_id = global_reg_id("FUNC_DECL");
    size_t type_decl_id = global_reg_id("TYPE_DECL");
    uint32_t hide_block_id = global_reg_id("HIDE_BLOCK");

    uint32_t sub_pass_id = global_reg_id("SUB_PASS");
    uint32_t process_node_pass_id = global_reg_id("PROCESS_NODE");
    uint32_t direct_pass_id = global_reg_id("DIRECT_PASS");
    uint32_t resolving_pass_id = global_reg_id("RESOLVING_PASS");
    uint32_t travel_pass_id = global_reg_id("TRAVEL_PASS");
    uint32_t backwards_pass_id = global_reg_id("BACKWARDS_PASS");
    uint32_t memory_backwards_pass_id = global_reg_id("MEMORY_BACKWARDS_PASS");

    uint32_t headerpool_id = global_reg_id("headerpool");  uint32_t header_id = global_register_type_ids("header");
    uint32_t messagepool_id = global_reg_id("messagepool");
    uint32_t footerpool_id = global_reg_id("footerpool");
    
    

    size_t tombstone_col = 0; 
    size_t refs_col = 0;

    Ptr global_add_layout_to_col(uint32_t type) {
        Ptr p((uint32_t)0,layout_type_id,note_value(global[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0);
        note_value(global[layout_type_id],"Tags",4,int_id);
        note_value(global[layout_type_id],"Sizes",4,int_id);
        note_value(global[layout_type_id],"Labels",sizeof(Ptr),string_id);
        note_value(global[layout_type_id],"Subtags",4,int_id);
        note_value(global[layout_type_id],"Subsizes",4,int_id);
        note_value(global[layout_type_id],"Ptrs",sizeof(Ptr),ptr_id);
        note_value(global[layout_type_id],"Overloads",sizeof(Ptr4),ptr4_id);
        global[handler_type_id][type].set(0,(void*)&p);
        return p;
    }

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
    uint32_t context_pass_offset = 0;
    uint32_t context_parent_offset = 0;

    uint32_t node_total_size = 0;
    uint32_t value_total_size = 0;
    uint32_t context_total_size = 0;

    uint32_t init_node_type();
    uint32_t init_value_type();
    uint32_t init_context_type();

    uint32_t name_store_id = make_store_type();
    uint32_t node_type_id = init_node_type();
    uint32_t value_type_id = init_value_type();
    uint32_t context_type_id = init_context_type();
    uint32_t children_store_id = make_store_type();
    uint32_t quals_store_id = make_store_type();
    uint32_t node_table_store_id = make_store_type(); 
    uint32_t value_table_store_id = make_store_type(); 
    uint32_t scopes_store_id = make_store_type(); 
    uint32_t opt_str_store_id = make_store_type();
    uint32_t data_store_id = make_store_type();
    uint32_t sub_value_store_id = make_store_type();

    string get_global_string_ticket() {
        Ptr ticket((uint32_t)0,name_store_id,create_column(global[name_store_id],1,char_id),0);
        return ticket;
    }

    Ptr global_add_template(uint32_t for_type) {
        Ptr p = global_add_layout_to_col(for_type);
        return p;
    }

    uint32_t init_node_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];
        _layout ntemp(global_add_template(node_id)); //Node template
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
        return at;
    }

    uint32_t init_value_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];

        _layout vtemp(global_add_template(value_id)); //Value template
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
        return at;
    }

    uint32_t init_context_type() {
        uint32_t at = add_type();
        ColCol& t = global[at];
        _layout ctemp(global_add_template(context_id)); //Context template
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
            Ptr dataptr = get_ticket_from_unit(*this,data_store_id,size(),type());
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
                Ptr qualsptr = get_ticket_from_unit(*this, quals_store_id, sizeof(Ptr), ptr_id);
                Col& new_quals = resolve_to_col(qualsptr);
                Col& old_quals = o.quals_col();
                new_quals.reserve(old_quals.size);
                memcpy(new_quals.storage, old_quals.storage, old_quals.size);
                new_quals.size = old_quals.size;
                resolve_to_col(*this).qset(value_quals_offset,(void*)&qualsptr,sizeof(Ptr));
                Ptr subvalsptr = get_ticket_from_unit(*this, sub_value_store_id, sizeof(Ptr), ptr_id);
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
    
    struct QNode : public Col {
        inline uint32_t& type()                   {return *(uint32_t*)&storage[node_type_offset];}
        inline void      type(uint32_t t)         {memcpy(&storage[node_type_offset],(void*)&t,4);}
        inline uint32_t& sub_type()               {return *(uint32_t*)&storage[node_sub_type_offset];}
        inline void      sub_type(uint32_t st)    {memcpy(&storage[node_sub_type_offset],(void*)&st,4);}
    };

    struct Node : public Ptr {
        Node() {}
        Node(Ptr p) : Ptr(p) {}
    
        inline QNode& toQ() {return (QNode&)resolve_to_col(*this);}

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
    uint32_t Value::count_qual(uint32_t q_id) {
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

    
    bool init_type_pool() {
        global[handler_type_id].label = "handlers";
        global[layout_type_id].label = "layouts";
        global[node_type_id].label = "nodes";
        global[value_type_id].label = "values";
        global[context_type_id].label = "contexts";
        global[name_store_id].label = "names";
        global[children_store_id].label = "children";
        global[quals_store_id].label = "quals";
        global[node_table_store_id].label = "node table";
        global[value_table_store_id].label = "value table";
        global[scopes_store_id].label = "scopes";
        global[opt_str_store_id].label = "opt_str";
        global[data_store_id].label = "data";
        global[sub_value_store_id].label = "sub_value";
        return true;
    }
    bool type_pool_intilized = init_type_pool();

    uint32_t message_id = global_register_type_ids("message");
    uint32_t message_from_offset = 0;
    uint32_t message_to_offset = 0;
    uint32_t message_status_offset = 0;
    uint32_t message_total_size = 0;

    bool init_message_type() {
        _layout mtemp(global_add_template(message_id)); //Message template
        message_from_offset = mtemp.add_prop(int_id, 4, "from");
        message_to_offset = mtemp.add_prop(int_id, 4, "to");
        message_status_offset = mtemp.add_prop(int_id, 4, "status");
        message_total_size = mtemp.total_size;
        return true;
    }
    bool message_type_ready = init_message_type();

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
                col().cells.scan_for_slot(c);
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
        Handler passstart = nullptr;
    };  


    uint64_t get_real_size_of_col(Col& col) {
        uint64_t result = col.size;
        return result;
    }
    uint64_t get_real_size_of_colcol(ColCol& col) {
        uint64_t result = 0;
        for(int i=0;i<col.length();i++) {
            result += get_real_size_of_col(col[i]);
        }
        return result;
    }
    uint64_t get_real_size_of_colcolcol(ColColCol& col) {
        uint64_t result = 0;
        for(int i=0;i<col.length();i++) {
            result += get_real_size_of_colcol(col[i]);
        }
        return result;
    }




    static void write_TypeCol(std::ostream& out, ColCol& type) {
        write_raw<uint32_t>(out, type.length());
        write_col_header(out, type);
        for(int c = 0; c < type.length(); c++) {
            Col& col = type[c];
            write_col(out, col);
        }
    }
    
    static ColCol read_TypeCol(std::istream& in) {
        uint32_t len = read_raw<uint32_t>(in);
        ColCol type = read_col_header(in);
        for(uint32_t i = 0; i < len; i++) {
            Col col = read_col(in);
            type.push(col);
        }
        return type;
    }

    static void write_TypeTypeCol(std::ostream& out, ColColCol& col) {
        write_raw<uint32_t>(out, col.length());
        write_col_header(out, col);
        for(int i = 0; i < col.length(); i++) {
            write_TypeCol(out,col[i]);
        }
    }

    static ColColCol read_TypeTypeCol(std::istream& in) {
        uint32_t len = read_raw<uint32_t>(in);
        ColColCol col = read_col_header(in);
        for(uint32_t p = 0; p < len; p++) {
            ColCol cc = read_TypeCol(in);
            col.push(cc);
        }
        return col;
    }

    static void write_ColColList(std::ostream& out, list<ColCol*> cols) {
        write_raw<uint32_t>(out, cols.length());
        for(int i=0;i<cols.length();i++) {
            write_TypeCol(out,*cols[i]);
        }
    }
    static list<ColCol> read_ColColList(std::ifstream& in) {
        list<ColCol> to_return;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            to_return << read_TypeCol(in);
        }
        return to_return;
    }

    class Unit : public q_object {
        public:
        Stage* active_stage;
        list<Watcher> watchers;
        g_ptr<Log::Span> uspan = make<Log::Span>();
        list<std::string> uargs;

        bool UERROR_FLAG = false;
        list<std::string> UERRORS;

        bool running = true; 
        Context unit_ctx = deadptr;

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
        
                    if(handler_col.empty()) continue;
                    Ptr lptr = *(Ptr*)handler_col.sget(0);
                    if(!is_live(lptr)) continue;
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

            std::lock_guard<std::mutex> lock(units_mutex);
            units << this;
            return (uint16_t)units.length()-1;
        }

        Unit() : types(global), uid(derive_uid(true)) {ColColCol* types_ptr = &types; subunits.push(&types_ptr); init();}
        Unit(const ColColCol& starter) : types(starter), uid(derive_uid(false)) {ColColCol* types_ptr = &types; subunits.push(&types_ptr); init();}
        Unit(bool do_not_init) {
            ColColCol* types_ptr = &types; subunits.push(&types_ptr); 
        }

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
                if(ERROR_FLAG) {log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
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
                //if(ERROR_FLAG) {uspan->log(red("Marked "+Ptr_as_string(ctx.node())+" as error")); mark_error(ctx.node());}
                //uspan->log(green("After: "),node_info(ctx.node()));
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
        

        map<uint32_t, std::string> labels;
        map<std::string,uint32_t> labels_lookup;
        map<uint32_t,_layout> layouts;
        map<std::string, g_ptr<Stage>> stages;
        uint32_t next_id = 0;
        uint16_t uid;

        Node unit_root = deadptr;
        std::string unit_label = "";

        ColColCol types;
        PtrColColCol subunits;
        inline ColColCol* get_subunit(uint32_t idx) {return subunits[idx];}
        ColCol& operator[](uint16_t index) {return types[index];}

        ColColCol setup_mailbox_subunit() {
            ColColCol mailbox;
            ColCol instr_plate; instr_plate.label = "Instructions";
            mailbox.push(instr_plate);
            mailbox.unlock();
            return mailbox;
        }

        ColColCol sendunit = setup_mailbox_subunit();
        ColColCol recvunit = setup_mailbox_subunit();

        virtual void init() {
           
        }
        virtual Node process(std::string path) {return deadptr;}
        virtual void run(Node root) {}

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
                Ptr ticket(storeptr.unit,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                ticket.gen = resolve_to_col(ticket).gen;
                return ticket;
            } else {
                Ptr ticket(storeptr.cache,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                ticket.gen = resolve_to_col(ticket).gen;
                return ticket;
            }
        }

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
            if(marked_ptrs.has(p)) {
                return red(Ptr_to_string(p,p.cachelevel));
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
                return Ptr_to_string(p,p.cachelevel);
            #endif
        }

        void mark_error(Ptr ptr) {marked_ptrs << ptr;}

        void print_layout(_layout& l) {
            for(int i=0;i<l.offsets.length();i++) {
                print(i,": ",labels[i],": ",l.offsets[i],", ",labels[l.tags[i]],", ",labels[l.subtags[i]],"[",l.sizes[i],"]");
            }
            // for(auto e : l.overload.entrySet()) {
            //     auto keyl = decode_key(e.key);
            //     print(labels[keyl.first]," ",labels[keyl.second],"(",keyl.second,"): ",labels[e.value.type]);
            // }
        }


        uint32_t reg_id(const std::string& label) {
            //print("Registering: ",label," LEN: ",types[handler_type_id].length()," GLOBAL LEN: ",global[handler_type_id].length());
            uint32_t at = types[handler_type_id].length();
            note_value(types[handler_type_id],label,sizeof(Ptr),ptr_id);
            types[handler_type_id][at].push_default();
            labels[at] = label;
            labels_lookup[label] = at;
            //print("Registered: ",label," LEN: ",types[handler_type_id].length()," GLOBAL LEN: ",global[handler_type_id].length());
            return at;
        }

        Ptr add_layout_to_col(uint32_t type) {
            Ptr p(uid,layout_type_id,note_value(types[layout_type_id],std::to_string(type)+" Offsets",4,int_id),0);
            note_value(types[layout_type_id],"Tags",4,int_id);
            note_value(types[layout_type_id],"Sizes",4,int_id);
            note_value(types[layout_type_id],"Labels",sizeof(Ptr),string_id);
            note_value(types[layout_type_id],"Subtags",4,int_id);
            note_value(types[layout_type_id],"Subsizes",4,int_id);
            note_value(types[layout_type_id],"Ptrs",sizeof(Ptr),ptr_id);
            note_value(types[layout_type_id],"Overloads",sizeof(Ptr4),ptr4_id);
            types[handler_type_id][type].set(0,(void*)&p);
            return p;
        }

        Ptr add_template(uint32_t for_type) {
            Ptr p = add_layout_to_col(for_type);
            return p;
        }

        std::string make_wrapper_for_layout(_layout& l, const std::string& name) {
            std::string s = "";
            std::string pad = "     ";
            s+="struct "+name+" : Ptr {\n";
            s += pad+name+"() {}\n";
            s += pad+name+"(Ptr p) : Ptr(p) {}\n";
            for(int i=0;i<l.offsets.length();i++) {
                s+="\n";
                std::string type = labels[l.tags[i]];
                std::string label = l.labels[i];
                uint32_t offset = l.offsets[i];
                uint32_t size = l.sizes[i];

                bool is_compound = false; //Not sure what this was meant to do 
                //layouts.hasKey(l.tags[i]);
                bool is_ptr = l.tags[i]==ptr_id||l.tags[i]==string_id;

                if(is_ptr) {
                    s += pad+pad_str("inline "+pad_str("Ptr&",12)+" "+label+"_ptr()",32)+"{return *(Ptr*)resolve_to_col(*this).qget(sidx+"+std::to_string(offset)+"); }\n";
                    s += pad+pad_str("inline "+pad_str("Col&",12)+" "+label+"_col()",32)+"{return resolve_to_col("+label+"_ptr());}\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"(Ptr p)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", (void*)&p, "+std::to_string(size)+"); }\n";
                } else if(is_compound) {
                    s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return {pool, idx, sidx+"+std::to_string(offset)+"}; }\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", types[t.pool][t.idx].qget(t.sidx), "+std::to_string(size)+"); }\n";
                } else {
                    s += pad+pad_str("inline "+pad_str(type,12)+" "+label+"()",32)+"{return *("+type+"*)resolve_to_col(*this).qget(sidx+"+std::to_string(offset)+"); }\n";
                    s += pad+pad_str("inline "+pad_str("void",12)+" "+label+"("+type+" t)",32)+"{resolve_to_col(*this).qset(sidx+"+std::to_string(offset)+", (void*)&t, "+std::to_string(size)+"); }\n";
                }
            }
            s+="};";
            return s;
        }

        Node make_node(uint32_t type = 0, uint32_t sub_type = 0, std::string name = "", float x = -1.0f, float y = -1.0f, float z = -1.0f,
            Value value = deadptr, Ptr childrenptr = deadptr, Ptr qualsptr = deadptr, Ptr nodetableptr = deadptr, 
            Ptr valuetableptr = deadptr, Ptr scopesptr = deadptr, Ptr parent = deadptr, Ptr owner = deadptr, 
            Ptr in_scope = deadptr, std::string opt_str = "", bool mute = false, bool resolved = false) 
        {
            Node n;
            n.pool = node_type_id;
            n.idx = push_column(types[node_type_id], node_total_size, node_id);
            n.sidx = 0;
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
        
            if(!is_live(childrenptr)) {childrenptr = get_ticket(children_store_id,sizeof(Ptr),ptr_id);}
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
    
        Node make_node(uint32_t type, std::string name, Value value, Ptr in_scope) {
            return make_node(type,0,name,-1.0f,-1.0f,-1.0f,value,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,deadptr,in_scope);
        }
    
        void recycle_column(Ptr p) {
            if(is_live(p)) {
                Acorn::recycle_column(resolve_to_pool(p), p.idx);
            }
            CHECK_ERROR("Error while recycling ",Ptr_to_string(p,p.cachelevel),is_live(p)?"":"[X]");
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
            v.unit = uid;
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
            } else if(tag==ptr_id) {
                Ptr p = *(Ptr*)data;
                if(p.specialization==_DEADSPEC) return "x|x|x";
                return Ptr_to_string(p,p.cachelevel);
            } else if(is_ptr_alias(tag)||tag==function_id) {
                return Ptr_as_string(*(Ptr*)data);
            } else if(tag==ptr4_id) {
                Ptr4 p = *(Ptr4*)data;
                std::string s = labels[p.midx]+"> "+Ptr_as_string(p.ptr);
                return s;
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
                if(layouts.hasKey(tag)) {
                    std::string to_return = "";
                    _layout& l = layouts.get(tag);
                    for(int o=0;o<l.offsets.length();o++) {
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
            //print("Printing columar with ",lines.length()," lines ");
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

            if(longest_row>50) longest_row = 50; //Truncation for large fields

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
                    //print("On line ",l," row ",r);
                    if(lines[l].length()>r) {line = lines[l][r];}
                    //print("Line: ",line);
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

        list<std::string> heterogenous_col_to_lines(Col& col) {
            list<std::string> to_return;
            if(col.heterogenous) {
                if(layouts.hasKey(col.tag)) {
                    _layout& l = layouts.get(col.tag);
                    for(int i=0;i<col.length();i++) {
                        for(int o=0;o<l.offsets.length();o++) {
                            std::string line = "";
                            line+=pad_str(l.labels[o]+": ",12);
                            line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]+(l.total_size*i)));
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
                subline << col.label.to_std()+(col.live?"":" [FREE]");
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
                                line += "["+labels[cell->tag]+"?] ";
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

        list<list<std::string>> TypeCol_to_lines(ColCol& t) {
            list<list<std::string>> lines;
            for(int c=0;c<t.length();c++) {
                Col& col = t[c];
                list<std::string> subline;
                subline << col.label.to_std();
                if(col.heterogenous) {
                    subline << heterogenous_col_to_lines(col);
                } else {
                    for(int r=0;r<col.length();r++) {
                        std::string line = "";
                        CCol* cell = col.cells.find_cell(r);
                        if(cell) {
                            if(cell->tag==string_id) {
                                line += "["+((QString&)*cell).to_std()+"] ";
                            } else {
                                line += "["+labels[cell->tag]+"?] ";
                            }
                        }
                        line += tag_to_str(col.tag,col[r]);
                        subline << line;
                    }
                }
                lines << subline;
            }
            return lines;
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
                            line += "["+labels[cell->tag]+"?] ";
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

        std::string pool_info(uint32_t poolid) {
            Ptr pptr(&types,poolid,0,0);
            ColCol& pool = resolve_to_pool(pptr);
            uint32_t plen = pool.length();
            std::string to_return = "";
            if(!pool.label.empty()) {
                to_return+=pool.label.to_std();
            } else {
                to_return+="Type "+std::to_string(poolid);
            }
            if(pool.tag!=0) {
                to_return+="["+labels[pool.tag]+"]";
            }

            to_return+=" : ";
            to_return+="Mem: "+fmem(get_real_size_of_colcol(pool));
            to_return+=", Len: "+add_commas(plen);
            return to_return;
        }
        std::string unit_info() {
            std::string to_return = "Unit "+std::to_string(uid)+" Mem: "+fmem(get_real_size_of_colcolcol(types))+"\n---------------\n";
            for(int i=0;i<types.length();i++) {
                to_return+=pool_info(i)+"\n";
            }
            return to_return;
        }

        void dump_pool(ColCol& pool, uint32_t index, bool clear_dump, std::string path = "printout.txt") {
            if(clear_dump) writeFile(path,"");
            std::string to_print = "";
            to_print += "TYPE "+std::to_string(index)+" "+pool.label.to_std()+(pool.tag!=0?" ["+labels[pool.tag]+"]":"")+":\n";
            to_print += type_to_string(pool);
            to_print += "\n\n\n";
            editTextFile(path,[to_print](std::string& source){
                source+=to_print;
            });
        }

        void dump_subunit(ColColCol& subunit, bool clear_dump, std::string path = "printout.txt") {
            if(clear_dump) writeFile(path,"");
            for(int t=0;t<subunit.length();t++) {
                dump_pool(subunit[t],t,false,path);
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
                    source+="SUBUNIT "+std::to_string(s)+":\n\n";
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
    

        void insert_pools(ColColCol& col3, list<ColCol*> cols, uint32_t at) {
            offset_pool_ptrs(col3,cols.length(),at);
            offset_pool_ptrs(cols,at);
            for(int p=cols.length()-1;p>=0;p--) {
                col3.insert(at,*cols[p]);
            }
        }
        void insert_pools(ColColCol& col3, ColColCol& cols, uint32_t at) {insert_pools(col3,ColColCol_to_group(cols),at);}
    
        void push_pools(ColColCol& col3, list<ColCol*> cols) {
            offset_pool_ptrs(cols,col3.length());
            for(int p=0;p<cols.length();p++) {
                col3.push(*cols[p]);
            }
        }
        void push_pools(ColColCol& col3, ColColCol& cols) {push_pools(col3,ColColCol_to_group(cols));}
    
        //Implment later if needed
        // void insert_elements(Col& col, QCol elements, uint32_t at) {

        // }
        QCol take_elements(Col& col, uint32_t from, uint32_t to) {
            QCol to_return = col.take_range(from,to);
            offset_sidx_ptrs(col, -(int)(to - from), from);
            return to_return;
        }

        ColColCol take_pools(ColColCol& col3, uint32_t from, uint32_t to) {
            QCol raw = col3.take_range(from,to);
            offset_pool_ptrs(col3, -(int)(to - from), from);
            ColColCol to_return;
            uint32_t count = to - from;
            for(uint32_t i = 0; i < count; i++) {
                ColCol& cc = *(ColCol*)raw.qget(i * sizeof(ColCol));
                to_return.push(cc);
            }
            offset_pool_ptrs(to_return, -(int)(from));
            return to_return;
        }
        void remove_pools(ColColCol& col3, uint32_t from, uint32_t to) {ColColCol returned = take_pools(col3,from,to);}

        void copy_subgraph_col(ColColCol& col3, ColColCol& subgraph, Ptr& target, map<uint32_t,uint32_t>& pool_aliases, list<map<uint32_t,uint32_t>>& col_aliases, bool should_bundle) {
            uint32_t target_pool = target.pool;
            uint32_t target_idx = target.idx;
            
            uint32_t& poolalias = target.pool;
            uint32_t& colalias = target.idx;
            if(pool_aliases.hasKey(target_pool)) {
                poolalias = pool_aliases.get(target_pool);
            } else {
                if(should_bundle&&subgraph.length()>1) {
                    poolalias = 1;
                    pool_aliases.put(target_pool, 1);
                } else {
                    ColCol innercopypool = col3[target_pool]; innercopypool.clear();
                    poolalias = col_aliases.length();
                    pool_aliases.put(target_pool,poolalias);
                    subgraph.push(innercopypool);
                    col_aliases.push(map<uint32_t,uint32_t>{});
                }
            }
            
            if(col_aliases[poolalias].hasKey(target_idx)&&!should_bundle) {
                colalias = col_aliases[poolalias].get(target_idx);
            } else {
                Col& col = col3[target_pool][target_idx];
                Col outercopycol = col; outercopycol.clear();
                colalias = subgraph[poolalias].length();
                subgraph[poolalias].push(outercopycol);
                col_aliases[poolalias].put(target_idx,colalias);

                for(int r=0;r<col.length();r++) {
                    if(col.heterogenous) {
                        //Add special seperate handeling later
                    } else {
                        void* field = col[r];
                        if(is_ptr_alias(col.tag)) {
                            Ptr p = *(Ptr*)field;
                            if(is_live(p)) {
                                copy_subgraph_col(col3,subgraph,p,pool_aliases,col_aliases,should_bundle);
                            }
                            subgraph[poolalias][colalias].push((void*)&p);
                        } else {
                            subgraph[poolalias][colalias].push(field);
                        }
                    }
                }
            }
        }   
        ColColCol copy_subgraph(ColColCol& col3, uint32_t target_pool, bool should_bundle = false) {
            ColColCol to_return;
            if(target_pool>=col3.length()) {throw_error("core:copy_subgraph target pool ",target_pool," is out of bounds for col3 len ",col3.length()); return to_return;}
            ColCol& pool = col3[target_pool];
            map<uint32_t,uint32_t> pool_aliases;
            list<map<uint32_t,uint32_t>> col_aliases;

            ColCol outercopypool = pool; outercopypool.clear();
            uint32_t outerpoolalias = col_aliases.length();
            pool_aliases.put(target_pool,outerpoolalias);
            to_return.push(outercopypool);
            col_aliases.push(map<uint32_t,uint32_t>{});

            for(int c=0;c<pool.length();c++) {
                Ptr target_ptr;
                target_ptr.pool = target_pool; target_ptr.idx = c;
                copy_subgraph_col(col3,to_return,target_ptr,pool_aliases,col_aliases,should_bundle);
            }
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

        Message make_message(ColColCol& in, uint32_t from, uint32_t to, uint32_t status) {
            Message m = Ptr(&in,0,push_column(in[0],message_total_size,message_id),0);
            resolve_to_col(m).heterogenous = true;
            m.from() = from; m.to() = to; m.status() = status;
            return m;
        }

        Header emplace_message(ColColCol& in, uint32_t status = 0) {
            Header header = make_header(in);
            header.add_ribbon();
            make_message(in,header.pool,header.pool+1,status);
            return header;
        }

        void send_message(list<ColCol*> messagepools) {
            if(sendunit.try_lock_forever()) {
                ColCol& instrs = sendunit[0];
                uint32_t from = sendunit.length();
                uint32_t to = from+messagepools.length();
                uint32_t status = 0;

                ColCol* header = find_pool(messagepools,headerpool_id);
                if(header) {
                    Ptr refptr = *(Ptr*)header->get(0)[0]; //Getting the first Ptr from the ribbon so we know how to offset
                    ColColCol mesagepools_copy; //If we did a proper copy out then this wouldn't be a problem
                    for(int i=0;i<messagepools.length();i++) {
                        ColCol msgcopy = *messagepools[i];
                        mesagepools_copy.push(msgcopy);
                    }
                    offset_pool_ptrs(mesagepools_copy,-refptr.pool); //We also wouldn't need to offset
                    adopt_ptrs(mesagepools_copy,&sendunit);
                    push_pools(sendunit,mesagepools_copy);
                    make_message(sendunit,from,to,status);
                } else {
                    throw_error("Failed to send message because there was no header pool");
                }
                sendunit.unlock();
            }
        }
        //The pool needs to be in types
        void send_message(uint32_t pool) {send_message({&types[pool]});}

        Header make_header(ColColCol& in) {
            uint32_t at = in.length();
            ColCol pool; pool.tag = headerpool_id;
            in.push(pool);
            return Ptr(&in,at,0,0);
        }

        uint32_t make_headerpool(ColColCol& in, std::string send_to, std::string message) {
            Header header = make_header(in);
            header.add_ribbon();
            header.putString("Send to", send_to);
            header.putString("Message", message);
            return header.pool;
        }

        void send_message(std::string to, std::string message) {
            send_message(make_headerpool(types,to,message));
        }

        uint32_t courier_type_id = 0;
        void become_courier(bool auto_start = true) {
            unit_label = "Thorn";
            ColCol couriertype;
            courier_type_id = types.length();
            couriertype.label = "Leg";
            types.push(couriertype);
            if(auto_start) {
                start_thread([this](){
                    run_courier();
                });
            }
        }

        void courier_pick_up_messages(g_ptr<Unit> unit) {
            if(courier_type_id==0) {
                throw_error("Unit ",uid," is not a courier, can't pick up mesages");
                return;
            }

            if(unit->sendunit.try_lock_for(0.005)) {
                ColCol& instrs = unit->sendunit[0];

                list<Message> messages;
                for(int i=0;i<instrs.length();i++) {
                    if(!instrs[i].live) continue;
                    Message msg = Ptr(&instrs,i,0);
                    messages.push(msg);
                }
                messages.sort([](Message a, Message b){return a.from()>b.from();});
                for(int i=0;i<messages.length();i++) {
                    // print("Courier found an instruction to iterate over in ",unit->unit_label);
                    // unit->print_column(resolve_to_col(messages[i]));
                    Message msg = messages[i];                
                    if(msg.status()==0) {
                        list<ColCol*> sample = unit->gather_pools(unit->sendunit,msg.from(),msg.to());
                        ColCol* header_ptr = find_pool(sample,headerpool_id);
                        if(header_ptr) {
                            Header header = makePtr(unit->sendunit,header_ptr);
                            if(header.ribbon().hasKey("Send to")) {
                                string send_to_list = header.getString("Send to");
                                list<std::string> send_to = split_str(send_to_list.to_std(),',');

                                //Has valid recipiants
                                ColColCol group = unit->take_pools(unit->sendunit,msg.from(),msg.to());
                                adopt_ptrs(group);

                                for(int s=0;s<send_to.length();s++) {
                                    ColCol& leg = types[courier_type_id];
                                    Message record = deadptr;
                                    Col* col = nullptr;
                                    if(is_str_num(send_to[s])) {
                                        uint32_t asnum = std::stoi(send_to[s]);
                                        if(leg.hasKey(asnum)) {
                                            col = (Col*)leg.Col::get((void*)&asnum,4);
                                        }
                                    } else {
                                        if(leg.hasKey(send_to[s])) {
                                            col = (Col*)leg.Col::get(send_to[s]);
                                        }
                                    }
                                    if(col) {
                                        record = Ptr(col,col->length()); col->push_default();
                                    } else {
                                        uint32_t ncol_at = push_column(leg,message_total_size,message_id);
                                        leg[ncol_at].heterogenous = true;
                                        record = Ptr(&leg,ncol_at,0);
                                        CCol key;
                                        if(is_str_num(send_to[s])) {
                                            key.element_size = 4;
                                            key.tag = int_id;
                                            uint32_t asnum = std::stoi(send_to[s]);
                                            key.hash = hashBytes((void*)&asnum, 4);
                                            key.push((void*)&asnum);
                                        } else {
                                            key.element_size = send_to[s].length(); 
                                            key.tag = string_id;
                                            key.hash = hashBytes(send_to[s].data(), send_to[s].length());
                                            ((QString&)key) = send_to[s];
                                        }
                                        key.index = ncol_at;
                                        leg.cells.scan_for_slot(key);
                                    }     
                                    record.from() = types.length(); record.to() = types.length()+sample.length(); record.status() = 0;
                                    ColColCol copy_to_store = group;
                                    push_pools(types,copy_to_store);
                                }

                                // editTextFile("printout.txt",[&](std::string& source){source+="Pickup from "+unit->unit_label+"\n";});
                                // dump_unit(false,"printout.txt",14);
                            } else {
                                throw_error("core:courier_pick_up_messages encountered a message whose headerpool has no field 'Send to'!");
                            }
                            recycle_column(msg);
                        } else {
                            throw_error("core:courier_pick_up_messages encountered a message with no headerpool "
                            "send message was bypassed or some memory was corrupted!");
                            break;
                        }
                    } else {
                        //For when I add more status codes
                    }
                }
                unit->sendunit.unlock();
            }
        }
        void courier_drop_off_messages(g_ptr<Unit> unit) {
            if(courier_type_id==0) {
                throw_error("Unit ",uid," is not a courier, can't drop off mesages");
                return;
            }

            uint32_t asnum = unit->uid;
            ColCol& leg = types[courier_type_id];
            Col* col = nullptr;
            if(leg.hasKey(unit->unit_label)) {
                col = (Col*)leg.Col::get(unit->unit_label);
            } else if(leg.hasKey(asnum)) {
                col = (Col*)leg.Col::get((void*)&asnum,4);
            }
            if(col&&col->live&&!col->empty()) {
                if(unit->recvunit.try_lock_for(0.005)) {
                    list<Message> messages;
                    for(int i=0;i<col->length();i++) {
                        Message msg = Ptr(col,i);
                        messages.push(msg);
                    }
                    messages.sort([](Message a, Message b){return a.from()>b.from();});
                    for(int i=0;i<messages.length();i++) {
                        Message msg = messages[i];
                        if(msg.status()==0) {
                            ColColCol group = take_pools(types,msg.from(),msg.to());

                            uint32_t recv_at = unit->recvunit.length();
                            unit->make_message(unit->recvunit,recv_at,recv_at+group.length(),0);

                            unit->adopt_ptrs(group,&unit->recvunit);
                            unit->push_pools(unit->recvunit,group);

                            for(int t=0;t<leg.length();t++) {
                                Col& tube = leg[t];
                                if(tube.storage!=col->storage&&!tube.empty()&&tube.live) {
                                    for(int m=0;m<tube.length();m++) {
                                        Message tube_msg = Ptr(&tube,m);
                                        uint32_t removed = msg.to() - msg.from();
                                        if(tube_msg.from()>=msg.to()) {
                                            tube_msg.from() -= removed;
                                            tube_msg.to() -= removed;
                                        } else if(tube_msg.from()>=msg.from()) {
                                            throw_error(red("core:courier_drop_off_messages encountered a message which pointed into a taken range, corrupted!"));
                                        }
                                    }
                                }
                            }
                        } else {
                            //For when I add more status codes
                        }
                    }
                    unit->recvunit.unlock();
                }
                col->clear();

                // editTextFile("printout.txt",[&](std::string& source){source+="Drop off at "+unit->unit_label+"\n";});
                // dump_unit(false,"printout.txt",14);
            }
        }
        void run_courier() {
            while(running) {
                list<g_ptr<Unit>> snapshot;
                {
                    std::lock_guard<std::mutex> lock(units_mutex);
                    snapshot = units;
                }
                for(auto& unit : snapshot) {
                    if(unit.getPtr()==this) continue;
                    courier_pick_up_messages(unit);
                    courier_drop_off_messages(unit);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        ColColCol check_messages() {
            if(recvunit.try_lock(0.005)) {
                ColCol& instrs = recvunit[0];
                for(int i=0;i<instrs.length();i++) {
                    if(!instrs[i].live) continue;
                    Message msg = Ptr(&instrs, i, 0);
                    if(msg.status()==0) {
                        ColColCol group = take_pools(recvunit,msg.from(),msg.to());
                        recycle_column(msg);
                        for(int t=0;t<instrs.length();t++) {
                            if(!instrs[t].live) continue;
                            Message instr = Ptr(&instrs,t,0);
                            uint32_t removed = msg.to() - msg.from();
                            if(instr.from()>=msg.to()) {
                                instr.from() -= removed;
                                instr.to() -= removed;
                            } else if(instr.from()>=msg.from()) {
                                throw_error(red("core:check_messages encountered a message which pointed into a taken range, corrupted!"));
                            }
                        }
                        recvunit.unlock();
                        return group;
                    }                        
                }
                recvunit.unlock();
            }
            return col3_ref;
        }

        enum class SnapField : uint8_t {
            //QCol fields
            Size = 1, Data = 2,
            //CCol fields  
            Esize = 3, Tag = 4, Hash = 5, Index = 6, Cachelevel = 7, Live = 8, Gen = 9,
            //Col fields
            Hetero = 10, Label = 11, Cells = 12, Free = 13,
            //Structural
            Cols = 14, End = 255,
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

        void snapshot_qcol(std::ostream& out, QCol& col, bool include_data) {
            snapshot_field<uint32_t>(out, SnapField::Size, col.size);
            if(include_data) {
                write_raw<uint8_t>(out, (uint8_t)SnapField::Data);
                write_raw<uint32_t>(out, col.size); 
                out.write((const char*)col.storage, col.size);
            }
            snapshot_end(out);
        }
        QCol load_snapshot_qcol(std::istream& in, bool include_data) {
            QCol col;
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Size: col.resize(read_raw<uint32_t>(in)); break;
                    case SnapField::Data: if(include_data) {in.read((char*)col.storage, len); col.size = len;} else {in.seekg(len, std::ios::cur);} break;
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            return col;
        }

        void snapshot_ccol(std::ostream& out, CCol& col, bool include_data) {
            snapshot_qcol(out, col, include_data);
            snapshot_field<uint32_t>(out, SnapField::Esize, col.element_size);
            snapshot_string(out, SnapField::Tag, labels[col.tag]);
            snapshot_field<uint32_t>(out, SnapField::Hash, col.hash);
            snapshot_field<uint32_t>(out, SnapField::Index, col.index);
            snapshot_field<uint8_t>(out, SnapField::Live, col.live);
            snapshot_field<uint16_t>(out, SnapField::Gen, col.gen);
            snapshot_end(out);
        }        
        CCol load_snapshot_ccol(std::istream& in, bool incldue_data) {
            CCol col = load_snapshot_qcol(in,incldue_data);
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
                    case SnapField::Hash:  col.hash = read_raw<uint32_t>(in); break;
                    case SnapField::Index: col.index = read_raw<uint32_t>(in); break;
                    case SnapField::Live:  col.live = read_raw<uint8_t>(in); break;
                    case SnapField::Gen:   col.gen = read_raw<uint16_t>(in); break;
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            return col;
        }


        void snapshot_qcellcol(std::ostream& out, QCellCol& col) {
            uint32_t count = 0;
            list<CCol*> to_save;
            for(uint32_t i = 0; i < col.length(); i++) {
                if(col.get(i).storage) {
                    count++;
                    to_save << &col.get(i);
                }
            }
            write_raw<uint8_t>(out, (uint8_t)SnapField::Cols); write_raw<uint32_t>(out, count);
            for(uint32_t i = 0; i < to_save.length(); i++) snapshot_ccol(out, *to_save[i], true);
            snapshot_end(out);
        }
        
        QCellCol load_snapshot_qcellcol(std::istream& in) {
            QCellCol col;
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Cols: {
                        uint32_t count = len;
                        for(uint32_t i = 0; i < count; i++) {
                            CCol c = load_snapshot_ccol(in, true);
                            col.scan_for_slot(c);
                        }
                        break;
                    }
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            return col;
        }

        void snapshot_col(std::ostream& out, Col& col, bool include_data) {
            snapshot_ccol(out, col, include_data);
            snapshot_qcellcol(out, col.cells);
            snapshot_field<bool>(out, SnapField::Hetero, col.heterogenous);
            snapshot_string(out, SnapField::Label, col.label.to_std());
            write_raw<uint8_t>(out, (uint8_t)SnapField::Free);    write_raw<uint32_t>(out, col.free.length() * 4);
            for(int i = 0; i < col.free.length(); i++) write_raw<uint32_t>(out, col.free[i]);
            snapshot_end(out);
        }
        Col load_snapshot_col(std::istream& in, bool include_data) {
            Col col = load_snapshot_ccol(in,include_data);
            col.cells = load_snapshot_qcellcol(in);
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
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            return col;
        }

        void snapshot_colcol(std::ostream& out, ColCol& col) {
            snapshot_col(out, col,false);
            write_raw<uint8_t>(out, (uint8_t)SnapField::Cols);  write_raw<uint32_t>(out, col.length());
            for(uint32_t i = 0; i < col.length(); i++) snapshot_col(out, col[i], true);
            snapshot_end(out);
        }
        ColCol load_snapshot_colcol(std::istream& in) {
            ColCol col = load_snapshot_col(in,false);
            col.clear();
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Cols: {
                        uint32_t count = len;
                        for(uint32_t i = 0; i < count; i++) {
                            Col c = load_snapshot_col(in,true);
                            col.push(c);
                        }
                        break;
                    }
                    default: in.seekg(len, std::ios::cur); break;
                }
            }

            // if(!col.cells.empty()) {
            //     for(int i=0;i<col.cells.length();i++) {
            //         CCol& cell = col.cells.get(i);
            //         if(cell.storage) {
            //             std::string tpr((const char*)cell.storage,cell.size);
            //             print("Loaded: ",tpr," for ",cell.index);
            //         }
            //     }
            // }

            return col;
        }
        
        void snapshot_colcollist(std::ostream& out, list<ColCol*> cols) {
            write_raw<uint32_t>(out, cols.length());
            for(int i=0;i<cols.length();i++) {
                snapshot_colcol(out,*cols[i]);
            }
        }

        void snapshot_colcolcol(std::ostream& out, ColColCol& col) {
            snapshot_col(out, col, false);
            write_raw<uint8_t>(out, (uint8_t)SnapField::Cols);  write_raw<uint32_t>(out, col.length());
            for(uint32_t i = 0; i < col.length(); i++) snapshot_colcol(out, col[i]);
            snapshot_end(out);
        }
        ColColCol load_snapshot_colcolcol(std::istream& in) {
            ColColCol col = load_snapshot_col(in,false);
            col.clear();
            while(true) {
                SnapField field = (SnapField)read_raw<uint8_t>(in);
                uint32_t len = read_raw<uint32_t>(in);
                if(field == SnapField::End) break;
                switch(field) {
                    case SnapField::Cols: {
                        uint32_t count = len;
                        for(uint32_t i = 0; i < count; i++) {
                            ColCol c = load_snapshot_colcol(in);
                            col.push(c);
                        }
                        break;
                    }
                    default: in.seekg(len, std::ios::cur); break;
                }
            }
            return col;
        }


        void save_subunit(ColColCol* subunit, bool snapshot = true) {
            auto out = openWriteStream(subunit->label.to_std());
            if(snapshot) {
                snapshot_colcolcol(out, *subunit);
            } else {
                write_TypeTypeCol(out,*subunit);
            }
            out.close();
        }

        void load_subunit(ColColCol* subunit) {
            if(!subunit->empty()) return;
            if(subunit->try_lock_forever()) {
                if(!subunit->empty()) {subunit->unlock(); return;}
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
                (*subunit) = std::move(load_snapshot_colcolcol(in));
                in.close();
                subunit->unlock();
            }
        }

        Ptr load_subunit(const std::string& path) {
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
                subunit = new ColColCol(std::move(load_snapshot_colcolcol(in)));
                adopt_ptrs(*subunit,subunit);
                subunit->label = path;
                at = subunits.length();
                subunits.qput((void*)&subunit,path.data(),path.size(),string_id);
                subunit->unlock();
            } else {
                at = subunits.getidx(path.data(),path.size());
                subunit = subunits.get(at);
                if(subunit->empty()&&subunit->try_lock_forever()) {
                    if(subunit->empty()) {
                        (*subunit) = std::move(load_snapshot_colcolcol(in));
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
                subunit->QCol::clear();
                subunit->gen++;
                subunit->unlock();
            }
        }


        #define ACORN_DISPLAY_SUB_VALUES 0

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

            #if ACORN_DISPLAY_SUB_VALUES 
                if(!value.sub_values().empty()) {
                    to_return += "\n" + indent + "   Sub values:";
                    for(int i=0;i<value.sub_values().length();i++) {
                        to_return += "\n"+indent+"     "+std::to_string(i)+": "+value_info(value.sub_values()[i],indent);
                    }
                }
            #else
                to_return+=(!value.sub_values().empty()?", subvals: "+std::to_string(value.sub_values().length()):"");
            #endif
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
        
        std::string node_info(Node node, int verbosity = 20, std::string indent = "") {
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
                    if(verbosity<2) {
                        if(node.quals()[i].mute()) {
                            if(verbosity<1) {continue;}
                            qual_list += italic_str(Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()]);
                        } else {
                            qual_list += Ptr_as_string(node.quals()[i])+">"+labels[node.quals()[i].type()];
                        }
                        qual_list+=(i!=node.quals().length()-1?", ":"");
                    }
                    else {
                        to_return += "\n " + node_to_string(node.quals()[i], (indent.length()/2) + 1, i, verbosity,"q");
                    }
                }
                if(qual_list.length()>0) {
                    to_return += "[Q: "+qual_list+"]";
                }
            }
            return to_return;
        }

        #define ACORN_DISPLAY_TABLES 0

        std::string node_to_string(Node node, int depth = 0, int index = 0, int verbosity = 1, std::string sigil = "") {
            DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_as_string(node)),red(" while another error was active")); return "";})
            std::string indent(depth * 2, ' ');
            std::string to_return = "";
            
            to_return += indent + sigil + std::to_string(index) + ": " + node_info(node,verbosity,indent);
        
            #if ACORN_DISPLAY_TABLES 
            if(node.value_table().length()>0) {
                to_return += "\n" + indent + "   Value table:";
                QCellCol& cells = node.value_table().col().cells;
                for(int i=0;i<node.value_table().length();i++) {
                    to_return += "\n" + indent + "     Key: "+((QString&)*cells.find_cell(i)).to_std()+" | "+value_info(node.value_table().get(i),indent+"     ");
                }
            }
            if(node.node_table().length()>0) {
                to_return += "\n" + indent + "   Node table:";
                QCellCol& cells = node.node_table().col().cells;
                for(int i=0;i<node.node_table().length();i++) {
                    to_return += "\n" + indent + "     Key: "+((QString&)*cells.find_cell(i)).to_std()+" | "+node_info(node.node_table().get(i),indent+"     ");
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

            print("UNITS: ",units.length());
            print("TYPES: ",types.length());

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

        uint32_t indexof_subunit(uint8_t* subunit_storage) {
            for(int i=0;i<subunits.length();i++) {
                if(((Col*)subunits[i])->storage==subunit_storage) {
                    return i;
                }
            }
            return 0;
        }

        void test_pool_groups() {

        //Test subunits memory
            // Log::Line l; 
            // size_t mem_base = current_memory_usage();
            //print("Baseline: ",mem_base);
            //Creation
                // ColColCol* subunit = subunits.create("savetest");
                // l.start();
                // //subunit->reserve(sizeof(Col)*100);
                // for(int i=0;i<100;i++) {
                //     ColCol pool; //pool.reserve(sizeof(Col)*100);
                //     for(int n=0;n<100;n++) {
                //         Col col(4,int_id); //col.reserve(400);
                //         for(int j=0;j<100;j++) {
                //             col.push((void*)&j);
                //         }
                //         pool.push(col);
                //     }
                //     subunit->push(pool);
                // }
                // print("Created subunit in ",ftime(l.end()));
                // size_t mem_post = current_memory_usage();
                // uint64_t real_size = get_real_size_of_colcolcol(*subunit);
                // print("Size is ",real_size," Memory used now is ",mem_post," expected around ",mem_base+real_size," ratio is ",(double)mem_post/(double)(mem_base+real_size));
                // l.start();
                // save_subunit(subunit);
                // print("Saved ",real_size," bytes in ",ftime(l.end()));
            //Subunit memory
                // l.start();
                // Ptr p = load_subunit("savetest");
                // print("Loaded subunit in ",ftime(l.end()));
                // ColColCol& subunit = resolve_to_subunit(p);

                // size_t mem_post = current_memory_usage();
                // uint64_t real_size = get_real_size_of_colcolcol(subunit);
                // uint64_t post_size = mem_base+real_size;
                // print("Size is ",real_size," Memory used now is ",mem_post," expected around ",post_size," ratio is ",(double)mem_post/(double)post_size);
                // bounce_subunit(&subunit);
                // size_t mem_bounce = current_memory_usage();
                // real_size = get_real_size_of_colcolcol(subunit);
                // print("Bounced, real size is now ",real_size," mem is now ",mem_bounce," expected around ",mem_base," ratio is ",(double)mem_bounce/mem_base);
                
                // l.start();
                // Ptr p2 = load_subunit("savetest");
                // print("Loaded subunit again in ",ftime(l.end()));
                // print("Old: [",ptr_to_string(p.cache),"]",Ptr_as_string(p)," New: [",ptr_to_string(p2.cache),"]",Ptr_as_string(p2));
                // real_size = get_real_size_of_colcolcol(subunit);
                // print("Size is now ",real_size," and there are ",subunits.length()," subunits");
            //Subunit usage
                // ColColCol& subunit = resolve_to_subunit(load_subunit("savetest"));
                // int six = 22;
                // subunit[3][10].set(4,(void*)&six);  
                // bounce_subunit(&subunit);
                // if(acquire_subunit(&subunit)) {
                //     print(*(int*)(subunit[3][10][4]));
                //     subunit.unlock();
                // }
                // bounce_subunit(&subunit);

        //Test subunits
            // ColColCol* a = create_subunit();
            // ColColCol* b = create_subunit();
            
            // uint16_t a_gen = a->gen;
            // uint32_t a_idx = indexof_subunit(a->storage);
            
            // recycle_subunit(a_idx);
            // ColColCol* c = create_subunit();
            // print(a_gen != c->gen ? "PASS: gen incremented" : "FAIL: stale gen");
            // print(indexof_subunit(c->storage) == indexof_subunit(a->storage) ? "Recycled correctly" : "WARN: got different slot");
            // print(b->gen == 0 ? "PASS: b unaffected" : "FAIL: b was modified");

        //Test concurrency
            // CCol desirable;
            // list<g_ptr<Thread>> threads;
            // for(int t=0;t<4;t++) {
            //     g_ptr<Thread> thread = make<Thread>();
            //     threads << thread;
            //     thread->run([&desirable,t](){
            //         while(true) {
            //             uint8_t expected = 1;
            //             if(desirable.live.compare_exchange_strong(expected, 0)) {
            //                 print(t," claimed it");
            //                 std::this_thread::sleep_for(std::chrono::milliseconds(randi(1,100)));
            //                 desirable.live = 1;
            //             }   
            //         }
            //     });
            //     thread->start();
            // }
            // bool should_dump = false;
            // while(!should_dump) {
            //     should_dump = true;
            //     for(int i=0;i<threads.length();i++) {
            //         if(threads[i]->runningTurn) {should_dump = false;}
            //     }
            // }

        //Benchmark QCellCol
            // int ITS = 100;
            // list<std::string> titles;
            // for(int i = 0; i < ITS; i++) {
            //     int rl = randi(5,20);
            //     std::string rstr = sgen::randsgen(sgen::TRUE_RANDOM);
            //     for(int r=0;r<rl;r++) {
            //         rstr+=sgen::randsgen(sgen::TRUE_RANDOM);
            //     }
            //     titles << rstr;
            // }
        
            // for(int i=0;i<20;i++) {
            //     print(titles[i]," : ",to_bin(hashBytes(titles[i].data(),titles[i].size())));
            // }

            // // --- Three structures under test ---
            // Col col(4); col.tag = 0; // element_size=4 (storing ints), matches CCol(uint32_t _size) ctor
            // map<std::string,int> g_map;
            // std::unordered_map<std::string,int> std_map;
        
            // Log::rig r;
        
            // // --- Cleanup, run once per pass (i==0), mirrors old bench's "clean" processes ---
            // r.add_process("clean_col",[&](int i){
            //     if(i==0) { col = Col(4); col.tag = 0; }
            // },1);
            // r.add_process("clean_g_map",[&](int i){
            //     if(i==0) { g_map.clear(); }
            // },1);
            // r.add_process("clean_std_map",[&](int i){
            //     if(i==0) { std_map.clear(); }
            // },1);
        
            // // --- Populate: insert ITS string-keyed int entries into each structure ---
            // r.add_process("populate_col",[&](int i){
            //     int val = i;
            //     col.put(titles[i], (void*)&val);
            // },1);
            // r.add_process("populate_g_map",[&](int i){
            //     g_map.put(titles[i], i);
            // },1);
            // r.add_process("populate_std_map",[&](int i){
            //     std_map.emplace(titles[i], i);
            // },1);
        
            // // --- Access: point lookup by key, same key set used to populate ---
            // r.add_process("access_col",[&](int i){
            //     volatile int a = *(int*)col.get(titles[i]);
            // },1);
            // r.add_process("access_g_map",[&](int i){
            //     volatile int a = g_map.get(titles[i]);
            // },1);
            // r.add_process("access_std_map",[&](int i){
            //     volatile int a = std_map.at(titles[i]);
            // },1);
        
            // // --- hasKey: presence check, same key set ---
            // r.add_process("haskey_col",[&](int i){
            //     volatile bool b = col.hasKey(titles[i]);
            // },1);
            // r.add_process("haskey_g_map",[&](int i){
            //     volatile bool b = g_map.hasKey(titles[i]);
            // },1);
            // r.add_process("haskey_std_map",[&](int i){
            //     volatile bool b = (std_map.find(titles[i]) != std_map.end());
            // },1);
        
            // r.add_comparison("populate_col","populate_g_map");
            // r.add_comparison("populate_col","populate_std_map");
            // r.add_comparison("access_col","access_g_map");
            // r.add_comparison("access_col","access_std_map");
            // r.add_comparison("haskey_col","haskey_g_map");
            // r.add_comparison("haskey_col","haskey_std_map");
        
            // r.run(1000,true,ITS);
        
            // // --- Scale up: 100 -> 10,000 -> 1,000,000, same shape as the old bench's ITS^(i+1) loop ---
            // for(int i = 0; i < 2; i++) {
            //     int new_its = (int)std::pow(ITS, i+1);
            //     print("ITS: ", new_its);
        
            //     titles.clear();
            //     for(int j = 0; j < new_its; j++) titles << std::to_string(j);
        
            //     r.run(1000,false,new_its);
            // }
        
            // print("Final col length: ", col.length());
            // print("Final g_map size: ", g_map.size());
            // print("Final std_map size: ", std_map.size());


        //Benchmark materialization via lowering Ptrs
            // uint32_t dataidx = types.length();
            // uint32_t storeidx = dataidx+1;
            
            // int elements = 10000;

            // ColCol tdata;
            // Col tdribbon(sizeof(Ptr)); tdribbon.tag = ptr_id;
            // for(int i=0;i<elements;i++) {
            //     Ptr p(uid,storeidx,0,i);
            //     tdribbon.push((void*)&p);
            // }
            // tdata.push(tdribbon);
            // types.push(tdata);

            // list<int> direct;

            // ColCol tstore;
            // Col tsribbon(4); tsribbon.tag = int_id;
            // for(int i=0;i<elements;i++) {
            //     int r = randi(0,1000000);
            //     direct << r;
            //     tsribbon.push((void*)&r);
            // }
            // tstore.push(tsribbon);
            // types.push(tstore);

            // ColCol& data = types[dataidx];
            // ColCol& store = types[storeidx];
            // Col& dribbon = data[0];
            // Col& sribbon = store[0];

            // uint8_t* snapshot = (uint8_t*)malloc(dribbon.size);
            // memcpy(snapshot, dribbon.storage, dribbon.size);

            // Log::rig r;

            // r.add_process("restore_snapshot_0",[&](int i){
            //     if(i==0) memcpy(dribbon.storage, snapshot, dribbon.size);
            // });
            // r.add_process("reset_to_0",[&](int i){
            //     Ptr& p = *(Ptr*)dribbon.sget(i);
            //     p.cache = nullptr;
            //     p.cachelevel = 0;
            // });
            // r.add_process("sort_cachelevel_0",[&](int i){
            //     if(i==0) {
            //         std::sort((Ptr*)dribbon.storage, (Ptr*)dribbon.storage + elements,
            //             [](Ptr& a, Ptr& b){ return *(int*)resolve_ptr(a) < *(int*)resolve_ptr(b); });
            //     }
            // });
            
            // r.add_process("restore_snapshot_3",[&](int i){
            //     if(i==0) memcpy(dribbon.storage, snapshot, dribbon.size);
            // });
            // r.add_process("lower_to_3",[&](int i){
            //     Ptr& p = *(Ptr*)dribbon.sget(i);
            //     p.cache = &types;
            //     p.cachelevel = 3;
            // });
            // r.add_process("sort_cachelevel_3",[&](int i){
            //     if(i==0) {
            //         std::sort((Ptr*)dribbon.storage, (Ptr*)dribbon.storage + elements,
            //             [](Ptr& a, Ptr& b){ return *(int*)resolve_ptr(a) < *(int*)resolve_ptr(b); });
            //     }
            // });
            
            // r.add_process("restore_snapshot_2",[&](int i){
            //     if(i==0) memcpy(dribbon.storage, snapshot, dribbon.size);
            // });
            // r.add_process("lower_to_2",[&](int i){
            //     Ptr& p = *(Ptr*)dribbon.sget(i);
            //     p.cache = &store;
            //     p.cachelevel = 2;
            // });
            // r.add_process("sort_cachelevel_2",[&](int i){
            //     if(i==0) {
            //         std::sort((Ptr*)dribbon.storage, (Ptr*)dribbon.storage + elements,
            //             [](Ptr& a, Ptr& b){ return *(int*)resolve_ptr(a) < *(int*)resolve_ptr(b); });
            //     }
            // });
            
            // r.add_process("restore_snapshot_1",[&](int i){
            //     if(i==0) memcpy(dribbon.storage, snapshot, dribbon.size);
            // });
            // r.add_process("lower_to_1",[&](int i){
            //     Ptr& p = *(Ptr*)dribbon.sget(i);
            //     p.cache = &sribbon;
            //     p.cachelevel = 1;
            // });
            // r.add_process("sort_cachelevel_1",[&](int i){
            //     if(i==0) {
            //         std::sort((Ptr*)dribbon.storage, (Ptr*)dribbon.storage + elements,
            //             [](Ptr& a, Ptr& b){ return *(int*)cache_as_col(a).sget(a.sidx) < *(int*)cache_as_col(b).sget(b.sidx); });
            //     }
            // });

            // uint8_t* sribbon_snapshot = (uint8_t*)malloc(sribbon.size);
            // memcpy(sribbon_snapshot, sribbon.storage, sribbon.size);
            // r.add_process("sort_sribbon",[&](int i){
            //     if(i==0) {
            //         std::sort((int*)sribbon.storage, (int*)sribbon.storage + elements);
            //     }
            // });
            // r.add_process("restore_sribbon",[&](int i){
            //     if(i==0) memcpy(sribbon.storage, sribbon_snapshot, sribbon.size);
            // });

            // int* direct_snapshot = (int*)malloc(elements * sizeof(int));
            // memcpy(direct_snapshot, direct.data(), elements * sizeof(int));
            // r.add_process("sort_direct",[&](int i){
            //     if(i==0) {
            //         std::sort(direct.data(), direct.data() + elements);
            //     }
            // });
            // r.add_process("restore_direct",[&](int i){
            //     if(i==0) memcpy(direct.data(), direct_snapshot, elements * sizeof(int));
            // });

        
            
            // r.add_comparison("sort_cachelevel_1","sort_cachelevel_3");
            // r.add_comparison("sort_cachelevel_3","sort_cachelevel_0");
            // r.add_comparison("sort_cachelevel_1","sort_cachelevel_0");
            
            // r.run(100,true,elements);
        
        //Dump unit testing
        // print("Producing all code");
        //     Node the_all_code = make_node();
        //     for(int i=0;i<100;i++) {
        //         the_all_code.name().push(sgen::randsgen(sgen::RANDOM));
        //     }
        //     print("Name: ",the_all_code.name().to_std());

        //     for(int i=0;i<10000;i++) {
        //         Node node = make_node(identifier_id,sgen::randsgen(sgen::AVAL_CENTRAL_FIRST_MALE)+" "+sgen::randsgen(sgen::AVAL_CENTRAL_LAST),deadptr,deadptr);
        //         for(int c=0;c<randi(0,6);c++) {
        //             int r = randi(-100,100);
        //             Node child = make_node(literal_id,std::to_string(r),make_value(int_id,4),deadptr);
        //             child.set((void*)&r);
        //             node.children() << child;
        //         }
        //     }
        //     print("Dumping");
        //     dump_unit(true);
        //     print("Dumped");


        //Testing
            // int six = 6;

            // auto in = openReadStream("savetest");
            // ColColCol main_pool = load_snapshot_colcolcol(in);

            // ColColCol main_pool;
            // for(uint32_t p=0;p<3;p++) {
            //     ColCol subpool; 

            //     Col sc(sizeof(Ptr)); sc.tag = ptr_id;
            //     Ptr sp(p,1,0);
            //     sc.push((void*)&sp);
            //     Ptr ssp((p==2?0:p+1),0,0);
            //     sc.push((void*)&ssp);
            //     subpool.push(sc);

            //     for(uint32_t i=0;i<3;i++) {
            //         Col c(4); c.tag = int_id;
            //         c.push((void*)&i);
            //         c.push((void*)&six);
            //         subpool.push(c);
            //     }
            //     main_pool.push(subpool);
            // }
            // writeFile("printout.txt","");
            // editTextFile("printout.txt",[](std::string& source){source+="\n=====MAIN POOL DUMP=====\n\n";});
            // for(int p=0;p<main_pool.length();p++) {
            //     dump_pool(main_pool[p],p,false);
            // }

            // ColCol pool_a; pool_a.tag = func_call_id; pool_a.label = "data";
            // ColCol pool_b; pool_b.tag = function_id; pool_b.label = "meta";
            // ColCol pool_c; pool_c.tag = func_decl_id; pool_c.label = "store";
        
            // for(uint32_t i=0;i<3;i++) {
            //     Col col(sizeof(Ptr)); col.tag = ptr_id;
            //     Ptr p(&types,(i==0?1:i), 1, 0);
            //     col.push((void*)&p);
            //     pool_a.push(col);

            //     Col subcol(4); subcol.tag = int_id;
            //     subcol.push((void*)&i);

            //     Col str_col(sizeof(Ptr)); str_col.tag = string_id;
            //     Ptr strptr(&types,i,2,0); str_col.push((void*)&strptr);
            //     Col char_col(1); char_col.tag = char_id;
            //     std::string tstr = "test";
            //     for(auto& ch : tstr) {
            //         char_col.push((void*)&ch);
            //     }

            //     if(i==0) {
            //         pool_a.push(subcol);
            //     } else if(i==1) {
            //         pool_b.push(subcol);
            //         pool_b.push(str_col);
            //         pool_b.push(char_col);
            //     } else if(i==2) {
            //         pool_c.push(subcol);
            //         pool_c.push(str_col);
            //         pool_c.push(char_col);
            //     }
            // }
        
            // uint32_t base = main_pool.length();
            // print("Base offset: ", base);
        
            // list<ColCol*> group = {&pool_a, &pool_b, &pool_c};

            // print("Dumping subpools");
            // editTextFile("printout.txt",[](std::string& source){source+="\n=====SUB POOL DUMP=====\n\n";});
            // for(int p=0;p<group.length();p++) {
            //     dump_pool(*group[p],p,false);
            // }

            // insert_pools(main_pool, group, 1);
            // // uint32_t oldlen = types.length();
            // // uint32_t mainlen = main_pool.length();
            // push_pools(types,main_pool);
            // //list<ColCol*> subgroup; for(int i=oldlen;i<oldlen+mainlen;i++) {subgroup << &types[i];}

            // editTextFile("printout.txt",[](std::string& source){source+="\n=====POST PUSH=====\n\n";});
            // dump_unit(false);
            // // for(int p=0;p<subgroup.length();p++) {
            // //     dump_pool(*subgroup[p],p,false);
            // // }

            // auto out = openWriteStream("savetest");
            // snapshot_colcolcol(out,types);
            // out.close();

            // Ptr sssp(&types,5,2,0);
            // Col ssscol(sizeof(Ptr)); ssscol.tag = ptr_id;
            // ssscol.push((void*)&sssp);
            // main_pool[1].push(ssscol);

            // ColColCol copyc3 = copy_subgraph(main_pool,1);

            // editTextFile("printout.txt",[](std::string& source){source+="\n=====DUMPING COPY=====\n\n";});
            // for(int p=0;p<copyc3.length();p++) {
            //     dump_pool(copyc3[p],p,false);
            // }


            // ColColCol c3 = take_pools(main_pool,2,3);

            // editTextFile("printout.txt",[](std::string& source){source+="\n=====TOOK 1=====\n\n";});
            // for(int p=0;p<main_pool.length();p++) {
            //     dump_pool(main_pool[p],p,false);
            // }

            // editTextFile("printout.txt",[](std::string& source){source+="\n=====DUMPING 1=====\n\n";});
            // for(int p=0;p<c3.length();p++) {
            //     dump_pool(c3[p],p,false);
            // }

            // insert_pools(main_pool,c3,3);

            // editTextFile("printout.txt",[](std::string& source){source+="\n=====INSERTED AT 2=====\n\n";});
            // for(int p=0;p<main_pool.length();p++) {
            //     dump_pool(main_pool[p],p,false);
            // }
        }


        void test_courier() {
            start_thread([this](){
                while(true) {
                    ColColCol msgpools = check_messages();
                    if(msgpools.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        adopt_ptrs(msgpools,&msgpools);
                        Header header = makePtr(msgpools,find_pool(msgpools,headerpool_id));
                        print(unit_label," recived a message: ",header.getString("Message"));
                    }
                }
            });
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

        void walk_nodenet(Node root, std::function<void(Node)> func) {
            func(root);
            for(int i=0;i<root.children().length();i++) walk_nodenet(root.children()[i],func);
            for(int i=0;i<root.quals().length();i++) walk_nodenet(root.quals()[i],func);
            for(int i=0;i<root.scopes().length();i++) if(root.scopes()[i].owner()==root) walk_nodenet(root.scopes()[i],func);
            if(is_live(root.value())) {
                for(int i=0;i<root.value().quals().length();i++) walk_nodenet(root.value().quals()[i],func);
            }
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

        uint32_t resume_travel_pass(Context ctx);

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

        void suspend() {running = false;}
        void resume() {running = true;}

        inline void standard_process(Context& ctx, uint32_t type) {
            DEBUG_ONLY(for(auto& w : watchers) {if(w.prefix) w.prefix(ctx);})
            while(!running) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
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
                if(i==0) {
                    process_node(sub_ctx, sub_ctx.result().get(i));
                } else {
                    process_node(sub_ctx, sub_ctx.result().get(i), sub_ctx.result().get(i-1));
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


        inline void normalize_Col_tags(Col& col, map<uint32_t,uint32_t>& fixup) {
            if(fixup.hasKey(col.tag)) {
                //print("Fixup ",col.tag," as ",fixup[col.tag]);
                col.tag = fixup.get(col.tag);
            }
        }
        inline void normalize_ColCol_tags(ColCol& col, map<uint32_t,uint32_t>& fixup) {
            normalize_Col_tags(col,fixup);
            for(int i=0;i<col.length();i++) {
                normalize_Col_tags(col[i],fixup);
            }
        }
        inline void normalize_ColColCol_tags(ColColCol& col, map<uint32_t,uint32_t>& fixup) {
            normalize_Col_tags(col,fixup);
            for(int i=0;i<col.length();i++) {
                normalize_ColCol_tags(col[i],fixup);
            }
        }

        map<uint32_t,uint32_t> remap_ids_by_label(map<uint32_t,std::string>& ids) {
            map<uint32_t,uint32_t> fixup;
            for(auto e : ids.entrySet()) {
                uint32_t old_id = e.key;
                std::string label = e.value;
                if(labels_lookup.hasKey(label)) {
                    uint32_t new_id = labels_lookup.get(label);
                    if(old_id != new_id) {
                        //print("Registered id fixup from ",old_id,"[",label,"] to ",new_id,"[",labels[new_id],"]");
                        fixup.put(old_id, new_id);
                    }
                } else {
                    //print("Label lookup does not have key ",label);
                }
            }
            return fixup;
        }

        enum norm_ops {
            NORM_IDS = 0, NORM_PTRS = 1
        };

        void write_normalize_trailer(std::ostream& out,list<uint32_t> passes) {
            uint32_t pass_count = passes.length();
            write_raw(out, pass_count);
            for(int p=0;p<pass_count;p++) {
                switch(passes[p]) {
                    case NORM_IDS: {
                        write_raw<uint32_t>(out, NORM_IDS);
                        write_raw<uint32_t>(out, labels.entrySet().length());
                        for(auto e : labels.entrySet()) {
                            write_raw<uint32_t>(out, e.key);
                            write_string(out, e.value);
                        }
                    }
                    break;
                    case NORM_PTRS: {
                        write_raw<uint32_t>(out, NORM_PTRS);
                    }
                    break;

                    default:
                    print(red("core:write_normalize unrecognized pass: "+std::to_string(passes[p])));
                    break;
                }
            }
        }
    
        void normalize(std::ifstream& in, list<void*> data, uint8_t level) {
            uint32_t pass_count = read_raw<uint32_t>(in);
            for(uint32_t p=0;p<pass_count;p++) {
                uint32_t block_op = read_raw<uint32_t>(in);
                switch(block_op) {
                    case NORM_IDS: {
                        uint32_t count = read_raw<uint32_t>(in);
                        print("Normalizing ",count," ids");
                        map<uint32_t,std::string> block_ids;
                        for(uint32_t j=0;j<count;j++) {
                            uint32_t id = read_raw<uint32_t>(in);
                            std::string label = read_string(in);
                            block_ids.put(id,label);
                            //print(label,": ",id," [",labels[id],"]");
                        }
                        map<uint32_t,uint32_t> fixup = remap_ids_by_label(block_ids);
                        if(level==0) { //Col
                            for(int i=0;i<data.length();i++) {
                                Col& col = *(Col*)data[i];
                                normalize_Col_tags(col,fixup);
                            }
                        } else if(level==1) { //ColCol
                            for(int i=0;i<data.length();i++) {
                                ColCol& col = *(ColCol*)data[i];
                                normalize_ColCol_tags(col,fixup);
                            }
                        } else if(level==2) { //ColColCol
                            for(int i=0;i<data.length();i++) {
                                ColColCol& col = *(ColColCol*)data[i];
                                normalize_ColColCol_tags(col,fixup);
                            }
                        }
                    }
                    break;

                    default:
                    print(red("core:normalize unrecognized block opperation: "+std::to_string(block_op)));
                    break;
                }
            }
        }

        void save_acorn(const std::string& path) {
            auto out = openWriteStream(path);
            write_TypeTypeCol(out,types);
    
        }
        void load_acorn(const std::string& path) {
            auto in = openReadStream(path);
            types = read_TypeTypeCol(in);
            init();
            ERROR_FLAG = false;
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

    std::ostream& operator<<(std::ostream& os, Acorn::string& s) {
        os.write((const char*)s.col().storage, s.length());
        return os;
    }

    
    //To prove a point about continuations and Seaside
    uint32_t Unit::resume_travel_pass(Context ctx) {
        int& i = ctx.index();
        while(i < ctx.result().length()) {
            ctx.node(ctx.result().get(i));
            standard_process(ctx);
            Node nleft = ctx.result().get(i);
            ctx.left(nleft);
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(ctx.state()>0) { //This is the return/break process
                endline();
                deep_recycle_context(ctx);
                return ctx.state();
            }
            i++;
        }
        endline();
        deep_recycle_context(ctx);
        return 0;
    }

    ColColCol& init_first_unit() {
        g_ptr<Unit> u = make<Unit>(false);
        units << u;
        return *u->get_subunit(0);
    }
}


    //An experiment
        // void snapshot_object(std::ostream& out, Ptr p, _layout& l) {
        //     Col& col = resolve_to_col(p);
        //     snapshot_string(out, SnapField::Tag, labels[col.tag]);
        //     for(int i = 0; i < l.labels.length(); i++) {
        //         snapshot_string(out, SnapField::Label, l.labels[i]);
        //         snapshot_string(out, SnapField::Tag, labels[l.tags[i]]);
        //         write_raw<uint8_t>(out, (uint8_t)SnapField::Data);
        //         if(is_ptr_alias(l.tags[i])) {
        //             Col& subcol = resolve_to_col(*(Ptr*)col.qget(p.sidx+l.offsets[i]));
        //             write_raw<uint32_t>(out, subcol.size); 
        //             out.write((const char*)subcol.storage, subcol.size);
        //         } else {
        //             write_raw<uint32_t>(out, l.sizes[i]); 
        //             out.write((const char*)col.qget(p.sidx+l.offsets[i]), l.sizes[i]);
        //         }
        //     }
        //     snapshot_end(out);
        // }        
        // void load_snapshot_object(std::istream& in, CCol& col) {
        //     _layout* layout = nullptr;
        //     uint32_t current_index = 0;
        //     uint32_t layout_tag = 0;
        //     uint32_t current_tag = 0;
        //     while(true) {
        //         SnapField field = (SnapField)read_raw<uint8_t>(in);
        //         uint32_t len = read_raw<uint32_t>(in);
        //         if(field == SnapField::End) break;
        //         switch(field) {
        //             case SnapField::Tag: {
        //                 std::string s(len, '\0');
        //                 in.read(s.data(), len);
        //                 uint32_t zero = 0;
        //                 uint32_t tag = labels_lookup.getOrDefault(s, zero);
        //                 if(!layout) { //Is the first load
        //                     if(layouts.hasKey(tag)) {
        //                         layout = &layouts.get(tag);
        //                         col.push_default();
        //                         layout_tag = tag;
        //                     } else {
        //                         throw_error("core:load_snapshot_object No layout was found for tag ",labels[tag]," in unit ",uid);
        //                         return;
        //                     }
        //                 } else { //Load per element
        //                     if(tag!=layout->tags[current_index]) { //Could also add coercsion here instead
        //                         throw_error("core:load_snapshot_object Type disagreement between saved tag ",labels[tag],
        //                         " and current tag ",labels[layout->tags[current_index]]," in layout ",labels[layout_tag]," in unit ",uid);
        //                         return;
        //                     }
        //                     current_tag = tag;
        //                 }
        //                 break;
        //             }
        //             case SnapField::Label: {
        //                 std::string s(len, '\0');
        //                 in.read(s.data(), len);
        //                 current_index = layout->label_to_index.get(s);
        //                 break;
        //             }
        //             case SnapField::Data: {
        //                 if(is_ptr_alias(current_tag)) {

        //                 } else {
        //                     char* into = (char*)col.qget(layout->offsets[current_index]);
        //                     in.read(into, len); 
        //                 }
        //                 break;
        //             } 
        //             default: in.seekg(len, std::ios::cur); break;
        //         }
        //     }
        // }