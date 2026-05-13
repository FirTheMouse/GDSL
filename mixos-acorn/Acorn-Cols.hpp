#pragma once
#include "../mixos-acorn/Acorn-Core.hpp"
namespace Acorn {
    struct AcornCol {
        AcornCol(uint32_t _size = 1) : element_size(_size) {}
        uint8_t* storage = nullptr;
        uint32_t element_size;
        uint32_t size = 0;
        uint32_t capacity = 0;
        uint32_t tag = 0;

        inline uint32_t length() {return size/ element_size;}
        inline bool empty() {return size==0;}

        void reserve(uint32_t new_capacity) {
            if (new_capacity <= capacity) return;
            
            uint8_t* newPtr = new uint8_t[new_capacity];
            for (size_t i = 0; i < size; ++i) {
                newPtr[i] = std::move(storage[i]);
            }
            
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

        void push(const void* element) {
            size_t old_size = size;
            resize(old_size + element_size);
            memcpy(&storage[old_size], element, element_size);
        }
        void operator<<(const void* element) {push(element);}
        void push_default() {
            size_t old_size = size;
            resize(old_size + element_size);
            memset(&storage[old_size], 0, element_size);
        }

        void insert(const void* element, uint32_t index) {
            push_default();
            for(uint32_t i = length()-1; i > index; --i) {
                memcpy(get(i), get(i-1), element_size);
            }
            memcpy(get(index), element, element_size);
        }
        
        inline void* get(uint32_t index) {return &storage[index * element_size];}
        inline void* operator[](uint32_t index) {return get(index);}
        inline void* last() {return get(size-1);}

        inline void set(size_t index, const void* element) {memcpy(&storage[index * element_size], element, element_size);}

        void removeAt(uint32_t index) {
            size_t byte_start = index * element_size;
            for(size_t i = byte_start; i < size - element_size; i++) {
                storage[i] = storage[i + element_size];
            }
            resize(size - element_size);
        }

        void clear() {
            size = 0;
        }

        void pop(void* out) {
            memcpy(out, get(length()-1), element_size);
            resize(size - element_size);
        }
    };

    static void write_acorn_col(std::ostream& out, AcornCol& col) {
        write_raw<uint32_t>(out, col.element_size);
        write_raw<uint32_t>(out, col.length());
        out.write((const char*)col.storage, col.size);
        write_raw<uint32_t>(out, col.tag);
    }

    static AcornCol read_acorn_col(std::istream& in) {
        AcornCol col;
        col.element_size = read_raw<uint32_t>(in);
        uint32_t len = read_raw<uint32_t>(in);
        col.resize(len * col.element_size);
        in.read((char*)col.storage, col.size);
        col.tag = read_raw<uint32_t>(in);
        return col;
    }

    static void write_acorn_types(std::ostream& out, AcornCol& types, int regtick) {
        write_raw<int>(out,regtick);
        write_acorn_col(out, types);
        for(int i = 0; i < types.length(); i++) {
            AcornCol& type = *(AcornCol*)types[i];
            write_acorn_col(out, type);
            for(int c = 0; c < type.length(); c++) {
                AcornCol& col = *(AcornCol*)type[c];
                write_acorn_col(out, col);
            }
        }
    }

    static AcornCol read_acorn_types(std::istream& in, int& regtick) {
        regtick = read_raw<int>(in);
        AcornCol types = read_acorn_col(in);
        for(uint32_t p = 0; p < types.length(); p++) {
            AcornCol type = read_acorn_col(in);
            for(uint32_t i = 0; i < type.length(); i++) {
                AcornCol col = read_acorn_col(in);
                type.set(i, (void*)&col);
            }
            types.set(p, (void*)&type);
        }
        return types;
    }

    static std::string acorn_type_to_string(AcornCol& type) {
        list<list<std::string>> lines;
        for(int c = 0; c < type.length(); c++) {
            AcornCol& col = *(AcornCol*)type[c];
            list<std::string> subline;
            if(type.tag==1) { //For col trunctable plates, we skip if a column starts with 0
                if(*(uint32_t*)col[0]==0||*(uint32_t*)col[0]==3596551104) continue;
                else subline << (std::to_string(c)+":["+std::to_string(col.element_size)+"]");
            } else {
                subline << ("["+std::to_string(col.element_size)+"]");
            }
            for(int r = 0; r < col.length(); r++) {
                if(col.tag==char_id) {
                    char ch = *(char*)col[r];
                    if(ch=='\n') {
                        subline << "\\n";
                    } else {
                        subline << std::string(1,ch);
                    }
                    // subline << std::to_string(*(uint8_t*)col[r]);
                } else {
                    if(col.element_size == 4) {
                        subline << std::to_string(*(uint32_t*)col[r]);
                    } else if(col.element_size == 1) {
                        subline << std::to_string(*(uint8_t*)col[r]);
                    } else if(col.element_size == 12) {
                        uint32_t* p = (uint32_t*)col[r];
                        subline << (std::to_string(p[0])+"|"+
                                std::to_string(p[1])+"|"+
                                std::to_string(p[2]));
                    } else {
                        subline << ("?["+std::to_string(col.element_size)+"]");
                    }
                }
            }
            lines << subline;
        }
        return print_columnar_table(lines);
    }

    static std::string acorn_types_to_string(AcornCol& types, list<std::string> type_names) {
        std::string result = "";
        for(int t = 0; t < types.length(); t++) {
            AcornCol& type = *(AcornCol*)types[t];
            if(t<type_names.length()&&!type_names[t].empty()) {
                result+=type_names[t]+"\n";
            } else {
                result += "Type " + std::to_string(t) + 
                        " (tag:" + std::to_string(type.tag) + "):\n";
            }
            result += acorn_type_to_string(type) + "\n";
        }
        return result;
    }

}