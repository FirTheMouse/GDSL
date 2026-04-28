#pragma once

#include "../ext/g_lib/core/typePool.hpp"
#include "../ext/g_lib/util/logger.hpp"
#include "../core/Golden.hpp"
#include <sys/mman.h>

namespace GDSL {

//GDSL, Golden Dynamic Systems Language
//TAST: Transform, assemble, scope, type.
//DRE: Discover, resolve, evaluate
//MIX: Model, interpret, execute

class Node;
class Value;

static int _ctx_dummy_index = 0;

struct Context {
    Context() : index(_ctx_dummy_index) {}
    Context(int& _index) : index(_index) {}
    Context(list<g_ptr<Node>>& _result, int& _index) : result(&_result), index(_index) {}

    g_ptr<Node> node;
    g_ptr<Node> qual;
    g_ptr<Node> left;
    g_ptr<Node> out;
    g_ptr<Node> root;
    list<g_ptr<Node>>* result = nullptr;
    list<g_ptr<Node>> nodes;
    g_ptr<Value> value;
    int& index;
    uint32_t state = 0;
    bool flag = false;

    Context* sub;

    std::string source;

    Context duplicate() {
        return Context((*result), index);
    }
};

inline void copy_value(g_ptr<Value> val, g_ptr<Value> o, bool is_deep = false);
inline uint32_t type_of_node(g_ptr<Node> node);

class Value : public Object {
public:
    Value() {}
    Value(uint32_t _type) : type(_type) {}
    Value(uint32_t _type, size_t _size) : type(_type), size(_size) {}
    Value(uint32_t _type, size_t _size, uint32_t _sub_type) : type(_type), size(_size), sub_type(_sub_type) {}
    ~Value() {}
    uint32_t type = 0;
    uint32_t sub_type = 0;
    void* data = nullptr;
    int address = 0;
    int reg = -1;
    int loc = -1;
    int save_idx = -1;
    size_t size = 0;
    int sub_size = 0;
    list<g_ptr<Node>> quals;
    list<g_ptr<Value>> sub_values;
    Node* type_scope = nullptr;
    g_ptr<Type> store = nullptr;

    void copy(g_ptr<Value> o, bool is_deep = false) {
        copy_value(this, o, is_deep);
    }

    int find_qual(size_t q_id) {
        for(int i=0;i<quals.length();i++){ 
            if(type_of_node(quals[i])==q_id) {return i;}
        }
        return -1;
    } 

    g_ptr<Node> get_qual(size_t q_id) {
        int q_at = find_qual(q_id);
        if(q_at!=-1) {
            return quals[q_at];
        }
        return nullptr;
    } 

    bool has_qual(size_t q_id) {
        return find_qual(q_id)!=-1;
    }

    inline void query_store(_note& item_data, g_ptr<Type> target) {
        _column& col = target->columns[item_data.index];
        data = col.get(item_data.sub_index);
        size = col.element_size;
        type = item_data.tag;
    }

    inline void query_store(int index, g_ptr<Type> target = nullptr) {
        if(!target) target = store;
        query_store(target->get_note(index),target);
    }

    inline void query_store(const std::string& label, g_ptr<Type> target = nullptr) {
        if(!target) target = store;
        query_store(target->get_note(label),target);
    }

    inline void store_value(g_ptr<Type> to) {
        to->push(data,size,-1,type);
    }

    inline void store_value(g_ptr<Type> to, const std::string& as) {
        to->add(as, data, size,-1,type);
    }

    template<typename T>
    T& get() { return *(T*)data; }
    
    template<typename T>
    void set(const T& value) { 
        if (!data) {
            data = malloc(sizeof(T));
            size = sizeof(T);
        }
        new(data) T(value);
    }

    bool is_true() {
        if(data) {
            return *(bool*)data; 
        }
        return false;
    }
};

g_ptr<Value> fallback_value = nullptr;

//Single unified Node for everything
class Node : public Object {
public:
    Node() {
        value = make<Value>();
    }
    Node(std::string _name) : name(_name) {
        value = make<Value>();
    }
    Node(std::string _name, std::string _opt_str) : name(_name), opt_str(_opt_str) {
        value = make<Value>();
    }
    Node(uint32_t _type, char c) : type(_type) {
        name += c;
        value = make<Value>();
    }
    Node(uint32_t _type) : type(_type) {
        value = make<Value>();
    }
    Node(uint32_t _type, std::string _name) : type(_type), name(_name) {
        value = make<Value>();
    }
    Node(uint32_t _type, uint32_t _sub_type, std::string _name) : type(_type), sub_type(_sub_type), name(_name) {
        value = make<Value>();
    }

    uint32_t type = 0;
    uint32_t sub_type = 0;
    std::string name = "";
    float x = -1.0f;
    float y = -1.0f;
    float z = -1.0f;
    g_ptr<Value> value = nullptr;
    list<g_ptr<Node>> children;

    list<g_ptr<Node>> quals;

