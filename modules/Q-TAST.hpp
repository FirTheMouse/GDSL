#pragma once

#include "../core/GDSL.hpp"

namespace GDSL {
    struct TAST_Unit : public virtual Unit {
            //Qual handlers which act on the value
            size_t to_prefix_id(size_t id) {return id+1;}
            //Qual handlers which act on the node
            size_t to_suffix_id(size_t id) {return id+2;}

            size_t identifier_id = reg_id("IDENTIFIER");
            size_t object_id = reg_id("OBJECT");
            size_t literal_id = reg_id("LITERAL");

            size_t var_decl_id = reg_id("VAR_DECL");
            size_t func_call_id = reg_id("FUNC_CALL");
            size_t function_id = reg_id("FUNCTION");
            size_t method_id = reg_id("METHOD");
            size_t func_decl_id = reg_id("FUNC_DECL");
            size_t type_decl_id = reg_id("TYPE_DECL");

            map<std::string,g_ptr<Value>> keywords;

            void a_pass_resolve_keywords(list<g_ptr<Node>>& nodes, int context = -1) {
                for(g_ptr<Node>& node : nodes) {
                    log("Keyword resolving ",node_info(node));
                    g_ptr<Value> value = keywords.getOrDefault(node->name,fallback_value);
                    if(value!=fallback_value) {
                        for(auto v : keywords.getAll(node->name)) {
                            if(v->reg==-1||v->reg==context) { //By default is -1
                                node->value->copy(v);
                                node->value->reg = -1; //Reset to -1 for cleanliness
                            }
                        }
                    }
                    a_pass_resolve_keywords(node->children, node->value->sub_type);
                    for(auto scope : node->scopes) {
                        a_pass_resolve_keywords(scope->children, node->value->sub_type);
                    }
                }
            };



            list<size_t> discard_types;

            g_ptr<Value> make_registered_value(const std::string& name, size_t size,uint32_t sub_type, int context) {
                g_ptr<Value> value = make<Value>(sub_type,size,reg_id(name));
                value->reg = context;
                return value;
            }

            //A keyword which only resolves when it's the child of the type in context.
            size_t make_contextual_keyword(const std::string& name, int context = -1, std::string type_name = "") {
                g_ptr<Value> val = make_registered_value(type_name==""?name:type_name,0,0,context);
                keywords.put(name,val);
                return val->sub_type;
            }



            g_ptr<Value> make_value(const std::string& name, size_t size = 0,uint32_t sub_type = 0) {
                return make<Value>(sub_type,size,reg_id(name));
            }


            size_t make_keyword(const std::string& name, size_t size = 0, std::string type_name = "", uint32_t sub_type = 0) {
                g_ptr<Value> val = make_value(type_name==""?name:type_name,size,sub_type);
                keywords.put(name,val);
                return val->sub_type;
            }

            void add_alias(const std::string& name, uint32_t type) {
                keywords.put(name,keywords.get(labels[type]));
            }

            size_t add_function(const std::string& f, uint32_t return_type = 0) {
                g_ptr<Value> val = make_value(f,0,return_type);
                keywords.put(f,val);
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

            size_t add_type(const std::string& f, size_t size = 0) {
                g_ptr<Value> val = make_type(f,size);
                keywords.put(f,val);
                return val->type;
            }

            size_t add_qualifier(const std::string& f, size_t size = 0) {
                g_ptr<Value> val = make_qual_value(f,size);
                keywords.put(f,val);
                return val->type;
            }
    };
}