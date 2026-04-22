#pragma once

#include "../modules/GDSL-Core.hpp"
#include <bitset>



namespace GDSL {

     //AArch64 comparison codes
    // ==
    const int COND_EQ = 0b0000;  
    // !=
    const int COND_NE =  0b0001;
    // >=
    const int COND_GE =  0b1010;
    // <
    const int COND_LT =  0b1011; 
    // >
    const int COND_GT =  0b1100;   
    // <=
    const int COND_LE =  0b1101; 

    //x0 - function return value
    const int REG_RETURN_VALUE  = 0;
    //x29 - base of current stack frame
    const int REG_FRAME_POINTER = 29; 
    //x30 - return address (written by call/BL)
    const int REG_LINK          = 30; 
    //xzr - always zero, writes discarded
    const int REG_ZERO          = 31; 
    //x16 - load left opperand
    const int LEFT_REG          = 16;
    //x17 - load right opperand
    const int RIGHT_REG         = 17;
    //x31 - again
    const int REG_SP = 31;

    static void* jit_buf_start = nullptr;
    static size_t jit_buf_size = 0;

    // #include <signal.h>
    // #include <execinfo.h>


    // static void sigill_handler(int sig, siginfo_t* info, void* ctx) {
    //     ucontext_t* uc = (ucontext_t*)ctx;
    //     // PC is in the machine context
    //     #ifdef __APPLE__
    //         uint64_t pc = uc->uc_mcontext->__ss.__pc;
    //     #else
    //         uint64_t pc = uc->uc_mcontext.pc;
    //     #endif
    //     print("Illegal instruction at PC: 0x", std::hex, pc);
        
    //     if(jit_buf_start) {
    //         uint64_t offset = pc - (uint64_t)jit_buf_start;
    //         int instruction_index = offset / 4;
    //         print("Buffer offset: ", offset, " bytes, instruction index: ", std::dec, instruction_index);
    //     }
    //     exit(1);
    // }

    // static void sigsegv_handler(int sig, siginfo_t* info, void* ctx) {
    //     ucontext_t* uc = (ucontext_t*)ctx;
    //     uint64_t pc = uc->uc_mcontext->__ss.__pc;
    //     uint64_t sp = uc->uc_mcontext->__ss.__sp;
    //     // uint64_t x29 = uc->uc_mcontext->__ss.__x[29];
    //     // uint64_t x30 = uc->uc_mcontext->__ss.__x[30];
        
    //     print("SIGSEGV/SIGBUS at PC: 0x", std::hex, pc);
    //     //print("SP: 0x", sp, " X29: 0x", x29, " X30: 0x", x30);
        
    //     if(jit_buf_start) {
    //         uint64_t offset = pc - (uint64_t)jit_buf_start;
    //         int instruction_index = offset / 4;
    //         print("Buffer offset: ", std::dec, instruction_index);
    //     }
        
    //     // Print all registers
    //     for(int i = 0; i < 30; i++) {
    //         print("x", i, ": 0x", std::hex, uc->uc_mcontext->__ss.__x[i]);
    //     }
        
    //     exit(1);
    // }

    struct ARM64_Unit : public virtual Unit {
        list<uint32_t> emit_buffer;

        list<list<uint32_t>> buffer_stack;
        void push_buffer() {
            buffer_stack << emit_buffer;
            emit_buffer = {};
        }
        list<uint32_t> pop_buffer() {
            list<uint32_t> emitted = emit_buffer;
            if(!buffer_stack.empty()) {
                emit_buffer = buffer_stack.pop();
            }
            return emitted;
        }

        int size_to_sf(int size) {
            return size == 8 ? 1 : 0;
        }

        //Store to address, rt = read reg, rn = pointer reg
        uint32_t STR(int rt, int rn, int offset, int size = 8) {
            int sz = (size == 8) ? 0b11 : 0b10; // 64-bit vs 32-bit
            return (sz << 30) //64-bit vs 32-bit (2 bits because LDR/STR support more sizes than just 32/64)
                | (0b111001 << 24) //Metadata
                | (0b00 << 22) //Oppcode
                | ((offset & 0xFFF) << 10) //Offset from rn to write to (in units of size, 0 would just be the pointer)
                | ((rn & 0x1F) << 5) //Base (rn), the pointer. This relationship scales isomorphicly
                | (rt & 0x1F); //Source to read from
        }

        //Load from address, rt = write reg, rn = pointer reg
        uint32_t LDR(int rt, int rn, int offset, int size = 8) {
            int sz = (size == 8) ? 0b11 : 0b10;
            return (sz << 30)
                | (0b111001 << 24)
                | (0b01 << 22) //Oppcode
                | ((offset & 0xFFF) << 10) //Offset from rn to read from
                | ((rn & 0x1F) << 5) 
                | (rt & 0x1F); //Destination to write to
        }

        //Put an immediate value into a register
        uint32_t MOVZ(int rd, int imm16, int sf = 0) {
            return (sf << 31) // 0=32bit (w registers), 1=64bit (x registers)
                | (0b10100101 << 23) //The oppcode
                | (0 << 21) //Section in the register (hw)
                | (imm16 << 5) //16 bit immediate
                | rd; //Where to put it
        }