    map<std::string,g_ptr<Node>> node_table;
    map<std::string,g_ptr<Value>> value_table;
    list<g_ptr<Node>> scopes;
    Node* parent = nullptr;
    Node* owner = nullptr;
    Node* in_scope = nullptr;
    bool is_scope = false;
    std::string opt_str;

    int save_idx = -1;
    bool mute = false;




    const inline g_ptr<Node> left() { 
        return children.length() > 0 ? children[0] : nullptr; 
    }
    const inline g_ptr<Node> right() { 
        return children.length() > 1 ? children[1] : nullptr; 
    }

    bool find_value_in_scope() {
        if(in_scope->value_table.hasKey(name)) {
            value = in_scope->value_table.get(name);
            return true;
        }
        return false;
    }


    bool find_node_in_scope() {
        if(in_scope->node_table.hasKey(name)) {
            if(scopes.length()>0) scopes.clear(); 
            scopes << in_scope->node_table.get(name);
            return true;
        }
        return false;
    }

    bool value_is_valid() {
        if(value) {
            return value->type != 0;
        }
        return false;
    }

    void copy(g_ptr<Node> o, bool is_deep = false) {
        type = o->type;
        sub_type = o->sub_type;
        name = o->name;
        x = o->x;
        y = o->y;
        z = o->z;
        mute = o->mute;
        is_scope = o->is_scope;
        if(is_deep) {
            value->copy(o->value,true);
            children.clear();
            for(auto c : o->children) {
                g_ptr<Node> newc = make<Node>();
                newc->copy(c,true);
                children << newc;
            }
            quals.clear();
            for(auto q : o->quals) {
                g_ptr<Node> newq = make<Node>();
                newq->copy(q,true);
                quals << newq;
            }
            scopes.clear();
            for(auto s : o->scopes) {
                g_ptr<Node> news = make<Node>();
                news->copy(s,true);
                o->in_scope->scopes << news;
                scopes << news;
            }
        } else {
            value = o->value;
            children = o->children;
            quals = o->quals;
            scopes = o->scopes;
        }
        value_table = o->value_table;
        node_table = o->node_table;
        parent = o->parent;
        owner = o->owner;
        in_scope = o->in_scope;
        opt_str = o->opt_str;
    }

    g_ptr<Node> scope() { return scopes.empty() ? nullptr : scopes[0]; }
    uint32_t getType() {return type;}
    void setType(uint32_t _type) {type = _type;}

    std::string addr_str() {return std::to_string((size_t)(void*)this);}

    int find_qual(size_t q_id) {
        for(int i=0;i<quals.length();i++){ 
            if(quals[i]->type==q_id) {return i;}
        }
        return -1;
    } 

    int find_qual_in_value(size_t q_id) {
        if(value)
            return value->find_qual(q_id);
        return -1;
    }
    
    bool has_qual(size_t q_id, bool check_value = true) {
        for(auto q : quals){ if(q->type==q_id) {return true;}}
        if(value&&check_value) {for(auto q : value->quals){if(q->type==q_id){return true;}}}
        return false;
    }

    void distribute_qual_to_values(g_ptr<Node> qual,size_t exclude_id = 0) {
        if(!has_qual(exclude_id))
            value->quals << qual;
        for(auto c : children) {
            c->distribute_qual_to_values(qual,exclude_id);
        }
    }

    g_ptr<Node> spawn_sub_scope() {
        g_ptr<Node> new_node = make<Node>();
        new_node->parent = this;
        new_node->is_scope = true;
        scopes << new_node;
        return new_node; 
    }


    void place_in_scope(Node* scope) {
        in_scope = scope;
        for(auto c : children) 
            c->place_in_scope(scope);
    }

    void shunt_to_scope(Node* scope) {
        if(in_scope) {
            in_scope->children.erase(this);
        }
        in_scope = scope;
        in_scope->children << this;
        for(auto c : children)  {
            c->shunt_to_scope(scope);
        }
    }

    g_ptr<Value> distribute_value(const std::string& label, g_ptr<Value> val) {
        //log("Distributing a value: ",label," through ",name);
        if(value_table.hasKey(label)) {
            g_ptr<Value> table_value = value_table.get(label);
            if(table_value->type == 0) {
                //log("Replacing the value with already existing value");
                table_value->copy(val);
                val = table_value;
            } else {
                //log("Doing nothing because there's an existing type with a valid value");
            }
        } else {
            //log("Putting into table");
            value_table.put(label, val);
        }
        for(auto s : scopes) {
            val = s->distribute_value(label, val);
        }
        return val;
    }

    g_ptr<Node> distribute_node(const std::string& label, g_ptr<Node> node) {
        if(node_table.hasKey(label)) {
            g_ptr<Node> table_node = node_table.get(label);
            if(table_node->name.empty()) {
                table_node->copy(node);
                node = table_node;
            }
        } else {
            node_table.put(label, node);
        }
        for(auto s : scopes) {
            node = s->distribute_node(label,node);
        }
        return node;
    }

