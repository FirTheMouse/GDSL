#pragma once

#include "../mixos-acorn/Acorn-Dirt.hpp"

#ifdef __APPLE__ 
    #include <pthread.h>
    #include <libkern/OSCacheControl.h>
#endif

namespace Acorn {
    static void JIT_dirt(){
        std::string path = "mixos-acorn/tests/dirtoutput.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            emit_buffer << read_raw<uint32_t>(in);
        }
        in.close();


        #ifdef __APPLE__
            pthread_jit_write_protect_np(0);
        #else
            #define MAP_JIT 0x0800
        #endif

        size_t byte_size = emit_buffer.length() * sizeof(uint32_t);
        void* buf = mmap(nullptr, byte_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
            -1, 0);

        if(buf == MAP_FAILED) {
            print("mmap failed: ", strerror(errno));
            return;
        }

        //Copy the instructions
        uint32_t* ptr = (uint32_t*)buf;
        for(int i=0;i<emit_buffer.length();i++) {
            ptr[i] = emit_buffer[i];
        }

        #ifdef __APPLE__
            pthread_jit_write_protect_np(1); // re-enable write protection before executing
            sys_icache_invalidate(buf, byte_size); // flush instruction cache
        #endif

        mprotect(buf, byte_size, PROT_READ | PROT_EXEC);

        print("==EXECUTING==");
        //0 = unclassified
        uint8_t char_class[256] = {0};
        //1 = numbers
        char_class['0'] = 1; char_class['1'] = 1; char_class['2'] = 1; char_class['3'] = 1; char_class['4'] = 1;
        char_class['5'] = 1; char_class['6'] = 1; char_class['7'] = 1; char_class['8'] = 1; char_class['9'] = 1;
        
        //2 = shift
        char_class['|'] = 2; char_class['{'] = 2; char_class['}'] = 2;

        //3 = escape
        char_class['~'] = 3;

        //4 = opperations
        char_class['='] = 4; char_class['+'] = 4; char_class['*'] = 4; char_class['/'] = 4;
        char_class['L'] = 4;

        //5 = terminator
        char_class[':'] = 5;

        //Top level indexing
        char_class['P'] = 6;
        char_class['I'] = 7;
        char_class['S'] = 8;

        char_class['v'] = 9; //Descend
        char_class['^'] = 10; //Ascend

        //Reg plate instructions
        char_class['$'] = 11; //Reg plate row n
        char_class['!'] = 12; //Reg plate row 0
        char_class['_'] = 13; //Reg plate row n+1
        char_class['#'] = 14; //Instr plate

        char_class['?'] = 15; //Qmark, where comparisons and such go

        char_class['J'] = 16; //J, for jumps
        char_class['C'] = 17; //C, for conditonal jumps

        char_class['-'] = 18; //Dash, for negative numbers


        char_class['>'] = 19; //Rangle, defref right
        char_class['<'] = 20; //Langle, defref left

        char_class['\\'] = 21; //Comment


        typedef int (*JitFunc)(const char*, uint32_t, uint8_t*);
        JitFunc func = (JitFunc)buf;
        std::string source = readFile("mixos-acorn/tests/acorn.gld");

        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        
        int result = func(source.data(), source.length(),char_class); //Giving the source

        if(result!=7) {
            print(red("WHO REMOVED THE WUB"));
        }
        munmap(buf, byte_size); //Cleanup


        print("==ACORN BUFFER==");
        path = "mixos-acorn/tests/acornoutput.gld";
        std::ofstream out(path, std::ios::binary);
        if(!out) throw std::runtime_error("Can't write to file: " + path);
        write_raw<uint32_t>(out,sub_instruction_buffer.length());
        for(int i=0;i<sub_instruction_buffer.length();i++) {
            write_raw<uint32_t>(out,sub_instruction_buffer[i]);
            uint32_t instr = sub_instruction_buffer[i];
            print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);
        }
        
        out.close();
    }


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

    static void write_acorn_types(std::ostream& out, AcornCol& types) {
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

    static AcornCol read_acorn_types(std::istream& in) {
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
            subline << ("["+std::to_string(col.element_size)+"]");
            for(int r = 0; r < col.length(); r++) {
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
            lines << subline;
        }
        return print_columnar_table(lines);
    }

    static std::string acorn_types_to_string(AcornCol& types) {
        std::string result = "";
        for(int t = 0; t < types.length(); t++) {
            AcornCol& type = *(AcornCol*)types[t];
            result += "Type " + std::to_string(t) + 
                      " (tag:" + std::to_string(type.tag) + "):\n";
            result += acorn_type_to_string(type) + "\n";
        }
        return result;
    }

    static void JIT_Acorn(){
        std::string path = "mixos-acorn/tests/acornoutput.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            emit_buffer << read_raw<uint32_t>(in);
        }
        in.close();


        #ifdef __APPLE__
            pthread_jit_write_protect_np(0);
        #else
            #define MAP_JIT 0x0800
        #endif

        size_t byte_size = emit_buffer.length() * sizeof(uint32_t);
        void* buf = mmap(nullptr, byte_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
            -1, 0);

        if(buf == MAP_FAILED) {
            print("mmap failed: ", strerror(errno));
            return;
        }

        //Copy the instructions
        uint32_t* ptr = (uint32_t*)buf;
        for(int i=0;i<emit_buffer.length();i++) {
            ptr[i] = emit_buffer[i];
        }

        #ifdef __APPLE__
            pthread_jit_write_protect_np(1); // re-enable write protection before executing
            sys_icache_invalidate(buf, byte_size); // flush instruction cache
        #endif

        mprotect(buf, byte_size, PROT_READ | PROT_EXEC);

        print("==EXECUTING==");

        AcornCol types;
        types.element_size = sizeof(AcornCol);

        AcornCol reg_plate; // $ - the core plate for fast opperations, all Ptrs
        reg_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<4;c++) { //32 columns
            AcornCol column;
            column.element_size = 12; //96bits, for a Ptr
            for(int r=0;r<6;r++) { //6 rows
                column.push_default();
            }
            reg_plate.push((void*)&column);
        }

        AcornCol instr_plate; // # - where all the instructions live
        instr_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol column;
            column.element_size = 4; //32bits, for one word on a processer
            for(int r=0;r<64;r++) { //64 rows
                column.push_default();
            }
            instr_plate.push((void*)&column);
        }

        AcornCol context_plate; // 0 - the stack and storage spot for reg_plate rows 
        context_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<32;c++) { //32 columns
            AcornCol column;
            column.element_size = 12; //96bits, for a Ptr
            for(int r=0;r<12;r++) { //12 rows
                column.push_default();
            }
            context_plate.push((void*)&column);
        }
        types.push((void*)&context_plate);

        // path = "mixos-acorn/tests/acornsock.gld";
        // std::ifstream in2(path, std::ios::binary);
        // AcornCol types = read_acorn_types(in2);


        typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol);
        JitFunc func = (JitFunc)buf;

        //Arbitray ceremony because Arm64, this is why I'm making my own ISA
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        int result = func(types,reg_plate,instr_plate); //Giving the source

        print("FINISHED: ",result);
        
        std::ofstream out2(path, std::ios::binary);
        write_acorn_types(out2,types);

        std::string printout = "";
        printout+="Reg plate:\n";
        printout+=acorn_type_to_string(reg_plate)+"\n\n";
        printout+=acorn_types_to_string(types);
        editTextFile("mixos-acorn/tests/printout.txt",[printout](std::string& source){
            source=printout;
        });

        munmap(buf, byte_size); //Cleanup
    }
}