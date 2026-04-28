#pragma once

#include "../modules/GDSL-TwigSnap.hpp"
#include "../modules/GDSL-PineNeedle.hpp"
#include "../modules/GDSL-GQL.hpp"

namespace GDSL {
    struct Thistle_Unit : public virtual TwigSnap_DSL_Frontend, public virtual PineNeedle_Unit {
        Thistle_Unit() { init(); }

        size_t sheet_id = make_tokenized_keyword("sheet");
        size_t form_id = make_tokenized_keyword("form");
        size_t test_sheet_id = make_tokenized_keyword("test_sheet");
        size_t test_form_id = make_tokenized_keyword("test_form");
        size_t recive_sheet_edit_id = make_tokenized_keyword("recive_sheet_edit");

        list<g_ptr<Thistle_Unit>> twig_daycare;

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

        void recive_sheet_edit(Context& ctx){
            list<std::string> elements = split_str(ctx.sub->source,' ');
            std::string note_name = "";
            int column = 0;
            int row = 0;
            std::string value = "";
            std::string target = "";
            for(auto e : elements) print(e);
            if(elements.length()>4) {
                note_name = elements[0];
                column = std::stoi(elements[1]);
                row = std::stoi(elements[2]);
                target = elements[3];
                for(int i=4;i<elements.length();i++) {
                    value+=elements[i];
                    if(i!=elements.length()-1) {value+=" ";}
                }
            } else if(elements.length()<=4) {
                print(red("recive_sheet_edit:x_handler invalid source: "+ctx.sub->source));
                return;
            }
            g_ptr<Node> sheet_node = scan_for_node(target,ctx.sub->node->scope());
            if(!sheet_node) {
                print(red("COULD NOT FIND THE TYPE"));
                return;
            }
            g_ptr<Type> ts = sheet_node->value->store;
            if(!ts) {
                print(red("SHEET NODE HAS NO SHEET: make sure the right sheet name is being found!"));
                return;
            }
        
            print(red("EDIT: "+value+" "+target));
            // print(ts->table_to_string(4));

            int tag = 0;
            int esize = ts->columns[column].element_size;
            if(note_name=="") {
                //This is why Acorn's column tags are better
                for(auto e : ts->notes.entrySet()) {
                    if(e.value.index==column) {
                        tag = e.value.tag;
                        break;
                    }
                }
                // if(esize==4) {
                //     tag = int_id;
                // } else if(esize==1) {
                //     tag = bool_id;
                // } else if(esize==sizeof(std::string)) {
                //     tag = string_id;
                // }
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
            } else if(tag == string_ptr_id) {
                set_string(ts->get<Ptr>(column,row),value);
            } else {
                print(red("cell_edit:post_handler unrecognized tag: "+labels[tag]));
            }

            //And run scripts
            if(g_ptr<Node> scriptqual = sheet_node->value->get_qual(scripted_id)) {
                if(g_ptr<Type> scripts = scriptqual->value->store) {
                    for(int c = 0; c<scripts->column_count();c++) {
                        for(int r = 0; r<scripts->row_count(c);r++) {
                            Ptr script_ticket = scripts->get<Ptr>(c, r);
                            if(script_ticket.idx!=0) {
                                g_ptr<GQL_Unit> script_unit = make<GQL_Unit>();
                                script_unit->self = ts;
                                std::string codestr = retrive_string(script_ticket);
                                g_ptr<Node> script_root = script_unit->process(codestr);
                                script_unit->run(script_root);
                                if(script_root->left() && script_root->left()->value->data) {
                                    ts->set(c, r, script_root->left()->value->data);
                                }
                            }
                        }
                    }
                }
            }
        }

