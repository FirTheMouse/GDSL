


#pragma once

#include "../mixos-acorn/Acorn-Dirt.hpp"

#ifdef __APPLE__ 
    #include <pthread.h>
    #include <libkern/OSCacheControl.h>
#endif

namespace Acorn {
    static void JIT_dirt(std::string source, bool include_ambles = false){
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

        if(include_ambles) print("==EXECUTING==");
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
        char_class['L'] = 4; char_class['E'] = 4;

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


        char_class['>'] = 19; //Rangle, defref rigth
        char_class['<'] = 20; //Langle, defref left

        char_class['\\'] = 21; //Comment

        uint64_t fya64[4]; fya64[0] = 0; fya64[1] = 0; fya64[2] = 0; fya64[3] = 0;
        typedef int (*JitFunc)(const char*, uint32_t, uint8_t*, uint64_t[4]);
        JitFunc func = (JitFunc)buf;

        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        
        int result = func(source.data(), source.length(),char_class,fya64); //Giving the source

        if(result!=7) {
            print(red("WHO REMOVED THE WUB"));
        }
        munmap(buf, byte_size); //Cleanup

        if(include_ambles) {
            list<uint32_t> preamble = { //Preamble
                2847898621,
                2852127734,
                2853569524,
                2853569525,
                2852193271,
                1384120344,
                2852258809
            };
            sub_instruction_buffer.insertAll(preamble,0);

            list<uint32_t> postamble = { //Postamble
                2831252477,
                3596551104
            };
            sub_instruction_buffer.pushAll(postamble);

            print("==ACORN BUFFER==");
            path = "mixos-acorn/tests/acornoutput.gld";
            std::ofstream out(path, std::ios::binary);
            if(!out) throw std::runtime_error("Can't write to file: " + path);
            write_raw<uint32_t>(out,sub_instruction_buffer.length());
            for(int i=0;i<sub_instruction_buffer.length();i++) {
                write_raw<uint32_t>(out,sub_instruction_buffer[i]);
                uint32_t instr = sub_instruction_buffer[i];
                //print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);
            }
            sub_instruction_buffer.clear();
            out.close();
        }
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
            if(type.tag==1) { //For col trunctable plates, we skip if a column starts with 0
                if(*(uint32_t*)col[0]==0) continue;
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

    map<char,list<uint32_t>> instrs; 

    static void burn_instrs(){
        std::string isrc = readFile("mixos-acorn/tests/acorninstrs.gld");
        list<std::string> l = split_str(isrc,'{');
        char parsing_for = 'a';
        for(int i=0;i<l.length();i++) {
            if(i%2==1) {
                parsing_for = l[i].at(0);
            } else {
                JIT_dirt(l[i]);
                instrs[parsing_for] = sub_instruction_buffer;
                sub_instruction_buffer.clear();
            }
        }

        //Preamble
        instrs[1] = {  
            2847898621,
            2852127734,
            2853569524,
            2853569525,
            2852193271,
            1384120344,
            2852258809
        };
        //Postamble
        instrs[2] = { 
            2831252477,
            3596551104
        };
    };

    #define IS_SETUP 1
    uint32_t instr_r = 64;
    uint32_t ribbon_r = 2560;

    static void JIT_Acorn(){
        std::string path = "mixos-acorn/tests/acornoutput.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            uint32_t instr = read_raw<uint32_t>(in);
            emit_buffer << instr;
            //print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);
        }
        in.close();


        print("==EXECUTING ACORN==");

        

        std::string source = readFile("mixos-acorn/tests/sock.gld");
       // source = '\x01' + source + '\x02';

        #if IS_SETUP
            AcornCol types;
            types.element_size = sizeof(AcornCol);

            AcornCol reg_plate; // $ - the core plate for fast opperations, all Ptrs
            reg_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<6;c++) { //32 columns
                AcornCol column;
                column.element_size = 12; //96bits, for a Ptr
                for(int r=0;r<6;r++) { //6 rows
                    if(r==0) { //Special bang sets
                        uint32_t th[3] = {0, 0, 0};
                        if(c==2) {th[0] = source.length(); column.push((void*)th); }
                        else if(c==4) {th[0] = instr_r; column.push((void*)th); }
                        else {column.push_default(); }
                    } else {
                        column.push_default();
                    }
                }
                reg_plate.push((void*)&column);
            }
            types.push((void*)&reg_plate);

            AcornCol instr_plate; // # - where all the instructions live
            instr_plate.element_size = sizeof(AcornCol);
            instr_plate.tag = 1; //Make col trunctable
            for(int c=0;c<256;c++) { //256 columns, one for each char
                AcornCol column;
                column.element_size = 4; //32bits, for one word on a processer
                list<uint32_t> instr; //Lookup and use
                if(instrs.hasKey(c)) instr = instrs.get(c);
                size_t instr_len = instr.length();
                for(int r=0;r<instr_len;r++) { 
                    column.push((void*)&instr[r]);
                }
                int instr_itr = instr_r - instr_len;
                for(int r=0;r<instr_itr;r++) { //64 rows
                    uint32_t ret = 0xD65F03C0;
                    column.push((void*)&ret);
                }
                instr_plate.push((void*)&column);
            }
            types.push((void*)&instr_plate);

            print("instr_plate.storage: ", (uint64_t)instr_plate.storage);
            print("instr_plate[97]: ", (uint64_t)instr_plate[97]);
            AcornCol& col_a = *(AcornCol*)instr_plate[97]; // 'a' handler
            print("col_a ptr: ", (uint64_t)&col_a);
            print("col_a storage: ", (uint64_t)col_a.storage);

            AcornCol string_plate; // " - where strings are stored
            string_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<1;c++) { //1 column, because we've got a lot of rows and this is for tests
                AcornCol column;
                column.element_size = 4; //8bits, for one char (padded)
                column.tag = char_id;
                for(int r=0;r<source.length();r++) { //as many rows as in source
                    uint32_t ch = source.at(r);
                    column.push((void*)&ch);
                }
                string_plate.push((void*)&column);
            }
            types.push((void*)&string_plate);

