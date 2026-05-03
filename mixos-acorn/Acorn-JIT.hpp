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
        char_class['='] = 4; char_class['+'] = 4; char_class['-'] = 4; char_class['*'] = 4; char_class['/'] = 4;

        //5 = terminator
        char_class[':'] = 5;

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
            print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec);
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

        for(int i=0;i<5;i++) {
            AcornCol type;
            type.element_size = sizeof(AcornCol);
            for(int c=0;c<5;c++) {
                AcornCol column;
                column.element_size = 4;
                for(int r=0;r<5;r++) {
                    column.push_default();
                }
                type.push((void*)&column);
            }
            types.push((void*)&type);
        }
       
        print("TYPES INITLIZED");
        int five = 5;
        print("FIRST: ",types.size," ESIZE: ",types.element_size);
        print("SECOND: ",(*(AcornCol*)types[0]).size," ESIZE: ",(*(AcornCol*)types[0]).element_size);
        print("THIRD: ",(*(AcornCol*)(*(AcornCol*)types[0])[0]).size," ESIZE: ",(*(AcornCol*)(*(AcornCol*)types[0])[0]).element_size);
        (*(AcornCol*)(*(AcornCol*)types[0])[0]).set(0,(void*)&five);
        print("SHOULD BE 5: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[0])[0])[0]));
        print("TYPES WORKING");

        int three = 3;
        (*(AcornCol*)(*(AcornCol*)types[0])[1]).set(0,(void*)&three);

        // (*(AcornCol*)(*(AcornCol*)types[1])[0]).element_size = 6;
        // (*(AcornCol*)(*(AcornCol*)types[1])[1]).element_size = 7;
        //(*(AcornCol*)(*(AcornCol*)types[2])[0]).element_size = 8;

        // Level 0: base of types
        print("types base: 0x", std::hex, (uint64_t)types.storage, std::dec);

        // Level 1: type 2's AcornCol
        AcornCol* type2 = (AcornCol*)types[0];
        print("type 2: 0x", std::hex, (uint64_t)type2, std::dec);

        // Level 2: col 0 of type 2  
        AcornCol* col0 = (AcornCol*)(*type2)[0];
        print("type2 col0: 0x", std::hex, (uint64_t)col0, std::dec);

        // Level 3: row 0 of col 0
        void* row3 = (*col0)[0];
        print("type2 col0 row0: 0x", std::hex, (uint64_t)row3, std::dec);

        col0->set(0,(void*)&five);
        col0->set(1,(void*)&three);

        // print((*(AcornCol*)(*(AcornCol*)types[2])[0]));

        typedef int (*JitFunc)(AcornCol);
        JitFunc func = (JitFunc)buf;

        //Arbitray ceremony because Arm64, this is why I'm making my own ISA
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        int result = func(types); //Giving the source

        print("FINISHED: ",result);
        print("types base: 0x", std::hex, (uint64_t)types.storage, std::dec);
        print("SHOULD BE 5: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[0])[0])[0]));
        print("0|0|0: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[0])[0])[0])," 1|0|0: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[1])[0])[0]),"\n",
              "0|0|1: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[0])[0])[1])," 1|0|1: ",*(int*)((*(AcornCol*)(*(AcornCol*)types[1])[0])[1])
        );

        munmap(buf, byte_size); //Cleanup
    }
}