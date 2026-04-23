#pragma once

#include "../modules/GDSL-Script.hpp"

namespace GDSL {
    struct PineNeedle_Unit : public virtual Q_Script_Unit {
        PineNeedle_Unit() { init(); }

        size_t type_id = add_type("type");
        size_t eval_id = add_type("eval");
        size_t onform_id = make_tokenized_keyword("onform");

        size_t type_col_id = reg_id("type_col");

        size_t qscript_id = add_qualifier("qscript");

        size_t formed_id = add_qualifier("formed");
        size_t calculator_id = add_qualifier("calculator");

        void init() override {
            set_binding_powers(colon_id,4,6);
            n_handlers[onform_id] = [this](Context& ctx){
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->quals << copy_as_token(ctx.node);
                    ctx.node->quals << copy_as_token(ctx.result->get(ctx.index+1));
                    ctx.node->name = ctx.result->take(ctx.index+1)->name;
                }
            };

            d_handlers[func_decl_id] = [this](Context& ctx){
                if(ctx.node->value->type==type_id) {
                    for(auto c : ctx.node->scope()->children) {
                        if(c->type==func_decl_id) c->type = type_col_id; 
                    }
                    ctx.node->type = type_col_id;
                }
            };

            i_handlers[type_col_id] = [this](Context& ctx){
                g_ptr<Value> v = ctx.node->value;
                g_ptr<Type> t = nullptr;
                if(v->type==type_id) {
                    g_ptr<Type> new_type = make<Type>();
                    ctx.node->value->store = new_type;
                    standard_travel_pass(ctx.node->scope(),&ctx);
                    return;
                } else {
                    t = ctx.sub ? ctx.sub->node->value->store : nullptr;
                }

                if(t) {
                    if(!v->data) {
                        if(v->type==string_id) {v->set<std::string>("wub");}
                        else if(v->type==int_id) {v->set<int>(0);}
                        else if(v->type==float_id) {v->set<float>(0.0f);}
                        else {v->data = malloc(v->size);}
                    }
                    size_t col = t->new_column(ctx.node->name, v->data, v->size, v->type);

                    if(v->has_qual(qscript_id)) {
                        g_ptr<Q_Script_Unit> script = make<Q_Script_Unit>();
                        g_ptr<Node> script_root = script->process(ctx.node->scope()->left()->value->get<std::string>());
                        script->run(script_root);
                        t->set(col,0,script_root->left()->value->data);
                    } else {
                        g_ptr<Type> form = nullptr;
                        int formed_at = ctx.sub->node->value->find_qual(formed_id);
                        if(formed_at!=-1) {
                            g_ptr<Value> formqualv = ctx.sub->node->value->quals[formed_at]->value;
                            if(!formqualv->store) formqualv->store = make<Type>();
                            form = formqualv->store;
                        }
                        if(ctx.node->scope()) {
                            for(auto c : ctx.node->scope()->children) {
                                int col_idx = t->column_count()-1;
                                int row_idx = t->row_count(col_idx)-1;

                                if(c->type==onform_id) {
                                    if(form) {
                                        std::string cords = std::to_string(col_idx)+"-"+std::to_string(row_idx);
                                        size_t f_col = form->new_column<std::string>(c->name,cords,string_id);
                                        if(c->scope()) {
                                            for(auto c2 : c->scope()->children) {
                                                if(c2->type==colon_id) {
                                                    if(!c2->left()||!c2->right()) continue;
                                                    g_ptr<Value> rv = c2->right()->value;
                                                    form->push<std::string>(value_as_string(rv)+"-"+labels[rv->type]+"-"+c2->left()->name,f_col,string_id);
                                                }
                                            }
                                        }
                                    } else {
                                        print(red("func_decl:i_handler attempted onform in a type with no form"));
                                    }
                                } else {
                                    t->push(c->value->data,c->value->size,col,c->value->type);
                                }
                            }
                        }
                    }
                }
            };

            i_handlers[make_tokenized_keyword("from_disk")] = [this](Context& ctx) {
                g_ptr<Type> t = nullptr;
                if(ctx.sub) {
                    t = ctx.sub->node->value->store;
                }
                if(ctx.node->left()&&t) {
                    if(ctx.node->left()->value->type==string_id) {
                        std::string saved_name = ctx.sub->node->name;
                        ctx.sub->node->copy(loadBinary("savetest.wub"));
                        ctx.sub->node->name = saved_name;
                    }
                }
            };

            x_handlers[make_tokenized_keyword("to_disk")] = [this](Context& ctx){
                if(ctx.node->left()&&ctx.node->right()) {
                    if(ctx.node->left()->value->type==type_id&&ctx.node->right()->value->type==string_id) {
                        serialize_node(ctx.node->left()->value->type_scope->owner);
                        saveBinary("savetest.wub");
                    }
                }
            };

            x_handlers[make_tokenized_keyword("inspect")] = [this](Context& ctx){
                if(ctx.node->left()) {
                    if(ctx.node->left()->value->type==type_id) {
                        g_ptr<Node> tnode = ctx.node->left()->value->type_scope->owner;
                        g_ptr<Type> t = ctx.node->left()->value->type_scope->owner->value->store;
                        print(t->table_to_string(4));
                        print("3rd: ",t->get<int>(3,0));

                        if(tnode->has_qual(formed_id)) {
                            print("==FORM==");
                            g_ptr<Type> t2 = tnode->value->get_qual(formed_id)->value->store;
                            print(t2->table_to_string(4));
                            print("A COLUMN: ");
                            for(int i =0;i<t2->row_count(0);i++) {
                                print(t2->get<std::string>(0,i));
                            }
                        }
                    }
                }
            };
        }   
    };
}