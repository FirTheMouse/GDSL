#pragma once

#include "../core/object.hpp"
#include "../util/util.hpp"
#include "../util/logger.hpp"

struct value_ {
    value_(size_t _size, std::string _name) 
    : size(_size), name(_name) {}
    value_() {}
    size_t size;
    std::string name;
};

struct _note {
    _note() {}
    _note(int _index, size_t _size) : index(_index), size(_size) {}
    _note(int _index, size_t _size, int _sub_index) 
        : index(_index), size(_size), sub_index(_sub_index) {}
    
    // Copy constructor - deep copy the fallback
    _note(const _note& other) 
        : index(other.index), sub_index(other.sub_index), size(other.size) {
        if(other.fallback) {
            fallback = new std::any(*other.fallback);
        }
    }
    
    // Move constructor - steal the pointer
    _note(_note&& other) noexcept
        : index(other.index), sub_index(other.sub_index), 
          size(other.size), fallback(other.fallback) {
        other.fallback = nullptr;  // Don't delete when other dies
    }
    
    // Copy assignment
    _note& operator=(const _note& other) {
        if(this != &other) {
            if(fallback) delete fallback;
            index = other.index;
            sub_index = other.sub_index;
            size = other.size;
            if(other.fallback) {
                fallback = new std::any(*other.fallback);
            } else {
                fallback = nullptr;
            }
        }
        return *this;
    }
    
    // Move assignment
    _note& operator=(_note&& other) noexcept {
        if(this != &other) {
            if(fallback) delete fallback;
            index = other.index;
            sub_index = other.sub_index;
            size = other.size;
            fallback = other.fallback;
            other.fallback = nullptr;
        }
        return *this;
    }
    
    ~_note() {
        if(fallback) delete fallback;
    }
    
    int index = -1;
    int sub_index = -1;
    size_t size = 0;
    std::any* fallback = nullptr;
};

static _note note_fallback;

struct byte16_t { uint8_t data[16]; };
struct byte24_t { uint8_t data[24]; };
struct byte32_t { uint8_t data[32]; };
struct byte64_t { uint8_t data[64]; };
struct _fallback_list {
    _fallback_list(size_t _size = 1) : component_size(_size) {}
    size_t component_size;
    list<uint8_t> storage;
    
    void* get(size_t index) {
        return &storage[index * component_size];
    }

    size_t length() {
       return storage.length() / component_size;
    }
    
    void push(const void* component) {
        size_t old_size = storage.size();
        size_t new_size = old_size + component_size;
        
        if (new_size > storage.capacity()) {
            storage.reserve(std::max(new_size, storage.capacity() * 2));
        }
        storage.resize(new_size);
        memcpy(&storage[old_size], component, component_size);
    }
};

class Type : public q_object {
private:
  
public:
    Type() {
        free_stack_top.store(0);
    }

    ~Type() {}

    list<g_ptr<Object>> objects;

    //Consider making notes be uint32_t,_note, and have all the puts pre-hash, this will allow us to do enum notes and avoid the string stuff
    map<std::string,_note> notes; // Where reflection info is stored
    list<_note> array; //Ordered array for usage of Type as a MultiArray
    map<std::string,std::any> fallback_map; //Fallback for non POD types, old Data style

    size_t sizes[8] = {1,2,4,8,16,24,32,64};
    list<list<uint8_t>> byte1_columns;   // bool, char (1 byte)
    list<list<uint16_t>> byte2_columns;  // uint16_t (2 bytes)
    list<list<uint32_t>> byte4_columns;  // int, float (4 bytes)  
    list<list<uint64_t>> byte8_columns;  // double, pointers (8 bytes)
    list<list<byte16_t>> byte16_columns; // vec4, padded vec3 (16 bytes)
    list<list<byte24_t>> byte24_columns; // strings, list, medium objects (24 bytes)
    list<list<byte32_t>> byte32_columns; // strings on some systems (32 bytes)
    list<list<byte64_t>> byte64_columns; // mat4, large objects (64 bytes)
    map<size_t,list<_fallback_list>> fallback_columns;

