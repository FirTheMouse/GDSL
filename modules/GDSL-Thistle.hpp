#pragma once

#include "../modules/GDSL-TwigSnap.hpp"

namespace GDSL {
    struct Thistle_Unit : public virtual TwigSnap_DSL_Frontend {
        Thistle_Unit() { init(); }

        size_t sheet_id = make_tokenized_keyword("sheet");
        size_t form_id = make_tokenized_keyword("form");
        size_t test_sheet_id = make_tokenized_keyword("test_sheet");
        size_t test_form_id = make_tokenized_keyword("test_form");
        size_t recive_sheet_edit_id = make_tokenized_keyword("recive_sheet_edit");

        struct style_manager : public q_object {
            style_manager(HTML_Unit* _unit) : unit(_unit) {}
            style_manager(HTML_Unit* _unit, list<std::string> init) : unit(_unit) {
                for(auto s : init) add_prop(s);
            }
            HTML_Unit* unit;
            list<g_ptr<Node>> props;
            list<std::string> prop_names;

            void add_prop(const std::string& name, g_ptr<Node> prop = nullptr) {
                props << prop;
                prop_names << name;
            }

            void match_prop(const std::string& name, g_ptr<Node> prop) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name) {
                        props[i] = prop;
                        return;
                    }
                }
            }

            std::string resolve_prop(Context& ctx, const std::string& name) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name&&props[i]) {
                        return unit->emit_inline_html(ctx,props[i]);
                    }
                }
                return "";
            }
        };

        void init() override {
            g_ptr<Type> ts = make<Type>();
            ts->add<int>("one",1,-1,int_id);
            ts->add<int>("two",2,-1,int_id);
            ts->add<int>("three",3,-1,int_id);
            ts->push<int>(5,0);
            ts->push<int>(8,1);
            ts->push<int>(12,2);
            
            g_ptr<Type> tf = make<Type>();
            tf->add<std::string>("Direct input","0-0",-1,string_id);
            tf->push<std::string>("",0);
            
            tf->add<std::string>("Wumbers","1-0",-1,string_id);
            tf->push<std::string>("0-int-Default",1);
            tf->push<std::string>("1-int-First",1);
            tf->push<std::string>("2-int-Second",1);
            tf->push<std::string>("3-int-Third",1);
            
            tf->add<std::string>("Twubmbers","1-1",-1,string_id);
            tf->push<std::string>("0-int-Default",2);
            tf->push<std::string>("11-int-Firsty",2);
            tf->push<std::string>("22-int-Secondy",2);
            tf->push<std::string>("33-int-Thirdy",2);
            
            x_handlers[test_form_id] = [this,tf](Context& ctx) {
                ctx.node->value->store = tf;
            };
            x_handlers[test_sheet_id] = [this,ts](Context& ctx) {
                ctx.node->value->store = ts;
            };
            
            r_handlers[form_id] = [this](Context& ctx){
                standard_gather_from_scope(ctx);
            };
            html_handlers[form_id] = [this](Context& ctx) {
                if(ctx.node->left()) {
                    process_node(ctx.node->left());
                    ctx.node->value->store = ctx.node->left()->value->store;
                    ctx.node->name =  ctx.node->left()->name;
                    ctx.node->in_scope->node_table[ctx.node->name] = ctx.node;
                } 
                std::string target_sheet = "";
                if(ctx.node->right()) {
                    process_node(ctx.node->right());
                    if(ctx.node->right()->value->type==string_id) {
                        target_sheet = ctx.node->right()->value->get<std::string>();
                    }
                }

                g_ptr<Type> t = ctx.node->value->store;
                _type_image img = t->get_image();
            

                g_ptr<style_manager> styles = make<style_manager>(this);

                std::string out = "<div class='form'";
                if(ctx.node->scope()) {
                    out += emit_inline_html(ctx, ctx.node->scope());
                    for(auto c : ctx.node->scope()->children) {
                        if(c->scope()) {
                            styles->add_prop(c->name,c->scope());
                        }
                    }    
                }
                out += ">\n";

                if(ctx.node->scope()) {
                    for(auto c : ctx.node->scope()->children) {
                        if(!c->has_qual(invisible_id))
                            out += html_encode_node(c);
                    }
                }

                for(auto& col_img : img.columns) {
                    std::string addr = t->get<std::string>(col_img.index, 0);
                    std::string col_str = split_str(addr,'-')[0];
                    std::string row_str = split_str(addr,'-')[1];
                    
                    std::string label_sub_style = "";
                    list<std::string> split_col = split_str(col_img.label,':');
                    if(split_col.length()>1) {
                        label_sub_style = split_col[1]+"_";
                    }

                    out += "<div "; 
                    out += styles->resolve_prop(ctx,"field_style");
                    out+=" class='field'>\n<label "; 
                    out += styles->resolve_prop(ctx,label_sub_style+"label_style");
                    out+=">\n" + split_col[0] + "\n</label>\n";
            
                    std::string current = t->columns[col_img.index].length() > 1 ? t->get<std::string>(col_img.index, 1) : "";
                    
                    if(t->columns[col_img.index].length() <= 2) {
                        out += "<input ";
                        out += styles->resolve_prop(ctx,"input_style");
                        out += " value='"+current+"' onchange=\""
                        +"cell_post(this,''," + col_str + "," + row_str + ",'" + target_sheet + "');" 
                        +"cell_post(this,''," + std::to_string(col_img.index) + ",1,'" + ctx.node->name + "')\""
                        +">\n";
                    } else {
                        out += "<select "; 
                        out += styles->resolve_prop(ctx,"select_style");
                        out += " value='"+current+"' onchange=\""
                        +"cell_post(this,''," + col_str + "," + row_str + ",'" + target_sheet + "');"
                        +"cell_post(this,''," + std::to_string(col_img.index) + ",1,'" + ctx.node->name + "')\""
                        +">\n";
                        for(int r = 2; r < t->columns[col_img.index].length(); r++) {
                            std::string option = t->get<std::string>(col_img.index, r);
                            list<std::string> opt = split_str(option,'-');
                            std::string val = opt[0];
                            std::string tag = opt[1];
                            std::string label = opt[2];
                            std::string selected = (val == current) ? " selected" : "";
                            out += "<option value='" + val + "'" + selected + ">\n" + label + "\n</option>\n";
                        }
                        out += "</select>\n";
                    }
                    out += "</div>\n";
                }
                out += "</div>";
                ctx.source = out;
            };
            
            x_handlers[recive_sheet_edit_id] = [this](Context& ctx) {
                list<std::string> elements = split_str(ctx.sub->source,' ');
                std::string note_name = "";
                int column = 0;
                int row = 0;
                std::string value = "";
                std::string target = "";
                if(elements.length()==5) {
                    note_name = elements[0];
                    column = std::stoi(elements[1]);
                    row = std::stoi(elements[2]);
                    value = elements[3];
                    target = elements[4];
                } else if(elements.length()==4) {
                    column = std::stoi(elements[0]);
                    row = std::stoi(elements[1]);
                    value = elements[2];
                    target = elements[3];
                } else if(elements.length()<4) {
                    print(red("recive_sheet_edit:x_handler invalid source: "+ctx.sub->source));
                    return;
                }
                g_ptr<Node> sheet_node = scan_for_node(target,ctx.sub->root);
                if(!sheet_node) {
                    print(red("COULD NOT FIND THE TYPE"));
                    return;
                }
                g_ptr<Type> ts = sheet_node->value->store;
            
                print(red("EDIT: "+value+" "+target));
                int tag = 0;
                int esize = ts->columns[column].element_size;
                if(note_name=="") {
                    if(esize==4) {
                        tag = int_id;
                    } else if(esize==1) {
                        tag = bool_id;
                    } else if(esize==sizeof(std::string)) {
                        tag = string_id;
                    }
                } else {
                    _note& note = ts->get_note(note_name);
                    tag = note.tag;
                }
            
            
                if(tag == int_id) {
                    int val = std::stoi(value);
                    ts->set(column,row,(void*)&val);
                } else if(tag == float_id) {
                    float val = std::stof(value);
                    ts->set(column,row,(void*)&val);
                } else if(tag == bool_id) {
                    bool val = (value=="true");
                    ts->set(column,row,(void*)&val);
                } else if(tag == string_id) {
                    ts->set(column,row,(void*)&value);
                } else {
                    print(red("cell_edit:post_handler unrecognized tag: "+labels[tag]));
                }
            };
            

            r_handlers[sheet_id] = [this](Context& ctx){
                standard_gather_from_scope(ctx);
            };
            html_handlers[sheet_id] = [this](Context& ctx) {
                if(ctx.node->left()) {
                    process_node(ctx.node->left());
                    ctx.node->value->store = ctx.node->left()->value->store;
                    ctx.node->name =  ctx.node->left()->name;
                    ctx.node->in_scope->node_table[ctx.node->name] = ctx.node;
                } 

                g_ptr<style_manager> styles = make<style_manager>(this);

                g_ptr<Type> t = ctx.node->value->store;
                _type_image img = t->get_image();
                std::string out = "";
                out += "<table id='" + ctx.node->name + "' ";
                if(ctx.node->scope()) {
                    out += emit_inline_html(ctx, ctx.node->scope());
                    for(auto c : ctx.node->scope()->children) {
                        if(c->scope()) {
                            styles->add_prop(c->name,c->scope());
                        }
                    }    
                }
                out += ">\n";

                if(ctx.node->scope()) {
                    for(auto c : ctx.node->scope()->children) {
                        if(!c->has_qual(invisible_id))
                            out += html_encode_node(c);
                    }
                }
                
                out+= "<tr "; 
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">\n";

                for(auto& col_img : img.columns) {
                    out += "<th";
                    out+=styles->resolve_prop(ctx, "header_style"); 
                    out+= ">\n"+col_img.label;
                    out += "\n</th>\n";
                }
                out += "</tr>\n";
                
                for(int r = 0; r < img.row_count; r++) {
                    out+= "<tr "; 
                    out+=styles->resolve_prop(ctx, "row_style"); 
                    out+=">\n";
                    for(auto& col_img : img.columns) {
                        std::string note_label = col_img.label;
                        int access_index = col_img.index;
                        int access_sub_index = r;
            
                        if(r < col_img.cells.length() && col_img.cells[r]) {
                            _note& note = *col_img.cells[r];
                            //Future metadata hooks in here
                        }
            
                        std::string column =  std::to_string(access_index);
                        std::string row = std::to_string(access_sub_index);
            
                        out += "<td ";
                        out+=styles->resolve_prop(ctx, "column_style"); 
                        out+=">\n<input "; 
                        out+=styles->resolve_prop(ctx, "input_style"); 
                        out+=" value=\""+value_to_string.get(col_img.tag)(t->columns[access_index].get(access_sub_index))+"\""
                        + " onchange=\"cell_post(this,'" + note_label + "'," + column + "," + row + ",'" + ctx.node->name + "')\""
                        +"/>\n</td>\n";
                    }
                    out += "\n</tr>\n";
                }
                
                out += "</table>\n";
                ctx.source = out;
            };
        }
    };
}

