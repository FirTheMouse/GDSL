//#include "modules/GDSL-C.hpp"
#include "modules/GDSL-LISP.hpp"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        print("Usage: ./gdsl <file>");
        return 1;
    }
    std::string path = argv[1];
    GDSL::test_module(path);
    return 0;

}