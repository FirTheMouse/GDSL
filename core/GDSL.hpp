#pragma once

#include "../ext/g_lib/core/type.hpp"
#include <sys/mman.h>

namespace GDSL {

//Controls for the compiler printing, for debugging
#define PRINT_ALL 0
#define PRINT_STYLE 0

//GDSL, Golden Dynamic Systems Language
//TAST: tokenizer, a stage, scope stage, t stage.
//DRE: Discover, resolve, error
//MIX: Model, interpret, execute

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

#ifndef MAX_TYPES
    #define MAX_TYPES 1024
#endif

#ifndef HANDLER_COUNT
    #define HANDLER_COUNT 3
#endif

map<uint32_t,std::string> labels;

static std::string fallback = "[undefined]";

//Used for debugging, and for printing 
map<uint32_t,std::function<std::string(void*)>> value_to_string;
map<uint32_t, std::function<void(void*)>> negate_value;

static std::string data_to_string(uint32_t type,void* data) {    
    try {
        return value_to_string.get(type)(data);
    }
    catch(std::exception e) {
        return "[missing value_to_string for type "+labels[type]+"]";
    }
}

class Node;
class Value;

struct Qual {
    Qual() {}
    Qual(size_t _type) : type(_type) {}
    Qual(size_t _type, g_ptr<Value> _value) : type(_type), value(_value) {}
    Qual(size_t _type, size_t _sub_type) : type(_type), sub_type(_sub_type) {}
    Qual(size_t _type, size_t _sub_type, g_ptr<Value> _value) : type(_type), sub_type(_sub_type), value(_value) {}
    Qual(size_t _type, bool _mute) : type(_type), mute(_mute) {}

    uint32_t type = 0;
    uint32_t sub_type = 0;
    Qual* parent = nullptr;
    g_ptr<Value> value = nullptr;
    bool mute = false;
};

static int _ctx_dummy_index = 0;

struct Context {
    Context() : index(_ctx_dummy_index) {}
    Context(list<g_ptr<Node>>& _result, int& _index) : result(&_result), index(_index) {}

    g_ptr<Node> node;
    g_ptr<Node> left;
    g_ptr<Node> out;
    g_ptr<Node> root;
    list<g_ptr<Node>>* result = nullptr;
    list<g_ptr<Node>> nodes;
    Qual qual;
    g_ptr<Value> value;
    int& index;
    uint32_t state = 0;
    bool flag = false;

    std::string source;

    Context sub() {
        return Context((*result), index);
    }
};



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
    size_t size = 0;
    int sub_size = 0;
    list<Qual> quals;
    list<g_ptr<Value>> sub_values;
    Node* type_scope = nullptr;
    g_ptr<Type> store = nullptr;

    void copy(g_ptr<Value> o, bool is_deep = false) {
        type = o->type; 
        sub_type = o->sub_type; 
        address = o->address; 
        size = o->size; 
        sub_size = o->sub_size;
        type_scope = o->type_scope;
        store = o->store;
        if(is_deep) {
            for(auto oq : o->quals) {
                Qual cpy;
                cpy.type = oq.type;
                cpy.parent = nullptr; //Parent is positional, doesn't survive a deep copy
                if(oq.value) {
                    cpy.value = make<Value>();
                    cpy.value->copy(oq.value, true);
                }
                quals << cpy;
            }
        } else {
            quals = o->quals;
        }

        if(o->data) {
            data = malloc(size);
            memcpy(data,o->data,size);
        }
    }

    Qual to_qual() {
        Qual to_return;
        to_return.value = this;
        to_return.type = type;
        to_return.sub_type = sub_type;
        return to_return;
    }

    int find_qual(size_t q_id) {
        for(int i=0;i<quals.length();i++){ 
            if(quals[i].type==q_id) {return i;}
        }
        return -1;
    } 

    bool has_qual(size_t q_id) {
        return find_qual(q_id)!=-1;
    }

