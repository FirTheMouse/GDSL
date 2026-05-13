


#pragma once

#include "../mixos-acorn/Acorn-Dirt.hpp"
#include "../mixos-acorn/Acorn-Cols.hpp"

#ifdef __APPLE__ 
    #include <pthread.h>
    #include <libkern/OSCacheControl.h>
#endif

namespace Acorn {

    list<uint32_t> descend = {0x52800023, 0x0B030318};
    list<uint32_t> ascend = {0x52800023, 0x4B030318};

    list<uint32_t> preamble = { //Preamble
        2847898621,
        2852127734,
        2853569524,
        2853569525,
        2852193271,
        704840696,
        2852258809,
        0x52800023, 0x0B030318
    };
    list<uint32_t> postamble = { //Postamble
        706216928,
        2831252477,
        3596551104
    };


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

        AcornCol emittarget;
        if(include_ambles) {
            print("==EXECUTING==");
            emittarget.element_size = 4; //uint32_t
            emitcol = &emittarget;
        }
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

        char_class['['] = 22; //Lbracket
        char_class[']'] = 23; //Rbracket

        char_class['`'] = 24; //Tick

        uint64_t fya64[11]; 
        fya64[0] = 0; fya64[1] = 0; fya64[2] = 0; fya64[3] = 0;
        fya64[4] = 0; fya64[5] = 0; fya64[6] = 0; fya64[7] = 0;
        fya64[8] = 0; fya64[9] = 0; fya64[10] = 0;
        typedef int (*JitFunc)(const char*, uint32_t, uint8_t*, uint64_t[11]);
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
            for(int i=preamble.length()-1;i>=0;i--) {
                (*emitcol).insert((void*)&preamble[i],0);
            }
            for(int i=0;i<postamble.length();i++) {
                (*emitcol).push((void*)&postamble[i]);
            }

