#pragma once

#include "../mixos-acorn/Acorn-Script.hpp"
#include <bitset>

namespace Acorn {
    list<uint32_t> sub_instruction_buffer;
    struct Acorn_Dirt : public virtual Compiler_Unit {
        Acorn_Dirt() {init();}
        
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
            int scaled = offset / size;  // scale the offset
            return (sz << 30) //64-bit vs 32-bit (2 bits because LDR/STR support more sizes than just 32/64)
                | (0b111001 << 24) //Metadata
                | (0b00 << 22) //Oppcode
                | ((scaled & 0xFFF) << 10) //Offset from rn to write to (in units of size, 0 would just be the pointer)
                | ((rn & 0x1F) << 5) //Base (rn), the pointer. This relationship scales isomorphicly
                | (rt & 0x1F); //Source to read from
        }

        uint32_t STR_reg(int rt, int rn, int rm, int size = 8) {
            int sz = (size == 8) ? 0b11 : 0b10;
            return (sz << 30)
                | (0b111000001 << 21)
                | ((rm & 0x1F) << 16)
                | (0b011010 << 10)
                | ((rn & 0x1F) << 5)
                | (rt & 0x1F);
        }

        //Load from address, rt = write reg, rn = pointer reg
        uint32_t LDR(int rt, int rn, int offset, int size = 8) {
            int sz = (size == 8) ? 0b11 : 0b10;
            int scaled = offset / size;  // scale the offset
            return (sz << 30)
                | (0b111001 << 24)
                | (0b01 << 22) //Oppcode
                | ((scaled & 0xFFF) << 10) //Offset from rn to read from
                | ((rn & 0x1F) << 5) 
                | (rt & 0x1F); //Destination to write to
        }

        uint32_t LDR_reg(int rt, int rn, int rm, int sf = 1) {
            return (sf == 1 ? 0b11 : 0b10) << 30
                | (0b111000011 << 21)
                | ((rm & 0x1F) << 16)
                | (0b011010 << 10)  // LSL extend
                | ((rn & 0x1F) << 5)
                | (rt & 0x1F);
        }

        //Put an immediate value into a register
        static uint32_t MOVZ(int rd, int imm16, int sf = 0) {
            return (sf << 31) // 0=32bit (w registers), 1=64bit (x registers)
                | (0b10100101 << 23) //The oppcode
                | (0 << 21) //Section in the register (hw)
                | (imm16 << 5) //16 bit immediate
                | rd; //Where to put it
        }

