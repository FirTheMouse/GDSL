#include "modules/GDSL-Test.hpp"
// #include "modules/Q-HTML.hpp"
// #include "modules/GDSL-C.hpp"
// #include "modules/GDSL-LISP.hpp"
// #include "modules/GDSL-TwigSnap.hpp"
// #include "modules/GDSL-Script.hpp"
// #include "modules/GDSL-GQL.hpp"
#include "modules/GDSL-Thistle.hpp"
// #include "modules/GDSL-PineNeedle.hpp"
// #include "mixos-acorn/Acorn-Kernel.hpp"
// #include "mixos-acorn/Acorn-Dirt.hpp"
// #include "mixos-acorn/Acorn-JIT.hpp"
// #include "mixos-acorn/Acorn-Script.hpp"
#include "mixos-acorn/web/Webcorn-Core.hpp"
// #include "mixos-acorn/Acorn-Core.hpp"

// using namespace GDSL;


std::string resolve(const std::string& current_file, const std::string& include_path) {
    std::string dir = current_file.substr(0, current_file.find_last_of("/"));
    std::string joined = dir + "/" + include_path;
    return std::filesystem::path(joined).lexically_normal().string();
}


list<std::string> system_includes;
void strip_pragmas_and_system_includes(std::string& s) {
    size_t pos = s.find("#pragma once");
    while (pos != std::string::npos) {
        size_t line_end = s.find("\n", pos);
        s.erase(pos, line_end - pos + 1);
        pos = s.find("#pragma once");
    }
    pos = s.find("#include <");
    while (pos != std::string::npos) {
        size_t path_start = pos + 10;
        size_t path_end = s.find(">", path_start);
        
        std::string include = s.substr(path_start, path_end - path_start);
        system_includes.push_if_absent(include);
        
        size_t line_end = s.find("\n", path_end);
        s.erase(pos, line_end - pos + 1);
        
        pos = s.find("#include <");
    }
}

map<std::string,bool> included_paths;
std::string vendor_file(const std::string& input_path, bool is_first = true) {
    std::string s = readFile(input_path);
    strip_pragmas_and_system_includes(s);
    size_t pos = s.find("#include \"");
    while (pos != std::string::npos) {
        size_t path_start = pos + 10; //Len of '#include "'
        size_t path_end = s.find("\"", path_start);
        
        std::string path = s.substr(path_start, path_end - path_start);
        path = resolve(input_path,path);
        
        size_t line_end = s.find("\n", path_end);
        s.erase(pos, line_end - pos + 1); //Erase the line
        
        uint32_t offset = 0;
        if(!included_paths.hasKey(path)) {
            std::string vendored = vendor_file(path,false);
            s.insert(pos, vendored);
            offset = vendored.size();
            included_paths.put(path,true);
        }
        
        pos = s.find("#include \"", pos + offset);
    }

    if(is_first) {
        s = s.substr(s.find_first_not_of('\n'));

        for(auto i : system_includes) {
            s.insert(0,"#include <"+i+">\n");
        }
        s.insert(0,"#pragma once\n\n");
    
        included_paths.clear();
        system_includes.clear();
    }   

    return s;
}