    g_ptr<Value> obliterate_value(const std::string& label, g_ptr<Value> val) {
        print(red("OBLITERATING: "),red(label),"!");
        if(value_table.hasKey(label)) {
            g_ptr<Value> table_value = value_table.get(label);
            if(table_value==val) {
                value_table.remove(label);
                print(red("FOUND"));
            } else print(red("NOT FOUND"));
        }
        for(auto s : scopes) {
            val = s->obliterate_value(label, val);
        }
        return val;
    }
};

inline uint32_t type_of_node(g_ptr<Node> node) {
    return node->type;
}

void copy_value(g_ptr<Value> val, g_ptr<Value> o, bool is_deep) {
    val->type = o->type; 
    val->sub_type = o->sub_type; 
    val->address = o->address; 
    val->size = o->size; 
    val->sub_size = o->sub_size;
    val->type_scope = o->type_scope;
    val->store = o->store; //Add deep copying here too
    if(is_deep) {
        for(auto oq : o->quals) {
            g_ptr<Node> cpy = make<Node>();
            cpy->copy(oq,is_deep);
            val->quals << cpy;
        }
    } else {
        val->quals = o->quals;
    }

    if(o->data) {
        val->data = malloc(val->size);
        memcpy(val->data,o->data,val->size);
    }
}

g_ptr<Node> value_to_qual(g_ptr<Value> val) {
    g_ptr<Node> to_return = make<Node>();
    to_return->value = val;
    to_return->type = val->type;
    to_return->sub_type = val->sub_type;
    return to_return;
}


void deep_copy_node(g_ptr<Node> n, g_ptr<Node> o, map<g_ptr<Value>,g_ptr<Value>>& value_alias_table, map<g_ptr<Node>,g_ptr<Node>>& node_alias_table) {
    n->type = o->type;
    n->sub_type = o->sub_type;
    n->name = o->name;
    n->x = o->x;
    n->y = o->y;
    n->z = o->z;
    n->mute = o->mute;
    n->is_scope = o->is_scope;
    n->children.clear();
    for(auto c : o->children) {
        g_ptr<Node> newc = make<Node>();
        deep_copy_node(newc,c,value_alias_table,node_alias_table);
        n->children << newc;
    }
    n->quals.clear();
    for(auto q : o->quals) {
        g_ptr<Node> newq = make<Node>();
        deep_copy_node(newq,q,value_alias_table,node_alias_table);
        n->quals << newq;
    }
    n->scopes.clear();
    for(auto s : o->scopes) {
        g_ptr<Node> news = make<Node>();
        deep_copy_node(news,s,value_alias_table,node_alias_table);
        if(n->in_scope) 
            n->in_scope->scopes << news;
        n->scopes << news;
    }

    if(value_alias_table.hasKey(o->value)) {
        n->value = value_alias_table.get(o->value);
        if(n->value->type_scope) {
            if(n->scope())
                n->scopes[0] = n->value->type_scope;
            else 
                n->scopes << n->value->type_scope;
            n->in_scope->scopes << n->scope();
        } 
    } else {
        n->value->copy(o->value,true);
    }

    n->value_table = o->value_table;
    n->node_table = o->node_table;
    n->parent = o->parent;
    n->owner = o->owner;
    n->in_scope = o->in_scope;
    n->opt_str = o->opt_str;
}

using Handler = std::function<void(Context&)>;


struct Unit : public Object {

    map<uint32_t,std::string> labels;
    list<g_ptr<Type>> types;

    //Used for debugging, and for printing 
    map<uint32_t,std::function<std::string(void*)>> value_to_string;
    map<uint32_t, std::function<void(void*)>> negate_value;

    std::string data_to_string(uint32_t type,void* data) {    
        try {
            return value_to_string.get(type)(data);
        }
        catch(std::exception e) {
            return "[missing value_to_string for type "+labels[type]+"]";
        }
    }

    std::string value_as_string(g_ptr<Value> value) {
        if (!value->data) {
            return "[null]";
        }
        std::function<std::string(void*)> fallback_func = [value,this](void* ptr){return "[missing value_to_string for type "+labels[value->type]+"]";};
        return value_to_string.getOrDefault(value->type,fallback_func)(value->data);
    }

    std::string value_info(g_ptr<Value> value) {
        std::string to_return = "";
        to_return += "["+ptr_to_string(value.getPtr())+"]"
        + "(type: " + labels[value->type]
        + (value->reg!=-1?", reg: "+std::to_string(value->reg):"")
        + (value->data?", value: "+value_as_string(value)+" @"+ptr_to_string(value->data):"")
        + (value->sub_type!=0?", sub_type: "+labels[value->sub_type]:"")
        + (value->type_scope?", has scope":"")
        + (value->size!=0?", size: "+std::to_string(value->size):"")
        + (value->address!=0?", address: "+std::to_string(value->address):"")
        + (value->store?", has store":"")
        + (!value->sub_values.empty()?", sub: "+std::to_string(value->sub_values.length()):"");
        if(!value->quals.empty()) {
            to_return += ", Quals: ";
            for(int i=0;i<value->quals.length();i++) { //Drop in v +ptr_to_string(value->quals[i].getPtr())
                to_return += labels[type_of_node(value->quals[i])]+(i!=value->quals.length()-1?", ":"");
            }
        }
        to_return += ")";
        return to_return;
    }