    template<typename T>
    T get() { return *(T*)data; }
    
    template<typename T>
    void set(const T& value) { 
        if (!data) {
            data = malloc(sizeof(T));
            size = sizeof(T);
        }
        new(data) T(value);
    }

    std::string to_string() {
        if (!data) {
            return "[null]";
        }
        std::function<std::string(void*)> fallback_func = [this](void* ptr){return "[missing value_to_string for type "+labels[type]+"]";};
        return value_to_string.getOrDefault(type,fallback_func)(data);
    }

    std::string info() {
        std::string to_return = "";
        to_return += "Value [@" + std::to_string((size_t)(void*)this) + "]"
        + "(type: " + labels[type]
        + (data?", value: "+to_string():"")
        + (sub_type!=0?", sub_type: "+labels[sub_type]:"")
        + (type_scope?", type_scope: [yes]":"")
        + (size!=0?", size: "+std::to_string(size):"")
        + (address!=0?", address: "+std::to_string(address):"")
        + (reg!=-1?", reg: "+std::to_string(reg):"")
        + (store?", store: "+store->type_name:"")
        + (!sub_values.empty()?", sub: "+std::to_string(sub_values.length()):"");
        if(!quals.empty()) {
            to_return += ", Quals: ";
            for(int i=0;i<quals.length();i++) { //+"[@" + std::to_string((size_t)(void*)quals[i].value.getPtr()) + "]"
                to_return += labels[quals[i].type]+(i!=quals.length()-1?", ":"");
            }
        }
        to_return += ")";
        return to_return;
    }