        void emit_load_literal(g_ptr<Value> v) {
            int  t_reg = v->reg != -1 ? v->reg : LEFT_REG;
            log(emit_buffer.length(),": immediate ",*(int*)&v->data," to ",t_reg);
            emit_buffer << MOVZ(
                t_reg,
                v->get<int>(),
                size_to_sf(v->size)
            );
        }

        //Put an immediate into a specific part of a register
        uint32_t MOVK(int rd, int imm16, int shift, int sf = 1) {
            return (sf << 31)
                | (0b11100101 << 23)
                | ((shift/16) << 21) //Section in the register, MOVK is just MOVZ with hw controls
                | ((imm16 & 0xFFFF) << 5)
                | rd;
        }

        void emit_load_64(int reg, uint64_t value) {
            int start_len =  emit_buffer.length();
                emit_buffer << MOVZ(reg, (value      ) & 0xFFFF, 1); // bits 0-15
            if(value > 0xFFFF) {
                emit_buffer << MOVK(reg, (value >> 16) & 0xFFFF, 16); // bits 16-31
            }
            if(value > 0xFFFFFFFF) {
                emit_buffer << MOVK(reg, (value >> 32) & 0xFFFF, 32); // bits 32-47
            }
            if(value > 0xFFFFFFFFFFFF) {
                emit_buffer << MOVK(reg, (value >> 48) & 0xFFFF, 48); // bits 48-63
            }
            int end_len =  emit_buffer.length()-1;

            log(start_len,"-",end_len,": load ",ptr_to_string(value)," to ",reg);
        }

        void emit_load_from_ptr(int reg, uint64_t ptr, int size) {
            emit_load_64(reg, ptr);
            log(emit_buffer.length(),": load ",ptr_to_string(ptr)," at ",reg);
            emit_buffer << LDR(reg, reg, 0, size); //Offset of 0 because we're using the pointer itself
        }
        void emit_save_to_ptr(int value_reg, uint64_t ptr, int size) {
            emit_load_64(RIGHT_REG, ptr);
            log(emit_buffer.length(),": save ",value_reg," to ",ptr_to_string(ptr));
            emit_buffer << STR(value_reg, RIGHT_REG, 0, size); // store value to that address
        }

        //Will also emit load instructions
        int get_reg(g_ptr<Value> v, int fallback_reg) {
            if(v->reg==-1) {
                emit_load_from_ptr(fallback_reg,(uint64_t)v->data,v->size);
                return fallback_reg;
            } else {
                return v->reg;
            }
        }

        //Add
        uint32_t ADD_reg(int rd, int rn, int rm, int sf = 0) { 
            return (sf << 31)
                | (0b00001011000 << 21)
                | ((rm & 0x1F) << 16) //Right
                | (0 << 10) 
                | ((rn & 0x1F) << 5) //Left
                | (rd & 0x1F); //Result
        }
        void emit_add(g_ptr<Value> v, g_ptr<Value> l, g_ptr<Value> r) {
            int left = get_reg(l, LEFT_REG);
            int right = get_reg(r, RIGHT_REG);
            int result = v->reg != -1 ? v->reg : LEFT_REG; //Reuse LEFT_REG for result
            emit_buffer << ADD_reg(result, left, right, size_to_sf(v->size));
            if(v->reg == -1) {
                emit_save_to_ptr(result, (uint64_t)v->data, v->size);
            }
        }

        uint32_t RET() {
            return 0xD65F03C0;
        }
        //Return from the current function
        void emit_return() {
            log(emit_buffer.length(),": emitting return");
            emit_buffer << RET();
        }

        uint32_t MOV_reg(int rd, int rm, int sf = 0) { 
            return (sf << 31)
                | (0b00101010000 << 21) //Oppcode
                | ((rm & 0x1F) << 16) //Register to copy from
                | (REG_ZERO << 5) //Zero register as the first opperand
                | (rd & 0x1F); //Regiser to copy to
        }
        //Copy a value from one register to another
        void emit_copy(int to, int from, int sf = 0) {
            log(emit_buffer.length(),": copy ",from," to ",to);
            emit_buffer << MOV_reg(to, from, sf);
        }


        uint32_t B(int imm26) {
            return (0b000101 << 26)
                | (imm26 & 0x3FFFFFF);
        }
        //Always jump
        void emit_jump(int offset) {
            emit_buffer << B(offset);
        }

        uint32_t B_cond(int cond, int imm19) {
            return (0b01010100 << 24)
                | ((imm19 & 0x7FFFF) << 5)
                | (cond & 0xF);
        }
        //Conditional jump
        void emit_jump_if(int condition, int offset) {
            emit_buffer << B_cond(condition, offset);
        }

        uint32_t BL(int imm26) {
            return (0b100101 << 26)
                | (imm26 & 0x3FFFFFF);
        }
        //Call a function at offset, remembers where to return to
        void emit_call(int offset) {
            emit_buffer << BL(offset);
        }