    //The _fallback_list strategy, while vastly less performant than the pointer strategy for allocating >64 byte obejcts
    //should have better memory saftey, if in the future leaks and memory saftey become an issue, consdier switching to using it


    size_t next_size(size_t size) {
        size_t toReturn = 0;
        for(int i=0;i<8;i++) {
            if(sizes[i]>=size) {
                toReturn = sizes[i];
                break;
            }
        }
        return toReturn;
    }


    template<typename T>
    std::string make_table_from_size(const T& columns,size_t size,int mode) {
        std::string table = "";
        if(columns.length()==0) return table;
        int lr = -1; //Longest length
        for(int c=0;c<columns.length();c++) {
            int l = columns[c].length();
            if(l>lr) lr = l;
        }

        if(mode==1) { //Auto
            if(columns.length()<10) mode = 2;
            else mode = 3;
        }
        if(mode==2) //Full
        {
            std::string header = "COL: ";
            for(int c=0;c<columns.length();c++) {
                header.append(std::to_string(c)+":"+std::to_string((uintptr_t)&columns[c]).append(" ").insert(5,"-").insert(8,"-"));
            }
            table.append(header+"\n");
            for(int r=-1;r<lr;r++) {
                std::string row = "";
                if(r==-1) {
                    std::string size_s = std::to_string(size);
                    row.append(size_s);
                    if(size_s.length()==1) row.append("   ");
                    if(size_s.length()==2) row.append("  ");
                    if(size_s.length()==3) row.append(" ");
                    for(int c=0;c<columns.length();c++) {
                        if(c<10)
                            row.append("  -------------");
                        else
                            row.append("   -------------");
                    }
                } else {
                    if(r<10) row.append("   ");
                    else if(r<100) row.append("  ");
                    row.append(std::to_string(r)+":");
                    for(int c=0;c<columns.length();c++) {
                        if(r<columns[c].length())
                            row.append("  "+std::to_string((uintptr_t)&columns[c][r]).append(" ").insert(5,"-").insert(8,"-"));
                        else
                            row.append("             ");
                    }
                }
                table.append(row+(r!=lr-1?"\n":""));
            }
        }
        else if(mode==3) //Compact
        {
            std::string header = "COL: ";
            for(int c=0;c<columns.length();c++) {
                header.append(std::to_string(c)+" ");
            }
            table.append(header+"\n");

            for(int r=-1;r<lr;r++) {
                std::string row = "";
                if(r==-1) {
                    std::string size_s = std::to_string(size);
                    row.append(size_s);
                    if(size_s.length()==1) row.append("   ");
                    if(size_s.length()==2) row.append("  ");
                    if(size_s.length()==3) row.append(" ");
                    for(int c=0;c<columns.length();c++) {
                        if(c<10)
                            row.append(" =");
                        else
                            row.append(" ==");
                    }
                } else {
                if(r<10) row.append("  ");
                else if(r<100) row.append(" ");
                row.append(std::to_string(r)+":");
                for(int c=0;c<columns.length();c++) {
                    if(r<columns[c].length())
                        row.append(" O");
                    else
                        row.append("  ");
                }
                }
                table.append(row+(r!=lr-1?"\n":""));
            }
        }
        else if(mode==4) {
            list<list<std::string>> ids(columns.length());
            list<int> ids_max_size(columns.length());
            list<std::string> headers(columns.length());
            for(int i=0;i<columns.length();i++) {
                ids_max_size[i] = 0;
                list<std::string> sl(lr);
                for(int r=0;r<lr;r++) sl[r] = "";
                ids[i] = sl;
                headers[i] = "";
            }
            if(notes.size()!=0) { //Do we have map entries?
                for(auto e : notes.entrySet()) {
                    if(e.value.size==size) {
                        if(e.value.sub_index!=-1) {
                            ids[e.value.index][e.value.sub_index] = e.key;
                            if(e.key.length()>ids_max_size[e.value.index]) {
                                ids_max_size[e.value.index] = e.key.length();
                            }
                        } else {
                            headers[e.value.index] = e.key;
                            if(e.key.length()>ids_max_size[e.value.index]) {
                                ids_max_size[e.value.index] = e.key.length();
                            }
                        }
                    }
                }
            }
            if(array.size()!=0) { //Do we have array entires?
                for(int a=0;a<array.length();a++) {
                    if(ids[array[a].index][array[a].sub_index]=="") {
                        std::string s_of = std::to_string(a);
                        ids[array[a].index][array[a].sub_index] = s_of;
                        if(s_of.length()>ids_max_size[array[a].index]) {
                            ids_max_size[array[a].index] = s_of.length();
                        }
                    } 
                }
            }
            if(objects.size()!=0) { //Do we have objects?
                for(int o=0;o<objects.length();o++) {
                    int rid = objects[o]->TID;
                    for(int c=0;c<columns.length();c++) {
                        if(ids[c][rid]=="") {
                            std::string s_of = type_name+":"+std::to_string(rid);
                            ids[c][rid] = s_of;
                            if(s_of.length()>ids_max_size[c]) {
                                ids_max_size[c] = s_of.length();
                            }
                        } else {
                            std::string s_of = std::to_string(rid)+":"+ids[c][rid];
                            ids[c][rid] = s_of;
                            if(s_of.length()>ids_max_size[c]) {
                                ids_max_size[c] = s_of.length();
                            }
                        }
                    }
                }
            }
            std::string header = "COL: ";
            for(int c=0;c<columns.length();c++) {
                std::string c_h = headers[c]==""?std::to_string(c):headers[c];
                header.append(c_h+" ");
                for(int i = 0;i<ids_max_size[c]-c_h.length();i++) {
                    header.append(" ");
                }
            }
            table.append(header+"\n");

            for(int r=-1;r<lr;r++) {
                std::string row = "";
                if(r==-1) {
                    std::string size_s = std::to_string(size);
                    row.append(size_s);
                    if(size_s.length()==1) row.append("   ");
                    if(size_s.length()==2) row.append("  ");
                    if(size_s.length()==3) row.append(" ");
                    for(int c=0;c<columns.length();c++) {
                        row.append(" ");
                        for(int i = 0;i<ids_max_size[c]+1-std::to_string(c).length();i++) {
                            row.append("=");
                        }
                    }
                } else {
                if(r<10) row.append("  ");
                else if(r<100) row.append(" ");
                row.append(std::to_string(r)+":");
                for(int c=0;c<columns.length();c++) {
                    if(ids[c][r]!="") {
                        for(int i = 0;i<ids_max_size[c]-ids[c][r].length();i++)
                            row.append(" ");
                        row.append(" "+ids[c][r]);
                    }
                    else {
                        row.append(" ");
                        for(int i = 0;i<ids_max_size[c];i++) 
                            row.append(" ");
                    }
                }
                }
                table.append(row+(r!=lr-1?"\n":""));
            }

            
        }
        return table;
    }

