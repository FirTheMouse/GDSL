#pragma once

#include <core/type.hpp>

namespace GDSL {

//Controls for the compiler printing, for debugging
#define PRINT_ALL 1
#define PRINT_STYLE 0
//Very important when adding modules, but will add overhead on execution, causes the reg class
//to check if a hash exists before using it
#define CHECK_REG 1


//GDSL, Golden Dynamic Systems Language
//TAST: tokenizer, a stage, scope stage, t stage.
//DRE: Discover, resolve, execute
//MIX: Memory, instruction, execute

g_ptr<Log::Span> span = nullptr;
static void newline(const std::string& label) {
    span->add_line(label);
}
static double endline() {
    return span->end_line();
}


#if PRINT_ALL
    template<typename... Args>
    static void log(Args&&... args) {
        span->log(std::forward<Args>(args)...);
    }
#else
    #define log(...)
#endif


constexpr uint32_t hashString(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

uint32_t hashString(const std::string& str) {
    return hashString(str.c_str());
}

#define TYPE_ID(name) hashString(#name)

class reg {
    static inline map<uint32_t,uint32_t> hash_to_enum;
    static inline map<uint32_t,std::string> enum_to_string;
    static inline uint32_t next_id = 0; //Consider making this atomic if the maps become thread safe
public:
    static uint32_t new_type(const std::string& name) {
        //Put logic here to warn if there's duplicate keys
        uint32_t hash = hashString(name.c_str());
        uint32_t enum_id = next_id++;
        
        hash_to_enum.put(hash,enum_id);
        enum_to_string.put(enum_id,name);
        return enum_id;
    }

    static uint32_t ID(uint32_t hash) {
        #if CHECK_REG
        if(!hash_to_enum.hasKey(hash)) {
            print("reg::40 Warning, key not found for enum ",hash,"!");
            return 0;
        }
        #endif 
        return hash_to_enum.get(hash);
    }

    static std::string name(uint32_t ID) {
        #if CHECK_REG
        if(!enum_to_string.hasKey(ID)) {
            print("reg::50 Warning, key not found for enum ",ID,"!");
            return "[null]";
        } 
        #endif
        return enum_to_string.get(ID);
    }

    static void printRegistry() {
        print("-- registry print --");
        for (const auto& pair : enum_to_string.entrySet()) {
            print(pair.key," -> ",pair.value);
        }
    }
};


//For debug
//#define GET_TYPE(name) (print("Looking up: " #name), reg::ID(TYPE_ID(name)))
#define GET_TYPE(name) reg::ID(TYPE_ID(name))
#define TO_STRING(ID) reg::name(ID)
#define IS_TYPE(node, type_name) ((node)->type == GET_TYPE(type_name))

static std::string fallback = "[undefined]";

//Used for debugging, and for printing 
map<uint32_t,std::function<std::string(void*)>> value_to_string;
map<uint32_t, std::function<void(void*)>> negate_value;

static std::string data_to_string(uint32_t type,void* data) {    
    try {
        return value_to_string.get(type)(data);
    }
    catch(std::exception e) {
        return "[missing value_to_string for type "+TO_STRING(type)+"]";
    }
}

class Node;

//NOTE TO SELF: Remember to clean up circiular refrences for Quals at some point
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
    list<g_ptr<Value>> quals;
    g_ptr<Value> parent;
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
                g_ptr<Value> cpy = make<Value>();
                cpy->copy(oq,true);
                quals << cpy;
            }
            parent = nullptr;
        } else {
            quals = o->quals;
            parent = o->parent;
        }

        if(o->data) {
            data = malloc(size);
            memcpy(data,o->data,size);
        }
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
        std::function<std::string(void*)> fallback_func = [this](void* ptr){return "[missing value_to_string for type "+TO_STRING(type)+"]";};
        return value_to_string.getOrDefault(type,fallback_func)(data);
    }

    std::string info() {
        std::string to_return = "";
        to_return += "Value [@" + std::to_string((size_t)(void*)this) + "]"
        + "(type: " + TO_STRING(type)
        + (data?", value: "+to_string():"")
        + (sub_type!=0?", sub_type: "+TO_STRING(sub_type):"")
        + (type_scope?", type_scope: [yes]":"")
        + (size!=0?", size: "+std::to_string(size):"")
        + (address!=0?", address: "+std::to_string(address):"");
        if(!quals.empty()) {
            to_return += ", Quals: ";
            for(auto q : quals) {
                to_return += TO_STRING(q->type)+(q!=quals.last()?", ":"");
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
                print("value::300 missing negate_value handler for ",TO_STRING(type));
            }
        }
    }

    bool is_true() {
        if (data && IS_TYPE(this,BOOL)) {
            return *(bool*)data; 
        }
        return false;
    }
};

