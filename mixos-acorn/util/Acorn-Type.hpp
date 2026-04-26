#pragma once

#include "ext/g_lib/util/util.hpp"
#include "../util/Acorn-Object.hpp"

namespace Acorn {
    struct Col {
        Col(size_t _size = 1) : element_size(_size) {}
        uint32_t element_size;
        list<uint8_t> storage;

        std::string label;
        uint32_t tag = 0;

        map<std::string,uint32_t> cells;

        inline uint32_t length() {return storage.length() / element_size;}
        inline bool empty() {return storage.length()==0;}
        
        inline void* get(uint32_t index) {return &storage[index * element_size];}
        inline void* operator[](uint32_t index) {return get(index);}
        template<typename T> inline T& get(uint32_t index) {return *(T*)get(index);}

        inline bool hasKey(const std::string& key) {return cells.hasKey(key);}

        inline void* get(const std::string& key) {
            if(!hasKey(key)) {print(red("acorntype:col:get does not have key "+key+"!")); return nullptr;}
            return &storage[cells.get(key) * element_size];
        }
        inline void* operator[](const std::string& key) {return get(key);}
        template<typename T> inline T& get(const std::string& key) {return *(T*)get(key);}

        inline void set(size_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}
        inline void set(const std::string& key, const void* element) {
            if(!hasKey(key)) {print(red("acorntype:col:set does not have key "+key+"!"));} 
            else {memcpy(&storage[cells.get(key) * element_size], element, element_size);}
        }

