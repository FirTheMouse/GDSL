#pragma once

#include "../modules/GDSL-Script.hpp"

namespace GDSL {
    struct GQL_Unit : public virtual Q_Script_Unit {
        GQL_Unit() { init(); }

        size_t query_id = make_tokenized_keyword("query");
        size_t query_first_id = make_contextual_keyword("first",query_id,"query_first");
        size_t query_last_id = make_contextual_keyword("last",query_id,"query_last");
        size_t query_in_id = make_contextual_keyword("in",query_id,"query_in");
        size_t query_tag_id = make_contextual_keyword("tag",query_id);
        size_t query_where_id = make_contextual_keyword("where",query_id);
        size_t query_size_id = make_contextual_keyword("size",query_id);
        size_t query_or_id = make_contextual_keyword("or",query_id,"query_or");
        size_t query_and_id = make_contextual_keyword("and",query_id,"query_and");

        g_ptr<Type> self = nullptr;

        bool eval_terms(Context& ctx, g_ptr<Node> node, _note& note, g_ptr<Type> t) {
            if(node->type == query_where_id) {
                return eval_terms(ctx, node->children[0], note, t);
            }
            if(node->type == query_and_id) {
                return eval_terms(ctx, node->children[0], note, t) 
                    && eval_terms(ctx, node->children[1], note, t);
            }
            if(node->type == query_or_id) {
                return eval_terms(ctx, node->children[0], note, t) 
                    || eval_terms(ctx, node->children[1], note, t);
            }
            if(node->type == query_tag_id) {
                process_node(ctx, node->children[0]);
                return note.tag == node->children[0]->value->get<uint32_t>();
            }
            if(node->type == query_size_id) {
                process_node(ctx, node->children[0]);
                int expected = node->children[0]->value->get<int>();
                return t->columns[note.index].element_size == expected;
            }
            return true;
        };

        void init() override {

             //Aliases
             make_tokenized_keyword("find",query_id); 


            n_handlers[query_id] = [this](Context& ctx) {
                for(int i = 0; i<ctx.node->children.length();i++) { //First, extract all the children
                    g_ptr<Node> c = ctx.node->children[i];
                    if(!c->children.empty()) {
                        for(int j=c->children.length()-1;j>=0;j--) {
                            ctx.node->children.insert(c->children.take(j),i+1);
                        }
                    }
                }
                a_pass_resolve_keywords(ctx.node->children,query_id);
                for(int i = 0; i<ctx.node->children.length();i++) { //Then type them all
                    g_ptr<Node> c = ctx.node->children[i];
                    if(c->value->sub_type!=0) {
                        c->type = c->value->sub_type;
                    }
                }

                start_stage(a_handlers);
                standard_direct_pass(ctx.node); //Now let them do takes and resolve opperations
                start_stage(n_handlers);
            };

            a_handlers[query_first_id] = [this](Context& ctx){
                if(ctx.result->get(ctx.index+1)->type!=query_in_id)
                    n_take_right(ctx,1);
            };
            a_handlers[query_last_id] = a_handlers[query_first_id];

            a_handlers[query_tag_id] = [this](Context& ctx){
                n_take_right(ctx,1);
            };

            a_handlers[query_size_id] = [this](Context& ctx){
                n_take_right(ctx,1);
            };

            a_handlers[query_in_id] = [this](Context& ctx){
                n_take_left(ctx,1);
                n_take_right(ctx,2);
            };

            a_handlers[query_where_id] = [this](Context& ctx){
                n_take_right(ctx,2);
            };
            a_handlers[query_or_id] = [this](Context& ctx){
                n_take_left(ctx,1);
                n_take_right(ctx,2);
            };
            a_handlers[query_and_id] = [this](Context& ctx){
                n_take_left(ctx,1);
                n_take_right(ctx,2);
            };

            x_handlers[query_first_id] = [this](Context& ctx){
                if(ctx.node->left()) {
                    ctx.node->value->data = ctx.node->left()->value->data;
                } else {
                    ctx.node->value->set<int>(1);
                }
            };
            x_handlers[query_last_id] = x_handlers[query_first_id];

            x_handlers[query_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node->value = ctx.node->left()->value;
            };

            x_handlers[query_in_id] = [this](Context& ctx){
                if(ctx.node->children.length()==3) {
                    g_ptr<Node> mod = ctx.node->children[0];
                    g_ptr<Type> t = ctx.node->children[1]->value->store;
                    g_ptr<Node> terms = ctx.node->children[2];
                    ctx.node->value->store = t;
                    process_node(ctx,mod);
                    int amt = mod->value->get<int>();
                    bool get_last = mod->type==query_last_id?true:false;
                    bool accumulate = amt>1;

                    g_ptr<Type> temp_store = make<Type>();

                    for(
                        int i = (get_last ? t->array.length()-1 : 0) ; 
                        (get_last ? i>=0 : i<t->array.length()) ; 
                        (get_last ? i-- : i++) ) 
                    {
                        _note& note = t->array[i];
                        ctx.node->value->query_store(i);
                        bool meets_terms = eval_terms(ctx, terms, note, t);
                        if(meets_terms) {
                            ctx.node->value->query_store(i);
                            amt--;
                            if(accumulate) {
                                ctx.node->value->store_value(temp_store);
                            } 
                            if(amt<=0) {
                                if(accumulate) {
                                    ctx.node->value->set<g_ptr<Type>>(temp_store);
                                    ctx.node->value->type = store_id;
                                }
                                return;
                            }
                        }
                    }
                }
            };


            x_handlers[literal_id] = [this](Context& ctx){
                if(self) {
                    if(ctx.node->left()) {
                        process_node(ctx, ctx.node->left());
                        _type_image img = self->get_image();
                        int col = 0;
                        if(ctx.node->value->type==int_id) {
                            col = ctx.node->value->get<int>();
                        } else if(ctx.node->value->type==string_id) {
                            for(auto cimg : img.columns) {
                                if(cimg.label==ctx.node->value->get<std::string>()) {
                                    col = cimg.index;
                                    break;
                                }
                            }
                        }
                        int row = ctx.node->left()->value->get<int>();
                        ctx.node->value->data = self->get(col,row);
                        ctx.node->value->size = img.columns[col].size;
                        ctx.node->value->type = img.columns[col].tag;
                    }
                }
            };

        }
    };
}