g_ptr<Value> fallback_value = nullptr;
class Frame;

//Single unified Node for everything
class Node : public Object {
public:
    Node() {
        value = make<Value>();
    }
    Node(uint32_t _type, char c) : type(_type) {
        name += c;
    }

    uint32_t type = 0;
    std::string name = "";
    g_ptr<Value> value = nullptr;
    g_ptr<Node> left = nullptr;
    g_ptr<Node> right = nullptr;
    list<g_ptr<Node>> children;
    g_ptr<Frame> frame = nullptr;

    list<g_ptr<Value>> quals;

    list<g_ptr<Node>> scopes;
    Node* parent = nullptr;
    Node* owner = nullptr;
    Node* in_scope = nullptr;

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
        left = o->left;
        right = o->right;
        children = o->children;
        quals = o->quals;
        frame = o->frame;
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
        in_scope->children.erase(this);
        scope->children << this;
        in_scope = scope;
        for(auto c : children) 
            c->shunt_to_scope(scope);
    }

    g_ptr<Value> distribute_value(const std::string& label, g_ptr<Value> val) {
        log("Distributing a value: ",label," through ",name);
        if(value_table.hasKey(label)) {
            g_ptr<Value> table_value = value_table.get(label);
            if(table_value->type == 0) {
                log("Replacing the value with already existing value");
                table_value->copy(val);
                val = table_value;
            } else 
                log("Doing nothing because there's an existing type with a valid value");
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
        if(value_table.hasKey(label)) {
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

    void populate_lr() {
        left = nullptr;
        right = nullptr;
        for(int i = 0; i<children.length();i++) {
            if(i==0) left = children[i];
            else if(i==1) right = children[i];

            children[i]->populate_lr();
        }
    }

    std::string info() {
        std::string to_return = "";
        
        to_return += TO_STRING(type) + " [Name: " + name;
        if(value) {
            to_return += ", "+value->info();
        }
        to_return += "]";
        return to_return;
    }

    std::string to_string(int depth = 0, int index = 0, bool print_sub_scopes = false) {
        std::string indent(depth * 2, ' ');
        std::string to_return = "";
        
        to_return += indent + std::to_string(index) + ": " + TO_STRING(type)
        +(!name.empty()?green(" "+name):"") 
        +(in_scope?" {"+in_scope->name+"}":"");
        
        if(!quals.empty()) {
            to_return += " [";
            for(auto q : quals) {
                to_return += TO_STRING(q->type)+(q!=quals.last()?", ":"");
            }
            to_return += "]";
        }


        if(value) {
            to_return += "\n" + indent + "   "+value->info(); //TO_STRING(value->type);
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
     
        // if(left) {
        //     to_return +=  "\n" + indent + "  Left:\n";
        //     to_return += left->to_string(depth + 1, 0, print_sub_scopes);
        // }
    
        // if(right) {
        //     to_return +=  "\n" + indent + "  Right:\n";
        //     to_return += right->to_string(depth + 1, 0, print_sub_scopes);
        // }
    
        // if(!children.empty()) {
        //     int total_children = children.length();
        //     if(left) total_children--;
        //     if(right) total_children--;
        //     if(total_children>0) {
        //         to_return +=  "\n" + indent + "  Children: " + std::to_string(total_children);
        //         for(int i=0;i<children.length();i++) {
        //             if(i==0&&left) continue;
        //             if(i==1&&right) continue;
        //             if(children[i])
        //                 to_return += "\n " + children[i]->to_string(depth + 1, i, print_sub_scopes);
        //             else 
        //                 to_return += "\n" + indent + "[NULL]";
        //         }
        //     }
        // }
    
        // if(!opt_sub.empty()) {
        //     to_return +=  "\n" + indent + "  Opt_sub: " + std::to_string(opt_sub.size());
        //     int i = 0;
        //     for(auto& child : opt_sub) {
        //         to_return += "\n " + child->to_string(depth + 1, i++);
        //     }
        // }
    
        // if(!opt_sub_2.empty()) {
        //     to_return +=  "\n" + indent + "  Opt_sub_2: " + std::to_string(opt_sub_2.size());
        //     int i = 0;
        //     for(auto& child : opt_sub_2) {
        //         to_return += "\n " + child->to_string(depth + 1, i++);
        //     }
        // }
    
        return to_return;
    }
};


class Frame : public Object {
    public:
    Frame() {
        if(!context) {
            context = make<Type>();
        }
    }
    uint32_t type = 0;
    g_ptr<Type> context = nullptr;
    std::string name;
    list<g_ptr<Node>> nodes;
    list<g_ptr<Value>> stack;
    g_ptr<Node> return_to = nullptr;
    list<g_ptr<Object>> active_objects;
    list<void*> active_memory;
    list<std::function<void()>> stored_functions;
};

static int _ctx_dummy_index = 0;


struct Context {
    Context() : index(_ctx_dummy_index) {}
    Context(list<g_ptr<Node>>& _result, int& _index) : result(&_result), index(_index) {}

    g_ptr<Node> node;
    g_ptr<Node> left;
    g_ptr<Node> out;
    g_ptr<Node> root;
    g_ptr<Frame> frame;
    list<g_ptr<Node>>* result = nullptr;
    list<g_ptr<Node>> nodes;
    g_ptr<Value> qual;
    g_ptr<Value> value;
    int& index;
    uint32_t state;

    std::string source;


    Context sub() {
        return Context((*result), index);
    }
};

using Handler = std::function<void(Context& ctx)>;
map<char,Handler> tokenizer_functions;
map<char,Handler> tokenizer_state_functions;
Handler tokenizer_default_function = nullptr;

map<uint32_t,Handler> a_functions;
Handler a_parse_function;

map<uint32_t,Handler> t_functions;
Handler t_default_function;

map<uint32_t, Handler> discover_handlers;

map<uint32_t, Handler> r_handlers;
Handler r_default_function;

map<uint32_t, Handler> exec_handlers;


//Unused by quals for now
map<uint32_t, Handler> a_prefix_functions;
map<uint32_t, Handler> a_suffix_functions;


static void process_qual(Context& ctx, g_ptr<Value> qual, map<uint32_t, Handler>& handlers) {
    ctx.qual = qual;
    if(handlers.hasKey(qual->type))
        handlers.get(qual->type)(ctx);
    ctx.qual = nullptr;
}

map<uint32_t, Handler> t_prefix_functions; //Prefix opperates on values
map<uint32_t, Handler> t_suffix_functions; //Suffix opperates on nodes
static void parse_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) process_qual(ctx,qual,t_suffix_functions);
}
static void parse_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) process_qual(ctx,qual,t_prefix_functions);
}

