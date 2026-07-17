#pragma once

#include "../../ext/g_lib/util/util.hpp"

#define ACORN_DEBUG 1

namespace Acorn {

    inline thread_local bool ERROR_FLAG = false;
    inline thread_local std::string ERROR_MSG = "";

    template<typename... Args>
    void throw_error(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        ERROR_MSG = oss.str();
        ERROR_FLAG = true;
        print(red("ERROR: "),ERROR_MSG);
    }

    template<typename... Args>
    void append_error(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        ERROR_MSG+=yellow(" -> ")+oss.str();
        print(red("ERROR: "), oss.str());
    }
    
    #if ACORN_DEBUG
        #define DEBUG_ONLY(x) x
        #define CHECK_ERROR_VAL(value, ...)     \
            do {                                \
                if (ERROR_FLAG) {               \
                    append_error(               \
                        __VA_ARGS__,            \
                        " [", strip_path(__FILE__), ":", __LINE__, "]" \
                    );                          \
                    return value;               \
                }                               \
            } while (false)
    
        #define CHECK_ERROR(...)                \
            do {                                \
                if (ERROR_FLAG) {               \
                    append_error(               \
                        __VA_ARGS__,            \
                        " [", strip_path(__FILE__), ":", __LINE__, "]" \
                    );                          \
                    return;                     \
                }                               \
            } while (false)                     
    #else
        #define DEBUG_ONLY(x)
        #define CHECK_ERROR_VAL(value, ...)
        #define CHECK_ERROR(...)
    #endif

    inline uint32_t mix32_final(uint32_t x) {
        x ^= x >> 16;
        x *= 0x85ebca6bU;
        x ^= x >> 13;
        x *= 0xc2b2ae35U;
        x ^= x >> 16;
        return x;
    }

    inline uint32_t hashBytes(const void* data, uint32_t size) {
        uint32_t hash = 5381;
        const uint8_t* bytes = (const uint8_t*)data;
        for(uint32_t i = 0; i < size; i++) {
            hash = ((hash << 5) + hash) + bytes[i];
        }
        hash = mix32_final(hash);
        return hash;
    }