    void value_as_negative(g_ptr<Value> value) {
        if(value->data) {
            try {
                negate_value.get(value->type)(value->data);
            }
            catch(std::exception e) {
                print("value::300 missing negate_value handler for ",labels[value->type]);
            }
        }
    }


    std::string node_info(g_ptr<Node> node) {
        std::string to_return = "";
        to_return += labels[node->type]
        + (node->sub_type==0?"":":"+labels[node->sub_type])
        + (node->name.empty()?"":" "+green(node->name)+" ")  
        + (node->value?value_info(node->value):"")
        + (node->x!=-1.0f?"("+std::to_string((int)node->x)+","+std::to_string((int)node->y)+")":"")
        + (!node->children.empty()?"[C:"+std::to_string(node->children.length())+"]":"")  
        + (node->in_scope?"{"+node->in_scope->name+"}":"");
        return to_return;
    }

    #define MUTE_TABLES 0
    #define MUTE_MUTED_QUALS 0

    std::string node_to_string(g_ptr<Node> node, int depth = 0, int index = 0, bool print_sub_scopes = false) {
        std::string indent(depth * 2, ' ');
        std::string to_return = "";

        to_return += indent + std::to_string(index) + ": " + node_info(node);
        
        if(!node->quals.empty()) {
            to_return += " [";
            for(int i=0;i<node->quals.length();i++) {
                #if MUTE_MUTED_QUALS
                if(!node->quals[i]->mute)
                    to_return += labels[node->quals[i]->type]+(i!=node->quals.length()-1?", ":"");
                #else
                    to_return += labels[node->quals[i]->type]+(i!=node->quals.length()-1?", ":"");
                #endif
            }
            to_return += "]";
        }

        #if !MUTE_TABLES 
        if(node->value_table.size()>0) {
            to_return += "\n" + indent + "   Value table:";
            for(auto [key,val] : node->value_table.entrySet()) {
                to_return += "\n" + indent + "     Key: "+key+" | "+value_info(val);
            }
        }

        if(node->node_table.size()>0) {
            to_return += "\n" + indent + "   Node table:";
            for(auto [key,val] : node->node_table.entrySet()) {
                to_return += "\n" + indent + "     Key: "+key+" | "+node_info(val);
            }
        }
        #endif
    


        if(!node->opt_str.empty()) {
            to_return +=  "\n" + indent + "  Opt_str: " + node->opt_str;
        }

        if(!node->scopes.empty()) {
            to_return +=  "\n" + indent + "   Scopes: " + std::to_string(node->scopes.size());
            int i = 0;
            for(auto& scope : node->scopes) {
                to_return += "\n" + indent; 
                if(scope) {
                    if(print_sub_scopes) {
                        to_return += node_to_string(scope, depth + 3,index,print_sub_scopes);
                    }
                    else {
                        to_return += "    " + scope->name;
                    }
                } else {
                    to_return += "[NULL]";
                }
            }
        }
        if(node->owner) {
            to_return +=  "\n" + indent + "  Owner: " + node_info(node->owner);
        }

        if(!node->children.empty()) {
            //to_return += " ["+std::to_string(children.length())+"]";
            for(int i=0;i<node->children.length();i++) {
                if(node->children[i]) {
                    to_return += "\n " + node_to_string(node->children[i], depth + 1, i, print_sub_scopes);
                }
                else {
                    to_return += "\n" + indent + "[NULL]";
                }
            }
        }
    
        return to_return;
    }

    g_ptr<Node> copy_as_token(g_ptr<Node> node) {
        g_ptr<Node> copy = make<Node>(node->type);
        copy->name = node->name;
        copy->x = node->x;
        copy->y = node->y;
        copy->z = node->z;
        copy->mute = true;
        for(auto q : node->quals) {
            if(q->mute)  copy->quals << q;
        }
        return copy;
    }


    inline g_ptr<Node> test_gdsl_unit_get_nodenet() {
        g_ptr<Node> root = make<Node>();

        return root;
    } 

    inline void test_gdsl_unit_run(g_ptr<Node> root) {
        
    }


    size_t next_id = 0;
    size_t reg_id(std::string label) {
        size_t id = next_id;
        labels[id] = label;
        next_id++;
        return id;
    }

    size_t undefined_id = reg_id("UNDEFINED"); //So that it's always 0

    struct Stage : Object {
        Stage() {
            default_function = [](Context& ctx){};
        }

        map<uint32_t,Handler> handlers;
        Handler default_function;

        std::string label;

        bool has(uint32_t key){
            return handlers.hasKey(key);
        }

        Handler& run(uint32_t key){
            return handlers.getOrDefault(key,default_function);
        }

