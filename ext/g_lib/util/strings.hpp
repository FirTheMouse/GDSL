#pragma once

#include "../util/util.hpp"

inline std::string add_commas(int num) {
    std::string str = std::to_string(num);
    int insert_position = str.length() - 3;
    
    while(insert_position > 0) {
        str.insert(insert_position, ",");
        insert_position -= 3;
    }
    
    return str;
  }
  
  inline void indent_multiline(std::string& str, const std::string& pad) {
    size_t pos = 0;
    while((pos = str.find('\n', pos)) != std::string::npos) {
        str.replace(pos, 1, "\n" + pad);
        pos += pad.length() + 1;
    }
  }

    inline void strip_whitespace(std::string& s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == ' ' || c == '\n' || c == '\r' || c == '\t';
        }), s.end());
    }
  
  inline std::string wrap_str(const std::string& s,const std::string& c) {
    return c+s+c;
  }
  
  inline std::string trim_str(const std::string& s,const char c) {
    if (s.size() >= 2 && s.front() == c && s.back() == c)
        return s.substr(1, s.size() - 2);
    return s; 
  }


    std::string pad_str(const std::string& s, uint32_t width) {
        std::string to_return = s;
        while(width>to_return.length()) to_return+=" ";
        return to_return;
    }

    std::string center_pad(const std::string& s, uint32_t width) {
        if(s.length() >= width) return s;
        uint32_t total_pad = width - s.length();
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }
    std::string center_pad_known(const std::string& s, uint32_t s_visible_len, uint32_t width) {
        if(s_visible_len >= width) return s;
        uint32_t total_pad = width - s_visible_len;
        uint32_t left_pad = total_pad / 2;
        uint32_t right_pad = total_pad - left_pad;
        return std::string(left_pad, ' ') + s + std::string(right_pad, ' ');
    }

    uint32_t digit_count(uint32_t n) {
        if(n == 0) return 1;
        uint32_t digits = 0;
        while(n > 0) { n /= 10; digits++; }
        return digits;
    }

    bool is_str_num(const std::string& tocheck) {for(auto c : tocheck) {if(!std::isdigit(c)) return false;} return true;}

    std::string escape_string(const std::string& content, bool compact_spaces = true) {
        std::string escaped;
        int space_count = 0;
        for(char c : content) {
            if(compact_spaces) {
                if(c == ' ') {
                    if(space_count==1) {escaped.pop_back(); escaped += "..."; space_count++; continue;}
                    else if(space_count>1) {continue;}
                    else {space_count++;}
                } else {
                    space_count = 0;
                }
            }
            switch(c) {
                case '\n': escaped += "\\n"; break;
                case '\t': escaped += "\\t"; break;
                case '\r': escaped += "\\r"; break;
                //case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }

    std::string strip_ansi(const std::string& s) {
        std::string out;
        bool in_escape = false;
        for(int i = 0; i < s.length(); i++) {
            if(s[i] == '\033') { in_escape = true; continue; }
            if(in_escape) {
                if(s[i] == 'm') in_escape = false;
                continue;
            }
            out += s[i];
        }
        return out;
    }
    
    std::string html_escape_string(const std::string& content) {
        std::string escaped;
        int space_count = 0;
        for(char c : content) {
            switch(c) {
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '&': escaped += "&amp;"; break;
                case '\'': escaped += "&apos;"; break;
                case '"': escaped+="&quot;"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }
  
namespace sgen {
    struct namebase {
        namebase() {}
        explicit namebase(const list<list<std::string>>& _opts) : opts(_opts) {}
        explicit namebase(const std::string& seed) {
            list<std::string> lines = split_str(seed,',');
            for(const auto& l : lines) {
                opts << split_str(l,'|');
            }
        }
        list<list<std::string>> opts;
    };

    const namebase STANDARD("Ja|Be|Ma|Cer|Le,ck|de|ly|th|ch|un|el");
    const namebase TRUE_RANDOM("a|A|b|B|c|C|d|D|e|E|f|F|g|G|h|H|i|I|j|J|k|K|l|L|m|M|n|N|o|O|p|P|q|Q|r|R|s|S|t|T|u|U|v|V|w|W|x|X|y|Y|z|Z|0|1|2|3|4|5|6|7|8|9|_|+|-|*|/|=|<|>|!|&|^|.|,|:|;|(|)|[|]|{|}|\"|#|@|~|`|\\");
    const namebase RANDOM(
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z,"
        "A|a|B|b|C|c|D|d|E|e|F|f|G|g|H|h|I|i|J|j|K|k|L|l|M|m|N|n|O|o|P|p|Q|q|R|r|S|s|T|t|U|u|V|v|W|w|Y|y|X|x|Z|z");
    const namebase AVAL_WEST_TAMOR_FIRST(
        "Bu|Ahm|He|Ol|Mo|In|Bir|Ba|Tu," 
        "|||||||||||||||||||ha|ck|a|ch," 
        "el|ba|ak|ael|he|med");

    const namebase AVAL_CENTRAL_FIRST_MALE(
        "Al|Ed|Da|Ro|Wil|Tho|Hen|Mar|Reg|Cla|Luc|Aug,"  
        "||||||||||||an|ar|er|or|ald|ric|vid|lan|den|bert|tor|mon,"
        "us|d|n|rt|mer|son|ard|ton|las|ius|mond|iel");

    const namebase AVAL_WESTERN_FIRST_MALE(
        "Jo|Al|Con|Se|Sok|Va|Wel|Eg," 
        "|||||||||rgo|ra|ell|ber,"
        "der|us|ard|rk|on|th|n|l|vid");

    const namebase AVAL_CENTRAL_FIRST_FEMALE(
        "My|Al|Se|Ma|Eg|Cha|Sha|Tha," 
        "|||||ri|ex|il,"
        "|||na|der|ra|us|da|na|et");
    const namebase AVAL_CENTRAL_LAST(
        "Copper|Silver|Iron|Wood|High|Low|Swift|Old|New|Red|White|Black|Green|Blue|Yellow," 
        "paw|tail|fang|talon|wing|feather|river|hill|heart|claw|hall");

    inline std::string randsgen(const namebase& g) {
        std::string result;
        for(const auto& s : g.opts) 
            result.append(s.rand());
        return result;
    }
    
    inline std::string randsgen(const std::string& line) {
        list<std::string> lines = split_str(line,',');
        std::string result;
        for(const auto& l : lines) {
            list<std::string> sub = split_str(l,'|');
            std::string app = sub.rand();
            result.append(app);
        }
        return result;
    }
}

inline std::string rands() {
    return sgen::randsgen(sgen::TRUE_RANDOM);
}