map<uint32_t, Handler> discover_prefix_handlers;
map<uint32_t, Handler> discover_suffix_handlers;
static void discover_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) process_qual(ctx,qual,discover_suffix_handlers);
}
static void discover_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) process_qual(ctx,qual,discover_prefix_handlers);
}


map<uint32_t, Handler> r_prefix_handlers;
map<uint32_t, Handler> r_suffix_handlers;
static void resolve_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) process_qual(ctx,qual,r_suffix_handlers);
}
static void resolve_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) process_qual(ctx,qual,r_prefix_handlers);
}

map<uint32_t, Handler> exec_prefix_handlers;
map<uint32_t, Handler> exec_suffix_handlers;
static void execute_quals(Context& ctx, g_ptr<Node> node) {
    ctx.node = node;
    for(auto qual : node->quals) process_qual(ctx,qual,exec_suffix_handlers);
}
static void execute_quals(Context& ctx, g_ptr<Value> value) {
    ctx.value = value;
    for(auto qual : value->quals) process_qual(ctx,qual,exec_prefix_handlers);
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
            log(i++," ",TO_STRING(t->getType()),": ",t->name);
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
    if(!t_default_function)
        print("GDSL::parse_a_node t_stage requires a default function!");
    //newline("Parsing (T): "+node->info());
    log("Parsing: ",node->info());
    t_functions.getOrDefault(node->type,t_default_function)(ctx);
    // endline();
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
        t->populate_lr();
        log(t->to_string());
    }
    endline();
}