        //Used to read the flags
        uint32_t CSET(int rd, int cond, int sf = 0) {
            return (sf << 31)
                | (0b0011010100 << 21) // opcode
                | (0b11111 << 16)      // Rm = XZR
                | ((cond ^ 1) << 12)   // inverted condition (CSET is CSINC with inverted cond)
                | (0b01 << 10)         // CSINC variant bits
                | (0b11111 << 5)       // Rn = XZR
                | (rd & 0x1F);         // destination register
        }


        //Compare two registers, sets internal flags used by jumps
        uint32_t CMP_reg(int rn, int rm, int sf = 0) {
            return (sf << 31)
                | (0b1101011000 << 21)  
                | ((rm & 0x1F) << 16)
                | (0 << 10)
                | ((rn & 0x1F) << 5)
                | 0b11111; // XZR
        }
        //Compare two values and ouput the result as a 1 or 0
        void emit_compare(g_ptr<Value> v, g_ptr<Value> lv, g_ptr<Value> rv, int cond) {
            int left = get_reg(lv, LEFT_REG);
            int right = get_reg(rv, RIGHT_REG);
            int result = v->reg != -1 ? v->reg : LEFT_REG;
            
            emit_buffer << CMP_reg(left, right, size_to_sf(v->size));
            emit_buffer << CSET(result, cond);
            
            if(v->reg == -1) {
                emit_save_to_ptr(result, (uint64_t)v->data, v->size);
            }

        }

        uint32_t CMP_imm(int rn, int imm12, int sf = 0) {
            return (sf << 31)
                | (0b1 << 30)          // op = subtract
                | (0b1 << 29)          // S = set flags
                | (0b10001 << 24)      // add/subtract immediate class
                | (0b00 << 22)         // shift = 0
                | ((imm12 & 0xFFF) << 10)
                | ((rn & 0x1F) << 5)
                | REG_ZERO;            // XZR as destination
        }
        //Compare a register to a literal number
        void emit_compare_value(int a, int value, int sf = 0) {
            emit_buffer << CMP_imm(a, value, sf);
        }

        uint32_t BLR(int rn) {
            return (0b1101011000111111000000 << 10)
                | ((rn & 0x1F) << 5)
                | 0b00000;
        }
        void emit_call_register(int reg) {
            emit_buffer << BLR(reg);
        }

        uint32_t MOV_from_sp(int rd, int sf = 1) {
            return (sf << 31)
                | (0b0010001 << 24)
                | (0 << 22)          // shift = 0
                | (0 << 10)          // imm12 = 0
                | (31 << 5)          // SP as source
                | (rd & 0x1F);
        }

        uint32_t SUB_sp(int imm, int sf = 1) {
            return (sf << 31)
                | (1 << 30)          // op = subtract
                | (0 << 29)          // S = don't set flags
                | (0b10001 << 24)    // class identifier
                | (0 << 22)          // shift = 0
                | ((imm & 0xFFF) << 10)
                | (0b11111 << 5)     // SP as source
                | 0b11111;           // SP as destination
        }
        //Allocate stack space
        uint32_t stack_alloc(int bytes) {
            return SUB_sp(bytes);
        }

        //Push to stack (and resize):  rt1 = first index to save, rt2 = second index, rn = base register (SP), imm7 = byte offset 
        uint32_t STP(int rt1, int rt2, int rn, int imm7) {
            return (0b10 << 30)       // 64-bit
                | (0b1010011 << 23)  // pre-index fixed bits
                | (0b0 << 22)        // L = 0 (store)
                | ((imm7 & 0x7F) << 15)
                | ((rt2 & 0x1F) << 10)
                | ((rn & 0x1F) << 5)
                | (rt1 & 0x1F);
        }

        //Push to stack (and resize):  rt1 = first index to save, rt2 = second index, rn = base register (SP), imm7 = byte offset
        uint32_t LDP(int rt1, int rt2, int rn, int imm7) {
            return (0b10 << 30)
                | (0b101000 << 24)    // LDP post-index
                | (0b11 << 22)
                | ((imm7 & 0x7F) << 15)
                | ((rt2 & 0x1F) << 10)
                | ((rn & 0x1F) << 5)
                | (rt1 & 0x1F);
        }

        void emit_prologue() {
            log(emit_buffer.length(),": emitting prolouge STP");
            emit_buffer << STP(REG_FRAME_POINTER, REG_LINK, REG_SP, -2); //Offset is passed in units of 8
            log(emit_buffer.length(),": emitting prolouge MOV 29 to SP");
            emit_buffer << 0x910003FD; //MOV x29, sp
        }

        void emit_epilogue() {
            log(emit_buffer.length(),": emitting epilouge LDP");
            emit_buffer << LDP(REG_FRAME_POINTER, REG_LINK, REG_SP, 2);
            emit_return();
        }

        static void jint() {
            print("jint.");
        }

        static void jont() {
            print("JONT!");
        }


        static void jit_print_int(int val) {
            print("JIT reg: ", val);
        }

        void print_emit_buffer() {
            print("==EMIT BUFFER==");
            for(int i=0;i<emit_buffer.length();i++) {
                uint32_t instr = emit_buffer[i];
                print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec);
            }
        }
    };
}