    //Modes: 1 == Auto, 2 == Full, 3 == Compact, 4 == Pretty
    std::string table_to_string(size_t size,int mode = 1) {
        switch(size) {
            case 1: return make_table_from_size<list<list<uint8_t>>>(byte1_columns,1,mode);
            case 2: return make_table_from_size<list<list<uint16_t>>>(byte2_columns,2,mode);
            case 4: return make_table_from_size<list<list<uint32_t>>>(byte4_columns,4,mode);
            case 8: return make_table_from_size<list<list<uint64_t>>>(byte8_columns,8,mode);
            case 16: return make_table_from_size<list<list<byte16_t>>>(byte16_columns,16,mode);
            case 24: return make_table_from_size<list<list<byte24_t>>>(byte24_columns,24,mode);
            case 32: return make_table_from_size<list<list<byte32_t>>>(byte32_columns,32,mode);
            case 64: return make_table_from_size<list<list<byte64_t>>>(byte64_columns,64,mode);
            default: 
                return "print_column::84 invalid size for print "+std::to_string(size);
            break;
        }
    }

    std::string type_to_string(int mode = 1) {
        std::string result = "";
        for(int i=0;i<8;i++) {
            std::string table = table_to_string(sizes[i],mode);
            if(table!="")
                result.append(table_to_string(sizes[i],mode)+"\n");
        }
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return result;
    }

    

