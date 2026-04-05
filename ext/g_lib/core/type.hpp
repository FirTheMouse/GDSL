#pragma once

#include "../util/util.hpp"

struct _note {
    _note() {}
    _note(int _index) : index(_index) {}
    _note(int _index, int _sub_index) : index(_index), sub_index(_sub_index) {}
    
    int index = -1;
    int sub_index = -1;
};

static _note note_fallback;

struct _column {
    _column(size_t _size = 1) : element_size(_size) {}
    size_t element_size;
    list<uint8_t> storage;
    
    inline void* get(size_t index) {
        return &storage[index * element_size];
    }

    inline size_t length() {
       return storage.length() / element_size;
    }
    
    void push(const void* element) {
        size_t old_size = storage.size();
        storage.resize(old_size + element_size);
        memcpy(&storage[old_size], element, element_size);
    }

    void push_default() {
        size_t old_size = storage.size();
        storage.resize(old_size + element_size);
        memset(&storage[old_size], 0, element_size);
    }

    inline void set(size_t index, const void* element) {
        memcpy(&storage[index * element_size], element, element_size);
    }
};

class Type : public q_object {
public:
    Type() {}
    ~Type() {}

    //Consider making notes be uint32_t,_note, and have all the puts pre-hash, this will allow us to do enum notes and avoid the string stuff
    map<std::string,_note> notes; // Where reflection info is stored
    list<_note> array; //Ordered array for usage of Type as a MultiArray
    list<_column> columns;

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

            if(notes.size() != 0) {
                for(auto e : notes.entrySet()) {
                    int col = e.value.index;
                    int row = e.value.sub_index;
                    if(col == -1) continue;
                    if(row == -1) {
                        headers[col] = e.key;
                        ids_max_size[col] = std::max(ids_max_size[col], (int)e.key.length());
                    } else {
                        ids[col][row] = e.key;
                        ids_max_size[col] = std::max(ids_max_size[col], (int)e.key.length());
                    }
                }
            }

            if(array.size() != 0) {
                for(int a=0;a<array.length();a++) {
                    int col = array[a].index;
                    int row = array[a].sub_index;
                    if(col == -1 || row == -1) continue;
                    if(ids[col][row] == "") {
                        std::string s = std::to_string(a);
                        ids[col][row] = s;
                        ids_max_size[col] = std::max(ids_max_size[col], (int)s.length());
                    }
                }
            }

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
        columns.push(_column(size));
    }

    inline bool has_column(int index) {return index<columns.length();}
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

    //Creates a new column and note for an entry of the provided size, then returns the note
    _note& note_value(const std::string& name, size_t size) {
        _note note(columns.length(),0); add_column(size); notes.put(name,note);
        return notes.get(name);
    }

    //Creates a new column and note for an entry of the provided size, then returns the note
    template<typename T>
    _note& note_value(const std::string& name) {
        return note_value(name,sizeof(T));
    }

    //Returns the adress of the column at the index
    inline void* address_column(int index) {
        return &columns[index];
    }

    //Returns the adress of a column specificed by the note
    inline void* address_column(const std::string& name) {
        _note note = notes.getOrDefault(name,note_fallback);
        if(note.index==-1) return nullptr;
        return address_column(note.index);
    }



    //Direct get from a pointer to the adress of a column
    inline static void* get(void* ptr, size_t sub_index) {   
        return (*(_column*)ptr).get(sub_index);
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
    void push(void* value, size_t size, int column_index = -1) {
        if(column_index == -1) {
            column_index = columns.length();
            add_column(size);
        } else if(columns[column_index].element_size!=size) {
            print("push::365 provided column index ",column_index," is the wrong size, expected: ",size);
            return;
        }
        _note note(column_index, columns[column_index].length());
        columns[column_index].push(value);
        array << note;
    }

    //Same as push, except it will try to insert into the first column matching the provided size instead of creating one
    void push_and_bucket(void* value, size_t size, int column_index = -1) {
        if(column_index == -1) {
            for(int i=0;i<columns.length();i++) {
                if(columns[i].element_size==size) {
                    column_index = i;
                    break;
                }
            }
        } 
        push(value,size,column_index);
    }

    //Takes the result of the base push and inserts the note into the notes map with the provided name as well
    void add(const std::string& name, void* value, size_t size, int column_index = -1) {
        push(value,size,column_index);
        notes.put(name, array.last());
    }

    //Inserts into the provided index or creates a new column for it, and puts a note in the array, and puts a note in the array as well as the notes map
    template<typename T>
    void add(const std::string& name,T value,int column_index = -1) {
        add(name,&value,sizeof(T),column_index);
    }

    //Inserts into the provided index or creates a new column for it, and puts a note in the array, and puts a note in the array
    template<typename T>
    void push(T value, int column_index = -1) {
        push(&value, sizeof(T), column_index);
    }

    //Same as push, except it will try to insert into the first column matching the provided size instead of creating one
    template<typename T>
    void push_and_bucket(T value, int column_index = -1) {
        push_and_bucket(&value, sizeof(T), column_index);
    }

    //Direct set 
    static void set(void* ptr,void* value, int sub_index) {
        return (*(_column*)ptr).set(sub_index,value);
    }

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
        return set(ptr,value,sub_index);
    }

    template<typename T>
    void set(const std::string& label,T value,size_t index) {
        size_t size = sizeof(T);
        set(label,&value,index,size);
    }

    inline void* get(int index,int sub_index) {
       return columns[index].get(sub_index);
    }

    void* get_from_note(const _note& note) {
        return get(note.index,note.sub_index);
    }

    void* data_get(const std::string& name) {
        return get_from_note(notes.getOrDefault(name,note_fallback));
    }

    void* data_get_at(const std::string& name, int sub_index) {
        _note& note = notes.getOrDefault(name,note_fallback);
        return get(note.index,sub_index);
    }

    void* array_get(int index) {
        return get_from_note(array[index]);
    }

   //Uses data get
   template<typename T>
   T& get(const std::string& label) {
        return *(T*)data_get(label);
   }

   //Retrives based on an index into the array, only works if values were pushed in
   template<typename T>
   T& get(int index) {
        return *(T*)array_get(index);
   }

   template<typename T>
   T& get(int index,int sub_index) {
        return *(T*)get(index,sub_index);
   }

    void set(int index,int sub_index,void* value) {
        columns[index].set(sub_index,value);
    }

   void set_from_note(const _note& note,void* value) {
        set(note.index,note.sub_index,value);
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

    //Uses array set
    template<typename T>
    void set(int index,T value) {
     array_set(index,&value);
    }

    //Directly sets the value of a place
    template<typename T>
    void set(int index,int sub_index,T value) {
        set(index,sub_index,&value);
    }
};


