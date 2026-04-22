#pragma once

#include "../mixos-acorn/Acorn-Core.hpp"

namespace Acorn {
    struct Acorn_Script : public virtual Unit {
        Acorn_Script() {}

        size_t type_scope_id = reg_id("TYPE_SCOPE");

        // void init() override {
        //     Starter_DSL_Frontend::init();
            
        //     t_handlers[identifier_id] = [this](Context& ctx) {
        //         g_ptr<Node> node = ctx.node;
        //         g_ptr<Value> decl_value = make<Value>();
                
        //         bool found_a_value = node->find_value_in_scope();
        //         bool is_qualifier = node->value->type!=0;


        //         int root_idx = -1;
        //         if(is_qualifier && node->value->sub_type != 0) {
        //             decl_value->quals << value_to_qual(node->value);
        //             decl_value->quals.last()->name = node->name;
        //             decl_value->quals.last()->x = node->x;
        //             decl_value->quals.last()->y = node->y;
        //             for(int i = 0; i < node->children.length(); i++) {
        //                 g_ptr<Node> c = node->children[i];
        //                 c->find_value_in_scope();
        //                 if(c->value->type!=0) {
        //                     decl_value->quals << value_to_qual(c->value);
        //                     decl_value->quals.last()->name = c->name;
        //                     decl_value->quals.last()->x = c->x;
        //                     decl_value->quals.last()->y = c->y;
        //                 } else {
        //                     root_idx = i;
        //                     break;
        //                 }
        //             }
        //             if(root_idx!=-1) {
        //                 g_ptr<Node> root = node->children[root_idx];
        //                 node->name = root->name;
        //                 node->x = root->x;
        //                 node->y = root->y;
        //                 for(int i = root_idx+1; i < node->children.length(); i++) {
        //                     g_ptr<Node> c = node->children[i];
        //                     c->find_value_in_scope();
        //                     if(c->value->type!=0) {
        //                         node->quals << value_to_qual(c->value);
        //                         node->quals.last()->name = c->name;
        //                         node->quals.last()->x = c->x;
        //                         node->quals.last()->y = c->y;
        //                     } 
        //                 }
        //                 node->children = node->children.take(root_idx)->children;
        //             }
        //         }
                
        //         if(!node->scope()) { //Defer, the r_stage will do this later for scoped nodes
        //             standard_sub_process(ctx);
        //         }

        //         if(keywords.hasKey(node->name)) {
        //             if(node->value->sub_type!=0) {
        //                 node->type = node->value->sub_type;
        //                 return;
        //             }
        //         }

        //         node->value = decl_value;
        //         fire_quals(ctx, decl_value);

        //         bool has_scope = node->scope() != nullptr;
        //         bool has_type_scope = node->value->type_scope != nullptr;
        //         bool has_sub_type = node->value->sub_type != 0;
                
        //         if(has_scope) {
        //             node->scope()->owner = node.getPtr();
        //             node->scope()->name = node->name;
        //             if(has_sub_type) {
        //                 node->type = func_decl_id;
        //                 node->scopes[0] = node->in_scope->distribute_node(node->name,node->scope());
        //                 node->value->type_scope = node->scope().getPtr();
        //                 node->value = node->in_scope->distribute_value(node->name,node->value);
        //             } else {
        //                 node->type = type_decl_id;
        //                 node->value->copy(make_type(node->name,0));
        //                 node->value->type_scope = node->scope().getPtr();
        //                 node->value = node->in_scope->distribute_value(node->name,node->value);

        //                 node->scope()->type = type_scope_id;
        //             }
        //         } else {
        //             has_scope = node->find_node_in_scope(); //To distinquish func_calls from object identifiers
        //             if(has_sub_type) {
        //                 node->type = var_decl_id;
        //                 if(node->in_scope->type==type_scope_id) {
        //                     node->in_scope->value_table.put(node->name, decl_value);
        //                 } else {
        //                     node->value = node->in_scope->distribute_value(node->name, decl_value);
        //                 }
        //                 node->value->sub_type = 0;
        //             } else if(has_scope) {
        //                 node->type = func_call_id;
        //                 node->find_value_in_scope(); //Retrive our return value (could probably just do 'found_a_value' skips decl set...)
        //                 if(node->value->type_scope)
        //                     node->scopes[0] = node->value->type_scope; //Swap to the type scope
        //                 if(!node->children.empty()) {
        //                     node->name.append("(");
        //                     for(auto c : node->children) {node->name.append(c->name+(c!=node->children.last()?",":")"));}
        //                 }
        //             } else if(found_a_value) { //if we already had a value and nothing interesting happened to us, reclaim it
        //                 node->find_value_in_scope();
        //             } else {                                         
        //                 //We don't know what the thing is
        //             }
        //         }
        //     };
        // } 

        // void run(g_ptr<Node> root) override {
        //     g_ptr<Object> o = node_type->create();
        //     void* data = resolve_ptr(*(Ptr*)node_type->get(name_col,o->sidx));
        //     print("Hello world");
        // }
    };
}