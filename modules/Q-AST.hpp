#pragma once

#include "../core/GDSL.hpp"

//An example utility module for GDSL, headers you can drop in to use across projects

namespace GDSL {

struct AST_Unit : public virtual Unit {
        //Qual handlers which act on the value
        size_t to_prefix_id(size_t id) {return id+1;}
        //Qual handlers which act on the node
        size_t to_suffix_id(size_t id) {return id+2;}

        map<std::string,g_ptr<Value>> keywords;

        void a_pass_resolve_keywords(list<g_ptr<Node>>& nodes) {
            for(g_ptr<Node>& node : nodes) {
                a_pass_resolve_keywords(node->children);
                g_ptr<Value> value = keywords.getOrDefault(node->name,fallback_value);
                if(value!=fallback_value) {
                    if(!node->value)
                        node->value = make<Value>(0);
                    
                    node->value->copy(value);
                }
                for(auto scope : node->scopes) {
                    a_pass_resolve_keywords(scope->children);
                }
            }
        };

        list<size_t> discard_types;


        g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
            return make<Value>(sub_type,size,reg_id(name));
        }

        size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
            g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
            keywords.put(name,val);
            return val->sub_type;
        }

        g_ptr<Value> make_qual_value(const std::string& f, size_t size = 0) {
            size_t id = reg_id(f);
            size_t prefix_id = reg_id(f);
            size_t suffix_id = reg_id(f);
            g_ptr<Value> val = make<Value>(id,size,id);
            return val;
        }

        g_ptr<Value> make_type(const std::string& f, size_t size = 0) {
            g_ptr<Value> val = make_qual_value(f,size);

            t_handlers[to_prefix_id(val->type)] = [](Context& ctx){
                if(ctx.value->sub_type == 0) {
                    ctx.value->sub_type = ctx.qual->sub_type;
                    ctx.value->type = ctx.qual->type;
                    ctx.value->size = ctx.qual->value->size;
                    if(ctx.qual->value->type_scope)
                        ctx.value->type_scope = ctx.qual->value->type_scope;
                }
            };

            return  val;
        }

        size_t add_qualifier(const std::string& f, size_t size = 0) {
            g_ptr<Value> val = make_qual_value(f,size);
            keywords.put(f,val);
            return val->type;
        }
            
        size_t add_type(const std::string& f, size_t size = 0) {
            g_ptr<Value> val = make_type(f,size);
            keywords.put(f,val);
            return val->type;
        }

        size_t add_function(const std::string& f, uint32_t return_type = 0) {
            g_ptr<Value> val = make_value(f,0,return_type);
            keywords.put(f,val);
            size_t call_id = val->sub_type;
            return call_id;
        }
};
}