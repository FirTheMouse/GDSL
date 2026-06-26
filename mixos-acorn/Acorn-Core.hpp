#pragma once

#include <thread>
#include "../core/Golden.hpp"
#include "../mixos-acorn/util/Acorn-Type.hpp"
#include "../ext/g_lib/core/q_object.hpp"

#define NAMED_PTRS 1

namespace Acorn {   
    static int _ctx_dummy_index = 0;
    class Unit;
    struct Node;
    struct Value;

    struct ColCol : Col {
        ColCol() : Col(sizeof(Col)) {}
        ColCol(Col c) : Col(c) {}
        ColCol(const ColCol& o) : Col(sizeof(Col)) {
            element_size = o.element_size;
            tag = o.tag;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
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

        Col& get(uint32_t idx) {return *(Col*)Col::sget(idx);}
        void set(uint32_t idx, Col val) {
            get(idx).~Col();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.storage = nullptr;
        }
        Col& operator[](uint32_t idx) {return *(Col*)Col::sget(idx);}
        void push(Col t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
        void put(const std::string& key, Col t) {
            Col::put(key,(void*)&t); //This should probably have tag string id for display later, may require reordering how we register the ids
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
    };
    
    struct ColColCol : Col {
        ColColCol() : Col(sizeof(ColCol)) {}
        ColColCol(Col c) : Col(c) {}
        ColColCol(const ColColCol& o) : Col(sizeof(ColCol)) {
            element_size = o.element_size;
            tag = o.tag;
            heterogenous = o.heterogenous;
            label = QCol(o.label);
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
            ColColCol::operator=(std::move(o));
            return *this;
        }
        ~ColColCol() {
            if(!storage || element_size == 0) return;
            for(uint32_t i = 0; i < length(); i++) {
                get(i).~ColCol();
            }
        }
        ColCol& get(uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void set(uint32_t idx, ColCol val) {
            get(idx).~ColCol();
            Col::set(idx,(void*)&val);
            val.storage = nullptr;
            val.label.storage = nullptr;
            // for(uint32_t i = 0; i < val.cells.length(); i++) {
            //     val.cells.get(i).storage = nullptr;
            // }
            val.cells.storage = nullptr;
        }
        ColCol& operator[](uint32_t idx) {return *(ColCol*)Col::sget(idx);}
        void push(ColCol t) {
            Col::push((void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
        }
        void insert(uint32_t index, ColCol t) {
            CCol::insert(index,(void*)&t);
            t.storage = nullptr;
            t.label.storage = nullptr;
            t.cells.storage = nullptr;
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
            case 7: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 6: return std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 5: return std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 4: return std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 3: return std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 2: return std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            case 1: return std::to_string(p.sidx);
            case 0: return std::to_string(p.region)+"|"+std::to_string(p.zone)+"|"+std::to_string(p.device)+"|"+std::to_string(p.unit)+"|"+std::to_string(p.pool)+"|"+std::to_string(p.idx)+"|"+std::to_string(p.sidx);
            default: return "INVALID PRINT LEVEL FOR PTR_TO_STRING "+std::to_string(print_level);
        }
    }
    Ptr string_to_Ptr(const std::string& s) {
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

    static ColColCol col3_ref;
    inline ColColCol& resolve_to_unit(const Ptr& ptr);
    static ColCol col2_ref;
    inline ColCol& resolve_to_pool(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 3: {return resolve_to_unit(ptr)[ptr.pool];}
            case 2: return *(ColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to pool because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col2_ref;
        }
    }
    static Col col1_ref;
    inline Col& resolve_to_col(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: case 3: case 2: {return resolve_to_pool(ptr)[ptr.idx];}
            case 1: return *(Col*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to col because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col1_ref;
        }
    }
    inline void* resolve_ptr(const Ptr& ptr) {
        return resolve_to_col(ptr)[ptr.sidx];
    }
    inline void* resolve_ptr(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_ptr(ptr);}
    inline Col& resolve_to_col(Ptr ptr, const uint32_t& idx) {ptr.idx = idx; return resolve_to_col(ptr);}


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

    bool is_live(Ptr p) {return (p.pool!=0||p.idx!=0);}
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

            resolve_to_col(impl,impl.idx+offsets_col).push((void*)&total_size);
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

    uint32_t undefined_id = 0;
    uint32_t stages_id = 1;
    uint32_t ptr_id = 2;

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
        note_value(t,"ptr",sizeof(Ptr),ptr_id);
        t.get(2).push_default(); //Layout of Ptr
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

    uint32_t prefix_ptr_id = global_reg_id("prefix_ptr"); uint32_t suffix_ptr_id = global_reg_id("suffix_ptr");
    uint32_t float_id = global_reg_id("float"); uint32_t prefix_float_id = global_reg_id("prefix_float"); uint32_t suffix_float_id = global_reg_id("suffix_float");
    uint32_t int_id = global_reg_id("int"); uint32_t prefix_int_id = global_reg_id("prefix_int"); uint32_t suffix_int_id = global_reg_id("suffix_int");
    uint32_t bool_id = global_reg_id("bool"); uint32_t prefix_bool_id = global_reg_id("prefix_bool"); uint32_t suffix_bool_id = global_reg_id("suffix_bool");
    uint32_t string_id = global_reg_id("string"); uint32_t prefix_string_id = global_reg_id("prefix_string"); uint32_t suffix_string_id = global_reg_id("suffix_string");
    uint32_t char_id = global_reg_id("char"); uint32_t prefix_char_id = global_reg_id("prefix_char"); uint32_t suffix_char_id = global_reg_id("suffix_char");
    uint32_t ptr4_id = global_reg_id("ptr4"); uint32_t prefix_ptr4_id = global_reg_id("prefix_ptr4"); uint32_t suffix_ptr4_id = global_reg_id("suffix_ptr4");
    uint32_t duck_id = global_reg_id("duck"); uint32_t prefix_duck_id = global_reg_id("prefix_duck"); uint32_t suffix_duck_id = global_reg_id("suffix_duck");
    size_t list_id = global_reg_id("list");
    size_t map_id = global_reg_id("map");
    size_t weakptr_id = global_reg_id("weakptr");
    size_t col_id = global_reg_id("col");
    
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
    size_t method_call_id = global_reg_id("METHOD_CALL");
    size_t function_id = global_reg_id("FUNCTION");
    size_t method_id = global_reg_id("METHOD");
    size_t func_decl_id = global_reg_id("FUNC_DECL");
    size_t type_decl_id = global_reg_id("TYPE_DECL");

    uint32_t sub_pass = global_reg_id("SUB_PASS");
    uint32_t direct_pass = global_reg_id("DIRECT_PASS");
    uint32_t travel_pass = global_reg_id("TRAVEL_PASS");
    uint32_t resolving_pass = global_reg_id("RESOLVING_PASS");

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
        inline void put(const std::string& key, T t) {DEBUG_ONLY(if(safety_check("col_ptr:put")){return;}) col().put(key, (void*)&t);}
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
        inline ColColCol& store_unit()         {Ptr& p = store_ptr(); return resolve_to_unit(p);}
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
            return resolve_to_col(dataptr).get(dataptr.sidx);
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

        inline void copy(Value o, bool is_deep) {
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

        bool running = true; 
        Context unit_ctx = deadptr;

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

        Unit() : types(global), uid(derive_uid(true)) {init();}
        Unit(const ColColCol& starter) : types(starter), uid(derive_uid(false)) {init();}
        Unit(bool do_not_init) {}

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
                    uspan->newline(active_stage->label+": "+node_basic_info(ctx.node()));
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
        

        map<uint32_t, std::string> labels;
        map<std::string,uint32_t> labels_lookup;
        map<uint32_t,_layout> layouts;
        map<std::string, g_ptr<Stage>> stages;
        uint32_t next_id = 0;
        uint16_t uid;

        Node unit_root = deadptr;
        std::string unit_label = "";

        ColColCol types;
        ColCol& operator[](uint16_t index) {return types[index];}

        virtual void init() {
           
        }
        virtual Node process(std::string path) {return deadptr;}
        virtual void run(Node root) {}

        inline Ptr get_ticket(uint32_t type_id, uint32_t size, uint32_t tag) {
            Ptr ticket(&types,type_id,create_column(types[type_id],size,tag,true),0);
            return ticket;
        }

        inline Ptr get_ticket(ColCol* pool, uint32_t size, uint32_t tag) {
            Ptr ticket(pool,create_column(*pool,size,tag,true),0);
            return ticket;
        }

        inline Ptr get_ticket(Ptr storeptr, uint32_t size, uint32_t tag) {
            if(storeptr.cachelevel==0) {
                Ptr ticket(storeptr.unit,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
                return ticket;
            } else {
                Ptr ticket(storeptr.cache,storeptr.pool,create_column(resolve_to_pool(storeptr),size,tag,true),0);
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
            if(ERROR_FLAG) {
                return red("ERROR_ACTIVE:"+Ptr_to_string(p));
            } else {
                if(p.cachelevel==0) {
                    if(p.unit>=units.length()) {
                        return red("UNIT_OUT_OF_BOUNDS:"+Ptr_to_string(p));
                    }
                } else {
                    if(!p.cache) return red("PTR_CACHE_MISSING");
                }
                if(p.pool>=resolve_to_unit(p).length()) {
                    return red("POOL_OUT_OF_BOUNDS("+std::to_string(resolve_to_unit(p).length())+"):"+Ptr_to_string(p));
                } else if(p.idx>=resolve_to_pool(p).length()) {
                    return red("IDX_OUT_OF_BOUNDS("+std::to_string(resolve_to_pool(p).length())+"):"+Ptr_to_string(p));
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
                return red(Ptr_to_string(p));
            }

            //ADD CACHE LEVELS HERE LATER!!!
            #if NAMED_PTRS
                std::string plabel = resolve_to_pool(p).label.empty()?std::to_string(p.pool):resolve_to_pool(p).label.to_std();
                std::string pidx = resolve_to_col(p).label.empty()?std::to_string(p.idx):resolve_to_col(p).label.to_std();
                std::string pstring = std::to_string(p.unit)+"|"+plabel+"|"+pidx+"|"+std::to_string(p.sidx)+"";
                uint64_t key = Ptr_to_key(p);
            
                if(ptr_colors.hasKey(key)) {ptr_colors.get(key)(pstring);}
                return pstring;
            #else
                return Ptr_to_string(p);
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
            Acorn::recycle_column(resolve_to_pool(p), p.idx);
        }
        
        void recycle_value(Value v, bool recycle_data = true) {
            if(is_live(v)&&resolve_to_col(v).live) {
                for(int i=0;i<v.quals().length();i++) {
                    recycle_node(v.quals()[i]);
                }
                recycle_column(v.quals_ptr());
    
                for(int i=0;i<v.sub_values().length();i++) {
                    recycle_value(v.sub_values()[i]);
                }
                recycle_column(v.sub_values_ptr());
                
                if(recycle_data) {
                    recycle_column(v.data_ptr());
                }
            }
        }
    
        //Recycles everything
        void recycle_node(Node n) {
            //print("Recycling: ",node_info(n));
            if(is_live(n)&&resolve_to_col(n).live) {
                //print("CHILDREN");
                //print("CHILDREN LEN: ",types[6].length());
                for(int i=0;i<n.children().length();i++) {
                    recycle_node(n.children()[i]);
                }
                //print("CHILDREN LEN: ",types[6].length());
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
            // recycle_column(ctx.result_ptr()); //Because this is often somebodies children
            // recycle_column(ctx.source_ptr());
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
                if(ptr.pool>=types.length()||ptr.idx>=types[ptr.pool].length()) {
                    return red("STRING ERROR "+std::to_string(ptr.pool)+"|"+std::to_string(ptr.idx)+"|"+std::to_string(ptr.sidx));
                }
                std::string content = string(ptr).to_std();
                return Ptr_as_string(ptr)+"> \""+escape_string(content)+"\"";
            } else if(tag==ptr_id) {
                return Ptr_to_string(*(Ptr*)data);
            } else if(tag==ptr_id||tag==node_id||tag==value_id||tag==context_id) {
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

        list<list<std::string>> type_to_lines(ColCol& t) {
            list<list<std::string>> lines;
            list<uint32_t> dtypes;
            for(int c=0;c<t.length();c++) {
                Col& col = t[c];
                list<std::string> subline;
                subline << col.label.to_std()+(col.live?"":" [FREE]");
                //print("Pushed label ",subline[0]);
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
                        //print("Line ",lines.length()," Subline ",subline.length());
                        //print("Row ",r," Column ",c," Tag ",labels[col.tag],"(",col.tag,")");
                        if(col.cells.length()>r) {
                            if(col.cells[r].tag==string_id) {
                                line += "["+((QString&)col.cells[r]).to_std()+"] ";
                            } else {
                                line += "["+labels[col.cells[r].tag]+"?] ";
                            }
                        }
                        line += tag_to_str(col.tag,col[r]);
                        //print("Result: ",line);
                        subline << line;
                    }
                }
                //print("Pushed ",subline.length()," sublines");
                lines << subline;
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
                    if(layouts.hasKey(col.tag)) {
                        _layout& l = layouts.get(col.tag);
                        for(int o=0;o<l.offsets.length();o++) {
                            std::string line = "";
                            line+=pad_str(l.labels[o]+": ",12);
                            line+=tag_to_str(l.tags[o],col.qget(l.offsets[o]));
                            subline << line;
                        }
                    } else {
                        print(red("core::TypeCol_to_lines unable to print heteregenous column of type "+labels[col.tag]+" because no layout was found"));
                    }
                } else {
                    for(int r=0;r<col.length();r++) {
                        std::string line = "";
                        if(col.cells.length()>r) {
                            if(col.cells[r].tag==string_id) {
                                line += "["+((QString&)col.cells[r]).to_std()+"] ";
                            } else {
                                line += "["+labels[col.cells[r].tag]+"?] ";
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

        void dump_pool(ColCol& pool, uint32_t index, bool clear_dump) {
            if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");
            std::string to_print = "";
            to_print += "TYPE "+std::to_string(index)+" "+pool.label.to_std()+(pool.tag!=0?" ["+labels[pool.tag]+"]":"")+":\n";
            to_print += type_to_string(pool);
            to_print += "\n\n\n";
            editTextFile("mixos-acorn/tests/printout.txt",[to_print](std::string& source){
                source+=to_print;
            });
        }

        void dump_unit(bool clear_dump) {
            if(clear_dump) writeFile("mixos-acorn/tests/printout.txt","");

            for(int t=0;t<types.length();t++) {
                std::string to_print = "";
                to_print += "TYPE "+std::to_string(t)+" "+types[t].label.to_std()+":\n";
                //print("PRINTING: ",t);
                to_print += type_to_string(types[t]);
                to_print += "\n\n\n";
                // print("COMMITING: ",t);
                // print("TEXT: ",to_print);
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
                    std::string tagstr = tag_to_str(value.type(),value.get());
                    if(tagstr.length()>50) tagstr = tagstr.substr(0,50); //Disable this to disable truncation of large values
                    to_return += ", value: "+gray(tagstr)+" @"+ptr_addr;
                }
                DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted to print info of "),cyan(Ptr_to_string(value)),red(" but the value was invalid")); return to_return;})
            }
            to_return += (value.sub_type()!=0?", sub_type: "+labels[value.sub_type()]:"")
            + (is_live(value.type_scope())?", type_scope: "+blue(Ptr_as_string(value.type_scope())):"")
            + (value.size()!=0?", size: "+std::to_string(value.size()):"")
            + (value.sub_size()!=0?", sub_size: "+std::to_string(value.sub_size()):"")
            + (value.address()!=0?", address: "+std::to_string(value.address()):"")
            + (value.loc()!=-1?", loc: "+std::to_string(value.loc()):"")
            + (is_live(value.store_ptr())?", store: "+Ptr_as_string(value.store_ptr()):"")
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

        std::string node_basic_info(Node node) {
            std::string to_return = labels[node.type()]+ (node.name().length()==0?"":" "+node.name().to_std()+" ");
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
                    to_return += "\n" + indent + "     Key: "+((QString&)cells[i]).to_std()+" | "+value_info(node.value_table().get(cells.get(i).index));
                }
            }
            if(node.node_table().length()>0) {
                to_return += "\n" + indent + "   Node table:";
                QCellCol cells = node.node_table().col().cells;
                for(int i=0;i<cells.length();i++) {
                    to_return += "\n" + indent + "     Key: "+((QString&)cells[i]).to_std()+" | "+node_info(node.node_table().get(cells.get(i).index));
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
    
        Stage& reg_stage(std::string label) {
            g_ptr<Stage> new_stage = make<Stage>();
            new_stage->label = label;
            stages.put(label,new_stage);
            types[handler_type_id][stages_id].put(label,(void*)&deadptr);
            return *new_stage.getPtr();
        }

        map<uint32_t,Handler> value_printers; 

        std::string value_as_string(Value v) {
            Context ctx = make_context(); ctx.value(v);
            if(value_printers.hasKey(v.type())) {
                value_printers[v.type()](ctx);
            } else {
                return labels[v.type()]+"?";
            }
            std::string str = ctx.source().to_std();
            deep_recycle_context(ctx);
            return str;
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

        uint32_t standard_travel_pass(Node root, Context sub = deadptr);
        uint32_t resume_travel_pass(Context ctx);

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

        void standard_resolve_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            standard_resolving_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_resolve_child_scopes(child);
            }
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
                standard_resolve_child_scopes(ctx.result()[i]); //Processing closures and such, any child containing it's own scopes
                if(!ctx.result()[i].scopes().empty()) {
                    for(int s = 0;s<ctx.result()[i].scopes().length();s++) {
                        if(ctx.result()[i].scopes()[s].owner()==ctx.result()[i]) {
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

        void standard_backwards_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            standard_backwards_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_backwards_child_scopes(child);
            }
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
                standard_backwards_child_scopes(ctx.result().get(i));
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

        void standard_memory_backwards_child_scopes(Node node) {
            for(int c = 0; c < node.children().length(); c++) {
                Node child = node.children()[c];
                if(!child.scopes().empty()) {
                    for(int s = 0; s < child.scopes().length(); s++) {
                        if(child.scopes()[s].owner()==child) {
                            memory_backwards_pass(child.scopes()[s]);
                        }
                    }
                }
                standard_memory_backwards_child_scopes(child);
            }
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
                standard_memory_backwards_child_scopes(ctx.result().get(i));
                node_col scopes = ctx.result().get(i).scopes();
                for(int s = 0; s<scopes.length(); s++) {
                    if(is_live(scopes.get(s).owner())&&scopes.get(s).owner()==ctx.result().get(i)) {
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
            NORM_IDS = 0
        };
    
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

                    default:
                    print(red("core:write_normalize unrecognized pass: "+std::to_string(passes[p])));
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

    inline uint16_t make_unit(const ColColCol& starter) {
        g_ptr<Unit> u = make<Unit>(starter);
        return u->uid;
    }

   
    inline ColColCol& resolve_to_unit(const Ptr& ptr) {
        switch(ptr.cachelevel) {
            case 0: {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit]).types;}
            case 3: return *(ColColCol*)ptr.cache;
            default: 
                throw_error("Can not resolve Ptr ",Ptr_to_string(ptr)," to unit because it's cachelevel ",(uint32_t)ptr.cachelevel," is too low");
            return col3_ref;
        }
    }
    // inline void* resolve_ptr(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx][ptr.sidx];}
    // inline void* resolve_ptr(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    // inline Ptr& resolve_to_ptr(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return *(Ptr*)(*units[ptr.unit])[ptr.pool][ptr.idx].get(ptr.sidx);}
    // inline Ptr& resolve_to_ptr(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return *(Ptr*)(*units[ptr.unit])[ptr.pool][idx].get(ptr.sidx);}
    // inline Col& resolve_to_col(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx];}
    // inline Col& resolve_to_col(const Ptr& ptr, const uint32_t& idx) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][idx];}
    // inline ColCol& resolve_to_pool(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool];}
    // inline ColColCol& resolve_to_unit(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit]).types;}
    // inline Col& to_col(const Ptr& ptr) {std::lock_guard<std::mutex> lock(units_mutex); return (*units[ptr.unit])[ptr.pool][ptr.idx];}

    inline Ptr get_ticket_from_unit(Ptr p, uint32_t type_id, uint32_t size, uint32_t tag) {
        if(p.cachelevel==3) {
            Ptr ticket(p.cache,type_id,create_column(resolve_to_unit(p)[type_id],size,tag,true),0);
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

    //Returns true if flagged for a return/break
    uint32_t Unit::standard_travel_pass(Node root, Context sub) {
        DEBUG_ONLY(if(ERROR_FLAG) {log(red("Attempted a travel pass while an error was flagged")); return true;})
        node_col children = root.children();
        newline("Travel pass over "+std::to_string(children.length())+" nodes");
        Context ctx = make_context(children,is_live(sub)?sub.source_ptr():deadptr,travel_pass,unit_ctx);
        ctx.root(root);
        ctx.sub(sub);
        unit_ctx = ctx;
        DEBUG_ONLY(for(auto& w : watchers) {if(w.passstart) w.passstart(ctx);})
        while(unit_ctx.index() < unit_ctx.result().length()) {
            unit_ctx.node(unit_ctx.result().get(unit_ctx.index()));
            standard_process(unit_ctx);
            Node nleft = unit_ctx.result().get(unit_ctx.index());
            unit_ctx.left(nleft);
            DEBUG_ONLY(if(ERROR_FLAG) {endline(); return true;})
            if(unit_ctx.state()>0) { //This is the return/break process
                DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
                endline();
                uint32_t state = unit_ctx.state();
                unit_ctx = unit_ctx.parent();
                deep_recycle_context(ctx);
                return state;
            }
            unit_ctx.index()++;
        }
        DEBUG_ONLY(for(auto& w : watchers) {if(w.suffix) w.suffix(ctx);})
        endline();
        unit_ctx = unit_ctx.parent();
        deep_recycle_context(ctx);
        return 0;
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
        return u->types;
    }
}