        //Put an immediate into a specific part of a register
        static uint32_t MOVK(int rd, int imm16, int shift, int sf = 1) {
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

        //Add
        uint32_t ADD_reg(int rd, int rn, int rm, int sf = 0) { 
            return (sf << 31)
                | (0b00001011000 << 21)
                | ((rm & 0x1F) << 16) //Right
                | (0 << 10) 
                | ((rn & 0x1F) << 5) //Left
                | (rd & 0x1F); //Result
        }

        //Subtract
        uint32_t SUB_reg(int rd, int rn, int rm, int sf = 0) {
            return (sf << 31)
                | (0b01001011000 << 21)
                | ((rm & 0x1F) << 16) //Right
                | (0 << 10)
                | ((rn & 0x1F) << 5) //Left
                | (rd & 0x1F); //Result
        }

        //Multiply
        uint32_t MUL_reg(int rd, int rn, int rm, int sf = 0) {
            return (sf << 31)
                | (0b0011011000 << 21)
                | ((rm & 0x1F) << 16)
                | (0b011111 << 10)  // Ra = XZR for plain MUL
                | ((rn & 0x1F) << 5)
                | (rd & 0x1F);
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

        uint32_t ADD_sp(int imm, int sf = 1) {
            return (sf << 31)
                | (0 << 30)          // op = add (not subtract)
                | (0 << 29)          // S = don't set flags
                | (0b10001 << 24)    // class identifier
                | (0 << 22)          // shift = 0
                | ((imm & 0xFFF) << 10)
                | (0b11111 << 5)     // SP as source
                | 0b11111;           // SP as destination
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

        uint32_t LDRB(int rt, int rn, int offset) {
            return (0b00 << 30)      // size = byte
                | (0b111001 << 24)
                | (0b01 << 22)       // load
                | ((offset & 0xFFF) << 10)
                | ((rn & 0x1F) << 5)
                | (rt & 0x1F);
        }

        uint32_t LDRB_reg(int rt, int rn, int rm) {
            return (0b00 << 30)
                | (0b111000011 << 21)  // load register offset
                | ((rm & 0x1F) << 16)
                | (0b011 << 13)        // LSL extend
                | (0b0 << 12)          // shift amount 0
                | (0b10 << 10)
                | ((rn & 0x1F) << 5)
                | (rt & 0x1F);
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

        void patch_b(uint32_t idx, int offset) {
            emit_buffer[idx] = (emit_buffer[idx] & 0xFC000000) //preserve top 6 opcode bits
                             | (offset & 0x3FFFFFF);            //replace offset field
        }

        void patch_b_cond(uint32_t idx, int offset) {
            emit_buffer[idx] = (emit_buffer[idx] & 0xFF00000F) //preserve opcode and condition
                             | ((offset & 0x7FFFF) << 5);       //replace offset field
        }

        struct Patch {
            uint32_t idx;
            std::string label;
            uint32_t instr_type; //b_id or b_cond_id
        };
        list<Patch> patches;
        map<std::string, uint32_t> labels;

        uint32_t b_id = make_tokenized_keyword("b");

        int con(Context& ctx) {
            ctx.index++;
            process_node(ctx,ctx.result.get(ctx.index));
            Node next = ctx.result.take(ctx.index);
            ctx.index--;
            std::string name = next.name().to_std();
            if(std::isdigit(name[0]) || name[0]=='-') {
                return std::stoi(name);
            } else {
                if(labels.hasKey(name)) {
                    return (int)labels.get(name) - (int)emit_buffer.length();
                } else {
                    patches << Patch{(uint32_t)emit_buffer.length(), name, ctx.node.type()};
                    return 0;
                }
            }
        }

        void resolve_patches() {
            for(auto p : patches) {
                if(!labels.hasKey(p.label)) {
                    print(red("unresolved label: "+p.label));
                    continue;
                }
                int offset = (int)labels.get(p.label) - (int)p.idx;
                if(p.instr_type == b_id) {
                    patch_b(p.idx, offset);
                } else {
                    patch_b_cond(p.idx, offset);
                }
            }
        }

        static void syscall_col_new(uint64_t pool, uint64_t col, uint64_t sidx) {
            print("Hello world! ",pool,"|",col,"|",sidx);
        }
        
        
        static void syscall_readchar(uint64_t zero) {print((char)zero);}
        static void syscall_atzero(uint64_t zero) {print(zero);}
        static void syscall_atone(uint64_t zero, uint64_t one) {print(one);}
        static void syscall_append_to_buffer(uint64_t zero) {sub_instruction_buffer << (uint32_t)zero;}
        static void syscall_emit_movz_imm(uint64_t imm) {sub_instruction_buffer << MOVZ(2, imm & 0xFFFF, 0);}

        static void syscall_jint() {print("jint");}
        static void syscall_jont() {print("jont");}

        void emit_syscall(Context& ctx, uint64_t funcaddr) {
            emit_load_64(LEFT_REG, funcaddr); (*ctx.buffer) << BLR(LEFT_REG);
        }

        void init() override {

            a_handlers[dash_id] = [this](Context& ctx){
                ctx.node.name().push(ctx.result.take(ctx.index+1).name().to_std());
            };

            a_handlers[colon_id] = [this](Context& ctx){
                // The identifier before the colon is the label name
                if(ctx.index > 0) {
                    std::string name = ctx.result[ctx.index-1].name().to_std();
                    labels.put(name, emit_buffer.length());
                    // Remove the label nodes from the result
                    ctx.result.removeAt(ctx.index);   // colon
                    ctx.result.removeAt(ctx.index-1); // name
                    ctx.index -= 2;
                }
            };

            char_is_split.put('\n',true);
            char_is_split['.'] = false;

            a_handlers.default_function = [this](Context& ctx){}; //Do nothing
            a_handlers[comment_id] = [this](Context& ctx){ //Discard comments
                ctx.result.removeAt(ctx.index);
                ctx.index--;
            };

            a_handlers[make_tokenized_keyword("movz")] = [this](Context& ctx){ (*ctx.buffer) << MOVZ(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("movk")] = [this](Context& ctx){ (*ctx.buffer) << MOVK(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ldr")]  = [this](Context& ctx){ (*ctx.buffer) << LDR(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ldr32")]  = [this](Context& ctx){ (*ctx.buffer) << LDR(con(ctx),con(ctx),con(ctx),4); };
            a_handlers[make_tokenized_keyword("str")]  = [this](Context& ctx){ (*ctx.buffer) << STR(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("str32")]  = [this](Context& ctx){ (*ctx.buffer) << STR(con(ctx),con(ctx),con(ctx),4); };
            a_handlers[make_tokenized_keyword("add")]  = [this](Context& ctx){ (*ctx.buffer) << ADD_reg(con(ctx),con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("sub")]  = [this](Context& ctx){ (*ctx.buffer) << SUB_reg(con(ctx),con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("mul")]  = [this](Context& ctx){ (*ctx.buffer) << MUL_reg(con(ctx),con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("mov")]  = [this](Context& ctx){ (*ctx.buffer) << MOV_reg(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ret")]  = [this](Context& ctx){ (*ctx.buffer) << RET(); };
            a_handlers[make_tokenized_keyword("cmp")]  = [this](Context& ctx){ (*ctx.buffer) << CMP_reg(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("stp")]  = [this](Context& ctx){ (*ctx.buffer) << STP(con(ctx),con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ldp")]  = [this](Context& ctx){ (*ctx.buffer) << LDP(con(ctx),con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ldrb")] = [this](Context& ctx){ (*ctx.buffer) << LDRB(con(ctx),con(ctx),con(ctx));};
            //a_handlers[make_tokenized_keyword("adr")] = [this](Context& ctx){ (*ctx.buffer) << LDRB(con(ctx),con(ctx),con(ctx));};

            a_handlers[make_tokenized_keyword("ldrb_reg")] = [this](Context& ctx){ (*ctx.buffer) << LDRB_reg(con(ctx),con(ctx),con(ctx));};
            a_handlers[make_tokenized_keyword("str_reg")]  = [this](Context& ctx){ (*ctx.buffer) << STR_reg(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("ldr_reg")]  = [this](Context& ctx){ (*ctx.buffer) << LDR_reg(con(ctx),con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("sub_sp")]  = [this](Context& ctx){ (*ctx.buffer) << SUB_sp(con(ctx),con(ctx)); };
            a_handlers[make_tokenized_keyword("add_sp")] = [this](Context& ctx){ (*ctx.buffer) << ADD_sp(con(ctx),con(ctx)); };

            a_handlers[b_id]                           = [this](Context& ctx){ (*ctx.buffer) << B(con(ctx)); };
            a_handlers[ make_tokenized_keyword("bl")]  = [this](Context& ctx){ (*ctx.buffer) << BL(con(ctx)); };
            a_handlers[make_tokenized_keyword("blr")]  = [this](Context& ctx){ (*ctx.buffer) << BLR(con(ctx)); };
            a_handlers[make_tokenized_keyword("b.eq")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_EQ, con(ctx)); };
            a_handlers[make_tokenized_keyword("b.ne")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_NE, con(ctx)); };
            a_handlers[make_tokenized_keyword("b.gt")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_GT, con(ctx)); };
            a_handlers[make_tokenized_keyword("b.lt")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_LT, con(ctx)); };
            a_handlers[make_tokenized_keyword("b.ge")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_GE, con(ctx)); };
            a_handlers[make_tokenized_keyword("b.le")] = [this](Context& ctx){ (*ctx.buffer) << B_cond(COND_LE, con(ctx)); };

            a_handlers[make_tokenized_keyword("emit_movz_imm")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_emit_movz_imm);};
            a_handlers[make_tokenized_keyword("readchar")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_readchar);};
            a_handlers[make_tokenized_keyword("atzero")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_atzero);};
            a_handlers[make_tokenized_keyword("atone")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_atone);};
            a_handlers[make_tokenized_keyword("jint")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_jint);};
            a_handlers[make_tokenized_keyword("jont")] = [this](Context& ctx){emit_syscall(ctx,(uint64_t)&syscall_jont);};
            a_handlers[make_tokenized_keyword("wub")] = [this](Context& ctx){(*ctx.buffer) << MOVZ(0,7,0);}; //For security reasons


            a_handlers[pipe_id] = [this](Context& ctx){
                ctx.result.removeAt(ctx.index);
                Node next = ctx.result.get(ctx.index);
                next.sub_type(pipe_id);
                ctx.buffer = &sub_instruction_buffer;
                process_node(ctx,next);
                ctx.buffer = &emit_buffer;
                uint32_t instr = sub_instruction_buffer.pop(); //Grab it
                emit_buffer << MOVZ(0, instr & 0xFFFF, 0);
                emit_buffer << MOVK(0, (instr >> 16) & 0xFFFF, 16);
                emit_syscall(ctx,(uint64_t)&syscall_append_to_buffer);
            };
        }

        virtual Node process(std::string code) override {
            return tokenize(code);
        }

        virtual void run(Node root) override {

            //print(node_to_string(root,0,0,true));

            start_stage(a_handlers);
            standard_direct_pass(root,&emit_buffer);

            resolve_patches();

            print(node_to_string(root,0,0,true));
            print_emit_buffer();

            std::string path = "mixos-acorn/tests/dirtoutput.gld";
            std::ofstream out(path, std::ios::binary);
            if(!out) throw std::runtime_error("Can't write to file: " + path);
            write_raw<uint32_t>(out,emit_buffer.length());
            for(int i=0;i<emit_buffer.length();i++) {
                write_raw<uint32_t>(out,emit_buffer[i]);
            }
            out.close();
        }

    };
}