        void init() override {
            g_ptr<Type> ts = make<Type>();
            uint32_t one_id = ts->new_column<int>("one",1,int_id);
            ts->push<int>(5,one_id);

            uint32_t two_id = ts->new_column<int>("two",2,int_id);
            ts->push<int>(8,two_id);

            uint32_t three_id = ts->new_column<int>("three",3,int_id);
            ts->push<int>(12,three_id);
            
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



            x_handlers[make_tokenized_keyword("fragment_highlight")] = [this](Context& ctx) {
                std::string source = ctx.sub->source;
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                std::string out = "";
                g_ptr<Thistle_Unit> twig = make<Thistle_Unit>();
                if(instruction=="compile") {
                    Log::Line l; l.start();
                    g_ptr<Node> root = twig->process(content);
                    twig->resolve_and_evaluate(root);
                    double a_time = l.end(); l.start();
                    out += twig->nodenet_to_highlighted(root, content);
                    double b_time = l.end(); 
                    // l.start();
                    //print_root(root);
                    // double c_time = l.end();

                    print("A: ",ftime(a_time));
                    print("B: ",ftime(b_time));
                    //print("C: ",ftime(c_time));
                } else if(instruction=="end") {
                    print("REQUEST TO END: ",target," OF ",servers.length());
                    g_ptr<Server> to_end = get_server(target);
                    if(to_end) {
                        ::close(to_end->fd); 
                        to_end->fd = -1;
                        to_end->thread->end();
                        servers.erase(to_end);
                    } else {
                        print(red("Unable to find server "+target+" to end"));
                    }
                } else if(instruction=="preview") {
                    twig_daycare << twig;
                    g_ptr<Node> root = twig->process(content);
                    twig->run(root);
                    int port_num = 8082;
                    for(auto c : root->children) {
                        if(c->type==server_id) {
                            for(auto sc : c->scope()->children) {
                                if(sc->type==port_id) {
                                    port_num = sc->left()->value->get<int>();
                                }
                            }
                        }
                    }
                    servers << twig->servers;
                    servers.last()->label = target;
                    print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
                    out = std::to_string(port_num);
                } else if(instruction=="read") {
                    out = readFile(content);
                } else {
                    print(red("Unrecognized instruction for fragment: "+ctx.sub->source));
                }
                ctx.sub->source = out;
            };

            // n_handlers[form_id] = [this](Context& ctx){
            //     if(ctx.index+1<ctx.result->length()) {
            //         ctx.node->quals << copy_as_token(ctx.node);
            //         ctx.node->quals << copy_as_token(ctx.result->get(ctx.index+1));
            //         ctx.node->name = ctx.result->take(ctx.index+1)->name;
            //     }
            // };
            r_handlers[form_id] = [this](Context& ctx){
                standard_gather_from_scope(ctx);
            };
            html_handlers[form_id] = [this](Context& ctx) {
                std::string target_sheet = "sheet";
                if(ctx.node->left()) {
                    if(ctx.node->left()->value->has_qual(formed_id)) {
                        ctx.node->value = ctx.node->left()->value->get_qual(formed_id)->value;
                    } else {
                        process_node(ctx.node->left());
                        if(ctx.node->left()->value->type_scope) {
                            if(ctx.node->left()->value->type_scope->owner->value->has_qual(formed_id)) {
                                ctx.node->left()->value->store = ctx.node->left()->value->type_scope->owner->value->get_qual(formed_id)->value->store;
                            }
                        }
                        ctx.node->value->store = ctx.node->left()->value->store;
                    }
                } 
                if(ctx.node->right()) {
                    process_node(ctx.node->right());
                    if(ctx.node->right()->value->type==string_id) {
                        target_sheet = ctx.node->right()->value->get<std::string>();
                    }
                }
                if(!ctx.node->value->store) {
                    print(red("form:html_handler invalid node provided, no form qual found"));
                    return;
                }
                g_ptr<Type> t = ctx.node->value->store;
                _type_image img = t->get_image();            

                g_ptr<style_manager> styles = make<style_manager>(this);

                std::string out = "<div class='form' id='form'";
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
                    std::string addr = "";
                    if(col_img.tag==string_ptr_id) {
                        addr = retrive_string(*(Ptr*)t->columns[col_img.index].get(0));
                    } else {
                        addr = t->get<std::string>(col_img.index, 0);
                    }
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
            
                    std::string current = "";
                    if(t->columns[col_img.index].length() > 1) {
                        if(col_img.tag==string_ptr_id) {
                            current = retrive_string(*(Ptr*)t->columns[col_img.index].get(1));
                        } else {
                            current = t->get<std::string>(col_img.index, 1);
                        }
                    }
                    
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
                            std::string option = "";
                            if(col_img.tag==string_ptr_id) {
                                option = retrive_string(*(Ptr*)t->columns[col_img.index].get(r));
                            } else {
                                option = t->get<std::string>(col_img.index, r);
                            }
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
                recive_sheet_edit(ctx);
            };

            x_handlers[make_tokenized_keyword("thistle_editor")] = [this](Context& ctx){
                std::string source = ctx.sub->source;
                print("RECIVED: ",source);
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                std::string out = "";
                g_ptr<Node> node = ctx.sub->node;
                if(instruction=="toggle_visibility") {
                    if(node->mute) {
                        node->mute = false;
                        out = html_encode_node(node);
                    } else {
                        node->mute = true;
                        out = "<div id=\""+target+"\"></div>";
                    }
                } else if(instruction=="add_column") {
                    g_ptr<Type> t = node->value->store;
                    t->new_column<int>("NEW",0,int_id);
                    out = html_encode_node(node);
                } else if(instruction=="add_row") {
                    g_ptr<Type> t = node->value->store;
                    for(int i=0;i<t->column_count();i++) t->add_row(i);
                    out = html_encode_node(node);
                } else if(instruction=="swap") {
                    for(auto c : ctx.sub->root->children) {
                        if(c->type==type_col_id&&c->name==content) {
                            node->left()->value = c->value;
                            break;
                        }
                    }
                    out = html_encode_node(node);
                } else {
                    out = html_encode_node(node);
                }

                ctx.sub->source = out;
            };
            

            // n_handlers[sheet_id] = [this](Context& ctx){
            //     if(ctx.index+1<ctx.result->length()) {
            //         ctx.node->quals << copy_as_token(ctx.node);
            //         ctx.node->quals << copy_as_token(ctx.result->get(ctx.index+1));
            //         ctx.node->name = ctx.result->take(ctx.index+1)->name;
            //     }
            // };
            r_handlers[sheet_id] = [this](Context& ctx){
                standard_gather_from_scope(ctx);
            };
            html_handlers[sheet_id] = [this](Context& ctx) {
                if(ctx.node->left()) {
                    process_node(ctx.node->left());
                    if(ctx.node->left()->value->type==type_id) {
                        ctx.node->left()->value = ctx.node->left()->value->type_scope->owner->value;
                    }
                    ctx.node->value = ctx.node->left()->value;
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
                        std::string as_string = "";
                        if(r>=t->columns[access_index].length()) {
                            continue;
                        } else if(col_img.tag==string_ptr_id) {
                            as_string = retrive_string(*(Ptr*)t->columns[access_index].get(access_sub_index));
                        } else if(value_to_string.hasKey(col_img.tag)) {
                            as_string = value_to_string.get(col_img.tag)(t->columns[access_index].get(access_sub_index));
                        } else {
                           as_string = "[missing value to string for "+labels[col_img.tag]+"]";
                        }

                        out += "<td ";
                        out+=styles->resolve_prop(ctx, "column_style"); 
                        out+=">\n<input "; 
                        out+=styles->resolve_prop(ctx, "input_style"); 
                        out+=" value=\""+as_string+"\""
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