    void negate() {
        if(data) {
            try {
                negate_value.get(type)(data);
            }
            catch(std::exception e) {
                print("value::300 missing negate_value handler for ",labels[type]);
            }
        }
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
    Node(uint32_t _type, char c) : type(_type) {
        name += c;
        value = make<Value>();
    }

    uint32_t type = 0;
    std::string name = "";
    g_ptr<Value> value = nullptr;
    list<g_ptr<Node>> children;

    list<Qual> quals;

    list<g_ptr<Node>> scopes;
    Node* parent = nullptr;
    Node* owner = nullptr;
    Node* in_scope = nullptr;


    const inline g_ptr<Node> left() { 
        return children.length() > 0 ? children[0] : nullptr; 
    }
    const inline g_ptr<Node> right() { 
        return children.length() > 1 ? children[1] : nullptr; 
    }

    map<std::string,g_ptr<Value>> value_table;
    bool find_value_in_scope() {
        if(in_scope->value_table.hasKey(name)) {
            value = in_scope->value_table.get(name);
            return true;
        }
        return false;
    }

    map<std::string,g_ptr<Node>> node_table;
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

    void copy(g_ptr<Node> o) {
        type = o->type;
        name = o->name;
        value = o->value;
        children = o->children;
        quals = o->quals;
        scopes = o->scopes;
        parent = o->parent;
        owner = o->owner;
        in_scope = o->in_scope;
        value_table = o->value_table;
        node_table = o->node_table;
        opt_str = o->opt_str;
    }

    g_ptr<Node> scope() { return scopes.empty() ? nullptr : scopes[0]; }
    uint32_t getType() {return type;}
    void setType(uint32_t _type) {type = _type;}

    std::string addr_str() {return std::to_string((size_t)(void*)this);}

    int find_qual(size_t q_id) {
        for(int i=0;i<quals.length();i++){ 
            if(quals[i].type==q_id) {return i;}
        }
        return -1;
    } 

    int find_qual_in_value(size_t q_id) {
        if(value)
            return value->find_qual(q_id);
        return -1;
    }
    
    bool has_qual(size_t q_id, bool check_value = true) {
        for(auto q : quals){ if(q.type==q_id) {return true;}}
        if(value&&check_value) {for(auto q : value->quals){if(q.type==q_id){return true;}}}
        return false;
    }

    void distribute_qual_to_values(Qual& qual,size_t exclude_id = 0) {
        if(!has_qual(exclude_id))
            value->quals << qual;
        for(auto c : children) {
            c->distribute_qual_to_values(qual,exclude_id);
        }
    }

    g_ptr<Node> spawn_sub_scope() {
        g_ptr<Node> new_node = make<Node>();
        new_node->parent = this;
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

    std::string opt_str;

    std::string info() {
        std::string to_return = "";
        
        to_return += labels[type] + " [Name: " + name;
        if(value) {
            to_return += ", "+value->info();
        }
        to_return += "]";
        return to_return;
    }

    std::string to_string(int depth = 0, int index = 0, bool print_sub_scopes = false) {
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + std::to_string(index) + ": " + labels[type]
        +(!name.empty()?green(" "+name):"") 
        +(in_scope?" {"+in_scope->name+"}":"");
        
        if(!quals.empty()) {
            to_return += " [";
            for(int i=0;i<quals.length();i++) {
                to_return += labels[quals[i].type]+(i!=quals.length()-1?", ":"");
            }
            to_return += "]";
        }


        if(value) {
            to_return += "\n" + indent + "   "+value->info();
        }


        if(value_table.size()>0) {
            to_return += "\n" + indent + "   Value table:";
            for(auto [key,val] : value_table.entrySet()) {
                to_return += "\n" + indent + "     Key: "+key+" | "+val->info();
            }
        }

        if(node_table.size()>0) {
            to_return += "\n" + indent + "   Node table:";
            for(auto [key,val] : node_table.entrySet()) {
                to_return += "\n" + indent + "     Key: "+key+" | "+val->info();
            }
        }
    


        if(!opt_str.empty()) {
            to_return +=  "\n" + indent + "  Opt_str: " + opt_str;
        }

        if(!scopes.empty()) {
            to_return +=  "\n" + indent + "   Scopes: " + std::to_string(scopes.size());
            int i = 0;
            for(auto& scope : scopes) {
                to_return += "\n" + indent; 
                if(scope) {
                    if(print_sub_scopes) {
                        to_return += scope->to_string(depth + 3,index,print_sub_scopes);
                    }
                    else {
                        to_return += "    " + scope->name;
                    }
                } else {
                    to_return += "[NULL]";
                }
            }
        }
        if(owner) {
            to_return +=  "\n" + indent + "  Owner: " + owner->name;
        }

        if(!children.empty()) {
            //to_return += " ["+std::to_string(children.length())+"]";
            for(int i=0;i<children.length();i++) {
                if(children[i])
                    to_return += "\n " + children[i]->to_string(depth + 1, i, print_sub_scopes);
                else 
                    to_return += "\n" + indent + "[NULL]";
            }
        }
    
        return to_return;
    }
};


using Handler = std::function<void(Context&)>;

map<char, Handler> tokenizer_functions;
map<uint32_t, Handler> tokenizer_state_functions;
Handler tokenizer_default_function = nullptr;

//TAST
map<uint32_t,Handler> a_handlers; Handler a_parse_function;
map<uint32_t,Handler> s_handlers; Handler s_default_function;
map<uint32_t,Handler> t_handlers; Handler t_default_function;
//DRE
map<uint32_t,Handler> d_handlers; Handler d_default_function;
map<uint32_t,Handler> r_handlers; Handler r_default_function;
map<uint32_t,Handler> e_handlers; Handler e_default_function;
//MIX
map<uint32_t,Handler> m_handlers; Handler m_default_function;
map<uint32_t,Handler> i_handlers; Handler i_default_function;
map<uint32_t,Handler> x_handlers; Handler x_default_function;

map<uint32_t, std::function<void(Context&)>>* active_handlers = nullptr;
Handler active_default_function = nullptr;

size_t next_id = 0;
size_t reg_id(std::string label) {
    size_t id = next_id;
    labels[id] = label;
    next_id++;
    return id;
}

size_t undefined_id = reg_id("UNDEFINED"); //So that it's always 0

void start_stage(map<uint32_t,Handler>* handlers, Handler default_function) {
    active_handlers = handlers;
    active_default_function = default_function;
}

static void fire_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) {
        if(qual.mute) continue;
        ctx.qual = qual;
        if((*active_handlers).hasKey(qual.type+1))
            (*active_handlers)[qual.type+1](ctx);
    }
}
static void fire_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) {
        if(qual.mute) continue;
        ctx.qual = qual;
        if((*active_handlers).hasKey(qual.type+2))
            (*active_handlers)[qual.type+2](ctx);
    }
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