        Handler& getOrDefault(uint32_t key, Handler& fallback){
            return handlers.getOrDefault(key,fallback);
        }

        Handler& operator[](uint32_t key) {
            return handlers[key];
        }
    };

    map<std::string,g_ptr<Stage>> stages;

    Stage& reg_stage(std::string label) {
        g_ptr<Stage> new_stage = make<Stage>();
        new_stage->label = label;
        stages.put(label,new_stage);
        return *new_stage.getPtr();
    }

    Stage& a_handlers = reg_stage("assembling");
    Stage& s_handlers = reg_stage("scoping");
    Stage& t_handlers = reg_stage("typing");

    Stage& d_handlers = reg_stage("discovering");
    Stage& r_handlers = reg_stage("resolving");
    Stage& e_handlers = reg_stage("evaluating");

    Stage& m_handlers = reg_stage("modeling");
    Stage& i_handlers = reg_stage("inspecting");
    Stage& x_handlers = reg_stage("executing");

    Stage* active_stage;

    void start_stage(Stage& stage) {
        active_stage = &stage;
    }

    void start_stage(g_ptr<Stage> stage_ptr) {
        start_stage(*stage_ptr.getPtr());
    }

    void fire_quals(Context& ctx, g_ptr<Value> value) {
        g_ptr<Value> saved_value = ctx.value;
        ctx.value = value;
        for(auto qual : value->quals) {
            if(qual->mute) continue;
            ctx.qual = qual;
            if(active_stage->has(qual->type+1))
                (*active_stage)[qual->type+1](ctx);
        }
        ctx.value = saved_value;
    }
    void fire_quals(Context& ctx, g_ptr<Node> node) {
        g_ptr<Node> saved_node = ctx.node;
        ctx.node = node;
        for(auto qual : node->quals) {
            if(qual->mute) continue;
            ctx.qual = qual;
            if(active_stage->has(qual->type+2))
                (*active_stage)[qual->type+2](ctx);
        }
        ctx.node = saved_node;
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


    virtual void init() {

    }

    virtual g_ptr<Node> process(const std::string& path) {
        return nullptr;
    }

    virtual void run(g_ptr<Node> root) {
        
    }

    g_ptr<Node> scan_for_node_via_node_table(const std::string& label, g_ptr<Node> from) {
        if(from->node_table.hasKey(label)) {
            return from->node_table.get(label);
        } else {
            for(auto scope : from->scopes) {
                g_ptr<Node> found = scan_for_node_via_node_table(label,scope);
                if(found) {
                    return found;
                }
            }
            return nullptr;
        }
    }

    g_ptr<Node> scan_for_node(const std::string& label, g_ptr<Node> from) {
            for(auto c : from->children) {
                if(c->name==label) {
                    return c;
                }
            }
            for(auto scope : from->scopes) {
                g_ptr<Node> found = scan_for_node(label,scope);
                if(found) {
                    return found;
                }
            }
            return nullptr;
    }

    g_ptr<Node> find_scope(g_ptr<Node> start, std::function<bool(g_ptr<Node>)> check) {
        g_ptr<Node> on_scope = start;
        while(true) {
            if(check(on_scope)) {
                return on_scope;
            }
            for(auto c : on_scope->scopes) {
                if(c==start||c==on_scope) continue;
                if(check(c)) {
                return c;
                }
            }
            if(on_scope->parent) {
                on_scope = on_scope->parent;
            }
            else 
                break;
        }
        return nullptr;
    }

    g_ptr<Node> find_scope_name(const std::string& match,g_ptr<Node> start) {
        return find_scope(start,[match](g_ptr<Node> c){
            return c->name == match;
        });
    }

    void discover_symbol(g_ptr<Node> node, g_ptr<Node> root) {
        Context ctx;
        ctx.root = root;
        ctx.node = node;
        newline("Discovering: "+node_info(node));
        d_handlers.run(node->type)(ctx);
        endline();
    }

    void discover_symbols(g_ptr<Node> root) {
        newline("Discover symbols pass over "+std::to_string(root->children.size())+" nodes");   
        for(int i = 0; i < root->children.size(); i++) {
            discover_symbol(root->children[i], root);
        }
        
        for(auto child_scope : root->scopes) {
            discover_symbols(child_scope);
        }
        endline();
    }


    inline g_ptr<Node> standard_process(Context& ctx) {
        newline(active_stage->label+": "+node_info(ctx.node));
        active_stage->run(ctx.node->type)(ctx);
        endline();
        return ctx.node;
    }

    inline void standard_process(Context& ctx, size_t type) {
        active_stage->run(type)(ctx);
    }

    g_ptr<Node> process_node(Context& ctx, g_ptr<Node> node, g_ptr<Node> left = nullptr) {
        g_ptr<Node> saved_node = ctx.node;
        g_ptr<Node> saved_left = ctx.left;
        Context* saved_sub = ctx.sub;
        ctx.node = node;
        ctx.left = left;
        g_ptr<Node> to_return = standard_process(ctx);
        ctx.node = saved_node;
        ctx.left = saved_left;
        ctx.sub = saved_sub;
        return to_return;
    }

    g_ptr<Node> process_node(g_ptr<Node> node, g_ptr<Node> left = nullptr) {
        Context ctx;
        return process_node(ctx,node,left);
    }

    void standard_sub_process_node(g_ptr<Node> root) {
        Context ctx;
        ctx.node = root;
        standard_sub_process(ctx);
    }

    void standard_sub_process(Context& ctx) {
        int i = 0;
        Context sub_ctx(ctx.node->children,i);
        sub_ctx.root = ctx.node;
        sub_ctx.sub = ctx.sub;
        g_ptr<Node> left = nullptr;
        while(i < ctx.node->children.length()) {
            process_node(sub_ctx, ctx.node->children[i], left);
            left = ctx.node->children[i];
            i++;
        }
        ctx.flag = sub_ctx.flag;
    }

    void backwards_sub_process(Context& ctx) { 
        int i = ctx.node->children.length()-1;
        Context sub_ctx(ctx.node->children,i);
        sub_ctx.root = ctx.node;
        sub_ctx.sub = ctx.sub;
        g_ptr<Node> left = nullptr;
        while(i >= 0) {
            process_node(sub_ctx, ctx.node->children[i], left);
            left = ctx.node->children[i];
            i--;
        }
        ctx.flag = sub_ctx.flag;
    }

    void standard_resolving_pass(g_ptr<Node> root) {
        newline("Resolving pass over "+std::to_string(root->children.size())+" nodes");
        int i = 0;
        Context ctx(root->children,i);
        ctx.root = root;
        while(i < root->children.size()) {
            if(root->children[i]->scope()) {
                ctx.node = root->children[i];
                standard_process(ctx);
                ctx.left = root->children[i];
            }
            i++;
        }
        i = 0;
        while(i < root->children.size()) {
            if(!root->children[i]->scope()) {
                ctx.node = root->children[i];
                standard_process(ctx);
                ctx.left = root->children[i];
            }
            i++;
        }
        i = 0;
        while(i < root->children.size()) {
            if(root->children[i]->scope()) {
                standard_sub_process_node(root->children[i]);
            }
            i++;
        }
        for(auto child_scope : root->scopes) {
            standard_resolving_pass(child_scope);
        }
        endline();
    }

    void standard_direct_pass(g_ptr<Node> root) {
        newline("Direct pass over "+std::to_string(root->children.size())+" nodes");
        int i = 0;
        Context ctx(root->children,i);
        ctx.root = root;
        while(i < ctx.result->size()) {
            ctx.node = ctx.result->get(i);

            // log("\n");
            // for(int c=0;c<ctx.result->length();c++) {
            //     log((c==i?"-> ":"   "),c,":\n",node_to_string(ctx.result->get(c)));
            // }

            standard_process(ctx);
            ctx.left = ctx.result->get(i);
            i++;
        }

        for(auto scope : root->scopes) {
            standard_direct_pass(scope);
        }
        endline();
    }

    //Returns true if flagged for a return/break
    bool standard_travel_pass(g_ptr<Node> root, Context* sub = nullptr) {
        newline("Travel pass over "+std::to_string(root->children.size())+" nodes");
        int i = 0;
        Context ctx(root->children, i);
        ctx.root = root;
        if(sub) ctx.sub = sub;
        while(i < root->children.size()) {
            ctx.node = root->children[i];
            standard_process(ctx);
            ctx.left = root->children[i];
            if(ctx.flag) { //This is the return/break process
                endline();
                return true;
            }
            i++;
        }
        endline();
        return false;
    }

    void standard_backwards_pass(g_ptr<Node> root){
        newline("Backwards pass over "+std::to_string(root->children.size())+" nodes");
        Context ctx;
        ctx.root = root;
        ctx.result = &root->children;
        for(int i=root->children.length()-1;i>=0;i--) {
            ctx.index = i;
            ctx.node = root->children[i];
            standard_process(ctx);
            if(!ctx.node) {
                root->children.removeAt(i);
            } else {
                for(auto scope : root->children[i]->scopes) {
                    if(scope->owner&&scope->owner==root->children[i]) {
                        standard_backwards_pass(scope);
                    }
                }
                ctx.left = root->children[i];
            }
        }
        endline();
    }

    void memory_backwards_pass(g_ptr<Node> root){
        newline("Memory pass over "+std::to_string(root->children.size())+" nodes");
        Context ctx;
        ctx.root = root;
        ctx.result = &root->children;

        for(int i=root->children.length()-1;i>=0;i--) {
            ctx.node = root->children[i];
            for(auto scope : root->children[i]->scopes) {
                if(scope->owner==root->children[i])
                    memory_backwards_pass(scope);
            }
            standard_process(ctx);
            ctx.left = root->children[i];
        }
        endline();
    }

    list<g_ptr<Node>> node_buffer;
    list<g_ptr<Value>> value_buffer;
    list<g_ptr<Type>> type_buffer;

    void serialize_node(g_ptr<Node> node) {
        if(node->save_idx==-1) {
            node->save_idx = node_buffer.length();
            node_buffer << node;

            if(node->value) serialize_value(node->value);

            for(auto e : node->value_table.entrySet()) {
                serialize_value(e.value);
            }

            for(auto q : node->quals) {
                serialize_node(q);
            }

            list<g_ptr<Node>> to_serialize; 
            to_serialize << node->children;
            to_serialize << node->scopes;
            for(auto s: to_serialize) {
                serialize_node(s);
            }
        }
    }

    void serialize_type(g_ptr<Type> type) {
        if(type->save_idx == -1) {
            type->save_idx = type_buffer.length();
            type_buffer << type;
        }
    }

    void serialize_value(g_ptr<Value> value) {
        if(value->save_idx == -1) {
            value->save_idx = value_buffer.length();
            value_buffer << value;
            
            for(auto q : value->quals) {
                serialize_node(q);
            }
            for(auto v : value->sub_values) {
                serialize_value(v);
            }

            if(value->type_scope) serialize_node(value->type_scope);
            if(value->store) serialize_type(value->store);
        }
    }

    void write_value(g_ptr<Value> value, std::ostream& out) {
        write_raw(out, value->type);
        write_raw(out, value->sub_type);
        write_raw(out, value->size);
        write_raw(out, value->sub_size);
        write_raw(out, value->reg);
        write_raw(out, value->address);
        write_raw(out, value->loc);

        bool has_data = value->data != nullptr;
        write_raw(out, has_data);
        if(has_data) {
            out.write((const char*)value->data, value->size);
        }

        write_raw<uint32_t>(out, value->quals.length());
        for(auto q : value->quals) {
            write_raw<int>(out, q->save_idx);
        }

        write_raw<uint32_t>(out, value->sub_values.length());
        for(auto v : value->sub_values) {
            write_raw<int>(out, v->save_idx);
        }

        write_raw<int>(out, value->type_scope ? value->type_scope->save_idx : -1);

        write_raw<int>(out, value->store ? value->store->save_idx : -1);
    }

    void write_node(g_ptr<Node> node, std::ostream& out) {
        write_raw(out, node->type);
        write_string(out, node->name);
        write_raw<float>(out, node->x);
        write_raw<float>(out, node->y);
        write_raw<float>(out, node->z);
        write_string(out, node->opt_str);
        write_raw(out, node->is_scope);

        write_raw<int>(out, node->value ? node->value->save_idx : -1);

        write_raw<uint32_t>(out, node->children.length());
        for(auto c : node->children) {
            write_raw<int>(out, c->save_idx);
        }

        write_raw<uint32_t>(out, node->scopes.length());
        for(auto s : node->scopes) {
            write_raw<int>(out, s->save_idx);
        }

        write_raw<uint32_t>(out, node->quals.length());
        for(auto q : node->quals) {
            write_raw<int>(out, q->save_idx);
        }

        write_raw<uint32_t>(out, node->value_table.size());
        for(auto e : node->value_table.entrySet()) {
            write_string(out, e.key);
            write_raw<int>(out, e.value ? e.value->save_idx : -1);
        }

        write_raw<uint32_t>(out, node->node_table.size());
        for(auto e : node->node_table.entrySet()) {
            write_string(out, e.key);
            write_raw<int>(out, e.value ? e.value->save_idx : -1);
        }

        write_raw<int>(out, node->parent ? node->parent->save_idx : -1);
        write_raw<int>(out, node->owner ? node->owner->save_idx : -1);
        write_raw<int>(out, node->in_scope ? node->in_scope->save_idx : -1);
    }

    void read_value(g_ptr<Value> value, std::istream& in, map<uint32_t,uint32_t>& id_remap) {
        value->type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        value->sub_type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        value->size = read_raw<size_t>(in);
        value->sub_size = read_raw<int>(in);
        value->reg = read_raw<int>(in);
        value->address = read_raw<int>(in);
        value->loc = read_raw<int>(in);

        bool has_data = read_raw<bool>(in);
        if(has_data) {
            value->data = malloc(value->size);
            in.read((char*)value->data, value->size);
        }

        uint32_t qual_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < qual_count; i++) {
            int idx = read_raw<int>(in);
            if(idx != -1) value->quals << node_buffer[idx];
        }

        uint32_t sub_value_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < sub_value_count; i++) {
            int idx = read_raw<int>(in);
            if(idx >= 0) value->sub_values << value_buffer[idx];
        }