    // struct Ptr {
    //     Ptr() {}
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx, uint16_t _unit) : pool(_pool), idx(_idx), sidx(_sidx), unit(_unit) {}
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) : pool(_pool), idx(_idx), sidx(_sidx) {}
    //     uint32_t pool = 0; //Pool it's at
    //     uint32_t idx = 0; //Column
    //     uint32_t sidx = 0; //Row
        
    //     uint16_t unit = 0;

    //     inline bool operator==(const Ptr& other) const {return pool == other.pool && idx == other.idx && sidx == other.sidx;}
    //     inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    // };


    // struct Ptr {
    //     Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
    //     }
    //     Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         pool = _pool; idx = _idx; sidx = _sidx;
    //     }
    //     Ptr(void* _cache, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         pool = _pool; idx = _idx; sidx = _sidx;
    //         cache = _cache; cachelevel = 3;
    //     }
    //     Ptr(void* _cache, uint32_t _idx, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         idx = _idx; sidx = _sidx;
    //         cache = _cache; cachelevel = 2;
    //     }
    //     Ptr(void* _cache, uint32_t _sidx) {
    //         memset(this, 0, sizeof(Ptr));
    //         sidx = _sidx;
    //         cache = _cache; cachelevel = 1;
    //     }
    //     Ptr() { memset(this, 0, sizeof(Ptr)); }

    //     uint8_t region;
    //     uint16_t zone;

    //     uint8_t cachelevel;
    //     union {
    //         struct { 
    //             uint32_t unit; 
    //             uint32_t device; 
    //         };
    //         void* cache;
    //     };

    //     uint32_t pool;
    //     uint32_t idx;
    //     uint32_t sidx;

    //     inline bool operator==(const Ptr& other) const {
    //         return pool == other.pool && idx == other.idx && sidx == other.sidx && 
    //             zone == other.zone && region == other.region &&
    //             cachelevel == other.cachelevel && 
    //             (cachelevel == 0 ? (unit == other.unit && device == other.device) : (cache == other.cache));
    //     }
    //     inline bool operator!=(const Ptr& other) const {return !(*this == other);}
    // };



    //Cachelevels
    //0=no cache (full resolution)
    //1=sidx valid, cache is a col
    //2=idx valid, cache is a ColCol
    //3=pool valid, cache is a ColColCol

    struct Ptr {
        Ptr(uint32_t _device, uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            device = _device; unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _unit, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            unit = _unit; pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            pool = _pool; idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            idx = _idx; sidx = _sidx;
        }
        Ptr(uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            sidx = _sidx;
        }
        Ptr(void* _cache, uint32_t _pool, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            pool = _pool; idx = _idx; sidx = _sidx;
            cache = _cache; cachelevel = 3;
        }
        Ptr(void* _cache, uint32_t _idx, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            idx = _idx; sidx = _sidx;
            cache = _cache; cachelevel = 2;
        }
        Ptr(void* _cache, uint32_t _sidx) {
            memset(this, 0, sizeof(Ptr));
            sidx = _sidx;
            cache = _cache; cachelevel = 1;
        }
        Ptr() { memset(this, 0, sizeof(Ptr)); }
        
        union {
            struct { 
                uint32_t region;
                uint32_t zone;
            };
            void* cache;
        };
        uint16_t gen;
        uint8_t specialization; 
        uint8_t cachelevel;
        uint32_t device; 
        uint32_t unit; 
        uint32_t pool;
        uint32_t idx;
        uint32_t sidx;

        inline bool operator==(const Ptr& other) const {
            return pool == other.pool && idx == other.idx && sidx == other.sidx && 
                unit == other.unit && device == other.device &&
                cachelevel == other.cachelevel && gen == other.gen &&
                (cachelevel == 0 ? (zone == other.zone && region == other.region) : (cache == other.cache));
        }
        inline bool operator!=(const Ptr& other) const {return !(*this == other);}
        inline uint32_t& operator[](uint32_t field) {
            switch(field) {
                case 5: return device;
                case 4: return unit;
                case 3: return pool;
                case 2: return idx;
                case 1: return sidx;
                default: return sidx;
            }
        }
    };
    static_assert(sizeof(Ptr)==32," Size of Ptr must be 32 for cross platform");

    struct Ptr4 {
        Ptr4() {}
        Ptr4(uint32_t _midx, Ptr p) : midx(_midx), ptr(p) {}
        uint32_t midx = 0;
        Ptr ptr;
    };

    static const Ptr deadptr;
    static Ptr dead_ref;

    struct QCol {
        QCol() {}
        QCol(const QCol& o) {
            storage = nullptr;
            size = 0;
            capacity = 0;
            if(o.storage && o.size > 0) {
                resize(o.size);
                memcpy(storage, o.storage, o.size);
            }
        }
        QCol(QCol&& o) {
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
        }
        QCol& operator=(const QCol& o) {
            if(this == &o) return *this;
            if(storage) delete[] storage;
            size = o.size;
            capacity = o.capacity;
            if(o.storage && o.capacity > 0) {
                storage = new uint8_t[o.capacity];
                memcpy(storage, o.storage, o.size);
            } else {
                storage = nullptr;
            }
            return *this;
        }
        QCol& operator=(QCol&& o) {
            if(this == &o) return *this;
            if(storage) delete[] storage;
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
            return *this;
        }
        ~QCol() {
            if(storage) {
                delete[] storage;
                storage = nullptr;
            }
        }

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
        inline void* qget(uint32_t offset) const {
            DEBUG_ONLY(if(offset>=size) {throw_error(red("col:qget "),"offset ",offset," out of bounds for size ",size);return nullptr;})
            return &storage[offset];
        }
        inline void* operator[](uint32_t index) {return qget(index);}
        inline void qset(uint32_t offset, const void* element, uint32_t width) {memcpy(&storage[offset], element, width);}
        void removeAt(uint32_t index, uint32_t width) {
            size_t byte_start = index * width;
            size_t bytes_to_move = size - byte_start - width;
            if(bytes_to_move > 0)
                memmove(&storage[byte_start], &storage[byte_start + width], bytes_to_move);
            size -= width;
        }
        void clear() {size = 0;}
        void pop(void* out, uint32_t width) {
            memcpy(out, qget((size/width - 1) * width), width);
            resize(size - width);
        }

        QCol take_range(uint32_t from, uint32_t to, uint32_t width) {
            uint32_t byte_from = from * width;
            uint32_t byte_to = to * width;
            uint32_t remove_size = byte_to - byte_from;
            QCol to_return; to_return.resize(remove_size);
            memcpy(to_return.storage, &storage[byte_from], remove_size);
            memmove(&storage[byte_from], &storage[byte_to], size - byte_to);
            size -= remove_size;
            return to_return;
        }
        QCol take(uint32_t index, uint32_t width) {
            return take_range(index, index+1, width);
        }
    };

    struct QString : QCol {
        QString() {}
        QString(QCol q) : QCol(q) {}
        QString(const QString& o) : QCol(o) {}
        QString(QString&& o) : QCol(std::move(o)) {}
        QString& operator=(const QString& o) {
            if(this == &o) return *this;
            QCol::operator=(o);
            return *this;
        }
        QString& operator=(QString&& o) {
            if(this == &o) return *this;
            QCol::operator=(std::move(o));
            return *this;
        }
        ~QString() {}
        
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
        CCol(const CCol& o) : QCol(o) {
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
            gen = o.gen;
        }
        CCol(CCol&& o) : QCol(std::move(o)) {
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
            gen = o.gen;
        }
        CCol& operator=(CCol&& o) {
            if(this == &o) return *this;
            QCol::operator=(std::move(o));
            element_size = o.element_size;
            tag = o.tag;
            hash = o.hash;
            index = o.index;
            cachelevel = o.cachelevel;
            live = o.live;
            gen = o.gen;
            return *this;
        }
        uint32_t element_size = 1;
        uint32_t tag = 0;
        uint32_t hash = 0;
        uint32_t index = 0;
        uint8_t cachelevel = 0;
        bool live = true;
        uint16_t gen = 0;

        inline uint32_t length() const {if(element_size==0||size==0) {return 0;} else {return size / element_size;}}
        void push(const void* element) {
            QCol::push(element,element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {QCol::push_default(element_size);}
        void insert(uint32_t index, const void* element) {
            QCol::insert(index, element, element_size);
        }
        
        inline void* sget(uint32_t index) const {
            DEBUG_ONLY(if(index*element_size>=size) {throw_error(red("col:sget "),"index ",index," out of bounds for size ",size,", element size is ",element_size," tag is ",tag);return nullptr;})
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
        QCol take(uint32_t index) {return QCol::take_range(index, index+1, element_size);}
        QCol take_range(uint32_t from, uint32_t to) {return QCol::take_range(from, to, element_size);}
    };


    //Do not opperate on this like a normal QCol, it's meant to only ever be interacted with through a Col
    //Size = the count of occupied slots
    //Capacity = the total avaliable slots
    struct QCellCol : private QCol {
        QCellCol() {}
        QCellCol(QCol q) : QCol(q) {}
        QCellCol(const QCellCol& o) : QCol() {
            storage = new uint8_t[o.capacity*sizeof(CCol)];
            capacity = o.capacity;
            size = o.size;
            for(uint32_t i = 0; i < o.capacity; i++) {
                if(o.get(i).storage) {
                    CCol copy(o.get(i)); 
                    memcpy(&storage[i*sizeof(CCol)],(void*)&copy,sizeof(CCol));
                    copy.storage = nullptr;
                } else {
                    memset(&storage[i*sizeof(CCol)],0,sizeof(CCol));
                }
            }
        }
        QCellCol& operator=(QCellCol&& o) {
            if(this == &o) return *this;
            for(uint32_t i = 0; i < capacity; i++) get(i).~CCol();
            if(storage) delete[] storage;
            storage = o.storage;
            size = o.size;
            capacity = o.capacity;
            o.storage = nullptr;
            o.size = 0;
            o.capacity = 0;
            return *this;
        }
        QCellCol& operator=(const QCellCol& o) {
            if(this == &o) return *this;
            for(uint32_t i = 0; i < capacity; i++) get(i).~CCol();
            if(storage) delete[] storage;
            size = 0; capacity = 0;
            storage = new uint8_t[o.capacity*sizeof(CCol)];
            for(uint32_t i = 0; i < o.capacity; i++) {
                if(o.get(i).storage) {
                    CCol copy(o.get(i)); 
                    memcpy(&storage[i*sizeof(CCol)],(void*)&copy,sizeof(CCol));
                    copy.storage = nullptr;
                } else {
                    memset(&storage[i*sizeof(CCol)],0,sizeof(CCol));
                }
            }
            return *this;
        }
        ~QCellCol() {
            if(!storage) return;
            for(uint32_t i = 0; i < capacity; i++) {get(i).~CCol();}
        }
        uint32_t count() const {return size;}
        uint32_t length() const {return capacity;}
        void nullstorage() {storage = nullptr;}
        void clear() {
            clear();
        }
        bool empty() const {return size==0;}


        void grow() {
            uint32_t old_capacity = capacity;
            uint32_t new_capacity = (capacity==0?4:capacity*2);
            uint8_t* oldPtr = storage;
            uint8_t* newPtr = new uint8_t[new_capacity*sizeof(CCol)];
            memset(newPtr,0,new_capacity*sizeof(CCol));
            storage = newPtr;
            capacity = new_capacity;
            size = 0;
            for(int i=0;i<old_capacity;i++) {
                CCol& c = *(CCol*)(oldPtr+(i*sizeof(CCol)));
                if(c.storage) {
                    scan_for_slot(c);
                }   
            }
            delete[] oldPtr;
        }

        //Home = desired location in storage
        //Pos = current location in storage
        inline uint32_t cell_distance_from_home(uint32_t hash, uint32_t pos) {
            uint32_t home = hash%capacity;
            return (pos+capacity-home)%capacity;
        }
        uint32_t load_factor() {return (capacity==0?100:(size*100)/capacity);}
        void scan_for_slot(CCol c) {
            if(capacity==0||load_factor()>70) {
                grow(); 
                scan_for_slot(c);
                return;
            }
            uint32_t home = c.hash%capacity;
            uint32_t pos = home;
            uint32_t traversed = 0;

            while(traversed<capacity) {
                CCol& existing = get(pos);
                if(existing.storage==nullptr) {
                    size++;
                    memcpy(&storage[pos*sizeof(CCol)], (void*)&c, sizeof(CCol));
                    c.storage = nullptr;
                    return;
                }
                if(cell_distance_from_home(c.hash,pos)>cell_distance_from_home(existing.hash,pos)) {
                    CCol copy_of_existing = existing;
                    memcpy(&storage[pos*sizeof(CCol)], (void*)&c, sizeof(CCol));
                    c.storage = nullptr;
                    scan_for_slot(std::move(copy_of_existing));
                    return;
                }
                traversed++;
                pos = (pos+1)%capacity;
            }
            throw_error("QCellCol corrupted: scan_for_slot traversed the entirety without finding any avaliable position "
            ,"this means either size or capacity was corrupted causing load_factor to never fire");
        }

        void set(uint32_t idx, CCol& c) {memcpy(&storage[idx*sizeof(CCol)],(void*)&c,sizeof(CCol)); c.storage = nullptr;}
        CCol& get(uint32_t idx) const {return *(CCol*)&storage[idx*sizeof(CCol)];}
        CCol& operator[](uint32_t idx) {return get(idx);}

        //Reverse lookup by index (linear scan)
        CCol* find_cell(uint32_t idx) {
            for(int i=0;i<capacity;i++) {
                CCol& c = get(i);
                if(c.storage) {
                    if(c.index==idx) {return &c;}
                }
            }
            return nullptr;
        }
        uint32_t find_cell_idx(uint32_t idx) {
            CCol* c = find_cell(idx);
            if(c) {return (uint32_t)((uint8_t*)c-storage);}
            else {throw_error("QCellCol:find_cell_idx no cell was found for idx ",idx); return 0;}
        }

        CCol* find_cell(const void* key, uint32_t key_size) {
            uint32_t h = hashBytes(key, key_size);
            uint32_t pos = h%capacity;
            uint32_t traversed = 0;
            while(traversed<capacity) {
                CCol& c = get(pos);
                if(!c.storage) {return nullptr;}
                if(c.hash==h&&memcmp(c.storage, key, key_size)==0) {return &c;}
                traversed++;
                pos = (pos+1)%capacity;
            }
            return nullptr;
        }
        inline bool hasKey(const void* key, uint32_t key_size) {return find_cell(key, key_size)!=nullptr;}
        inline CCol& get(const void* key, uint32_t key_size) {
            CCol* cell = find_cell(key, key_size);
            if(cell) {return *cell;}
            else {throw_error("Key of size ",key_size," not found"); return get(0);}
        }

        void removeAtByValue(uint32_t target_index) {
            uint32_t pos = 0;
            bool found = false;
            for(int i=0;i<capacity;i++) {
                CCol& c = get(i);
                if(c.storage) {
                    if(c.index==target_index) {found = true; pos = i;}
                    else if(c.index>target_index) {c.index-=1;}
                }
            }
            if(!found) {return;} //Nothing corresponding to the index was found so return early
            uint32_t traversed = 0;
            while(traversed<capacity) {
                uint32_t next_pos = (pos+1)%capacity;
                CCol& next = get(next_pos);
                if(!next.storage||cell_distance_from_home(next.hash, next_pos)==0) {return;}
                memcpy(&storage[pos*sizeof(CCol)],(void*)&next,sizeof(CCol));
                memset(&storage[next_pos*sizeof(CCol)],0,sizeof(CCol));
                traversed++;
                pos = next_pos;
            }
        }

        void removeAt(uint32_t pos) {
            uint32_t traversed = 0;
            get(pos).~CCol();       
            size--;     
            while(traversed<capacity) {
                uint32_t next_pos = (pos+1)%capacity;
                CCol& next = get(next_pos);
                if(!next.storage||cell_distance_from_home(next.hash, next_pos)==0) {return;}
                memcpy(&storage[pos*sizeof(CCol)],(void*)&next,sizeof(CCol));
                memset(&storage[next_pos*sizeof(CCol)],0,sizeof(CCol));
                traversed++;
                pos = next_pos;
            }
            throw_error("QCellCol corrupted: removeAt traversed the entirety of capacity, meaning either capacity was corrupted or all cells were somehow set");
        }
    };

    struct Col : CCol {
        Col() {}
        Col(uint32_t _size) :  CCol(_size) {}
        Col(const Col& o) : CCol((const CCol&)o), heterogenous(o.heterogenous), label(o.label), cells(o.cells) {}
        Col(CCol q) : CCol(q) {}
        Col(Col&& o) noexcept : CCol(std::move(o)), heterogenous(o.heterogenous),label(std::move(o.label)), cells(std::move(o.cells)), free(std::move(o.free)) {}
        Col& operator=(Col&& o) {
            if(this == &o) return *this;
            CCol::operator=(std::move(o)); 
            heterogenous = o.heterogenous;
            label = std::move(o.label);
            cells = std::move(o.cells);
            free = std::move(o.free);
            return *this;
        }
        bool heterogenous = false;
        QString label;
        QCellCol cells;
        list<uint32_t> free;
        
        inline void* get(uint32_t index) {
            if(heterogenous) {
                return qget(index);
            } else {
                return sget(index);
            }
        }
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return qget(size-(element_size));}

        void qput(const void* element, const void* key, uint32_t key_size, uint32_t key_tag) {
            CCol c;
            c.element_size = key_size; 
            c.tag = key_tag;
            c.hash = hashBytes(key, key_size);
            c.index = length();
            c.push(key);
            push(element);
            cells.scan_for_slot(c);
        }

        inline CCol& getOrPut(const void* key, uint32_t key_size, uint32_t key_tag) {
            CCol* cell = cells.find_cell(key, key_size);
            if(cell) {return *cell;}
            else {
                CCol c;
                c.hash = hashBytes(key,key_size); c.element_size = key_size; 
                c.tag = key_tag; c.index = length(); c.push(key);
                cells.scan_for_slot(c);
                push_default();
                return cells.get(key,key_size);
            }
        }

        uint32_t getidx(const void* key, uint32_t key_size) {
            CCol* c = cells.find_cell(key,key_size);
            if(c) {return c->index;} 
            else {
                throw_error("Col:getidx Key not found of size ",key_size);
                return 0;
            }
        }

        void* get(const void* key, uint32_t key_size) {
            uint32_t idx = getidx(key,key_size);
            if(ERROR_FLAG) {return nullptr;}
            else return get(idx);
        }
        inline bool hasKey(const void* key, uint32_t key_size) {return cells.hasKey(key,key_size);}

        void put(const std::string& str, const void* element, uint32_t tag = 0) {qput(element,str.data(),str.length(),tag);}
        void* get(const std::string& str) {return get(str.data(), str.length());}
        bool hasKey(const std::string& str) {return hasKey(str.data(), str.length());}
        void put(uint64_t u64, const void* element, uint32_t tag = 0) {qput(element,(void*)&u64,8,tag);}
        void* get(uint64_t u64) {return get((void*)&u64, 8);}
        bool hasKey(uint64_t u64) {return hasKey((void*)&u64, 8);}
        void put(Ptr p, const void* element, uint32_t tag = 0) {qput(element, (void*)&p, sizeof(Ptr), tag);}
        void* get(Ptr p) {return get((void*)&p, sizeof(Ptr));}
        bool hasKey(Ptr p) {return hasKey((void*)&p, sizeof(Ptr));}

        void removeAt(uint32_t index) {
            if(!cells.empty()) {cells.removeAtByValue(index);}
            CCol::removeAt(index);
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
        //print("write_ccol size: ", col.size, " storage: ", (void*)col.storage);
        write_qcol(out,col);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<uint32_t>(out, col.index);
        write_raw<uint32_t>(out, col.gen);
        write_raw<bool>(out, col.live);
    }

    static CCol read_ccol(std::istream& in) {
        CCol col = read_qcol(in);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.index = read_raw<uint32_t>(in);
        col.gen = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        return col;
    }

    static void write_qcellcol(std::ostream& out, QCellCol& cells) {
        uint32_t count = 0;
        list<CCol*> to_save;
        for(uint32_t i = 0; i < cells.length(); i++) {
            if(cells.get(i).storage) {
                count++;
                to_save << &cells.get(i);
            }
        }
        write_raw<uint32_t>(out, count);
        for(uint32_t i = 0; i < to_save.length(); i++) {
            CCol& c = *to_save[i];
            write_ccol(out, *to_save[i]);
        }
    }
    
    static QCellCol read_qcellcol(std::istream& in) {
        QCellCol cells;
        uint32_t count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < count; i++) {
            CCol c = read_ccol(in);
            c.hash = hashBytes(c.storage,c.size);
            cells.scan_for_slot(c);
        }
        return cells;
    }

    static void write_col(std::ostream& out, Col& col) {
        write_ccol(out,col);
        write_raw<bool>(out, col.heterogenous);
        write_qcellcol(out, col.cells);
        write_qcol(out,col.label);
        write_raw<uint32_t>(out,col.free.length());
        for(int i=0;i<col.free.length();i++) {
            write_raw<uint32_t>(out,col.free[i]);
        }
    }

    static Col read_col(std::istream& in) {
        Col col = read_ccol(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcellcol(in);
        col.label = read_qcol(in);
        uint32_t freelen = read_raw<uint32_t>(in);
        for(int i=0;i<freelen;i++) {
            col.free << read_raw<uint32_t>(in);
        }
        return col;
    }

    static void write_col_header(std::ostream& out, Col& col) {
        //write_raw<uint32_t>(out, col.size);
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.tag);
        write_raw<uint32_t>(out, col.hash);
        write_raw<uint32_t>(out, col.index);
        write_raw<uint32_t>(out, col.gen);
        write_raw<bool>(out, col.live);
        write_raw<bool>(out, col.heterogenous);
        write_qcellcol(out, col.cells);
        write_qcol(out,col.label);
        write_raw<uint32_t>(out,col.free.length());
        for(int i=0;i<col.free.length();i++) {
            write_raw<uint32_t>(out,col.free[i]);
        }
    }

    static Col read_col_header(std::istream& in) {
        Col col;
        // uint32_t size = read_raw<uint32_t>(in);
        // col.resize(size);
        col.element_size = read_raw<uint32_t>(in);
        col.tag = read_raw<uint32_t>(in);
        col.hash = read_raw<uint32_t>(in);
        col.index = read_raw<uint32_t>(in);
        col.gen = read_raw<uint32_t>(in);
        col.live = read_raw<bool>(in);
        col.heterogenous = read_raw<bool>(in);
        col.cells = read_qcellcol(in);
        col.label = read_qcol(in);
        uint32_t freelen = read_raw<uint32_t>(in);
        for(int i=0;i<freelen;i++) {
            col.free << read_raw<uint32_t>(in);
        }
        return col;
    }
}

