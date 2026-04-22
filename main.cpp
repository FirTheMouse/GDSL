#include "modules/GDSL-Test.hpp"
#include "modules/Q-HTML.hpp"
#include "modules/GDSL-C.hpp"
#include "modules/GDSL-LISP.hpp"
#include "modules/GDSL-TwigSnap.hpp"
#include "modules/GDSL-Script.hpp"
#include "modules/GDSL-GQL.hpp"
#include "modules/GDSL-Thistle.hpp"
#include "mixos-acorn/Acorn-Script.hpp"

using namespace GDSL;

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
    g_ptr<Node> root = nullptr;

    span = make<Log::Span>();
    //span->log_everything = true; //While things are crashing
 
    // g_ptr<Acorn::Acorn_Script> test = make<Acorn::Acorn_Script>();
    // test->init();
    // test->run(test->process(""));

    // g_ptr<Thistle_Unit> twig = make<Thistle_Unit>();
    // root = twig->process(readFile("modules/tests/pebble.gld"));
    // twig->run(root);

    // g_ptr<Thistle_Unit> thistle = make<Thistle_Unit>();
    // g_ptr<Node> thistle_root = thistle->process(readFile("modules/tests/testpage.gld"));
    // thistle->run(thistle_root);
    
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
    // // root = c->process(readFile("modules/tests/ctest.gld"));
    // // c->run(root);

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
