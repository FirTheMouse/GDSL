#pragma once

#include "../modules/Q-TAST.hpp"

namespace GDSL {
    struct Literals_Unit : public virtual TAST_Unit {

        size_t float_id = add_type("float",4);
        size_t int_id = add_type("int",4);
        size_t bool_id = add_type("bool",1);
        size_t string_id = add_type("string",24);

        void init() override {
            value_to_string[object_id] = [](void* data) {
                return std::string("[object @") + std::to_string((size_t)data) + "]";
            };
            x_handlers[literal_id] = [](Context& ctx){};
        
            value_to_string[float_id] = [](void* data) {
                return std::to_string(*(float*)data);
            };
            negate_value[float_id] = [](void* data) {
                *(float*)data = -(*(float*)data);
            };
            t_handlers[float_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node->type = literal_id;
                try {
                    g_ptr<Value> value = make<Value>(float_id,4);
                    value->set<float>(std::stof(ctx.node->name));
                    ctx.node->value = value;
                } catch(std::exception& e) {
                    attach_error(ctx.node, major_error, "float:t_handler Name "+ctx.node->name+" is not valid for conversion to float");
                }
            }; 
    
            value_to_string[int_id] = [](void* data) {
                return std::to_string(*(int*)data);
            };
            negate_value[int_id] = [](void* data) {
                *(int*)data = -(*(int*)data);
            };
            t_handlers[int_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node->type = literal_id;
                try {
                    g_ptr<Value> value = make<Value>(int_id,4);
                    value->set<int>(std::stoi(ctx.node->name));
                    ctx.node->value = value;
                } catch(std::exception& e) {
                    attach_error(ctx.node, major_error, "int:t_handler Name "+ctx.node->name+" is not valid for conversion to int");
                }
            }; 
    
            value_to_string[bool_id] = [](void* data){
                return (*(bool*)data) ? "TRUE" : "FALSE";
            };
            t_handlers[bool_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node->type = literal_id;
    
                g_ptr<Value> value = make<Value>(bool_id,1);
                value->set<bool>(ctx.node->name=="true" ? true : false); 
                ctx.node->value = value;
            }; 
    
    
            value_to_string[string_id] = [this](void* data){
                return *(std::string*)data;
            };
            t_handlers[string_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node->type = literal_id;
                g_ptr<Value> value = make<Value>(string_id,24);
                value->set<std::string>(ctx.node->name);
                ctx.node->value = value;
            }; 
        }
    };
}