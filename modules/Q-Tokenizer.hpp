#include "../core/GDSL.hpp"

namespace GDSL {
    map<std::string,uint32_t> tokenized_keywords;
    map<char,bool> char_is_split;
    size_t make_tokenized_keyword(const std::string& f) {
        size_t id = reg_id(f);
        tokenized_keywords.put(f,id);
        return id;
    }

    size_t add_token(char c, const std::string& f) {
        size_t id = reg_id(f);
        tokenizer_functions.put(c,[id,c](Context& ctx) {
            ctx.node = make<Node>(id,c);
            ctx.result->push(ctx.node);
        });
        char_is_split.put(c, true);
        return id;
    }


    void init_q_tokenizer() {

    }


}