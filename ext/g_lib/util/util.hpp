#pragma once

#include <any>
#include <stdlib.h>

#include <initializer_list>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

#include "../util/list.hpp"
#include "../util/map.hpp"
#include "../util/basic.hpp"

inline list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}

class Data{
public:
    Data() {}

    map<std::string,std::any> notes;

    template<typename T = std::string>
    void add(const std::string& label,T info)
    {
        notes.put(label,std::any(info));
    }

    template<typename T = std::string>
    T get(const std::string& label)
    {
        #if !DISABLE_BOUNDS_CHECK
            if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
        #endif
        return std::any_cast<T>(notes.get(label));
    }

    bool has(const std::string& label)
    {
        return notes.hasKey(label);
    }

    bool check(const std::string& label)
    {
        if(!has(label)) return false;
        try {
            return get<bool>(label);
        }
        catch(std::exception e)
        {
            print("data::check::59 Attempted to check a non-bool in data");
            //Or just return false?
            return false;
        }
    }

    bool toggle(const std::string& label) {
        if(!has(label)) set<bool>(label,true);
        bool toReturn = !get<bool>(label);
        set<bool>(label,toReturn);
        return toReturn;
    }

    void flagOn(const std::string& label) {set<bool>(label,true);}
    void flagOff(const std::string& label) {set<bool>(label,false);}

    template<typename T>
    void set(const std::string& label,T info) {
        if(!notes.set(label,info))
            add<T>(label,info);
    }

    template<typename T = int>
    T inc(const std::string& label,T by)
    {
        if(has(label)) {set<T>(label,get<T>(label)+by);}
        else {add<T>(label,by);}
        return get<T>(label);
    }

    void debugData() {
        notes.debugMap();
    }
    
    /// @brief Scans through based on provided list, returns all missing labels
    list<std::string> validate(list<std::string> toCheck)
    {
        list<std::string> toReturn;
        for(auto s : toCheck) if(!has(s)) toReturn << s;
        return toReturn;
    }
};

inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }
  
  inline void writeFile(const std::string& filename, const std::string& contents) {
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filename);
  
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) throw std::runtime_error("Failed while writing file: " + filename);
  }
  
  inline void editTextFile(  const std::string& filename, const std::function<void(std::string&)>& editor) {
    std::string text = readFile(filename);
    editor(text);
    writeFile(filename, text);
  }

  template<typename T>
  inline void write_raw(std::ostream& out, const T& val) {
      out.write(reinterpret_cast<const char*>(&val), sizeof(T));
  }

  inline void write_string(std::ostream& out, const std::string& s) {
      uint32_t len = s.length();
      write_raw(out, len);
      out.write(s.data(), len);
  }

  template<typename T>
  inline T read_raw(std::istream& in) {
      T val;
      in.read(reinterpret_cast<char*>(&val), sizeof(T));
      return val;
  }

  inline std::string read_string(std::istream& in) {
      uint32_t len = read_raw<uint32_t>(in);
      std::string s(len, '\0');
      in.read(s.data(), len);
      return s;
  }