        int type_scope_idx = read_raw<int>(in);
        value->type_scope = type_scope_idx != -1 ? node_buffer[type_scope_idx].getPtr() : nullptr;

        int store_idx = read_raw<int>(in);
        value->store = store_idx != -1 ? type_buffer[store_idx] : nullptr;
    }

    void read_node(g_ptr<Node> node, std::istream& in, map<uint32_t,uint32_t>& id_remap) {
        node->type = id_remap.getOrDefault(read_raw<uint32_t>(in), (unsigned int)0);
        node->name = read_string(in);
        node->x = read_raw<float>(in);
        node->y = read_raw<float>(in);
        node->z = read_raw<float>(in);
        node->opt_str = read_string(in);
        node->is_scope = read_raw<bool>(in);

        int value_idx = read_raw<int>(in);
        node->value = value_idx != -1 ? value_buffer[value_idx] : nullptr;

        uint32_t child_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < child_count; i++) {
            int idx = read_raw<int>(in);
            if(idx != -1) node->children << node_buffer[idx];
        }

        uint32_t scope_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < scope_count; i++) {
            int idx = read_raw<int>(in);
            if(idx != -1) node->scopes << node_buffer[idx];
        }

        uint32_t qual_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < qual_count; i++) {
            int idx = read_raw<int>(in);
            if(idx != -1) node->quals << node_buffer[idx];
        }

        uint32_t value_table_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < value_table_count; i++) {
            std::string key = read_string(in);
            int idx = read_raw<int>(in);
            if(idx != -1) node->value_table.put(key, value_buffer[idx]);
        }

        uint32_t node_table_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < node_table_count; i++) {
            std::string key = read_string(in);
            int idx = read_raw<int>(in);
            if(idx != -1) node->node_table.put(key, node_buffer[idx]);
        }

        int parent_idx = read_raw<int>(in);
        node->parent = parent_idx != -1 ? node_buffer[parent_idx].getPtr() : nullptr;

        int owner_idx = read_raw<int>(in);
        node->owner = owner_idx != -1 ? node_buffer[owner_idx].getPtr() : nullptr;

        int in_scope_idx = read_raw<int>(in);
        node->in_scope = in_scope_idx != -1 ? node_buffer[in_scope_idx].getPtr() : nullptr;
    }

    void serialize(g_ptr<Node> root) {
        node_buffer.clear();
        value_buffer.clear();
        serialize_node(root);
    }

    void saveBinary(std::ostream& out) {
        write_raw<uint32_t>(out, labels.size());
        for(auto& e : labels.entrySet()) {
            write_raw<uint32_t>(out, e.key);
            write_string(out, e.value);
        }

        write_raw<uint32_t>(out, type_buffer.length());
        write_raw<uint32_t>(out, value_buffer.length());
        write_raw<uint32_t>(out, node_buffer.length());
        write_raw<uint32_t>(out, types.length());

        for(auto t : type_buffer) write_type(t, out);
        for(auto v : value_buffer) write_value(v, out);
        for(auto n : node_buffer) write_node(n, out);
        for(auto t2 : types) write_type(t2, out);
    }

    g_ptr<Node> loadBinary(std::istream& in) {
        type_buffer.clear();
        node_buffer.clear();
        value_buffer.clear();
        types.clear();

        //Remap the ids into a map for ease of acess 
        map<uint32_t, uint32_t> id_remap;
        uint32_t label_count = read_raw<uint32_t>(in);
        for(uint32_t i = 0; i < label_count; i++) {
            uint32_t saved_id = read_raw<uint32_t>(in);
            std::string str = read_string(in);
            bool already_exists = false;
            for(auto e : labels.entrySet()) {
                if(e.value == str) { 
                    id_remap.put(saved_id, e.key); 
                    already_exists = true;
                    break; 
                }
            }
            if(!already_exists) {
                id_remap.put(saved_id,reg_id(str));
            }
        }

        uint32_t type_count = read_raw<uint32_t>(in);
        uint32_t value_count = read_raw<uint32_t>(in);
        uint32_t node_count = read_raw<uint32_t>(in);
        uint32_t types_count = read_raw<uint32_t>(in);

        //Pre allocate
        for(uint32_t i = 0; i < type_count; i++) {
            auto t = make<Type>();
            t->save_idx = i;
            type_buffer << t;
        }
        for(uint32_t i = 0; i < value_count; i++) {
            auto v = make<Value>();
            v->save_idx = i;
            value_buffer << v;
        }
        for(uint32_t i = 0; i < node_count; i++) {
            auto n = make<Node>();
            n->save_idx = i;
            node_buffer << n;
        }
        for(uint32_t i = 0; i < types_count; i++) {
            auto t = make<Type>();
            t->save_idx = i;
            types << t;
        }
        //Annotate
        for(uint32_t i = 0; i < type_count; i++) read_type(type_buffer[i], in);
        for(uint32_t i = 0; i < value_count; i++) read_value(value_buffer[i], in, id_remap);
        for(uint32_t i = 0; i < node_count; i++) read_node(node_buffer[i], in, id_remap);
        for(uint32_t i = 0; i < types_count; i++) read_type(types[i], in);

        return node_buffer.empty() ? nullptr : node_buffer[0];
    }

    void saveBinary(const std::string& path) {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: " + path);
        saveBinary(out);
        out.close();
    }

    g_ptr<Node> loadBinary(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read from file: " + path);
        g_ptr<Node> to_return = loadBinary(in);
        in.close();
        return to_return;
    }

};




}
