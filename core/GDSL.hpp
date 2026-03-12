#pragma once

#include <core/type.hpp>

namespace GDSL {

//Controls for the compiler printing, for debugging
#define PRINT_ALL 1
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

std::string labels[MAX_TYPES*HANDLER_COUNT];

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
    size_t size = 0;
    int sub_size = 0;
    list<Qual> quals;
    list<g_ptr<Value>> sub_values;
    Node* type_scope = nullptr;

    void copy(g_ptr<Value> o, bool is_deep = false) {
        type = o->type; 
        sub_type = o->sub_type; 
        address = o->address; 
        size = o->size; 
        sub_size = o->sub_size;
        type_scope = o->type_scope;
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
        + (address!=0?", address: "+std::to_string(address):"");
        if(!quals.empty()) {
            to_return += ", Quals: ";
            for(int i=0;i<quals.length();i++) {
                to_return += labels[quals[i].type]+"[@" + std::to_string((size_t)(void*)quals[i].value.getPtr()) + "]"+(i!=quals.length()-1?", ":"");
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
        log("Distributing a value: ",label," through ",name);
        if(value_table.hasKey(label)) {
            g_ptr<Value> table_value = value_table.get(label);
            if(table_value->type == 0) {
                log("Replacing the value with already existing value");
                table_value->copy(val);
                val = table_value;
            } else {
                log("Doing nothing because there's an existing type with a valid value");
            }
        } else {
            log("Putting into table");
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

    //Could probably replace this using the Object data system, keep them for explicitness but may just cmd+f replace them later
    // list<g_ptr<Node>> opt_sub;
    // list<g_ptr<Node>> opt_sub_2; //Kludge for scopes
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

        //Add this back once we've figured out how to attach frame names, or just keep it in scope
        // if(frame) {
        //     to_return +=  "\n" + indent + "  Frame: " + "[yes]";
        // }
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

map<char, std::function<void(Context&)>> tokenizer_functions;
map<uint32_t, void(*)(Context&)> tokenizer_state_functions;
std::function<void(Context&)> tokenizer_default_function = nullptr;


void(*a_parse_function)(Context& ctx) = nullptr;

//TAST
void (*a_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*s_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*t_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
//DRE
void (*d_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*r_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*e_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
//MIX
void (*m_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*i_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};
void (*x_handlers[MAX_TYPES*HANDLER_COUNT])(Context&) = {};

void (**active_handlers)(Context&) = nullptr;

size_t next_id = 0;
size_t reg_id(std::string label) {
    size_t id = next_id;
    for(int i=0;i<HANDLER_COUNT;i++) {
        labels[next_id] = label;
        next_id++;
    }
    return id;
}

size_t undefined_id = reg_id("UNDEFINED"); //So that it's always 0

void init_handlers(void(**handlers)(Context&), void(*default_handler)(Context&), bool fill_all = false) {
    if(handlers==a_handlers) a_parse_function = default_handler; //Special case for convience

    for(int i = 0; i < MAX_TYPES*HANDLER_COUNT; i++) {
        if(!handlers[i]||fill_all)
            handlers[i] = default_handler;
    }
}

void start_stage(void(**handlers)(Context&)) {
    active_handlers = handlers;
}

static void fire_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) {
        if(qual.mute) continue;
        ctx.qual = qual;
        active_handlers[qual.type+1](ctx);
    }
}
static void fire_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) {
        if(qual.mute) continue;
        ctx.qual = qual;
        active_handlers[qual.type+2](ctx);
    }
}


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
            //a_functions.getOrDefault(tokens[index]->getType(),a_default_function)(ctx);
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


static g_ptr<Node> parse_a_node(g_ptr<Node> node,g_ptr<Node> root,g_ptr<Node> left = nullptr) {
    Context ctx;
    ctx.root = root;
    ctx.node = node;
    ctx.left = left;
    newline("Parsing (T): "+node->info());
    active_handlers[node->type](ctx);
    endline();
    return ctx.node;
}

static void parse_sub_nodes(Context& ctx) {
    for(int i = 0; i<ctx.node->children.length();i++) {
        parse_a_node(ctx.node->children[i], ctx.root);
    }
}

static void parse_nodes(g_ptr<Node> root) {
    newline("Parse nodes pass (T) over "+std::to_string(root->children.size())+" nodes");

    g_ptr<Node> last = nullptr;
    for(int i = 0; i < root->children.size(); i++) {
        if(root->children[i]->scope()) {
            last = parse_a_node(root->children[i],root,last);
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        auto node = root->children[i];
        if(!node->scope()) {
            last = parse_a_node(node,root,last);
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        if(root->children[i]->scope()) {
            for(auto c : root->children[i]->children) {
                parse_a_node(c,root);
            }
        }
    }
    for(auto child_scope : root->scopes) {
        parse_nodes(child_scope);
    }
    
    log("==T_NODES IN ",root->name,"==");
    for(auto t : root->children) {
        log(t->to_string());
    }
    endline();
}

static void discover_symbol(g_ptr<Node> node, g_ptr<Node> root) {
    Context ctx;
    ctx.root = root;
    ctx.node = node;
    newline("Discovering: "+node->info());
    active_handlers[node->type](ctx);
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

static g_ptr<Node> resolve_symbol(g_ptr<Node> node,g_ptr<Node> scope) {
    Context ctx;
    ctx.node = node;
    ctx.root = scope;
    newline("Resolving: "+node->info());
    active_handlers[node->type](ctx);
    endline();
    return ctx.node;
}

static void resolve_sub_nodes(Context& ctx) {
    for(int i = 0; i<ctx.node->children.length();i++) {
        resolve_symbol(ctx.node->children[i], ctx.root);
    }
}

static void resolve_symbols(g_ptr<Node> root) {
    newline("Resolve symbols pass (R) over "+std::to_string(root->children.size())+" nodes");
    for(int i = 0; i < root->children.size(); i++) {
        resolve_symbol(root->children[i],root);
    }

    for(auto child_scope : root->scopes) {
        resolve_symbols(child_scope);
    }

    log("==RESOLVED SYMBOLS: ",root->name,"==");
    for(auto r : root->children) {
        log(r->to_string());
    }
    endline();
}   

static void standard_process(Context& ctx) {
    active_handlers[ctx.node->type](ctx);
}

static void process_node(g_ptr<Node> node) {
    Context ctx;
    ctx.node = node;
    active_handlers[ctx.node->type](ctx);
}

static void standard_sub_process(g_ptr<Node> node) {
    for(int i = 0; i<node->children.length();i++) {
        process_node(node->children[i]);
    }
}

static void standard_sub_process(Context& ctx) {
    for(int i = 0; i<ctx.node->children.length();i++) {
        process_node(ctx.node->children[i]);
    }
}

static void standard_resolving_pass(g_ptr<Node> root) {
    newline("Standard pass over "+std::to_string(root->children.size())+" nodes");
    Context ctx;
    ctx.root = root;
    for(int i = 0; i < root->children.size(); i++) {
        if(root->children[i]->scope()) {
            ctx.node = root->children[i];
            standard_process(ctx);
            ctx.left = root->children[i];
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        if(!root->children[i]->scope()) {
            ctx.node = root->children[i];
            standard_process(ctx);
            ctx.left = root->children[i];
        }
    }
    for(int i = 0; i < root->children.size(); i++) {
        if(root->children[i]->scope()) {
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
    for(int i = 0; i < root->children.size(); i++) {
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
    for(int i = 0; i < root->children.size(); i++) {
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

void e_pass(Context& ctx, g_ptr<Node> root){
    ctx.root = root;
    for(int i=root->children.length()-1;i>=0;i--) {
        newline("Checking: "+root->children[i]->info());
        ctx.node = root->children[i];
        standard_process(ctx);
        if(!ctx.node) {
            root->children.removeAt(i);
        } else {
            for(auto scope : root->children[i]->scopes) {
                e_pass(ctx, scope);
            }
            ctx.left = root->children[i];
        }
        endline();
    }
}

void start_e_stage(g_ptr<Node> root) {
    start_stage(e_handlers);
    Context ctx;
    e_pass(ctx,root);
}




}
