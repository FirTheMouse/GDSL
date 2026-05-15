


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


    static void JIT_basic(){
        std::string path = "mixos-acorn/tests/dirtoutput.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            uint32_t instr = read_raw<uint32_t>(in);
            emit_buffer << instr;
            print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);
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

        typedef int (*JitFunc)();
        JitFunc func = (JitFunc)buf;
        
        uint64_t sp_val;
        asm volatile("mov %0, sp" : "=r"(sp_val));
        (void)sp_val;
        
        int result = func();
        print("FINISHED: ",result);
        
        munmap(buf, byte_size); //Cleanup
    }

    static void JIT_dirt(std::string source, AcornCol* instrplate = nullptr){
        std::string path = "mixos-acorn/tests/dirtoutput.gld";   
        std::ifstream in(path, std::ios::binary);
        if(!in) throw std::runtime_error("Can't read from file: " + path);
        list<uint32_t> emit_buffer;
        uint32_t len = read_raw<uint32_t>(in);
        for(int i=0;i<len;i++) {
            emit_buffer << read_raw<uint32_t>(in);
            //print(i,": ",emit_buffer[i]);
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
        emittarget.element_size = 4; //uint32_t
        emitcol = &emittarget;
        AcornCol tempinstrs;
        bool uses_ambles = false;
        if(!instrplate) {
            print("==EXECUTING==");

            tempinstrs.element_size = sizeof(AcornCol);
            for(int c=0;c<256;c++) { //256 columns, one for each char
                AcornCol column;
                column.element_size = 4; //32bits, for one word on a processer
                tempinstrs.push((void*)&column);
            }
            instrplate = &tempinstrs;
            uses_ambles = true;
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
        char_class['L'] = 4; //Less than
        char_class['E'] = 4; //Equals
        char_class['A'] = 4; //And 65
        char_class['O'] = 4; //Or 79
        char_class['F'] = 4; //Left shift 70
        char_class['G'] = 4; //Right shift 71

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

        char_class['{'] = 22; //Lbrace
        char_class['}'] = 23; //Rbrace

        char_class['`'] = 24; //Tick

        uint64_t fya64[11]; 
        fya64[0] = 0; fya64[1] = 0; fya64[2] = 0; fya64[3] = 0;
        fya64[4] = (uint64_t)&emitcol; fya64[5] = 0; fya64[6] = (uint64_t)instrplate; fya64[7] = 0;
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

        if(uses_ambles) {
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

    static void burn_instrs(const std::string& isrc){
        instrs.clear();
        instrs.element_size = sizeof(AcornCol);
        instrs.tag = 1; //Make col trunctable
        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol column;
            column.tag = 2; //Print as instrs
            column.element_size = 4; //32bits, for one word on a processer
            instrs.push((void*)&column);
        }

        JIT_dirt(isrc,&instrs);

        for(int c=0;c<256;c++) { //256 columns, one for each char
            AcornCol& col = *(AcornCol*)instrs[c];
            uint32_t ret = 0xD65F03C0;
            col.push((void*)&ret);
        }

        std::string printout = "";
        printout+=acorn_type_to_string(instrs);
        editTextFile("mixos-acorn/tests/printout2.txt",[printout](std::string& source){
            source=printout;
        });
    };

    std::string padding = "    ";
    static std::string instr_as_buffer(char c) {
        std::string to_return = "";
        AcornCol& col = *(AcornCol*)instrs[c];
        for(int i=0;i<col.length();i++) {
            to_return+=padding;
            to_return+=std::to_string(*(uint32_t*)col[i]);
            to_return+=" => 5$ : 1 + 5$S :\n";
        }
        return to_return;
    }

    static std::string instr_to_string(AcornCol instrplate, char c) {
        std::string to_return = "";
        AcornCol& col = *(AcornCol*)instrplate[c];
        for(int i=0;i<col.length();i++) {
            uint32_t instr = *(uint32_t*)col[i];
            if(instr==0) continue;
            std::string s = disassemble(instr);
            if(s.find("?")!=std::string::npos) to_return+=std::to_string(i)+": "+to_hex(instr)+" : "+to_bin(instr)+" : "+std::to_string(instr)+"\n";
            else to_return+=std::to_string(i)+": "+s+"\n";
        }
        return to_return;
    }

    static void splice_immediates_into_instr(std::string& instr) {
        list<std::string> s = split_str(instr,'\n');
        for(int i = s.length()-1; i>=0;i--) {
            if(s[i]==(padding+"4072669154 => 5$ : 1 + 5$S :")) {
                std::string to_insert = "\n";
                bool can_continue = false;
                if(s[i-1]==(padding+"1386217442 => 5$ : 1 + 5$S :")) { //Move accumulator
                    to_insert+=padding+"9$ = 9$S :\n";
                    can_continue = true;
                }
                if(s[i-1]==(padding+"1386217410 => 5$ : 1 + 5$S :")) { //Move left/right flag
                    to_insert+=padding+"10$ = 9$S :\n";
                    can_continue = true;
                }
                if(!can_continue) continue;
                s.removeAt(i); s.removeAt(i-1); //Remove the two canary instrs
                to_insert+=padding+"9$S = 9$I : 65535 A 9$I : 16 G 9$S :\n";
                to_insert+=padding+"5 F 9$I : 1384120322 O 9$I : 9$I => 5$ : 1 + 5$S :\n";
                to_insert+=padding+"5 F 9$S : 4070572034 O 9$S : 9$S => 5$ : 1 + 5$S   :\n";
                s.insert(to_insert,i-1);
            }
        }
        instr.clear(); for(auto l : s) instr+=l+"\n";
    }

    static void resolve_instr_file(const std::string& path) {
        editTextFile(path,[](std::string& s){
            for(int i=0;i<s.length();i++) {
                if(s.at(i)=='\\'&&s.at(i+1)=='|') {
                    if(s.at(i+2)=='\\') continue;
                    std::string instr = instr_as_buffer(s.at(i+2));
                    splice_immediates_into_instr(instr);
                    for(int j=i+4;j<s.length();j++) {
                        if(s.at(j)=='\\'&&s.at(j+1)=='|') {
                            s.erase(i+4,(j-1)-(i+4));
                            break;
                        }
                    }
                    s.insert(i+4,"\n"+instr+padding);
                }
            }
        });
    }


    uint32_t ribbon_r = 4096;

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

        print("==STARTING ACORN==");


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
            
            // AcornCol ribbon_plate; // 3 - Ribbon plate for output
            // ribbon_plate.tag = 1;
            // ribbon_plate.element_size = sizeof(AcornCol);
            // for(int c=0;c<256;c++) { //256 columns, one for each char
            //     AcornCol column;
            //     column.element_size = 4; //32bits, instruction size
            //     column.resize(ribbon_r*4);
            //     memset(column.storage, 0, column.size);
            //     uint32_t ret = 0xD65F03C0;
            //     column.set(0,(void*)&ret);
            //     ribbon_plate.push((void*)&column);
            // }
            // types.push((void*)&ribbon_plate);

            AcornCol instrbuff_plate; // 3 - Instr buffer plate for instructions
            instrbuff_plate.tag = 1;
            instrbuff_plate.element_size = sizeof(AcornCol);
            for(int c=0;c<256;c++) { //256 columns, one for each char
                AcornCol column;
                column.element_size = 4; //32bits, instruction size
                column.tag = 2; //To print as instructions
                column.resize(ribbon_r*4);
                memset(column.storage, 0, column.size);
                uint32_t ret = 0xD65F03C0;
                column.set(0,(void*)&ret);
                instrbuff_plate.push((void*)&column);
            }
            types.push((void*)&instrbuff_plate);

            AcornCol context_plate; // 5 - the stack and storage spot for reg_plate rows 
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
            (*(AcornCol*)(*(AcornCol*)types[0])[2]).set(regtick+1, (void*)th);

            AcornCol& instrplate = (*(AcornCol*)types[1]);
            AcornCol& stageplate = (*(AcornCol*)types[stage]);
            for(int c=0;c<stageplate.length();c++) {
                AcornCol& instrcol = (*(AcornCol*)instrplate[c]);
                AcornCol& stagecol = (*(AcornCol*)stageplate[c]);
                instrcol.clear();
                instrcol.resize(stagecol.size);
                for(int r=0;r<stagecol.length();r++) {
                    instrcol.set(r,stagecol[r]);
                }
            }

            print("INSTR $");
            print(instr_to_string(instrplate,'$'));
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

        print("==EXECUTING ACORN==");

        typedef int (*JitFunc)(AcornCol, AcornCol, AcornCol, int);
        JitFunc func = (JitFunc)buf;
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
}