    // for(int r=0;r<byte4_columns.length();r++) {
    //     std::string row = std::to_string(r)+":";
    //     for(int c=0;c<byte4_columns[r].length();c++) {
    //         row.append(" "+std::to_string(c));
    //     }
    //     column.append(row+(r!=byte4_columns.length()-1?"\n":""));
    // }

    //Consider splitting the stratgeies into their own types of Type via inhereitence, I'm doing it manual right now because I prefer
    //composition, but it may be better to have all this in constructers and private methods.


    // Returns the ammount of rows in a column
    size_t column_length(size_t size) {
        switch(size) {
            case 1: return byte1_columns.length();
            case 2: return byte2_columns.length();
            case 4: return byte4_columns.length();
            case 8: return byte8_columns.length();
            case 16: return byte16_columns.length();
            case 24: return byte24_columns.length();
            case 32: return byte32_columns.length();
            case 64: return byte64_columns.length();
            default: 
            size = next_size(size);
            if(size==0) {
                print("column_length::414 size is too large ");
                return 0;
            } 
            else 
                return column_length(size);
        }
    }

    //Returns the amount of places in a row
    size_t row_length(int index,size_t size) {
        switch(size) {
            case 1: return byte1_columns[index].length();
            case 2: return byte2_columns[index].length();
            case 4: return byte4_columns[index].length();
            case 8: return byte8_columns[index].length();
            case 16: return byte16_columns[index].length();
            case 24: return byte24_columns[index].length();
            case 32: return byte32_columns[index].length();
            case 64: return byte64_columns[index].length();
            default:
            size = next_size(size);
            if(size==0) {
                print("row_length::436 size is too large ");
                return 0;
            } 
            else 
                return row_length(index,size);
        }
    }

    
    //Adds a new place to all the rows in a column
    void add_rows(size_t size = 0) {
        switch(size) {
        case 0:
            for(int i=0;i<8;i++) add_rows(sizes[i]);
        break;
        case 1:
            for(auto& list : byte1_columns) {
                list.push(uint8_t{}); 
            }
        break;
        case 2:
        for(auto& list : byte2_columns) {
            list.push(uint16_t{}); 
        }
        break;
        case 4:
            for(auto& list : byte4_columns) {
                list.push(uint32_t{});   
            }
        break;
        case 8:
            for(auto& list : byte8_columns) {
                list.push(uint64_t{}); 
            }
        break;
        case 16:
            for(auto& list : byte16_columns) {
                list.push(byte16_t{});  
            }
        break;
        case 24:
            for(auto& list : byte24_columns) {
                list.push(byte24_t{});  
            }
        break;
        case 32:
        for(auto& list : byte32_columns) {
            list.push(byte32_t{});  
        }
        break;
        case 64:
            for(auto& list : byte64_columns) {
                list.push(byte64_t{});  
            }
        break;
        default: 
            size = next_size(size);
            if(size==0) {
                print("add_rows::331 size is too large "); 
            } 
            else 
                add_rows(size);
        break;
        }
    }

    // Adds a new place to the indicated row in a column
    void add_row(int index, size_t size = 0) {
        while(column_length(size)<=index) add_column(size);
        switch(size) {
        case 0: for(int i=0;i<8;i++) add_row(index,sizes[i]); break;
        case 1:byte1_columns[index].push(uint8_t{}); break;
        case 2: byte2_columns[index].push(uint16_t{}); break;
        case 4: byte4_columns[index].push(uint32_t{}); break;
        case 8: byte8_columns[index].push(uint64_t{}); break;
        case 16: byte16_columns[index].push(byte16_t{}); break;
        case 24: byte24_columns[index].push(byte24_t{}); break;
        case 32: byte32_columns[index].push(byte32_t{}); break;
        case 64: byte64_columns[index].push(byte64_t{}); break;
        default: size = next_size(size); if(size!=0) {add_row(size);} break;
        }
    }
    
