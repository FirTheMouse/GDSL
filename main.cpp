#include "modules/GDSL-C.hpp"
//#include "modules/GDSL-LISP.hpp"

int main(int argc, char* argv[]) {
    // if(argc < 2) {
    //     print("Usage: ./gdsl <file>");
    //     return 1;
    // }
    // std::string path = argv[1];
    // GDSL::test_module(path);
    // GDSL::test_module("modules/tests/lisptest.gld");
    // GDSL::test_module("modules/tests/ctest.gld");
    GDSL::test_module("modules/tests/maintest.gld");
    return 0;

}

// #include "modules/GDSL-CALC.hpp"

// int main() {
//     GDSL::my_module();
//     return 0;
// }
