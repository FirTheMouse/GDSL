#pragma once

#include <iostream>
#include <atomic>
#include "../util/util.hpp"
#include "../core/q_object.hpp"

class Type;

struct Storage : public q_object {
    list<float> data;
    
    Storage() = default;
    
    Storage(int size) {
        data.resize(size);
        for(int i = 0; i < size; i++) {
            data[i] = 0.0f;
        }
    }
    
    Storage(list<float>&& d) : data(std::move(d)) {}
};

struct ScriptContext
{
    Data data;

    ScriptContext() {

    }
    ~ScriptContext() {

    }

    template<typename T>
    T get(const std::string& label) { return data.get<T>(label); }

    template<typename T>
    void set(const std::string& label, T value) { data.set(label, value); }

    bool has (const std::string& label) {return data.has(label);}

    /// @brief  starts false, switches on each call
    bool toggle(const std::string& label) {return data.toggle(label);}
    bool check(const std::string& label) {return data.check(label);}
    void flagOn(const std::string& label) {data.flagOn(label);}
    void flagOff(const std::string& label) {data.flagOff(label);}

    template<typename T = int>
    T inc(const std::string& label,T by)
    { return data.inc<T>(label,by); }

};

template<typename func = std::function<void(ScriptContext&)>>
struct Script{
    std::string name;
    func run;
    Script() = default;
    Script (std::string _name,func&& behaviour) :
      name(_name), run(behaviour) {}
    // Copy constructor
    Script(const Script& other) = default;
    // Move constructor
    Script(Script&& other) noexcept = default;
    // Copy assignment operator
    Script& operator=(const Script& other) = default;
    // Move assignment operator
    Script& operator=(Script&& other) noexcept = default;
    ~Script() = default;

};

class Object : virtual public q_object {    
    public:
        Data data;
        std::string dtype = "";
        std::string debug_trace_path = "";
        int UUID = -1;
        int ID = -1;
        int TID = -1;
        std::atomic<bool> recycled{false};

        template<typename T = std::string>
        T get(const std::string& label)
        { return data.get<T>(label); }

        template<typename T = std::string>
        void add(const std::string& label,T info)
        { data.add<T>(label,info); }

        template<typename T = std::string>
        void set(const std::string& label, T info)
        { data.set<T>(label,info); }

        bool has(const std::string& label)
        { return data.has(label); }

        /// @brief  starts false, switches on each call
        bool toggle(const std::string& label) {return data.toggle(label);}
        bool check(const std::string& label) {return data.check(label);}
        void flagOn(const std::string& label) {data.flagOn(label);}
        void flagOff(const std::string& label) {data.flagOff(label);}

        template<typename T = int>
        T inc(const std::string& label,T by)
        { return data.inc<T>(label,by); }

        map<std::string,Script<>> scripts;

        template<typename F>
        void addScript(const std::string& name, F&& f) {
            scripts.put(name,Script<>(name, std::function<void(ScriptContext&)>(std::forward<F>(f))));
        }

        void addScript(const Script<>& script) {
            scripts.put(script.name,script);
        }

        template<typename F>
        void replaceScript(const std::string& name, F&& f)
        {
            removeScript(name);
            addScript(name,f);
        }

        void removeScript(const std::string& name) {
            scripts.remove(name);
        }

        //void runAll() {for (const Script<>& s : scripts) {if(s.run) s.run();}}

        void run(const std::string& type,ScriptContext& ctx) {
            for(auto s : scripts.getAll(type)) s.run(ctx);
        }

        /// @brief Convenience for running when a context is not needed to reduce boilerplate
        void run(const std::string& type) {
            ScriptContext ctx;
            for(auto s : scripts.getAll(type)) s.run(ctx);
        }
    
        Type* type_ = nullptr;

        Object() {

        }
        virtual ~Object() {}

        Object(Object&& other) noexcept 
        : q_object(std::move(other)),
          data(std::move(other.data)),
          dtype(std::move(other.dtype)) {
            // scripts = other.scripts;
        }

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                q_object::operator=(std::move(other));
                data = std::move(other.data);
                dtype = std::move(other.dtype);
                // scripts = std::move(other.scripts);
            }
            return *this;
        }
    };