    //Adds a new column and intilizes the rows
    void add_column(size_t size = 0) {
        size_t default_rows = objects.size();  
        
        switch(size) {
        case 0:
            for(int i=0;i<8;i++) add_column(sizes[i]);
        break;
        case 1: {
            list<uint8_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = uint8_t{};
            byte1_columns.push(col);
        }
        break;
        case 2: {
            list<uint16_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = uint16_t{};
            byte2_columns.push(col);
        }
        break;
        case 4: {
            list<uint32_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = uint32_t{};
            byte4_columns.push(col);
        }
        break;
        case 8: {
            list<uint64_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = uint64_t{};
            byte8_columns.push(col);
        }
        break;
        case 16: {
            list<byte16_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = byte16_t{};
            byte16_columns.push(col);
        }
        break;
        case 24: {
            list<byte24_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = byte24_t{};
            byte24_columns.push(col);
        }
        break;
        case 32: {
            list<byte32_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = byte32_t{};
            byte32_columns.push(col);
        }
        break;
        case 64: {
            list<byte64_t> col(default_rows);
            for(size_t i = 0; i < default_rows; i++) col[i] = byte64_t{};
            byte64_columns.push(col);
        }
        break;
        default: 
            size_t o_size = next_size(size);
            if(o_size==0) {
                print("add_column::406 size is too large: ",size); 
            } 
            else 
                add_column(o_size);
         break;
        }
    }

    template<typename T>
    void make_space_for(int ammount) {
        add_row(ammount,sizeof(T));
    }


    inline bool has(int index) {return index<array.length();}
    inline bool has(const std::string& label) {return notes.hasKey(label);}
    bool validate(list<std::string> check) {
        for(auto c : check) {
            if(!notes.hasKey(c)) {
                print("validate::470 type missing ",c);
                return false;
            }
        }
        return true;
    }
    //Maybe add some more explcitness to validate, like listing *what* is actually missing

    _note& get_note(const std::string& label) {
        return notes.getOrDefault(label,note_fallback);
    }
    _note& get_note(int index) {
        return array[index];
    }

    /// @brief For use in the MAP strategy
    void note_value(const std::string& name, size_t size) {
        size_t o_size = next_size(size);
        if(o_size==0) {
            note_value(name,8);
        } 
        else {
        _note note(column_length(size),size,0); add_column(size); notes.put(name,note);
        }
    }
    template<typename T>
    void note_value(const std::string& name) {
        note_value(name,sizeof(T));
    }

    // void note_value(const std::string& name, size_t size,int t = 0) {
    //     while(column_length(size)<=t) {add_column(size);}
    //     _note note(column_length(size),size,row_length(t,size)); 
    //     add_row(t,size); 
    //     notes.put(name,note);
    //     array.push(note);
    // }

    /// @brief For use in the MAP strategy
    void* address_column(const std::string& name) {
        _note note = notes.getOrDefault(name,note_fallback);
        if(note.index==-1) return nullptr;
        switch(note.size) {
            case 0: return nullptr; //print("address_column::175 Note not found for ",name); 
            case 1: return &byte1_columns[note.index];
            case 2: return &byte2_columns[note.index];
            case 4: return &byte4_columns[note.index];
            case 8: return &byte8_columns[note.index];
            case 16: return &byte16_columns[note.index];
            case 24: return &byte24_columns[note.index];
            case 32: return &byte32_columns[note.index];
            case 64: return &byte64_columns[note.index];
            default: print("address_column::180 Invalid note size ",note.size); return nullptr;
        }
    }

 


    inline static void* get(void* ptr, size_t index, size_t size) {        
        switch(size) {
            case 1: return &(*(list<uint8_t>*)ptr)[index];
            case 2: return &(*(list<uint16_t>*)ptr)[index];
            case 4: return &(*(list<uint32_t>*)ptr)[index]; 
            case 8: return &(*(list<uint64_t>*)ptr)[index];
            case 16: return &(*(list<byte16_t>*)ptr)[index].data;
            case 24: return &(*(list<byte24_t>*)ptr)[index].data;
            case 32: return &(*(list<byte32_t>*)ptr)[index].data;
            case 64: return &(*(list<byte64_t>*)ptr)[index].data;
            default: 
                print("type::get::312 Bad size for type value: ",size);
            return nullptr;
        }
    }

    void* get(const std::string& label, size_t index, size_t size) {
        void* ptr = address_column(label);
        if (!ptr) return nullptr;
        return get(ptr,index,size);
    }
    