static list<g_ptr<Node>> tokenize(const std::string& code) {
    list<g_ptr<Node>> result;
    uint32_t state = 0;
    int index = 0;
    Context ctx(result,index);
    ctx.source = code;

    #if PRINT_ALL
    newline("tokenize pass");
    #endif

    if(!tokenizer_default_function) {
        print("GDSL::tokenize warning! No defined default function, please define one");
    }

    while (index<code.length()) {
        char c = code.at(index);
        if(ctx.state!=0&&tokenizer_state_functions.hasKey(ctx.state)) {
            auto state_func = tokenizer_state_functions.get(ctx.state);
            state_func(ctx);
        } else {
            auto func = tokenizer_functions.getOrDefault(c,tokenizer_default_function);
            func(ctx);
        }
        ++index;
    }  

    #if PRINT_ALL
    int i = 0;
    for(auto t : result) {
        if(t->getType()) {
            log(i++," ",labels[t->getType()],": ",t->name);
        }
    }
    endline();
    #endif

    return result;
}

//Doesn't advance it's own index so be wary of infinite recursion with a bad a_parse_function
static list<g_ptr<Node>> parse_tokens(list<g_ptr<Node>> tokens,bool local = false) {
        newline("Parse tokens pass (A) over: "+std::to_string(tokens.length())+" tokens");
        
        if(!a_parse_function)
            print("GDSL::parse_tokens a_stage requires a parsing function!");

        list<g_ptr<Node>> result;
        int index = 0;
        
        while(index < tokens.length()) {
            if(!tokens[index]) break;
            Context ctx(result, index);
            ctx.nodes = tokens;
            
            newline("Parsing (A): "+tokens[index]->info());
            a_parse_function(ctx);
            endline();
            //index++;
        }

        log("==A_NODES==");
        int i = 0;
        for (auto& node : result) {
            log(node->to_string(0,i++));
        }

        endline();
        return result;
}