        void push(const void* element) {
            size_t old_size = storage.size();
            storage.resize(old_size + element_size);
            memcpy(&storage[old_size], element, element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {
            size_t old_size = storage.size();
            storage.resize(old_size + element_size);
            memset(&storage[old_size], 0, element_size);
        }

        void put(const std::string& key, const void* element) {
            cells[key] = length();
            push(element);
        }
    };

    //Convience for ergonomic white/blacklist things
    struct _lookup {
        _lookup(list<std::string> init, bool _default_state) 
        : default_state(_default_state)  {
            for(auto s : init) {
                lookup[s] = !default_state;
            }
        }

        map<std::string,bool> lookup;
        bool default_state;

        bool operator[](const std::string& key) {
            return lookup.getOrDefault(key,default_state);
        }
    };

    struct Ptr {
        uint32_t pool; //Pool it's at
        uint32_t idx; //Column OR type if a node/value
        uint32_t sidx; //Row
    };


    class Type {
    public:
        Type() {}
        ~Type() {}

        list<Ptr> array;
        list<Col> columns;

        int save_idx = -1;
        std::string type_name = "bullets";


        std::string make_table_string(int mode) {
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
                    std::string c_size = std::to_string(columns[c].element_size);
                    header.append("("+c_size+")");
                    if(c_size.length()==1) header.append("   ");
                    if(c_size.length()==2) header.append("  ");
                    if(c_size.length()==3) header.append(" ");
                }
                table.append(header+"\n");
                for(int r=0;r<lr;r++) {
                    std::string row = "";
                    if(r<10) row.append("   ");
                    else if(r<100) row.append("  ");
                    row.append(std::to_string(r)+":");
                    for(int c=0;c<columns.length();c++) {
                        if(r<columns[c].length()) {
                            row.append("  "+std::to_string((uintptr_t)columns[c].get(r)).append(" ").insert(5,"-").insert(8,"-"));
                        }
                        else {
                            row.append("             ");
                        }
                        row.append("      ");
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

                for(int r=0;r<lr;r++) {
                    std::string row = "";
                    if(r<10) row.append("  ");
                    else if(r<100) row.append(" ");
                    row.append(std::to_string(r)+":");
                    for(int c=0;c<columns.length();c++) {
                        if(r<columns[c].length())
                            row.append(" O");
                        else
                            row.append("  ");
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
                    headers[i] = std::to_string(i) + "(" + std::to_string(columns[i].element_size) + ")";

                    std::string c_size = std::to_string(columns[i].element_size);
                    headers[i] = std::to_string(i)+"("+c_size+")";
                    ids_max_size[i] = headers[i].length();
                }

                //Replace later
                // if(notes.size() != 0) {
                //     for(auto e : notes.entrySet()) {
                //         int col = e.value.index;
                //         int row = e.value.sub_index;
                //         if(col == -1) continue;
                //         if(row == -1) {
                //             headers[col] = e.key;
                //             ids_max_size[col] = std::max(ids_max_size[col], (int)e.key.length());
                //         } else {
                //             ids[col][row] = e.key;
                //             ids_max_size[col] = std::max(ids_max_size[col], (int)e.key.length());
                //         }
                //     }
                // }

                // if(array.size() != 0) {
                //     for(int a=0;a<array.length();a++) {
                //         int col = array[a].index;
                //         int row = array[a].sub_index;
                //         if(col == -1 || row == -1) continue;
                //         if(ids[col][row] == "") {
                //             std::string s = std::to_string(a);
                //             ids[col][row] = s;
                //             ids_max_size[col] = std::max(ids_max_size[col], (int)s.length());
                //         }
                //     }
                // }

                std::string header = "COL: ";
                for(int c=0;c<columns.length();c++) {
                    header.append(headers[c] + " ");
                    int pad = ids_max_size[c] - headers[c].length();
                    for(int i=0;i<pad;i++) header.append(" ");
                }
                table.append(header + "\n");

                for(int r=0;r<lr;r++) {
                    std::string row = "";
                    if(r<10) row.append("  ");
                    else if(r<100) row.append(" ");
                    row.append(std::to_string(r)+":");
                    for(int c=0;c<columns.length();c++) {
                        if(r<(int)columns[c].length() && ids[c][r]!="") {
                            int pad = ids_max_size[c] - ids[c][r].length();
                            row.append(" " + ids[c][r]);
                            for(int i=0;i<pad;i++) row.append(" ");
                        } else {
                            row.append(" ");
                            for(int i=0;i<ids_max_size[c];i++) row.append(" ");
                        }
                    }
                    table.append(row + (r!=lr-1?"\n":""));
                }
            }
            return table;
        }

        //Modes: 1 == Auto, 2 == Full, 3 == Compact, 4 == Pretty
        std::string table_to_string(int mode = 1) {
            return make_table_string(mode);
        }

        inline Col& operator[](size_t index) {
            return columns[index];
        }

        size_t column_count() {
            return columns.length();
        }
        
        size_t row_count(int column_index) {
            return columns[column_index].length();
        }
        
        void add_row(int index) {
            columns[index].push_default();
        }
        
        //Adds a new column and intilizes the rows
        void add_column(size_t size = 0) {
            columns.push(Col(size));
        }

        inline bool has_column(int index) {return index<columns.length();}

        //Returns the adress of the column at the index
        inline void* address_column(int index) {
            return &columns[index];
        }

        int get_column(const std::string& label) {
            for(int i=0;i<columns.length();i++) {
                if(columns[i].label==label) return i;
            }
            return -1;
        }

        //Returns the adress of a column with the label
        inline void* address_column(const std::string& label) {
            int at_id = get_column(label);
            if(at_id==-1) return nullptr;
            return address_column(at_id);
        }

        size_t note_value(const std::string& key, uint32_t size, uint32_t tag) {
            add_column(size);
            columns.last().label = key;
            columns.last().tag = tag;
            return columns.length()-1;
        }

        //Direct get from a pointer to the adress of a column
        inline static void* get(void* ptr, size_t sub_index) {   
            return (*(Col*)ptr).get(sub_index);
        }

        void* get(const std::string& label, size_t sub_index) {
            void* ptr = address_column(label);
            if (!ptr) return nullptr;
            return get(ptr,sub_index);
        }
        
        template<typename T>
        T& get(const std::string& label, size_t sub_index) {
            void* data_ptr = get(label, sub_index);
            if (!data_ptr) print("get::270 value not found");
            return *(T*)data_ptr;
        }

        template<typename T>
        T& get(void* ptr, size_t sub_index) {
            void* data_ptr = get(ptr, sub_index);
            if (!data_ptr) print("get::285 value not found");
            return *(T*)data_ptr;
        }
        
        //Inserts into the provided index or creates a new column for it, and puts a note in the array
        void push(void* value, size_t size, int column_index = -1, uint32_t tag = 0) {
            if(column_index == -1) {
                column_index = columns.length();
                add_column(size);
            } else if(columns[column_index].element_size!=size) {
                print("acorntype:push provided column index ",column_index," is the wrong size, expected: ",size);
                return;
            }
            if(tag!=0&&columns[column_index].tag!=tag) {
                print(yellow("acorntype:push columns tag is "+std::to_string(columns[column_index].tag)+" but an elment of tag "+std::to_string(tag)+" was pushed"));
            }
            array << Ptr{0,(uint32_t)column_index,(uint32_t)row_count(column_index)};
            columns[column_index].push(value);
        }

        //Same as push, except it will try to insert into the first column matching the provided size instead of creating one
        void push_and_bucket(void* value, size_t size, int column_index = -1, uint32_t tag = 0) {
            if(column_index == -1) {
                for(int i=0;i<columns.length();i++) {
                    if(columns[i].element_size==size) {
                        column_index = i;
                        break;
                    }
                }
            } 
            push(value,size,column_index,tag);
        }

        //Takes the result of the base push and inserts the note into the notes map with the provided name as well
        void add(const std::string& label, void* value, size_t size, int column_index = -1, uint32_t tag = 0) {
            if(column_index == -1) {
                column_index = columns.length();
                add_column(size);
                columns[column_index].tag = tag;
            } else if(columns[column_index].element_size!=size) {
                print("acorntype:add provided column index ",column_index," is the wrong size, expected: ",size);
                return;
            }
            if(tag!=0&&columns[column_index].tag!=tag) {
                print(yellow("acorntype:add columns tag is "+std::to_string(columns[column_index].tag)+" but an elment of tag "+std::to_string(tag)+" was pushed"));
            }
            array << Ptr{0,(uint32_t)column_index,(uint32_t)row_count(column_index)};
            columns[column_index].put(label,value);
        }

        //Inserts into the provided index or creates a new column for it, and puts a note in the array, and puts a note in the array as well as the notes map
        template<typename T>
        void add(const std::string& name,T value,int column_index = -1, uint32_t tag = 0) {
            add(name,&value,sizeof(T),column_index,tag);
        }

        //Add but without the column index to be prettier and it returns the column allocated
        template<typename T>
        uint32_t new_column(const std::string& name,T value, uint32_t tag = 0) {
            add(name,&value,sizeof(T),-1,tag);
            return columns.length()-1;
        }

        //Add but without the column index to be prettier and it returns the column allocated
        uint32_t new_column(const std::string& name,void* value, uint32_t size, uint32_t tag) {
            add(name,value,size,-1,tag);
            return columns.length()-1;
        }

        //Inserts into the provided index or creates a new column for it, and puts a note in the array, and puts a note in the array
        template<typename T>
        void push(T value, int column_index = -1, uint32_t tag = 0) {
            push(&value, sizeof(T), column_index, tag);
        }

        //Same as push, except it will try to insert into the first column matching the provided size instead of creating one
        template<typename T>
        void push_and_bucket(T value, int column_index = -1, uint32_t tag = 0) {
            push_and_bucket(&value, sizeof(T), column_index, tag);
        }
        
        //Direct set 
        static void set(void* ptr,void* value, int sub_index) {
            return (*(Col*)ptr).set(sub_index,value);
        }
        void set(int index,int sub_index,void* value) {columns[index].set(sub_index,value);}
        //Sets the value associated with the label 
        void set(const std::string& label,void* value, int sub_index, int size = -1) {
            void* ptr = address_column(label);
            if (!ptr) {
                if(size!=-1) {
                    add(label,value,size);
                } else {
                    print("Set::414 no address found associated with ",label," and no size was provided to default construct");
                }
                return;
            }
            set(ptr,value,sub_index);
        }
        template<typename T>
        void set(const std::string& label,T value,size_t index) {
            size_t size = sizeof(T);
            set(label,&value,index,size);
        }

        inline void* get(int index,int sub_index) {return columns[index].get(sub_index);}
        inline void* get_from_ptr(Ptr& ptr) {return columns[ptr.idx][ptr.sidx];}
        void* array_get(int index) {return get_from_ptr(array[index]);}

        //Retrives based on an index into the array, only works if values were pushed in
        template<typename T>
        T& get(int index) {return *(T*)array_get(index);}

        template<typename T>
        T& get(int index,int sub_index) {return *(T*)get(index,sub_index);}

        //Directly sets the value of a place
        template<typename T>
        void set(int index,int sub_index,T value) {set(index,sub_index,(void*)&value);}
    };


    static void write_col(std::ostream& out, Col& col) {
        write_raw<size_t>(out, col.element_size);
        write_raw<uint32_t>(out, (uint32_t)col.length());
        out.write((const char*)col.storage.data(), col.storage.size());
        write_string(out, col.label);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, (uint32_t)col.cells.size());
        for(auto e : col.cells.entrySet()) {
            write_string(out, e.key);
            write_raw<uint32_t>(out, e.value);
        }
    }

    static void write_ptr(std::ostream& out, Ptr& ptr) {
        write_raw<uint32_t>(out, ptr.pool);
        write_raw<uint32_t>(out, ptr.idx);
        write_raw<uint32_t>(out, ptr.sidx);
    }

    static void write_type(std::ostream& out, Type t) {
        write_raw<uint32_t>(out, t.columns.length());
        write_raw<uint32_t>(out, t.array.length());

        write_raw<int>(out, t.save_idx);
        write_string(out, t.type_name);

        for(int i = 0; i < t.columns.length(); i++) {write_col(out, t.columns[i]);}
        for(int i = 0; i < t.array.length(); i++) {write_ptr(out, t.array[i]);}
    }

    static void read_col(std::istream& in, Col& col) {
        col.element_size = read_raw<size_t>(in);
        uint32_t len = read_raw<uint32_t>(in);
        col.storage.resize(len * col.element_size);
        in.read((char*)col.storage.data(), len);
        col.label = read_string(in);
        col.tag = read_raw<uint32_t>(in);
        uint32_t cells_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < cells_count; i++) {
            std::string key = read_string(in);
            col.cells[key] = read_raw<uint32_t>(in);
        }
    }

