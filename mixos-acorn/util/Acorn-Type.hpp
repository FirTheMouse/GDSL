#pragma once

#include "../../ext/g_lib/util/util.hpp"

#define ACORN_DEBUG 1

namespace Acorn {

    bool ERROR_FLAG = false;
    std::string ERROR_MSG = "";

    template<typename... Args>
    void throw_error(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        ERROR_MSG = oss.str();
        ERROR_FLAG = true;
        print(red("ERROR: "),ERROR_MSG);
    }

    #if ACORN_DEBUG
        #define DEBUG_ONLY(x) x
    #else
        #define DEBUG_ONLY(x)
    #endif

    uint32_t hashBytes(const void* data, uint32_t size) {
        uint32_t hash = 5381;
        const uint8_t* bytes = (const uint8_t*)data;
        for(uint32_t i = 0; i < size; i++) {
            hash = ((hash << 5) + hash) + bytes[i];
        }
        return hash;
    }

    struct Ptr {
        Ptr() {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx, uint16_t _unit) : pool(_pool), idx(_idx), sidx(_sidx), unit(_unit) {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
        uint32_t pool = 0; //Pool it's at
        uint32_t idx = 0; //Column
        uint32_t sidx = 0; //Row
        
        uint16_t unit = 0;

        inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    };

    struct Ptr4 {
        Ptr4() {}
        Ptr4(uint32_t _midx, Ptr p) : midx(_midx), ptr(p) {}
        uint32_t midx = 0;
        Ptr ptr;
    };

    static const Ptr deadptr = {0,0,0,0};
    static Ptr dead_ref = {0,0,0,0};

    struct QCol {
        QCol() {}
        uint8_t* storage = nullptr;
        uint32_t size = 0;
        uint32_t capacity = 0;
        
        inline bool empty() {return size==0;}
        void reserve(uint32_t new_capacity) {
            if(new_capacity <= capacity) return;
            
            uint8_t* newPtr = new uint8_t[new_capacity];
            if(storage) memcpy(newPtr, storage, size);
            delete[] storage;
            storage = newPtr;
            capacity = new_capacity;
        }
        void resize(uint32_t new_size) {
            if (new_size > capacity) {
                reserve(new_size);
            }
            size = new_size;
        }
        void push(const void* element, uint32_t width) {
            uint32_t old_size = size;
            uint32_t new_size = size+width;
            if(new_size>=capacity) {
                reserve(new_size*2);
            }
            memcpy(&storage[old_size], element, width);
            size = new_size;
        }
        void push_default(uint32_t width) {
            size_t old_size = size;
            resize(old_size + width);
            memset(&storage[old_size], 0, width);
        }
        void insert(uint32_t index, const void* element, uint32_t width) {
            uint32_t byte_pos = index * width;
            uint32_t new_size = size + width;
            if(new_size >= capacity) reserve(new_size * 2);
            memmove(&storage[byte_pos + width], &storage[byte_pos], size - byte_pos);
            memcpy(&storage[byte_pos], element, width);
            size = new_size;
        }
        inline void* qget(uint32_t offset) {
            DEBUG_ONLY(if(offset>=size) {throw_error(red("col:qget "),"offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[offset];
        }
        inline void* operator[](uint32_t index) {return qget(index);}
        inline void qset(uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[offset], element, width);}
        void removeAt(uint32_t index, uint32_t width) {
            size_t byte_start = index * width;
            for(size_t i = byte_start; i < size - width; i++) {
                storage[i] = storage[i + width];
            }
            resize(size - width);
        }
        void clear() {size = 0;}
        void pop(void* out, uint32_t width) {
            memcpy(out, qget((size/width - 1) * width), width);
            resize(size - width);
        }
    };

    struct QString : QCol {
        QString() {}
        QString(QCol q) : QCol(q) {}
        char& at(uint32_t idx) {return *(char*)qget(idx);}
        char& operator[](uint32_t idx) {return *(char*)qget(idx);}
        void push(char c) {QCol::push((void*)&c,1);}
        uint32_t length() {return size;}

        void operator=(const std::string& s) {clear(); for(char c : s) push(c);}
        void operator=(const char* s) {clear(); while(*s) push(*s++);}
        bool operator==(const std::string& s) {
            if(size != s.length()) return false;
            return memcmp(storage, s.data(), size) == 0;
        }
        bool operator==(const char* s) {
            uint32_t len = strlen(s);
            if(size != len) return false;
            return memcmp(storage, s, size) == 0;
        }
        std::string to_std() {
            if(!storage) return "";
            return std::string((char*)storage, size);
        }
    };
    std::ostream& operator<<(std::ostream& os, QString& s) {
        if(s.storage) os.write((const char*)s.storage, s.size);
        return os;
    }


    struct CCol : QCol {
        CCol() {}
        CCol(uint32_t _size) : element_size(_size) {}
        CCol(QCol q) : QCol(q) {}
        uint32_t element_size = 1;
        uint32_t tag = 0;
        uint32_t hash = 0;
        uint32_t index = 0;
        bool live = true;

        inline uint32_t length() {return size / element_size;}
        void push(const void* element) {
            QCol::push(element,element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {QCol::push_default(element_size);}
        void insert(uint32_t index, const void* element) {
            QCol::insert(index, element, element_size);
        }
        
        inline void* sget(uint32_t index) {
            DEBUG_ONLY(if(index*element_size>=size) {throw_error(red("col:sget "),"index ",index," out of bounds for size ",size,", elment size is ",element_size," tag is ",tag);return nullptr;})
            return &storage[index * element_size];
        }
        inline void* iget(uint32_t index, uint32_t offset) {
            DEBUG_ONLY(if(index*element_size+offset>=size) {throw_error(red("col:iget "),"index ",index," plus offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[index * element_size + offset];
        }
        inline void set(uint32_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}
        inline void iset(uint32_t index, uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[index * element_size + offset], element, element_size);}
        void removeAt(uint32_t index) {QCol::removeAt(index,element_size);}
        void pop(void* out) {QCol::pop(out,element_size);}
    };

    struct QCellCol : QCol {
        QCellCol() {}
        QCellCol(QCol q) : QCol(q) {}
        CCol& get(uint32_t idx) {return *(CCol*)qget(idx*sizeof(CCol));}
        CCol& operator[](uint32_t idx) {return *(CCol*)qget(idx*sizeof(CCol));}
        void push(CCol c) {QCol::push((void*)&c,sizeof(CCol));}
        uint32_t length() {return size/sizeof(CCol);}
    };

    struct Col : CCol {
        Col() {}
        Col(uint32_t _size) :  CCol(_size) {}
        Col(CCol q) : CCol(q) {}
        bool heterogenous = false;
        QString label;
        QCellCol cells;
        
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return get(size-1);}

        void qput(const void* element, const void* key, uint32_t key_size, uint32_t key_tag) {
            CCol c;
            c.element_size = key_size; 
            c.tag = key_tag;
            c.hash = hashBytes(key, key_size);
            c.index = length();
            c.push(key);
            push(element);
            cells.push(c);
        }
        void* get(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return sget(c.index);
                    }
                }
            }
            return nullptr;
        }
        uint32_t getidx(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h) {
                    if(memcmp(c.storage, key, size) == 0) { //Collison check against the stored key
                        return c.index;
                    }
                }
            }
            return 0;
        }
        bool hasKey(const void* key, uint32_t size) {
            uint32_t h = hashBytes(key, size);
            for(int i = 0; i < cells.length(); i++) {
                CCol& c = cells[i];
                if(c.hash == h && memcmp(c.storage, key, size) == 0) return true;
            }
            return false;
        }

