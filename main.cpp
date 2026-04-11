#include "modules/GDSL-C.hpp"
#include "modules/GDSL-LISP.hpp"

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

    g_ptr<LISP_Unit> lisp = make<LISP_Unit>();
    lisp->init();

    root = lisp->process("modules/tests/lisptest.gld");
    lisp->run(root);
    span = make<Log::Span>();

    g_ptr<C_Compiler> c = make<C_Compiler>();

    c->emit_mode = true;
    c->init();
    c->span2->log_everything = true;
    root = c->process("modules/tests/cemittest.gld");
    c->serialize(root);
    c->saveBinary("savetest.wub");
    root = c->loadBinary("savetest.wub");
    c->run(root);
    span = make<Log::Span>();
    c->emit_mode = false;
    c->span2 = make<Log::Span>();
    c->emit_buffer.clear();

    root = c->process("modules/tests/ctest.gld");
    c->run(root);

    print("TEST FINISHED");



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
