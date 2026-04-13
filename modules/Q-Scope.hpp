#pragma once

#include "../modules/Q-Tokenizer.hpp"

namespace GDSL {
    struct Scope_Unit : public virtual Tokenizer_Unit {
        map<uint32_t, std::function<void(g_ptr<Node>, g_ptr<Node>, g_ptr<Node>)>> scope_link_handlers;
        map<uint32_t, int> scope_precedence;
    
        void print_scope(g_ptr<Node> scope, int depth = 0) {
            std::string indent(depth * 2, ' ');
            
            if (depth > 0) {
                log(indent, "{");
            }
            
            for (auto& subnode : scope->children) {
                log(indent, "  ", labels[subnode->type]);
            
                
                if(!subnode->scopes.empty()) {
                    for(auto subscope : subnode->scopes) {
                        print_scope(subscope, depth + 1);
                    }
                }
            }
            
            if (depth > 0) {
                log(indent, "}");
            }
        }
    
        struct g_value {
        public:
            g_value() {}
            g_value(g_ptr<Node> _owner) : owner(_owner) {}
            bool explc = false;
            g_ptr<Node> owner = nullptr;
            bool deferred = false;
        };
    
        size_t scope_id = reg_id("SCOPE");
    
        void parse_scope(g_ptr<Node> root) {
            root->name = "GLOBAL";
            root->type = scope_id;
            root->is_scope = true;
            list<g_ptr<Node>> nodes;
            nodes <= root->children;
            g_ptr<Node> current_scope = root;
            list<g_value> stack{g_value()};
    
            #if PRINT_ALL
                newline("Parse scope pass");
            #endif
    
            for (int i = 0; i < nodes.size(); ++i) {
                g_ptr<Node> node = nodes[i];
                g_ptr<Node> owner_node = (i>0) ? nodes[i - 1] : nullptr;
    
                int p = scope_precedence.getOrDefault(node->type,0);
                bool on_stack = stack.last().owner ? true : false;
                if(p<=0) {
                    if(p<0) {
                        if (current_scope->parent) {
                            current_scope = current_scope->parent;
                        }
                    }
                    else {
                        current_scope->children << node; 
                        node->place_in_scope(current_scope.getPtr());
                        if(on_stack && !stack.last().deferred && !stack.last().explc) {
                            if (current_scope->parent) {
                                current_scope = current_scope->parent;
                            }
                        }
                    }
                }
                else {
                    if(p<10) {
                        current_scope->children << node;
                        node->place_in_scope(current_scope.getPtr());
                        owner_node = node;
                    }
    
                    if(on_stack) {
                        int stack_precedence = scope_precedence.getOrDefault(stack.last().owner->type,0);
                        if(p >= stack_precedence) {
                            stack.last().deferred = true;
                        }
                    }
    
                    if(p == 10) {
                        if(on_stack) {
                            stack.last().explc = true;
                            if (current_scope->parent) {
                                current_scope = current_scope->parent;
                            }
                        }
                    }
                    else {
                        stack << g_value(owner_node);
                    }
    
                    g_ptr<Node> parent_scope = current_scope;
                    current_scope = current_scope->spawn_sub_scope();
                    current_scope->type = scope_id;
                    if (owner_node) {
                        //Deffensive check here
                        try {
                            auto func = scope_link_handlers.get(owner_node->type);
                            func(current_scope,parent_scope,owner_node);
                        }
                        catch(std::exception e) {
                            print("parse_scope::809 missing scope link handler for type: ",labels[owner_node->type]);
                        }
                    
                    } else {
                        current_scope->type = 0; //Suppoused to be GET_TYPE(BLOCK), doesn't matter, don't care
                    }
                }
            }
    
            #if PRINT_ALL
            //print_scope(root_scope);
            endline();
            #endif
        }
    
        size_t add_scoped_keyword(const std::string& name, int scope_prec)
        {
            size_t id = make_tokenized_keyword(name);
            scope_precedence.put(id, scope_prec);
            scope_link_handlers.put(id, [](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                if(owner_node->scope()) { //To handle implicit scoping
                    owner_node->in_scope->scopes.erase(owner_node->scopes.take(0));
                } 
                new_scope->owner = owner_node.getPtr();
                owner_node->scopes << new_scope.getPtr();
                for(auto c : owner_node->children) {
                    c->place_in_scope(new_scope.getPtr());
                }
                new_scope->name = owner_node->name;
            });
            t_handlers[id] = [](Context& ctx){};
            return id;
        }

        void standard_scope_link_handler(g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
            new_scope->owner = owner_node.getPtr();
            owner_node->scopes << new_scope.getPtr();
            for(auto c : owner_node->children) {
                c->place_in_scope(new_scope.getPtr());
            }
            new_scope->name = owner_node->name;
        }

        void init() override {
            Tokenizer_Unit::init();
            scope_precedence[lbrace_id] = 10;
            scope_precedence[rbrace_id] = -10;
        }
    };
}