        void put(const std::string& str, const void* element, uint32_t tag = 0) {qput(element,str.data(),str.length(),tag);}
        void* get(const std::string& str) {return get(str.data(), str.length());}
        bool hasKey(const std::string& str) {return hasKey(str.data(), str.length());}
        void put(uint64_t u64, const void* element, uint32_t tag = 0) {qput(element,(void*)&u64,8,tag);}
        void* get(uint64_t u64) {return get((void*)&u64, 8);}
        bool hasKey(uint64_t u64) {return hasKey((void*)&u64, 8);}
        void put(Ptr p, const void* element, uint32_t tag = 0) {qput(element, (void*)&p, sizeof(Ptr), tag);}
        void* get(Ptr p) {return get((void*)&p, sizeof(Ptr));}
        bool hasKey(Ptr p) {return hasKey((void*)&p, sizeof(Ptr));}
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

    uint32_t add_column(Col& col, size_t size = 0, uint32_t tag = 0) {
        Col ncol(size);
        ncol.tag = tag;
        col.push((void*)&ncol);
        return col.length()-1;
    }
    
    uint32_t note_value(Col& col, const std::string& key, uint32_t size, uint32_t tag) {
        uint32_t at = add_column(col, size, tag);
        (*(Col*)col.sget(at)).label = key;
        return at;
    }

    //Standard column create, use pooling means it will try to find a dead column first, tag sensitive means it will also ensure the column tag matches
    uint32_t create_column(Col& col, uint32_t size, uint32_t tag, bool use_pooling = true, bool tag_sensitive = false) {
        if(use_pooling) {
            for(int i=0;i<col.length();i++) {
                Col& ncol = *(Col*)col.sget(i);
                if(!ncol.live&&ncol.element_size==size&&(!tag_sensitive||ncol.tag==tag)) {
                    ncol.clear();
                    ncol.live = true;
                    return i;
                }
            }
        }
        add_column(col,size,tag);
        return col.length()-1;
    }
    //Creates a column from pool and intilizes it's memory if empty
    uint32_t push_column(Col& col, uint32_t size, uint32_t tag) {
        uint32_t at = create_column(col,size,tag);
        Col& ncol = *(Col*)col.sget(at);
        if(ncol.size<size) {
            ncol.resize(size);
        }
        return at;
    }
    static void recycle_column(Col& col, uint32_t id) {
       (*(Col*)col.sget(id)).live = false;
    }



    static void write_qcol(std::ostream& out, QCol& col) {
        write_raw<uint32_t>(out, col.size);
        out.write((const char*)col.storage, col.size);
    }

    static QCol read_qcol(std::istream& in) {
        QCol col;
        uint32_t size = read_raw<uint32_t>(in);
        col.resize(size);
        in.read((char*)col.storage, col.size);
        return col;
    }

    static void write_ccol(std::ostream& out, CCol& col) {
        write_qcol(out,col);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<bool>(out, col.live);
    }

    static CCol read_ccol(std::istream& in) {
        CCol col = read_qcol(in);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        return col;
    }

    static void write_col(std::ostream& out, Col& col) {
        write_ccol(out,col);
        write_raw<bool>(out, col.heterogenous);
        write_qcol(out,col.cells);
        write_qcol(out,col.label);
    }

    static Col read_col(std::istream& in) {
        Col col = read_ccol(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcol(in);
        col.label = read_qcol(in);
        return col;
    }
}