int main(int argc, char* argv[]) {
    // if(argc < 2) {
    //     print("Usage: ./gdsl <file>");
    //     return 1;
    // }
    // std::string path = argv[1];
    // GDSL::test_module(path);
    //GDSL::test_module("modules/tests/lisptest.gld");
    //GDSL::test_module("modules/tests/ctest.gld");

    print("TEST START");
    // g_ptr<Node> root = nullptr;


    writeFile("acorn_export.hpp",vendor_file("mixos-acorn/Acorn-Core.hpp"));
    //writeFile("mixos-acorn/tests/printout.txt",Acorn::make_wrapper_for_layout(Acorn::layouts[Acorn::node_id],"Node"));


    // span = make<Log::Span>();
    // //span->log_everything = true; //While things are crashing

    // Acorn::init_type_pool();
    // // // Acorn::test_acorn();
 
    // // g_ptr<Acorn::Acorn_Script> acorn = make<Acorn::Acorn_Script>();
    // // acorn->setup_trace_res_flipbook();
    // // //acorn->setup_stamp_res_flipbook();
    // // acorn->run(acorn->process(readFile("mixos-acorn/tests/acorntest.gld")));

    // g_ptr<Acorn::Webcorn_Core> webcorn = make<Acorn::Webcorn_Core>();
    // //webcorn->setup_trace_res_flipbook();
    // webcorn->run(webcorn->process(readFile("mixos-acorn/web/webtest.gld")));


    // Acorn::init_type_pool();
    // Acorn::save_acorn("basesave.wub");
    // g_ptr<Acorn::Acorn_Script> acorn = make<Acorn::Acorn_Script>();
    // Acorn::Node root = acorn->process(readFile("mixos-acorn/tests/acorntest.gld"));
    // Acorn::save_acorn("savetest.wub");
    // acorn->run(root);
    // Acorn::load_acorn("basesave.wub");
    // g_ptr<Acorn::Webcorn_Core> webcorn = make<Acorn::Webcorn_Core>();
    // webcorn->run(webcorn->process(readFile("mixos-acorn/web/webtest.gld")));
    // Acorn::load_acorn("savetest.wub");
    // acorn->run(root);
    

    // Acorn::Acorn_Kernel kernel;
    // Acorn::init_type_pool();
    // kernel.run(kernel.process(readFile("mixos-acorn/tests/acorntest.gld")));

    // Acorn::Acorn_Dirt dirt;
    // Acorn::init_type_pool();
    // dirt.run(dirt.process(
    //     "movz 3 42 0 "   // load 42 into x3
    //     "and 0 3 3 0 "   // x0 = x3 & x3 (just moves 42 into x0 via AND)
    //     "ret"
    // ));
    // Acorn::JIT_basic();
    // print("EXPECTED 42");

    // dirt.run(dirt.process(
    //     "movz 3 2 0 "
    //     "movz 4 5 0 "
    //     "lsl 0 3 4 0 " 
    //     "ret"
    // ));
    // Acorn::JIT_basic();
    // print("EXPECTED 64");

    // dirt.run(dirt.process(
    //     "movz 3 168 0 "   // x3 = 168
    //     "movz 4 2 0 "     // x4 = 2
    //     "lsr 0 3 4 0 "    // x0 = 168 >> 2 = 42
    //     "ret"
    // ));
    // Acorn::JIT_basic();
    // print("EXPECTED 42");

    // dirt.run(dirt.process(
    //     "movz 3 42 0 "    // x3 = 42  (0b00101010)
    //     "movz 4 21 0 "    // x4 = 21  (0b00010101)
    //     "orr 0 3 4 0 "    // x0 = 42 | 21 = 63
    //     "ret"
    // ));
    // Acorn::JIT_basic();
    // print("EXPECTED 63");

    // uint32_t instr = Acorn::Acorn_Dirt::MOVZ(2,0,0);
    // print("movz 2 0 0 : 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);

    // instr = Acorn::Acorn_Dirt::MOVK(2,0,16,1);
    // print("movk 2 0 16 1 : 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);

    // instr = Acorn::Acorn_Dirt::MOVZ(2,2,0);
    // print("movz 2 2 0 : 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec," | ",instr);

    // Acorn::Acorn_Dirt dirt;
    // Acorn::init_type_pool();
    // dirt.run(dirt.process(readFile("mixos-acorn/tests/dirt.gld")));

    // Acorn::burn_instrs(readFile("mixos-acorn/tests/acorninsx.gld"));
    // Acorn::resolve_instr_file("mixos-acorn/tests/acorninstrs.gld");
    // Acorn::burn_instrs(readFile("mixos-acorn/tests/acorninstrs.gld"));
    // Acorn::JIT_dirt(readFile("mixos-acorn/tests/acorn.gld"));
    // Acorn::JIT_Acorn();
    // Acorn::JIT_Ribbon();
    // // Acorn::JIT_Acorn(3);


    // g_ptr<GDSL::Thistle_Unit> twig = make<GDSL::Thistle_Unit>();
    // g_ptr<GDSL::Node> twig_root = twig->process(readFile("modules/tests/pebble.gld"));
    // twig->run(twig_root);

    // g_ptr<GDSL::Thistle_Unit> thistle = make<GDSL::Thistle_Unit>();
    // g_ptr<GDSL::Node> thistle_root = thistle->process(readFile("modules/tests/testpage.gld"));
    // thistle->run(thistle_root);

    // g_ptr<Thistle_Unit> pine_thistle = make<Thistle_Unit>();
    // g_ptr<Node> pine_thistle_root = pine_thistle->process(readFile("modules/tests/pinetest.gld"));
    // pine_thistle->run(pine_thistle_root);

    // g_ptr<PineNeedle_Unit> pine = make<PineNeedle_Unit>();
    // g_ptr<Node> pine_root = pine->process(readFile("modules/tests/pinetest.gld"));
    // pine->run(pine_root);
    
    // g_ptr<GQL_Unit> q_script = make<GQL_Unit>();
    // root = q_script->process(readFile("modules/tests/qdemo.gld"));
    // q_script->run(root);


    // std::string display = "";
    // display.append("GDSL-C: ");
    // display.append(readFile("modules/GDSL-C.hpp"));
    // display.append("Q-Arm64, an example of a Unit: ");
    // display.append(readFile("modules/Q-Arm64.hpp"));
    // display.append("GDSL-LISP: ");
    // display.append(readFile("modules/GDSL-LISP.hpp"));
    // display.append("And finnaly the core: ");
    // display.append(readFile("core/GDSL.hpp"));
    // print(display);


    // g_ptr<LISP_Unit> lisp = make<LISP_Unit>();
    // lisp->init();
    // root = lisp->process(readFile("modules/tests/lisptest.gld"));
    // lisp->run(root);
    
    //span = make<Log::Span>();

    // g_ptr<C_Compiler> c = make<C_Compiler>();
    // c->init();
    // root = c->process(readFile("modules/tests/ctest.gld"));
    // c->run(root);

    // c->emit_mode = true;
    // root = c->process(readFile("modules/tests/cemittest.gld"));
    // //c->run(root);
    // c->serialize(root);
    // c->saveBinary("savetest.wub");
    // root = c->loadBinary("savetest.wub");
    // c->run(root);
    // // span = make<Log::Span>();
    // // c->emit_mode = false;
    // // c->span2 = make<Log::Span>();
    // // c->emit_buffer.clear();

    // root = c->process(readFile("modules/tests/ctest.gld"));
    // c->run(root);

    print("TEST FINISHED");


 // Acorn::Unit acorn;     acorn.init();
    // GDSL::Unit gdsl;

    // g_ptr<GDSL::Node> gdsl_root = make<GDSL::Node>();
    // gdsl_root->sub_type = 3;

    // Acorn::Ptr acorn_root = acorn.make_node();
    // int three = 3;
    // acorn.types[acorn_root.pool].columns[acorn.sub_type_col].set(acorn_root.sidx,(void*)&three);

    // Acorn::Object acorn_obj = acorn.node_type.create();

    // list<g_ptr<GDSL::Node>> gdsl_nodes;
    // list<Acorn::Ptr> acorn_nodes;
    // for(int i=0;i<1000;i++) {
    //     gdsl_nodes << make<GDSL::Node>();
    //     acorn_nodes << acorn.make_node();
    // }

    // int ITS = 1000;
    // Log::rig r;
    // r.add_process("access_gdsl",[&](int i){
    //     //volatile int a = gdsl_root->sub_type;
    //     for(int m=0;m<1000;m++) {
    //         volatile int a = gdsl_nodes[m]->sub_type;
    //     }
    // },0);
    // Acorn::_column& acol = acorn.types[acorn_root.pool].columns[acorn.sub_type_col];
    // r.add_process("access_acorn",[&](int i){
    //     // volatile int a = *(int*)acorn.types[acorn_root.pool].columns[acorn.sub_type_col].get(acorn_root.sidx);
    //     for(int m=0;m<1000;m++) {
    //         volatile int a = *(int*)acol.get(acorn_nodes[m].sidx);
    //     }
    //     //volatile int a = *(int*)acol.get(acorn_root.sidx);
    // },0);
    // r.add_comparison("access_gdsl","access_acorn");
    // r.run(1000,true,ITS);


    //comp->test_module("modules/tests/ctest.gld");
    //GDSL::test_module("modules/tests/maintest.gld");

    // g_ptr<Node> n = make<Node>();
    // n->value = make<Value>();
    // n->value->store = make<Type>();

    // n->value->store->add<std::string>("1","one");
    // n->value->store->add<int>("2",2);

    // g_ptr<Node> c = make<Node>();
    // c->value = make<Value>();
    // c->value->store = n->value->store;

    // n->children << c;

    // c->value->store->set<int>("2",3);

    // serialize_node(n);
    // saveBinary("savetest.wub");

    // g_ptr<Node> root = loadBinary("savetest.wub");
    // print(root->value->store->get<std::string>("1"));
    // print(root->value->store->get<int>("2"));
    // print(root->left()->value->store->get<std::string>("1"));
    // print(root->left()->value->store->get<int>("2"));

    return 0;

}

