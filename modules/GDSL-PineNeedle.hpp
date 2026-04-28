#pragma once

#include "../modules/GDSL-Script.hpp"

namespace GDSL {
    struct PineNeedle_Unit : public virtual Q_Script_Unit {
        PineNeedle_Unit() { init(); }

        size_t type_id = add_type("type");
        size_t eval_id = add_type("eval");
        size_t onform_id = make_tokenized_keyword("onform");
        size_t onscript_id = make_tokenized_keyword("onscript");

        size_t type_col_id = reg_id("type_col");

        size_t string_ptr_id = reg_id("string_ptr");

        size_t formed_id = add_qualifier("formed");
        size_t scripted_id = add_qualifier("scripted");
        size_t calculator_id = add_qualifier("calculator");

        g_ptr<Type> string_daycare = make<Type>(); uint32_t string_daycare_id = 0;

        std::string retrive_string(Ptr& ticket){
            std::string to_return = "";
            if(ticket.idx!=0) {
                _column& get_from = types[ticket.pool]->columns[ticket.idx];
                for(int h=0;h<get_from.length();h++) {
                    to_return+=(*(char*)get_from.get(h));
                }
            }
            return to_return;
        }

        Ptr store_string(const std::string& str){
            uint32_t str_col = string_daycare->column_count(); 
            string_daycare->add_column(sizeof(char));
            for(char ch : str) {string_daycare->push<char>(ch, str_col);}
            Ptr str_ticket{string_daycare_id,str_col,0};
            return str_ticket;
        }

        void set_string(Ptr& ticket, const std::string& str) {
            if(ticket.idx!=0) {
                _column& get_from = types[ticket.pool]->columns[ticket.idx];
                get_from.storage.clear();
                for(char ch : str) {get_from.push((void*)&ch);}
            }
        }

        void init() override {
            string_daycare->add_column(); //For null
            string_daycare_id = types.length(); types << string_daycare; 
            set_binding_powers(colon_id,4,6);
            n_handlers[onform_id] = [this](Context& ctx){
                if(ctx.index+1<ctx.result->length()) {
                    ctx.node->quals << copy_as_token(ctx.node);
                    ctx.node->quals << copy_as_token(ctx.result->get(ctx.index+1));
                    ctx.node->name = ctx.result->take(ctx.index+1)->name;
                }
            };

            n_handlers[onscript_id] = [this](Context& ctx){
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
                    Ptr null_col{0,0,0};
                    size_t col = 0;
                    if(v->type==string_id) {
                        col = t->note_value(ctx.node->name,sizeof(Ptr),string_ptr_id).index;
                    } else {
                        col = t->note_value(ctx.node->name, v->size, v->type).index;
                    }

                    g_ptr<Type> form = nullptr;
                    int formed_at = ctx.sub->node->value->find_qual(formed_id);
                    if(g_ptr<Node> formqual = ctx.sub->node->value->get_qual(formed_id)) {
                        if(!formqual->value->store) formqual->value->store = make<Type>();
                        form = formqual->value->store;
                    }

                    g_ptr<Type> scripts = nullptr;
                    if(g_ptr<Node> scriptqual = ctx.sub->node->value->get_qual(scripted_id)) {
                        if(!scriptqual->value->store) scriptqual->value->store = make<Type>();
                        scripts = scriptqual->value->store;
                    }

                    if(ctx.node->scope()) {
                        for(auto c : ctx.node->scope()->children) {
                            int col_idx = t->column_count()-1;
                            int row_idx = t->row_count(col_idx)-1;

                            if(c->type==onform_id) {
                                if(form) {
                                    std::string cords = std::to_string(col_idx)+"-"+std::to_string(row_idx);
                                    size_t f_col = form->new_column<Ptr>(c->name,store_string(cords),string_ptr_id);
                                    form->push<Ptr>(store_string(""),f_col,string_ptr_id);
                                    if(c->scope()) {
                                        for(auto c2 : c->scope()->children) {
                                            if(c2->type==colon_id) {
                                                if(!c2->left()||!c2->right()) continue;
                                                g_ptr<Value> rv = c2->right()->value;
                                                form->push<Ptr>(store_string(value_as_string(rv)+"-"+labels[rv->type]+"-"+c2->left()->name),f_col,string_ptr_id);
                                            }
                                        }
                                    }
                                } else {
                                    print(red("func_decl:i_handler attempted onform in a type with no form"));
                                }
                            } else if(c->type==onscript_id) {
                                Ptr null_col{0,0,0};
                                while(col_idx>=scripts->column_count()) scripts->new_column<Ptr>("",null_col,string_ptr_id);
                                while(row_idx>=scripts->row_count(col_idx)) scripts->add_row(col_idx);
                            
                                scripts->set<Ptr>(col_idx,row_idx,store_string(c->name));
                            } else {
                                if(c->value->type==string_id) {
                                    t->push<Ptr>(store_string(c->name),col_idx,string_ptr_id);
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