            print("==ACORN BUFFER==");
            path = "mixos-acorn/tests/acornoutput.gld";
            std::ofstream out(path, std::ios::binary);
            if(!out) throw std::runtime_error("Can't write to file: " + path);
            write_raw<uint32_t>(out,(*emitcol).length());
            for(int i=0;i<(*emitcol).length();i++) {
                uint32_t instr = *(uint32_t*)((*emitcol)[i]);
                write_raw<uint32_t>(out,instr);
                //print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);
            }
            out.close();
        }
    }

    AcornCol instrs; 

    static void burn_instrs(){
        instrs.element_size = sizeof(AcornCol);
        instrs.tag = 1; //Make col trunctable
        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol column;
            column.element_size = 4; //32bits, for one word on a processer
            instrs.push((void*)&column);
        }

        std::string isrc = readFile("mixos-acorn/tests/acorninstrs.gld");
        list<std::string> l = split_str(isrc,'{');
        char parsing_for = 'a';
        for(int i=0;i<l.length();i++) {
            if(i%2==1) {
                parsing_for = l[i].at(0);
            } else {
                if(l[i].empty()) continue;
                emitcol = (AcornCol*)instrs[parsing_for];
                JIT_dirt(l[i]);
            }
        }

        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol& col = *(AcornCol*)instrs[c];
            uint32_t ret = 0xD65F03C0;
            col.push((void*)&ret);
        }
    };


    uint32_t instr_r = 64;
    uint32_t ribbon_r = 2560;

    static void JIT_Acorn(int stage = -1){
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

        path = "mixos-acorn/tests/acornsock.gld";

        AcornCol types;
        int regtick = 0;
        if(stage==-1) {
            types.element_size = sizeof(AcornCol);

            AcornCol reg_plate; // $ - the core plate for fast opperations, all Ptrs
            reg_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<32;c++) { //32 columns
                AcornCol column;
                column.element_size = 12; //96bits, for a Ptr
                for(int r=0;r<6;r++) { //6 rows
                    if(r==1) { //Special bang sets
                        uint32_t th[3] = {0, 0, 0};
                        if(c==2) {th[0] = source.length(); column.push((void*)th); }
                        else {column.push_default(); }
                    } else {
                        column.push_default();
                    }
                }
                reg_plate.push((void*)&column);
            }
            types.push((void*)&reg_plate);

            types.push((void*)&instrs); //# - where instructions go

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
            for(int c=0;c<2;c++) { //2 columns, one for the buffer one for the executing
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
        } else {
            std::ifstream in2(path, std::ios::binary);
            types = read_acorn_types(in2,regtick);

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

            AcornCol& instrplate = (*(AcornCol*)types[1]);
            AcornCol& stageplate = (*(AcornCol*)types[stage]);
            for(int c=0;c<stageplate.length();c++) {
                AcornCol& instrcol = (*(AcornCol*)instrplate[c]);
                AcornCol& stagecol = (*(AcornCol*)stageplate[c]);
                instrcol.clear();
                for(int r=0;r<stagecol.length();r++) {
                    instrcol.push(stagecol[r]);
                }
            }
        }

        //Make instrs executable
        for(int c = 0; c < 256; c++) {
            AcornCol& col = *(AcornCol*)(*(AcornCol*)types[1])[c];
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

        typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol, int);
        JitFunc func = (JitFunc)buf;
        print("buf base: ",(uint64_t)buf);
        //Arbitray ceremony because Arm64, this is why I'm making my own ISA
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        int result = func(types,*(AcornCol*)types[0],*(AcornCol*)types[1], regtick); //Giving the source

        print("FINISHED: ",result);
        
        std::ofstream out2(path, std::ios::binary);
        write_acorn_types(out2,types,result);
        out2.close();

        std::string printout = "";
        printout+=acorn_types_to_string(types,{"Reg plate","Instr plate","String plate","Ribbon plate"});
        editTextFile("mixos-acorn/tests/printout.txt",[printout](std::string& source){
            source=printout;
        });

        munmap(buf, byte_size); //Cleanup

        print("ACORN COMPLETE");
    }

    // static void JIT_Stage(uint32_t stage){
    //     std::string path = "mixos-acorn/tests/acornsock.gld";
    //     std::ifstream in2(path, std::ios::binary);
    //     int regtick = 0;
    //     AcornCol types = read_acorn_types(in2,regtick);

    //     #ifdef __APPLE__
    //         pthread_jit_write_protect_np(0);
    //     #else
    //         #define MAP_JIT 0x0800
    //     #endif

    //     if(stage!=1) {
    //         AcornCol& instrplate = (*(AcornCol*)types[1]);
    //         AcornCol& stageplate = (*(AcornCol*)types[stage]);
    //         for(int c=0;c<stageplate.length();c++) {
    //             AcornCol& instrcol = (*(AcornCol*)instrplate[c]);
    //             AcornCol& stagecol = (*(AcornCol*)stageplate[c]);
    //             instrcol.clear();
    //             for(int r=0;r<stagecol.length();r++) {
    //                 instrcol.push(stagecol[r]);
    //             }
    //         }
    //     }

    //     //Make instrs executable
    //     for(int c = 0; c < 256; c++) {
    //         AcornCol& col = *(AcornCol*)(*(AcornCol*)types[1])[c];
    //         if(col.size == 0) continue;
            
    //         size_t byte_size = col.size;
    //         void* exec_buf = mmap(nullptr, byte_size,
    //             PROT_READ | PROT_WRITE,
    //             MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
    //             -1, 0);
            
    //         memcpy(exec_buf, col.storage, byte_size);
            
    //         #ifdef __APPLE__
    //             pthread_jit_write_protect_np(1);
    //             sys_icache_invalidate(exec_buf, byte_size);
    //         #endif
            
    //         mprotect(exec_buf, byte_size, PROT_READ | PROT_EXEC);
            
    //         delete[] col.storage;
    //         col.storage = (uint8_t*)exec_buf;
    //     }

    //     size_t byte_size = ribbon.length() * sizeof(uint32_t);
    //     void* buf = mmap(nullptr, byte_size,
    //         PROT_READ | PROT_WRITE,
    //         MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
    //         -1, 0);

    //     if(buf == MAP_FAILED) {
    //         print("mmap failed: ", strerror(errno));
    //         return;
    //     }

    //     //Copy the instructions
    //     uint32_t* ptr = (uint32_t*)buf;
    //     for(int i=0;i<ribbon.length();i++) {
    //         ptr[i] = *(uint32_t*)ribbon[i];
    //     }

    //     #ifdef __APPLE__
    //         pthread_jit_write_protect_np(1); // re-enable write protection before executing
    //         sys_icache_invalidate(buf, byte_size); // flush instruction cache
    //     #endif

    //     mprotect(buf, byte_size, PROT_READ | PROT_EXEC);

    //     print("==EXECUTING RIBBON==");

    //     typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol, int);
    //     JitFunc func = (JitFunc)buf;

    //     //Arbitray ceremony because Arm64, this is why I'm making my own ISA
    //     uint64_t sp_val;
    //     asm volatile("mov %0, sp" : "=r"(sp_val));
    //     (void)sp_val;
    //     int result = func(types,*(AcornCol*)types[0],*(AcornCol*)types[1],regtick); //Giving the source

    //     print("FINISHED: ",result);
        
    //     std::string printout = "";
    //     printout+=acorn_types_to_string(types,{"Reg plate","Instr plate","String plate","Ribbon plate"});
    //     editTextFile("mixos-acorn/tests/printout.txt",[printout](std::string& source){
    //         source=printout;
    //     });

    //     munmap(buf, byte_size); //Cleanup
    // }
}


    // path = "mixos-acorn/tests/acornribbon.gld"; 
    // std::ofstream out3(path, std::ios::binary);

    // AcornCol& ribbon = *(AcornCol*)(*(AcornCol*)types[3])[0];
    // list<uint32_t> rinstrs;

    // //for(auto r : instrs[1]) rinstrs << r; //Preamble
    // for(int i=0;i<ribbon.length();i++) {
    //     uint32_t rib_instr = *(uint32_t*)ribbon[i];
    //     if(rib_instr==0) {break;}
    //     rinstrs << rib_instr;
    // }
    // //for(auto r : instrs[2]) rinstrs << r; //Postamble
    // print("Ribbon length: ",rinstrs.length(),"/",ribbon_r);
    // write_raw<uint32_t>(out3,(uint32_t)rinstrs.length());
    // for(int i=0;i<rinstrs.length();i++) {
    //     write_raw<uint32_t>(out3,rinstrs[i]);
    //     //print(i,": ",rinstrs[i]);
    // }
    // out3.close();