// #include "modules/GDSL-CALC.hpp"

// int main() {
//     GDSL::my_module();
//     return 0;
// }


//std::string s = readFile("mixos-acorn/web/Webcorn-Core.hpp");
// int csr = s.find("ctx",0);
// while((csr = s.find("ctx.", csr)) != std::string::npos) {
//     int dot = csr + 3;
//     int field_start = dot + 1;
//     int field_end = field_start;
//     while(field_end < s.length() && (std::isalpha(s[field_end]) || s[field_end]=='_')) {
//         field_end++;
//     }
//     char after = s[field_end];
//     std::string field = s.substr(field_start, field_end - field_start);
    
//     int op = field_end;
//     while(op < s.length() && s[op] == ' ') op++;
    
//     char op_char = s[op];
//     bool is_assign = op_char == '=' && s[op+1] != '=';
    
//     if(is_assign) {
//         int rhs_start = op + 1;
//         while(rhs_start < s.length() && s[rhs_start] == ' ') rhs_start++; // consume whitespace after =
//         int terminator = s.find(';', rhs_start);
//         int rhs_end = terminator;
//         while(s[rhs_end-1] == ' ') rhs_end--; // trim trailing whitespace
//         std::string rhs = s.substr(rhs_start, rhs_end - rhs_start);
//         std::string field = s.substr(field_start, field_end - field_start);
//         s.replace(csr, terminator - csr + 1, "ctx."+field+"("+rhs+");");
//     } else {
//         s.insert(field_end, "()");
//         csr = field_end + 2;
//         continue;
//     }
//     csr = field_end;
// }
// writeFile("mixos-acorn/web/Webcorn-Core.hpp",s);



