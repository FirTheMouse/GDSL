#pragma once

#include "../ext/g_lib/util/logger.hpp"
#include <sys/mman.h>

//Controls for the compiler printing, for debugging
#define PRINT_ALL 0

g_ptr<Log::Span> span = nullptr;
static inline void newline(const std::string& label) {
    #if PRINT_ALL
    span->add_line(label);
    #endif
}
static inline double endline() {
    #if PRINT_ALL
    return span->end_line();
    #else
    return 0;
    #endif
}

template<typename... Args>
static inline void log(Args&&... args) {
    #if PRINT_ALL
    span->log(std::forward<Args>(args)...);
    #endif
}

std::string ptr_to_string(uint64_t addr) {
    uint64_t varied = addr >> 4;
    
    const char* profiles[] = {"CGOQD", "IHLTFE", "AVWXZK", "BPRSM"};
    int prof = varied & 0x3;
    
    const char* group = profiles[prof];
    int letter_idx = (varied >> 2) % strlen(group);
    char letter = group[letter_idx];
    
    uint64_t tiebreaker = (varied >> 6) & 0xFFF;
    std::string tbstr = std::to_string(tiebreaker);
    std::reverse(tbstr.begin(), tbstr.end());
    
    return std::string({letter}) + "-" + tbstr + "-" + letter;
}

std::string ptr_to_string(void* ptr) {
    return ptr_to_string((uint64_t)ptr);
}

struct IdPool {
    list<int> ids;
    int top = 0;
    
    void init(list<int> available) {
        ids = available;
        top = available.length();
    }
    
    int alloc() {
        if(top == 0) return -1;
        return ids[--top];
    }
    
    void free(int id) {
        if(id != -1) 
            ids[top++] = id;
    }
};