static void discover_symbol(g_ptr<Node> node, g_ptr<Node> root) {
    if(discover_handlers.hasKey(node->type)) {
        auto func = discover_handlers.get(node->type);
        Context ctx;
        ctx.root = root;
        ctx.node = node;
        newline("Discovering: "+node->info());
        func(ctx);
        endline();
    }
    for(auto c : node->children) {
        discover_symbol(c,root);
    }
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

static g_ptr<Node> resolve_symbol(g_ptr<Node> node,g_ptr<Node> scope,g_ptr<Frame> frame) {
    Context ctx;
    ctx.node = node;
    ctx.root = scope;
    ctx.frame = frame;
    if(!r_default_function)
        print("GDSL::resolve_symbol r_stage requires a default function!");
    ctx.node->frame = scope->frame;
    if(ctx.node->scope()) 
        ctx.node->frame = ctx.node->scope()->frame;
    newline("Resolving: "+node->info());
    r_handlers.getOrDefault(node->type,r_default_function)(ctx);
    endline();
    return ctx.node;
}

static void resolve_sub_nodes(Context& ctx) {
    for(int i = 0; i<ctx.node->children.length();i++) {
        resolve_symbol(ctx.node->children[i], ctx.root, ctx.frame);
    }
}

static g_ptr<Frame> resolve_symbols(g_ptr<Node> root) {
    newline("Resolve symbols pass (R) over "+std::to_string(root->children.size())+" nodes");

    root->frame->type = root->type;
    root->frame->name = root->name;

    for(int i = 0; i < root->children.size(); i++) {
        g_ptr<Node> rnode = resolve_symbol(root->children[i],root,root->frame);
        if(rnode) 
            root->frame->nodes << rnode;
        else
            i--;
    }

    for(auto child_scope : root->scopes) {
        resolve_symbols(child_scope);
    }

    log("==RESOLVED SYMBOLS: ",root->frame->name,"==");
    for(auto r : root->frame->nodes) {
        log(r->to_string());
    }
    endline();
    

    return root->frame;
}   

static g_ptr<Node> execute_r_node(g_ptr<Node> node,g_ptr<Frame> frame) {
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.node = node;
    ctx.frame = frame;
    if(exec_handlers.hasKey(node->type)) {
        //newline("Executing: "+node->info());
        log("Executing: "+node->info());
        exec_handlers.get(node->type)(ctx);
        //endline();
    }
    else {
        print("execute_r_node::1343 Missing case for r_type: ",TO_STRING(node->type));
        return nullptr;
    }
    return node;
}


static void execute_r_nodes(g_ptr<Frame> frame, g_ptr<Object> context=nullptr) {
    if(!context) { 
        //This is bassicly just to push rows and mark them as usable or not to a thread
        //Want to replace this later with something more efficent, or, make more use of context
        context = frame->context->create();
    }

    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame);
        if(!frame->isActive()) {
            log("Frame was deactivated on ",node->info());
            break;
        }
    }
    if(!frame->isActive())
        frame->resurrect();

    frame->context->recycle(context);
    for(int i=frame->active_objects.length()-1;i>=0;i--) {
        frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    }
    for(int i=frame->active_memory.length()-1;i>=0;i--) {
        //Commented out for now
        //free(frame->active_memory[i]);
    }
}   

static void execute_constructor(g_ptr<Frame> frame) {
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        execute_r_node(node,frame);
        if(!frame->isActive()) {
            break;
        }
    }
}

map<uint32_t, std::function<std::function<void()>(Context&)>> stream_handlers;

static void stream_r_node(g_ptr<Node> node,g_ptr<Frame> frame) {
    int index = 0; list<g_ptr<Node>> results;
    Context ctx(results,index);
    ctx.node = node;
    ctx.frame = frame;
    if(stream_handlers.hasKey(node->type)) {
        std::function<void()> func = stream_handlers.get(node->type)(ctx);
        if(func) {
            frame->stored_functions << func;
        }
    }
    else {
        print("stream_r_node::1380 Missing case for r_type: ",TO_STRING(node->type));
    }
}

//Streaming is a diffrent execution approach that pre-resolves the values to direct addresses and bytes to feed into the Type
//while this looses flexibility, it can reach the same performance levels as C++ (within 30% in benchmarks), though, this has
//been shotgunned by the change to use Type indexes instead of adresses.
static void stream_r_nodes(g_ptr<Frame> frame,g_ptr<Object> context=nullptr) {
    #if PRINT_ALL
    print("==STREAM NODES==");
    #endif
    if(!context) { 
        context = frame->context->create();
    }
    for(int i = 0; i < frame->nodes.size(); i++) {
        auto node = frame->nodes[i];
        stream_r_node(node,frame);
    }
    frame->context->recycle(context);
    // for(int i=frame->active_objects.length()-1;i>=0;i--) {
    //     frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    // }
}   

static void execute_stream(g_ptr<Frame> frame) {
    for(int i =0;i<frame->stored_functions.length();i++) {
        frame->stored_functions[i]();
    }
    for(int i=frame->active_objects.length()-1;i>=0;i--) {
        frame->active_objects[i]->type_->recycle(frame->active_objects.pop());
    }
}   


}