// acorn->silence_blackfeather = true;
    // std::string code = readFile("mixos-acorn/tests/acorntest.gld");
    // std::string built_code = "";
    // for(int i=0;i<code.length();i++) {
    //     print("=====",i,"=====");
    //     built_code+=code.at(i);
    //     print("COMPILE:\n",built_code);
    //     acorn->run(acorn->process(built_code));
    //     Acorn::ERROR_FLAG = false;
    // }
    // built_code = "";
    // print("===== AND BACKWARDS =====");
    // for(int i=code.length()-1;i>=0;i--) {
    //     print("=====",i,"=====");
    //     built_code.insert(0,1,code.at(i));
    //     print("COMPILE:\n",built_code);
    //     acorn->run(acorn->process(built_code));
    //     Acorn::ERROR_FLAG = false;
    // }
    // for(int j=0;j<50;j++) {
    //     built_code = "";
    //     for(int i=0;i<500;i++) {
    //         print("=====",i,"=====");
    //         built_code+=rands();
    //         print("COMPILE:\n",built_code);
    //         acorn->run(acorn->process(built_code));
    //         Acorn::ERROR_FLAG = false;
    //     }
    // }

    // list<std::string> programs;
    // programs << "int x = ;";
    // programs << "int int int x = 5;";
    // programs << "float f = 5 + * 3;";
    // programs << "print(print(print(print(print(print(print(print(print(print(1))))))))));";
    // programs << "int a = b = c = d = e = 5;";
    // programs << "{{{{{{{{{{}}}}}}}}}};";
    // programs << "int f() {\n    int f() {\n        int f() {\n            f();\n        }\n    }\n}\nf();";
    // programs << "int x = (((((((((5 + 3))))))))));";
    // programs << ".x.y.z.w.v.u;";
    // programs << "int x = 5;\nint x = 10;\nprint(x);";
    // programs << "int sayHi(int a, int b, int c) {\n    print(a);\n}\nsayHi(1, 2, 3, 4, 5);";
    // programs << "int x = x + 1;";
    // programs << "##\nregister foo;\n\"foo\" {\n    in executing {\n        foo;\n    }\n}\n##\nfoo;";
    // programs << "int f() { return; return; return; }\nf();";
    // programs << "int x = 5 +";
    // programs << "\"unclosed string;\nint x = 5;";
    // programs << "int x = 5; int y = 10; int z = x + y * z - x / y + z * x;";
    // programs << "##\n##\n##\n##";
    // programs << "int f(int x) {\n    int g(int y) {\n        print(x);\n        print(y);\n    }\n    g(x);\n}\nf(5);";
    // programs << "print(1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20);";
    // programs << "int x = ;;;;;;;";
    // programs << "int f() {}\nint f() {}\nint f() {}\nf();";
    // programs << "((((;";
    // programs << "int x = 5;\n{\n    int x = 10;\n    {\n        int x = 15;\n        print(x);\n    }\n    print(x);\n}\nprint(x);";
    // programs << "int x = 5;\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);\nprint(x);";
    // programs << "int f(int x) { print(x); }\nf(f(f(f(f(1)))));";
    // programs << "int x = 5;\nint y = x;\nint z = y;\nprint(z);";
    // programs << "int x = 1;\nint y = 2;\nint z = 3;\nprint(x + y + z);\nprint(x * y * z);\nprint(x - y - z);";
    // programs << "int f() {\n    int x = 5;\n    print(x);\n}\nf();\nf();\nf();";
    // programs << "int x = 5 * 3 + 2 * 4 - 1;";
    // programs << "int x = (5);";
    // programs << "int x = 5;\n{\n    x = 10;\n    print(x);\n}\nprint(x);";
    // programs << "int f(int x) {\n    if(x < 0) {\n        print(x);\n    }\n}\nf(-1);\nf(1);";
    // programs << "int x = 5;\nint y = 0;\nint z = x * y;\nprint(z);";
    // programs << "int f(int a, int b) {\n    print(a + b);\n}\nf(3, 4);\nf(10, 20);\nf(0, 0);";
    // programs << "int x = 5;\nprint(x + x + x + x + x);";
    // programs << ";;;;;;;;;;";
    // programs << "int x = 5;\nint y = x + ;\nprint(y);";
    // programs << "int f(int x) { print(x); }\nint g(int x) { f(x); }\nint h(int x) { g(x); }\nh(42);";
    // programs << "int x = 5;\nprint(\n    x\n);";
    // programs << "int x = 5;\nint y = 10;\nif(x < y) {\n    print(x);\n}";
    // programs << "int f() {\n    int x = 5;\n    return;\n    print(x);\n}\nf();";
    // programs << "int x = 1+2+3+4+5+6+7+8+9+10;\nint y = x * 2;\nprint(y);";
    // programs << "int f(int x) {\n    int y = x + 1;\n    int z = y + 1;\n    print(z);\n}\nf(1);\nf(2);\nf(3);";
    // programs << "int x = 5;\n) x ( = 10;\nprint(x);";
    // programs << "int f(int x, int y, int z) {\n    print(x);\n    print(y);\n    print(z);\n}\nf(1,2,3);";
    // programs << "{}{}{}{}";
    // programs << "int x = 5;\nint x = x + 1;\nint x = x + 1;\nprint(x);";
    // programs << "int f() { int x = 5; }\nint g() { int y = f(); }\ng();";
    // for(int i=0;i<programs.length();i++) {
    //     print(blue("PROGRAM "),i,":\n",programs[i]);
    //     acorn->run(acorn->process(programs[i]));
    //     if(!Acorn::ERROR_FLAG) {
    //         print(green("DID NOT CRASH, CHECK STAMP: "));
    //         print(acorn->nodenet_to_string(acorn->unit_root));
    //     } else {
    //         Acorn::ERROR_FLAG = false;
    //     }
    // }