static g_ptr<Node> find_scope(g_ptr<Node> start, std::function<bool(g_ptr<Node>)> check) {
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

static g_ptr<Node> find_scope_name(const std::string& match,g_ptr<Node> start) {
    return find_scope(start,[match](g_ptr<Node> c){
        return c->name == match;
    });
}

static void discover_symbol(g_ptr<Node> node, g_ptr<Node> root) {
    Context ctx;
    ctx.root = root;
    ctx.node = node;
    newline("Discovering: "+node->info());
    (*active_handlers).getOrDefault(node->type,active_default_function)(ctx);
    endline();
}

static void discover_symbols(g_ptr<Node> root) {
    newline("Discover symbols pass over "+std::to_string(root->children.size())+" nodes");   
    for(int i = 0; i < root->children.size(); i++) {
        discover_symbol(root->children[i], root);
    }
    
    for(auto child_scope : root->scopes) {
        discover_symbols(child_scope);
    }
    endline();
}

list<uint32_t> emit_buffer;

static g_ptr<Node> standard_process(Context& ctx) {
    //log("Processing: ",ctx.node->info());
    (*active_handlers).getOrDefault(ctx.node->type,active_default_function)(ctx);
    print("Emmited at ",emit_buffer.length()-1,": ",ctx.node->info());
    return ctx.node;
}

static g_ptr<Node> process_node(Context& ctx, g_ptr<Node> node, g_ptr<Node> left = nullptr) {
    g_ptr<Node> saved_node = ctx.node;
    g_ptr<Node> saved_left = ctx.left;
    ctx.node = node;
    ctx.left = left;
    g_ptr<Node> to_return = standard_process(ctx);
    ctx.node = saved_node;
    ctx.left = saved_left;
    return to_return;
}



static void standard_sub_process(Context& ctx) {
    for(int i = 0; i < ctx.node->children.length(); i++) {
        process_node(ctx, ctx.node->children[i]);
    }
}

static void backwards_sub_process(Context& ctx) {
    for(int i = ctx.node->children.length()-1; i >= 0; i--) {
        process_node(ctx, ctx.node->children[i]);
    }
}

static void standard_resolving_pass(g_ptr<Node> root) {
    newline("Standard pass over "+std::to_string(root->children.size())+" nodes");
    Context ctx;
    ctx.root = root;
    ctx.result = &root->children;
    for(int i = 0; i < root->children.size(); i++) {
        ctx.index = i;
        if(root->children[i]->scope()) {
            ctx.node = root->children[i];
            standard_process(ctx);
            ctx.left = root->children[i];
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        ctx.index = i;
        if(!root->children[i]->scope()) {
            ctx.node = root->children[i];
            standard_process(ctx);
            ctx.left = root->children[i];
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        ctx.index = i;
        if(root->children[i]->scope()) {
            ctx.result = &root->children[i]->children;
            for(auto c : root->children[i]->children) {
                ctx.node = c;
                standard_process(ctx);
                ctx.left = c;
            }
        }
    }
    for(auto child_scope : root->scopes) {
        standard_resolving_pass(child_scope);
    }
    endline();
}

static void standard_direct_pass(g_ptr<Node> root) {
    newline("Standard pass over "+std::to_string(root->children.size())+" nodes");
    Context ctx;
    ctx.root = root;
    ctx.result = &root->children;
    for(int i = 0; i < root->children.size(); i++) {
        ctx.index = i;
        ctx.node = root->children[i];
        standard_process(ctx);
        ctx.left = root->children[i];
    }

    for(auto scope : root->scopes) {
        standard_direct_pass(scope);
    }
    endline();
}

//Returns true if flagged for a return/break
static bool standard_travel_pass(g_ptr<Node> root) {
    newline("Standard pass over "+std::to_string(root->children.size())+" nodes");
    Context ctx;
    ctx.root = root;
    ctx.result = &root->children;
    for(int i = 0; i < root->children.size(); i++) {
        ctx.index = i;
        newline("Looking at: "+root->children[i]->info());
        ctx.node = root->children[i];
        standard_process(ctx);
        ctx.left = root->children[i];
        endline();
        if(ctx.flag) { //This is the return/break process
            endline();
            return true;
        }
    }
    endline();
    return false;
}

void standard_backwards_pass(g_ptr<Node> root){
    newline("Standard pass over "+std::to_string(root->children.size())+" nodes");
    Context ctx;
    ctx.root = root;
    ctx.result = &root->children;
    for(int i=root->children.length()-1;i>=0;i--) {
        ctx.index = i;
        newline("Looking at: "+root->children[i]->info());
        ctx.node = root->children[i];
        standard_process(ctx);
        if(!ctx.node) {
            root->children.removeAt(i);
        } else {
            for(auto scope : root->children[i]->scopes) {
                standard_backwards_pass(scope);
            }
            ctx.left = root->children[i];
        }
        endline();
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
            memory_backwards_pass(scope);
        }
        standard_process(ctx);
        ctx.left = root->children[i];
    }

    // for(int i=root->children.length()-1;i>=0;i--) {
    //     ctx.index = i;
    //     if(root->children[i]->scope()) {
    //         ctx.node = root->children[i];
    //         standard_process(ctx);
    //         ctx.left = root->children[i];
    //         for(auto scope : root->children[i]->scopes) {
    //             memory_backwards_pass(scope);
    //         } 
    //     }
    // }
    // for(int i=root->children.length()-1;i>=0;i--) {
    //     ctx.index = i;
    //     if(!root->children[i]->scope()) {
    //         ctx.node = root->children[i];
    //         standard_process(ctx);
    //         ctx.left = root->children[i];
    //     }
    // }
    endline();
}




list<list<uint32_t>> buffer_stack;
void push_buffer() {
    buffer_stack << emit_buffer;
    emit_buffer = {};
}
list<uint32_t> pop_buffer() {
    list<uint32_t> emitted = emit_buffer;
    if(!buffer_stack.empty()) {
        emit_buffer = buffer_stack.pop();
    }
    return emitted;
}

//AArch64 comparison codes
// ==
const int COND_EQ = 0b0000;  
// !=
const int COND_NE =  0b0001;
// >=
const int COND_GE =  0b1010;
// <
const int COND_LT =  0b1011; 
// >
const int COND_GT =  0b1100;   
// <=
const int COND_LE =  0b1101; 

//x0 - function return value
const int REG_RETURN_VALUE  = 0;
//x29 - base of current stack frame
const int REG_FRAME_POINTER = 29; 
//x30 - return address (written by call/BL)
const int REG_LINK          = 30; 
//xzr - always zero, writes discarded
const int REG_ZERO          = 31; 
//x16 - load left opperand
const int LEFT_REG          = 16;
//x17 - load right opperand
const int RIGHT_REG         = 17;

int size_to_sf(int size) {
    return size == 8 ? 1 : 0;
}

//Store to address
uint32_t STR(int rt, int rn, int offset, int size = 8) {
    int sz = (size == 8) ? 0b11 : 0b10; // 64-bit vs 32-bit
    return (sz << 30) //64-bit vs 32-bit (2 bits because LDR/STR support more sizes than just 32/64)
         | (0b111001 << 24) //Metadata
         | (0b00 << 22) //Oppcode
         | ((offset & 0xFFF) << 10) //Offset from rn to write to (in units of size, 0 would just be the pointer)
         | ((rn & 0x1F) << 5) //Base (rn), the pointer. This relationship scales isomorphicly
         | (rt & 0x1F); //Source to read from
}

//Load from adress
uint32_t LDR(int rt, int rn, int offset, int size = 8) {
    int sz = (size == 8) ? 0b11 : 0b10;
    return (sz << 30)
         | (0b111001 << 24)
         | (0b01 << 22) //Oppcode
         | ((offset & 0xFFF) << 10) //Offset from rn to read from
         | ((rn & 0x1F) << 5) 
         | (rt & 0x1F); //Destination to write to
}

//Put an immediate value into a register
uint32_t MOVZ(int rd, int imm16, int sf = 0) {
    return (sf << 31) // 0=32bit (w registers), 1=64bit (x registers)
         | (0b10100101 << 23) //The oppcode
         | (0 << 21) //Section in the register (hw)
         | (imm16 << 5) //16 bit immediate
         | rd; //Where to put it
}

void emit_load_literal(g_ptr<Value> v) {
    emit_buffer << MOVZ(
        v->reg != -1 ? v->reg : LEFT_REG,
        v->get<int>(),
        size_to_sf(v->size)
    );
}

//Put an immediate into a specific part of a register
uint32_t MOVK(int rd, int imm16, int shift, int sf = 1) {
    return (sf << 31)
         | (0b11100101 << 23)
         | ((shift/16) << 21) //Section in the register, MOVK is just MOVZ with hw controls
         | ((imm16 & 0xFFFF) << 5)
         | rd;
}

void emit_load_64(int reg, uint64_t value) {
        emit_buffer << MOVZ(reg, (value      ) & 0xFFFF, 1); // bits 0-15
    if(value > 0xFFFF)
        emit_buffer << MOVK(reg, (value >> 16) & 0xFFFF, 16); // bits 16-31
    if(value > 0xFFFFFFFF)
        emit_buffer << MOVK(reg, (value >> 32) & 0xFFFF, 32); // bits 32-47
    if(value > 0xFFFFFFFFFFFF)
        emit_buffer << MOVK(reg, (value >> 48) & 0xFFFF, 48); // bits 48-63
}

void emit_load_from_ptr(int reg, uint64_t ptr, int size) {
    emit_load_64(reg, ptr);
    emit_buffer << LDR(reg, reg, 0, size); //Offset of 0 because we're using the pointer itself
}
void emit_save_to_ptr(int value_reg, uint64_t ptr, int size) {
    emit_load_64(RIGHT_REG, ptr);
    emit_buffer << STR(value_reg, RIGHT_REG, 0, size); // store value to that address
}

//Will also emit load instructions
int get_reg(g_ptr<Value> v, int fallback_reg) {
    if(v->reg==-1) {
        emit_load_from_ptr(fallback_reg,(uint64_t)v->data,v->size);
        return fallback_reg;
    } else {
        return v->reg;
    }
}

//Add
uint32_t ADD_reg(int rd, int rn, int rm, int sf = 0) { 
    return (sf << 31)
         | (0b00001011000 << 21)
         | ((rm & 0x1F) << 16) //Right
         | (0 << 10) 
         | ((rn & 0x1F) << 5) //Left
         | (rd & 0x1F); //Result
}
void emit_add(g_ptr<Value> v, g_ptr<Value> l, g_ptr<Value> r) {
    int left = get_reg(l, LEFT_REG);
    int right = get_reg(r, RIGHT_REG);
    int result = v->reg != -1 ? v->reg : LEFT_REG; //Reuse LEFT_REG for result
    emit_buffer << ADD_reg(result, left, right, size_to_sf(v->size));
    if(v->reg == -1) {
        emit_save_to_ptr(result, (uint64_t)v->data, v->size);
    }
}

uint32_t RET() {
    return 0xD65F03C0;
}
//Return from the current function
void emit_return() {
    emit_buffer << RET();
}

uint32_t MOV_reg(int rd, int rm, int sf = 0) { 
    return (sf << 31)
         | (0b00101010000 << 21) //Oppcode
         | ((rm & 0x1F) << 16) //Register to copy from
         | (REG_ZERO << 5) //Zero register as the first opperand
         | (rd & 0x1F); //Regiser to copy to
}
//Copy a value from one register to another
void emit_copy(int to, int from, int sf = 0) {
    emit_buffer << MOV_reg(to, from, sf);
}


uint32_t B(int imm26) {
    return (0b000101 << 26)
         | (imm26 & 0x3FFFFFF);
}
//Always jump
void emit_jump(int offset) {
    emit_buffer << B(offset);
}

uint32_t B_cond(int cond, int imm19) {
    return (0b01010100 << 24)
         | ((imm19 & 0x7FFFF) << 5)
         | (cond & 0xF);
}
//Conditional jump
void emit_jump_if(int condition, int offset) {
    emit_buffer << B_cond(condition, offset);
}

uint32_t BL(int imm26) {
    return (0b100101 << 26)
         | (imm26 & 0x3FFFFFF);
}
//Call a function at offset, remembers where to return to
void emit_call(int offset) {
    emit_buffer << BL(offset);
}

#include <signal.h>
#include <execinfo.h>
static void* jit_buf_start = nullptr;
static size_t jit_buf_size = 0;

static void sigill_handler(int sig, siginfo_t* info, void* ctx) {
    ucontext_t* uc = (ucontext_t*)ctx;
    // PC is in the machine context
    uint64_t pc = uc->uc_mcontext->__ss.__pc;
    print("Illegal instruction at PC: 0x", std::hex, pc);
    
    if(jit_buf_start) {
        uint64_t offset = pc - (uint64_t)jit_buf_start;
        int instruction_index = offset / 4;
        print("Buffer offset: ", offset, " bytes, instruction index: ", std::dec, instruction_index);
    }
    exit(1);
}

//Used to read the flags
uint32_t CSET(int rd, int cond, int sf = 0) {
    return (sf << 31)
         | (0b0011010100 << 21) // opcode
         | (0b11111 << 16)      // Rm = XZR
         | ((cond ^ 1) << 12)   // inverted condition (CSET is CSINC with inverted cond)
         | (0b01 << 10)         // CSINC variant bits
         | (0b11111 << 5)       // Rn = XZR
         | (rd & 0x1F);         // destination register
}


//Compare two registers, sets internal flags used by jumps
uint32_t CMP_reg(int rn, int rm, int sf = 0) {
    return (sf << 31)
         | (0b1101011000 << 21)  
         | ((rm & 0x1F) << 16)
         | (0 << 10)
         | ((rn & 0x1F) << 5)
         | 0b11111; // XZR
}
//Compare two values and ouput the result as a 1 or 0
void emit_compare(g_ptr<Value> v, g_ptr<Value> lv, g_ptr<Value> rv, int cond) {
    int left = get_reg(lv, LEFT_REG);
    int right = get_reg(rv, RIGHT_REG);
    int result = v->reg != -1 ? v->reg : LEFT_REG;
    
    emit_buffer << CMP_reg(left, right, size_to_sf(v->size));
    emit_buffer << CSET(result, cond);
    
    if(v->reg == -1) {
        emit_save_to_ptr(result, (uint64_t)v->data, v->size);
    }

}

uint32_t CMP_imm(int rn, int imm12, int sf = 0) {
    return (sf << 31)
         | (0b1 << 30)          // op = subtract
         | (0b1 << 29)          // S = set flags
         | (0b10001 << 24)      // add/subtract immediate class
         | (0b00 << 22)         // shift = 0
         | ((imm12 & 0xFFF) << 10)
         | ((rn & 0x1F) << 5)
         | REG_ZERO;            // XZR as destination
}
//Compare a register to a literal number
void emit_compare_value(int a, int value, int sf = 0) {
    emit_buffer << CMP_imm(a, value, sf);
}

uint32_t BLR(int rn) {
    return (0b1101011000111111000000 << 10)
         | ((rn & 0x1F) << 5)
         | 0b00000;
}
void emit_call_register(int reg) {
    emit_buffer << BLR(reg);
}
void jit_print_int(int value) {
    print(value);
}

uint32_t SUB_sp(int imm, int sf = 1) {
    return (sf << 31)
         | (1 << 30)          // op = subtract
         | (0 << 29)          // S = don't set flags
         | (0b10001 << 24)    // class identifier
         | (0 << 22)          // shift = 0
         | ((imm & 0xFFF) << 10)
         | (0b11111 << 5)     // SP as source
         | 0b11111;           // SP as destination
}
//Allocate stack space
uint32_t stack_alloc(int bytes) {
    return SUB_sp(bytes);
}




// int read_reg(g_ptr<Value> value) {
//     if(value->reg != -1) return value->reg;
//     emit_buffer << load_from_stack(16, spill_slot(value));
//     return 16;
// }

// // After writing a result, store it back if it lives on the stack
// void write_reg(g_ptr<Value> value, int computed_in) {
//     if(value->reg != -1) return; // already where it needs to be
//     emit_buffer << store_to_stack(computed_in, spill_slot(value));
// }





void print_emit_buffer() {
    print("==EMIT BUFFER==");
    for(int i=0;i<emit_buffer.length();i++) {
        uint32_t instr = emit_buffer[i];
        print(i,": 0x",std::hex,instr," | ",std::bitset<32>(instr),std::dec);
    }
}




}