            AcornCol ribbon_plate; // 3 - ribbon plate for instructions
            ribbon_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<1;c++) { //1 column, because we've got a lot of rows and this is for tests
                AcornCol column;
                column.element_size = 4; //32bits, instruction size
                for(int r=0;r<ribbon_r;r++) {
                    column.push_default();
                }
                ribbon_plate.push((void*)&column);
            }
            types.push((void*)&ribbon_plate);

            AcornCol context_plate; // 4 - the stack and storage spot for reg_plate rows 
            context_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<5;c++) { //32 columns
                AcornCol column;
                column.element_size = 12; //96bits, for a Ptr
                for(int r=0;r<12;r++) { //12 rows
                    column.push_default();
                }
                context_plate.push((void*)&column);
            }
            types.push((void*)&context_plate);

            for(int c = 0; c < 256; c++) {
                AcornCol& col = *(AcornCol*)instr_plate[c];
                if(col.size == 0) continue;
                
                size_t byte_size = col.size;
                void* exec_buf = mmap(nullptr, byte_size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
                    -1, 0);
                
                memcpy(exec_buf, col.storage, byte_size);
                
                #ifdef __APPLE__
                    pthread_jit_write_protect_np(1);
                    sys_icache_invalidate(exec_buf, byte_size);
                #endif
                
                mprotect(exec_buf, byte_size, PROT_READ | PROT_EXEC);
                
                delete[] col.storage;
                col.storage = (uint8_t*)exec_buf;
            }
        #else
            path = "mixos-acorn/tests/acornsock.gld";
            std::ifstream in2(path, std::ios::binary);
            AcornCol types = read_acorn_types(in2);

            AcornCol string_plate; // " - where strings are stored
            string_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<1;c++) { //1 column, because we've got a lot of rows and this is for tests
                AcornCol column;
                column.element_size = 4; //32bits, for one char (padded)
                column.tag = char_id;
                for(int r=0;r<source.length();r++) { //as many rows as in source
                    uint32_t ch = source.at(r);
                    column.push((void*)&ch);
                }
                string_plate.push((void*)&column);
            }
            types.set(2,(void*)&string_plate);

            uint32_t th[3] = {(uint32_t)source.length(), 0, 0};
            (*(AcornCol*)(*(AcornCol*)types[0])[2]).set(0, (void*)th);

            AcornCol& oldribbon = *(AcornCol*)(*(AcornCol*)types[3])[0]; //Grab the ribbon
            memset(oldribbon.storage, 0, oldribbon.size);
        #endif

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

        typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol);
        JitFunc func = (JitFunc)buf;
        print("buf base: ",(uint64_t)buf);
        //Arbitray ceremony because Arm64, this is why I'm making my own ISA
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        int result = func(types,*(AcornCol*)types[0],*(AcornCol*)types[1]); //Giving the source

        print("FINISHED: ",result);
        
        std::ofstream out2(path, std::ios::binary);
        write_acorn_types(out2,types);

        path = "mixos-acorn/tests/acornribbon.gld"; 
        std::ofstream out3(path, std::ios::binary);

        AcornCol& ribbon = *(AcornCol*)(*(AcornCol*)types[3])[0];
        uint32_t ribbonlen = 0;
        for(int i=0;i<ribbon.length();i++) {
            uint32_t rib_instr = *(uint32_t*)ribbon[i];
            if(rib_instr==0) {ribbonlen = i; break;}
        }
        print("Ribbon length: ",ribbonlen,"/",ribbon_r);
        write_raw(out3,ribbonlen);
        for(int i=0;i<ribbonlen;i++) {
            write_raw<uint32_t>(out3,*(uint32_t*)ribbon[i]);
        }

        std::string printout = "";
        printout+=acorn_types_to_string(types,{"Reg plate","Instr plate","String plate","Ribbon plate"});
        editTextFile("mixos-acorn/tests/printout.txt",[printout](std::string& source){
            source=printout;
        });

        munmap(buf, byte_size); //Cleanup
    }

    static void JIT_Ribbon(){
        std::string path = "mixos-acorn/tests/acornribbon.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            emit_buffer << read_raw<uint32_t>(in);
        }
        in.close();

        AcornCol types;
        types.element_size = sizeof(AcornCol);

        AcornCol reg_plate; // $ - the core plate for fast opperations, all Ptrs
        reg_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<6;c++) { //32 columns
            AcornCol column;
            column.element_size = 12; //96bits, for a Ptr
            for(int r=0;r<6;r++) { //6 rows
                column.push_default();
            }
            reg_plate.push((void*)&column);
        }
        types.push((void*)&reg_plate);

        AcornCol instr_plate; // # - where all the instructions live
        instr_plate.element_size = sizeof(AcornCol);
        instr_plate.tag = 1; //Make col trunctable
        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol column;
            column.element_size = 4; //32bits, for one word on a processer
            for(int r=0;r<instr_r;r++) { //64 rows
                column.push_default();
            }
            instr_plate.push((void*)&column);
        }
        types.push((void*)&instr_plate);

        AcornCol string_plate; // " - where strings are stored
        string_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<1;c++) { //1 column, because we've got a lot of rows and this is for tests
            AcornCol column;
            column.element_size = 4; //8bits, for one char (padded)
            column.tag = char_id;
            for(int r=0;r<24;r++) { //as many rows as in source
                uint32_t ch = 32; //A whitespace
                column.push((void*)&ch);
            }
            string_plate.push((void*)&column);
        }
        types.push((void*)&string_plate);

        AcornCol ribbon_plate; // 3 - ribbon plate for instructions
        ribbon_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<1;c++) { //1 column, because we've got a lot of rows and this is for tests
            AcornCol column;
            column.element_size = 4; //32bits, instruction size
            for(int r=0;r<ribbon_r;r++) {
                column.push_default();
            }
            ribbon_plate.push((void*)&column);
        }
        types.push((void*)&ribbon_plate);

        AcornCol context_plate; // 4 - the stack and storage spot for reg_plate rows 
        context_plate.element_size = sizeof(AcornCol);
        for(int c=0;c<5;c++) { //32 columns
            AcornCol column;
            column.element_size = 12; //96bits, for a Ptr
            for(int r=0;r<12;r++) { //12 rows
                column.push_default();
            }
            context_plate.push((void*)&column);
        }
        types.push((void*)&context_plate);

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

        typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol);
        JitFunc func = (JitFunc)buf;

        //Arbitray ceremony because Arm64, this is why I'm making my own ISA
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        int result = func(types,*(AcornCol*)types[0],*(AcornCol*)types[1]); //Giving the source

        print("FINISHED: ",result);
        
        std::string printout = "";
        printout+=acorn_types_to_string(types,{"Reg plate","Instr plate","String plate","Ribbon plate"});
        editTextFile("mixos-acorn/tests/printout.txt",[printout](std::string& source){
            source=printout;
        });

        munmap(buf, byte_size); //Cleanup
    }
}