    static Ptr read_ptr(std::istream& in) {
        uint32_t pool = read_raw<uint32_t>(in);
        uint32_t idx = read_raw<uint32_t>(in);
        uint32_t sidx = read_raw<uint32_t>(in);
        return Ptr{pool,idx,sidx};
    }

    static void read_type(Type t, std::istream& in) {
        uint32_t col_count   = read_raw<uint32_t>(in);
        uint32_t array_count = read_raw<uint32_t>(in);

        t.save_idx = read_raw<int>(in);
        t.type_name = read_string(in);

        //Columns
        for(uint32_t i = 0; i < col_count; i++) {
            Col col;
            t.columns.push(col);
        }

        //Array
        for(uint32_t i = 0; i < array_count; i++) {
            t.array << read_ptr(in);
        }
    }

    static void save_type(Type t, const std::string& path) {
        std::ofstream out(path, std::ios::binary);
        if(!out) throw std::runtime_error("Can't write to file: " + path);
        write_type(out, t);
        out.close();
    }

    static Type load_type(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        Type t;
        read_type(t,in);
        in.close();
        return t;
    }

    static void load_type(Type t, const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        read_type(t,in);
        in.close();
    }

    class TypePool : public Type {
    public:
        TypePool() {}

        list<Object> objects;
        list< std::function<void(Object&)> > init_funcs;
        list<int> free_ids;
        uint32_t free_stack_top = 0;