static void write_note(_note& note, std::ostream& out) {
    write_raw<int>(out, note.index);
    write_raw<int>(out, note.sub_index);
}

static void write_type(g_ptr<Type> t, std::ostream& out) {
    //Header
    write_raw<uint32_t>(out, t->columns.length());
    write_raw<uint32_t>(out, t->notes.size());
    write_raw<uint32_t>(out, t->array.length());

    write_raw<int>(out, t->save_idx);
    write_string(out, t->type_name);

    //Columns
    for(int i = 0; i < t->columns.length(); i++) {
        _column& col = t->columns[i];
        write_raw<size_t>(out, col.element_size);
        write_raw<uint32_t>(out, (uint32_t)col.length());
        out.write((const char*)col.storage.data(), col.storage.size());
    }

    //Notes
    for(auto e : t->notes.entrySet()) {
        write_string(out, e.key);
        write_note(e.value, out);
    }

    //Array
    for(int i = 0; i < t->array.length(); i++) {
        write_note(t->array[i], out);
    }
}

static _note read_note(std::istream& in) {
    int index     = read_raw<int>(in);
    int sub_index = read_raw<int>(in);
    return _note(index, sub_index);
}

static void read_type(g_ptr<Type> t, std::istream& in) {
    uint32_t col_count   = read_raw<uint32_t>(in);
    uint32_t notes_count = read_raw<uint32_t>(in);
    uint32_t array_count = read_raw<uint32_t>(in);

    t->save_idx = read_raw<int>(in);
    t->type_name = read_string(in);

    //Columns
    for(uint32_t i = 0; i < col_count; i++) {
        size_t   element_size = read_raw<size_t>(in);
        uint32_t row_count    = read_raw<uint32_t>(in);
        _column col(element_size);
        size_t byte_count = element_size * row_count;
        col.storage.resize(byte_count);
        in.read((char*)col.storage.data(), byte_count);
        t->columns.push(col);
    }

    //Notes
    for(uint32_t i = 0; i < notes_count; i++) {
        std::string key = read_string(in);
        t->notes.put(key, read_note(in));
    }

    //Array
    for(uint32_t i = 0; i < array_count; i++) {
        t->array << read_note(in);
    }
}

static void save_type(g_ptr<Type> t, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if(!out) throw std::runtime_error("Can't write to file: " + path);
    write_type(t, out);
    out.close();
}

static g_ptr<Type> load_type(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(!in) throw std::runtime_error("Can't read from file: " + path);
    g_ptr<Type> t = make<Type>();
    read_type(t,in);
    in.close();
    return t;
}

static void load_type(g_ptr<Type> t, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(!in) throw std::runtime_error("Can't read from file: " + path);
    read_type(t,in);
    in.close();
}

