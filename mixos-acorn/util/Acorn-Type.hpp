#pragma once

#include "ext/g_lib/util/util.hpp"

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

    struct Ptr {
        Ptr() {}
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
        uint32_t pool = 0; //Pool it's at
        uint32_t idx = 0; //Column
        uint32_t sidx = 0; //Row

        inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    };

    static const Ptr deadptr = {0,0,0};
    static Ptr dead_ref = {0,0,0};

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

    struct ColCell {
        uint32_t hash = 0;
        uint32_t index = 0;
    };
    struct QCellCol : QCol {
        ColCell& get(uint32_t idx) {return *(ColCell*)qget(idx*sizeof(ColCell));}
        ColCell& operator[](uint32_t idx) {return *(ColCell*)qget(idx*sizeof(ColCell));}
        void push(ColCell c) {QCol::push((void*)&c,sizeof(ColCell));}
        uint32_t length() {return size/sizeof(ColCell);}
    };

    struct Col : QCol {
        Col(uint32_t _size = 1) : element_size(_size) {}
        uint32_t element_size;
        uint32_t tag = 0;
        uint32_t hash = 0;
        bool live = true;
        bool heterogenous = false;

        QString label;
        QCellCol cells;
        inline uint32_t length() {return size / element_size;}
        void push(const void* element) {
            QCol::push(element,element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {QCol::push_default(element_size);}
        
        inline void* sget(uint32_t index) {
            DEBUG_ONLY(if(index*element_size>=size) {throw_error(red("col:sget "),"index ",index," out of bounds for size ",size,", elment size is ",element_size," tag is ",tag);return nullptr;})
            return &storage[index * element_size];
        }
        inline void* iget(uint32_t index, uint32_t offset) {
            DEBUG_ONLY(if(index*element_size+offset>=size) {throw_error(red("col:iget "),"index ",index," plus offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[index * element_size + offset];
        }
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return get(size-1);}

        void put(const std::string& key, const void* element) {
            ColCell c{hashString(key), length()};
            cells.push(c);
            push(element);
        }
        bool hasKey(const std::string& key) {
            uint32_t h = hashString(key);
            for(int i = 0; i < cells.length(); i++) {
                if(cells.get(i).hash == h) return true;
            }
            return false;
        }
        void* get(const std::string& key) {
            uint32_t h = hashString(key);
            for(int i = 0; i < cells.length(); i++) {
                ColCell& c = cells.get(i);
                if(c.hash == h) return sget(c.index);
            }
            return nullptr;
        }
        inline void* operator[](const std::string& key) {return get(key);}

        inline void set(const std::string& key, const void* element) {
            if(!hasKey(key)) {print(red("acorntype:col:set does not have key "+key+"!"));} 
            else {memcpy(get(key), element, element_size);}
        }

        inline void set(uint32_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}
        inline void iset(uint32_t index, uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[index * element_size + offset], element, element_size);}

        void removeAt(uint32_t index) {QCol::removeAt(index,element_size);}
        void pop(void* out) {QCol::pop(out,element_size);}
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
    void recycle_column(Col& col, uint32_t id) {
       (*(Col*)col.sget(id)).live = false;
    }

}

