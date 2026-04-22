#pragma once

#include "../core/type.hpp"
#include "../core/object.hpp"

class TypePool : public Type {
public:
    TypePool() {
        free_stack_top.store(0);
    }

    list<g_ptr<Object>> objects;
    list< std::function<void(g_ptr<Object>)> > init_funcs;
    list<int> free_ids;
    std::atomic<size_t> free_stack_top{0};  // Points to next free slot

    std::function<g_ptr<Object>()> make_func = [](){
        auto object = make<Object>();
        return object;
    };

    int get_next() {
        size_t current_top, new_top;
        do {
            current_top = free_stack_top.load();
            if (current_top == 0) return -1;
            new_top = current_top - 1;
        } while (!free_stack_top.compare_exchange_weak(current_top, new_top));
        
        return free_ids.get(new_top);
    }
    
    void return_id(int id) {
        size_t current_top = free_stack_top.load();
        if (current_top < free_ids.size()) {
            // There's space, just write and increment pointer
            size_t slot = free_stack_top.fetch_add(1);
            free_ids.get(slot) = id;
        } else {
            // Need to grow the list
            free_ids.push(id);
            free_stack_top.fetch_add(1);
        }
    }
    

    g_ptr<Object> create() {
        int next_id = get_next();
        g_ptr<Object> object = nullptr;
        if(next_id!=-1)
        {
            object = objects.get(next_id);
            for(int i=0;i<init_funcs.size();i++) {
                init_funcs[i](object);
            }
        }
        else
        {
            object = make_func();
            object->type_ = this;
            store(object);
            for(int i=0;i<init_funcs.size();i++) {
                init_funcs[i](object);
            }
        }
        reactivate(object);
        object->recycled.store(false);
        return object;
    }

    size_t add_column(size_t size) {
        _column col(size);
        for(size_t i = 0; i < objects.length(); i++) col.push_default();
        int col_len = columns.length();
        columns.push(col);
        return col_len;
    }

    private:
        void store(g_ptr<Object> object)
        {
            object->sidx = objects.size();
            for(int c = 0; c<columns.length(); c++) {
                columns[c].push_default();
            }
            objects.push(object);
        }
    public:

    void add_initializers(list<std::function<void(g_ptr<Object>)>> inits) {for(auto i : inits) add_initializer(i);}
    void add_initializer(std::function<void(g_ptr<Object>)> init) {init_funcs << init;}
    void operator+(std::function<void(g_ptr<Object>)> init) {add_initializer(init);}
    void operator+(list<std::function<void(g_ptr<Object>)>> inits) {for(auto i : inits) add_initializer(i);}

    void recycle(g_ptr<Object> object) {
        if(object->recycled.load()) {
            return;
        }
        object->recycled.store(true);

        return_id(object->sidx);
        deactivate(object);
    }

    void deactivate(g_ptr<Object> object) {
        object->stop();
    }

    void reactivate(g_ptr<Object> object) {
        object->resurrect();
    }

};