        std::function<Object()> make_func = [](){
            Object object;
            return object;
        };

        int get_next() {
            if (free_stack_top == 0) return -1;
            return free_ids.get(--free_stack_top);
        }
        
        void return_id(int id) {
            if (free_stack_top < free_ids.size()) {
                free_ids.get(free_stack_top++) = id;
            } else {
                free_ids.push(id);
                free_stack_top++;
            }
        }

        Object create() {
            int next_id = get_next();
            Object object;
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
                object.pool = this;
                store(object);
                for(int i=0;i<init_funcs.size();i++) {
                    init_funcs[i](object);
                }
            }
            object.recycled = false;
            return object;
        }

        size_t add_column(size_t size) {
            Col col(size);
            for(size_t i = 0; i < objects.length(); i++) col.push_default();
            int col_len = columns.length();
            columns.push(col);
            return col_len;
        }

        private:
            void store(Object object)
            {
                object.sidx = objects.size();
                for(int c = 0; c<columns.length(); c++) {
                    columns[c].push_default();
                }
                objects.push(object);
            }
        public:

        void add_initializers(list<std::function<void(Object)>> inits) {for(auto i : inits) add_initializer(i);}
        void add_initializer(std::function<void(Object)> init) {init_funcs << init;}
        void operator+(std::function<void(Object)> init) {add_initializer(init);}
        void operator+(list<std::function<void(Object)>> inits) {for(auto i : inits) add_initializer(i);}

        void recycle(Object object) {
            if(object.recycled) {
                return;
            }
            object.recycled = true;

            return_id(object.sidx);
        }
    };

}