    template<typename T>
    T& get(const std::string& label, size_t index) {
        size_t size = sizeof(T);
        void* data_ptr = get(label, index, size);
        if (!data_ptr) print("get::270 value not found");
        
        return *(T*)data_ptr;
    }

    template<typename T>
    T& get(void* ptr, size_t index) {
        size_t size = sizeof(T);
        void* data_ptr = get(ptr, index, size);
        if (!data_ptr) print("get::285 value not found");
        return *(T*)data_ptr;
    }

    //We're keeping these methods verbose for performance, drawing out every possible case means less indirection from method calls and switches

    /// @brief for use in the DATA strategy
    void push(const std::string& name,void* value,size_t size,int t = 0) {
        switch(size) {
            case 1: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,1,byte1_columns[t].length()); byte1_columns[t].push(*(uint8_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 2: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,2,byte2_columns[t].length()); byte2_columns[t].push(*(uint16_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 4: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,4,byte4_columns[t].length()); byte4_columns[t].push(*(uint32_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 8: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,8,byte8_columns[t].length()); byte8_columns[t].push(*(uint64_t*)value);  notes.put(name,note); array<<note;
            }
            break;
            case 16: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,16,byte16_columns[t].length()); byte16_columns[t].push(*(byte16_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 24: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,24,byte24_columns[t].length()); byte24_columns[t].push(*(byte24_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 32: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,32,byte32_columns[t].length()); byte32_columns[t].push(*(byte32_t*)value); notes.put(name,note); array<<note;
            }
            break;
            case 64: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,64,byte64_columns[t].length()); byte64_columns[t].push(*(byte64_t*)value);  notes.put(name,note); array<<note;
            }
            break;
            default: 
            size = next_size(size);
            if(size==0) {
                //ptr fallback
            } else push(name,value,size,t);
            break;
        }
    }

    void add(const std::string& name, void* value,size_t size,int t) {
        push(name,value,size,t);
    }

    template<typename T>
    void add(const std::string& name,T value,int t = 0) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            add(name,&value,sizeof(T),t);
        } else {
            _note note(-1, 0, -1);
            note.fallback = new std::any(value);  // Allocate and store
            array << note;
            fallback_map.put(name,std::any(value));
        }
    }

    static void set(void* ptr,void* value,size_t index,size_t size) {
        // print("S: ",size," I: ",index);
        // print("L: ",(*(list<uint32_t>*)ptr).length());
        switch(size) {
            case 1: memcpy(&(*(list<uint8_t>*)ptr)[index], value, 1); break;
            case 2: memcpy(&(*(list<uint16_t>*)ptr)[index], value, 2); break;
            case 4: memcpy(&(*(list<uint32_t>*)ptr)[index], value, 4); break;
            case 8: memcpy(&(*(list<uint64_t>*)ptr)[index], value, 8); break;
            case 16: memcpy(&(*(list<byte16_t>*)ptr)[index].data, value, 16); break;
            case 24: memcpy(&(*(list<byte24_t>*)ptr)[index].data, value, 24); break;
            case 32: memcpy(&(*(list<byte32_t>*)ptr)[index].data, value, 32); break;
            case 64: memcpy(&(*(list<byte64_t>*)ptr)[index].data, value, 64); break;
            default: print("type::set::280 Bad size for type value: ",size); break;
        }
    }

    void set(const std::string& label,void* value,size_t index,size_t size) {
        void* ptr = address_column(label);
        if (!ptr) {
            //WARNING! Won't work for non-trivially copyable types!
            push(label,value,size);
            return;
        }
        return set(ptr,value,index,size);
    }

    template<typename T>
    void set(const std::string& label,T value,size_t index) {
        size_t size = sizeof(T);
        set(label,&value,index,size);
    }

    inline void* get(int index,int sub_index,size_t size) {
        switch(size) {
            case 0: return nullptr;
            case 1: return &byte1_columns[index][sub_index];
            case 2: return &byte2_columns[index][sub_index];
            case 4: return &byte4_columns[index][sub_index];
            case 8: return &byte8_columns[index][sub_index];
            case 16: return &byte16_columns[index][sub_index];
            case 24: return &byte24_columns[index][sub_index];
            case 32: return &byte32_columns[index][sub_index];
            case 64: return &byte64_columns[index][sub_index];
            default: 
                size_t o_size = next_size(size);
                if(o_size==0) {
                    //print("get::633 Pointer fallback triggered");
                    return *(void**)&byte8_columns[index][sub_index];
                    //return fallback_columns.get(size)[index].get(sub_index);
                } 
                else 
                    return get(index,sub_index,o_size);
        }
    }

    void* get_from_note(const _note& note) {
        return get(note.index,note.sub_index,note.size);
    }

    void* data_get(const std::string& name) {
        return get_from_note(notes.getOrDefault(name,note_fallback));
    }

    void* data_get_at(const std::string& name, int sub_index) {
        _note& note = notes.getOrDefault(name,note_fallback);
        return get(note.index,sub_index,note.size);
    }

    void* array_get(int index) {
        return get_from_note(array[index]);
    }

   //Uses data get
   template<typename T>
   T& get(const std::string& label) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return *(T*)data_get(label);
    } else {
        std::any& a = fallback_map.get(label);
        return std::any_cast<T&>(a);
    }
   }

   //Uses array get
   template<typename T>
   T& get(int index) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return *(T*)array_get(index);
    } else {
        return std::any_cast<T&>(*array[index].fallback);
    }
   }

   template<typename T>
   T& get(int index,int sub_index) {
    return *(T*)get(index,sub_index,sizeof(T));
   }

   void set(int index,int sub_index,size_t size,void* value) {
    switch(size) {
        case 1: memcpy(&(byte1_columns[index])[sub_index], value, 1); break;
        case 2: memcpy(&(byte2_columns[index])[sub_index], value, 2); break;
        case 4: memcpy(&(byte4_columns[index])[sub_index], value, 4); break;
        case 8: memcpy(&(byte8_columns[index])[sub_index], value, 8); break;
        case 16: memcpy(&(byte16_columns[index])[sub_index], value, 16); break;
        case 24: memcpy(&(byte24_columns[index])[sub_index], value, 24); break;
        case 32: memcpy(&(byte32_columns[index])[sub_index], value, 32); break;
        case 64: memcpy(&(byte64_columns[index])[sub_index], value, 64); break;
        default: 
            size = next_size(size);
            if(size==0) {
                print("set::670 Pointer fallback triggered");
                memcpy(&(byte8_columns[index])[sub_index],&value, 8);
            } 
            else set(index,sub_index,size,value);
        break;
    }
    }

   void set_from_note(const _note& note,void* value) {
        set(note.index,note.sub_index,note.size,value);
    }

    template<typename T>
    void set(const std::string& label,T value) {
        _note& note = notes.getOrDefault(label,note_fallback);
        if(note.index==-1) {
            add<T>(label,value);
        } else {
            size_t size = sizeof(T);
            set_from_note(note,&value);
        }
    }

    void data_set(const std::string& name,void* value) {
        set_from_note(get_note(name),value);
    }

    void array_set(int index,void* value) {
        set_from_note(array[index],value);
    }

    //Uses data set
    // template<typename T>
    // void set(const std::string& label,T value) {
    //  data_set(label,&value);
    // }

    //Uses array set
    template<typename T>
    void set(int index,T value) {
     array_set(index,&value);
    }

    //Directly sets the value of a place
    template<typename T>
    void set(int index,int sub_index,T value) {
     set(index,sub_index,sizeof(T),&value);
    }
    
    /// @brief For use in the ARRAY strategy
    void push(void* value, size_t size,int t = 0) {
        switch(size) {
            case 1: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,1,byte1_columns[t].length()); byte1_columns[t].push(*(uint8_t*)value); array<<note;
            }
            break;
            case 2: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,2,byte2_columns[t].length()); byte2_columns[t].push(*(uint16_t*)value); array<<note;
            }
            break;
            case 4: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,4,byte4_columns[t].length()); byte4_columns[t].push(*(uint32_t*)value); array<<note;
            }
            break;
            case 8: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,8,byte8_columns[t].length()); byte8_columns[t].push(*(uint64_t*)value); array<<note;
            }
            break;
            case 16: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,16,byte16_columns[t].length()); byte16_columns[t].push(*(byte16_t*)value); array<<note;
            }
            break;
            case 24: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,24,byte24_columns[t].length()); byte24_columns[t].push(*(byte24_t*)value); array<<note;
            }
            break;
            case 32: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,32,byte32_columns[t].length()); byte32_columns[t].push(*(byte32_t*)value); array<<note;
            }
            break;
            case 64: 
            {
                while(column_length(size)<=t) {add_column(size);}
                _note note(t,64,byte64_columns[t].length()); byte64_columns[t].push(*(byte64_t*)value); array<<note;
            }
            break;
            default: 
                size_t o_size = next_size(size);
                if(o_size==0) {
                    //print("push::540 Pointer fallback triggered");
                    while(column_length(8)<=t) {add_column(8);}
                    _note note(t,size,byte8_columns[t].length()); byte8_columns[t].push(*(uint64_t*)&value); array<<note;

                    // list<_fallback_list>& fallback = fallback_columns.getOrPut(size,list<_fallback_list>{});
                    // while(fallback.length()<=t) {fallback.push(_fallback_list{size});}
                    // _note note(t,size,fallback[t].length()); fallback[t].push(value); array<<note;
                } 
                else push(value,o_size,t);
            break;
        }
    }

    /// @brief For use in the ARRAY strategy
    template<typename T>
    void push(T value, int t = 0) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            push(&value, sizeof(T), t);
        } else {
            _note note(-1, 0, -1);
            note.fallback = new std::any(value);  // Allocate and store
            array << note;
        }
    }

    std::string type_name = "bullets";
    list< std::function<void(g_ptr<Object>)> > init_funcs;
    list<int> free_ids;
    std::atomic<size_t> free_stack_top{0};  // Points to next free slot

    std::function<g_ptr<Object>()> make_func = [](){
        auto object = make<Object>();
        return object;
    };

    int get_next() {
        size_t current_top, new_top;
        do {
            current_top = free_stack_top.load();
            if (current_top == 0) return -1;
            new_top = current_top - 1;
        } while (!free_stack_top.compare_exchange_weak(current_top, new_top));
        
        return free_ids.get(new_top);
    }
    
    void return_id(int id) {
        size_t current_top = free_stack_top.load();
        if (current_top < free_ids.size()) {
            // There's space, just write and increment pointer
            size_t slot = free_stack_top.fetch_add(1);
            free_ids.get(slot) = id;
        } else {
            // Need to grow the list
            free_ids.push(id);
            free_stack_top.fetch_add(1);
        }
    }
    

    g_ptr<Object> create() {
        int next_id = get_next();
        g_ptr<Object> object = nullptr;
        if(next_id!=-1)
        {
            object = objects.get(next_id);
            for(int i=0;i<init_funcs.size();i++) {
                init_funcs[i](object);
            }
        }
        else
        {
            object = make_func();
            object->type_ = this; //May want to move this into the makeFunc to give more user control
            for(int i=0;i<init_funcs.size();i++) {
                init_funcs[i](object);
            }
            store(object);
        }
        reactivate(object);
        object->recycled.store(false);
        return object;
    }

    private:
    void store(g_ptr<Object> object)
    {
        object->TID = objects.size();
        add_rows();
        objects.push(object);
    }
    public:

    void add_initializers(list<std::function<void(g_ptr<Object>)>> inits) {for(auto i : inits) add_initializer(i);}
    void add_initializer(std::function<void(g_ptr<Object>)> init) {init_funcs << init;}
    void operator+(std::function<void(g_ptr<Object>)> init) {add_initializer(init);}
    void operator+(list<std::function<void(g_ptr<Object>)>> inits) {for(auto i : inits) add_initializer(i);}

    void recycle(g_ptr<Object> object) {
        if(object->recycled.load()) {
            return;
        }
        object->recycled.store(true);

        return_id(object->TID);
        deactivate(object);
    }

    void deactivate(g_ptr<Object> object) {
        object->stop();
    }

    void reactivate(g_ptr<Object> object) {
        object->resurrect();
    }

};

