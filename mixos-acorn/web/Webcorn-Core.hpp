#pragma once

#include "../Acorn-Script.hpp"
#include "../../ext/g_lib/core/thread.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET(fd) closesocket(fd)
    #define READ_SOCKET(fd, buf, len) recv(fd, buf, len, 0)
    #define WRITE_SOCKET(fd, buf, len) send(fd, buf, len, 0)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>

    #include <mach/mach.h>

    #define _UUID_T
    typedef unsigned char uuid_t[16];

    #define CLOSE_SOCKET(fd) ::close(fd)
    #define READ_SOCKET(fd, buf, len) ::read(fd, buf, len)
    #define WRITE_SOCKET(fd, buf, len) ::write(fd, buf, len)
#endif



size_t current_memory_usage() {
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size);
    return info.resident_size; // current RSS in bytes
}

namespace Acorn {
    struct Webcorn_Core : public virtual Acorn_Script {
        Webcorn_Core(uint16_t _uid) : Unit(_uid) {init();}
        Webcorn_Core() {init();}

        struct Server : q_object {
            int fd;
            std::string label;
            g_ptr<Thread> thread;
            Unit* unit;
        };
    
        list<g_ptr<Server>> servers;
    
        g_ptr<Server> get_server(int fd) {
            for(auto& s : servers) {
                if(s->fd == fd) return s;
            }
            return nullptr;
        }
    
        g_ptr<Server> get_server(const std::string& label) {
            for(auto& s : servers) {
                if(s->label == label) return s;
            }
            return nullptr;
        }

        uint32_t property_id = reg_id("property");
        uint32_t properties_id = reg_id("properties");
        uint32_t inlined_id = reg_id("inlined"); uint32_t suffix_inlined_id = reg_id("suffix_inlined"); uint32_t prefix_inlined_id = reg_id("prefix_inlined");
        uint32_t invisible_id = reg_id("invisible"); uint32_t suffix_invisible_id = reg_id("suffix_invisible"); uint32_t prefix_invisible_id = reg_id("prefix_invisible");
        uint32_t component_id = reg_id("component"); uint32_t suffix_component_id = reg_id("suffix_component"); uint32_t prefix_component_id = reg_id("prefix_component");

        uint32_t find_node_id = make_tokenized_keyword("find_node");

        Stage& html_handlers = reg_stage("htmlemiting");

        _lookup is_structural{{
            "id", "class", "name", "type",
            "value", "placeholder", "checked", "disabled",
            "readonly", "required", "selected", "multiple",
            "action", "method", "for", "maxlength", "min", "max", 
            "step", "href", "src", "alt", "target", "rel",
            "tabindex", "contenteditable", "draggable", "hidden",
            "onclick", "onchange", "onsubmit", "oninput",
            "onfocus", "onblur", "onkeydown", "onkeyup",
            "onmouseenter", "onmouseleave", "onload", "onmouseover",
            "role","lang","colspan", "rowspan", "scope",
            "rows", "cols", "autocorrect", "autocapitalize", "spellcheck", "wrap",
            "autocomplete", "autofocus", "enctype", "novalidate", "pattern", "size",
            "download", "controls", "autoplay", "loop", "muted", "poster"
        }, false};
        bool is_prop_structural(const std::string& name) {
            return is_structural[name] || name.substr(0,5) == "data-";
        }

        void emit_inline_html(Context& ctx) {
            if(ctx.node().mute()) return;
            std::string s = "";
            list<std::string> structural_prop_labels; list<std::string> structural_prop_values;
            list<std::string> style_prop_labels; list<std::string> style_prop_values;
            for(int q=0;q<ctx.node().quals().length();q++) {
                Node qual = ctx.node().quals()[q];
                for(int i=0;i<qual.children().length();i++) {
                    Node c = qual.children()[i];
                    if(c.type()==property_id) {
                        std::string prop = "";
                        std::string val = "";

                        if(c.children()[0].value().type()==string_id) {
                            process_node(ctx,c.children()[0]);
                            if(!is_live(c.children()[0].value().data_ptr())) { //For templates and such where we might use an identifer
                                continue;
                            }
                            prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                        } else {
                            prop = c.children()[0].name().to_std();
                        }

                        if(c.children()[1].value().type()==string_id) {
                            process_node(ctx,c.children()[1]);
                            if(!is_live(c.children()[1].value().data_ptr())) {
                                continue;
                            }
                            val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                        } else {
                            val = c.children()[1].name().to_std();
                        }

                        list<std::string>* prop_labels; list<std::string>* prop_values;
                        if(is_prop_structural(prop)) {
                            prop_labels = &structural_prop_labels; 
                            prop_values = &structural_prop_values;
                        } else {
                            prop_labels = &style_prop_labels; 
                            prop_values = &style_prop_values;
                        }

                        if(q==0||!prop_labels->has(prop)) {
                            prop_labels->push(prop);
                            prop_values->push(val);
                        }
                    } else {

                    }
                }
            }


            for(int i=0;i<structural_prop_labels.length();i++) {
                s += " "+structural_prop_labels[i]+"=\""+structural_prop_values[i]+"\"";
            }   
            if(!style_prop_labels.empty()) {
                s += " style=\"";
                for(int i=0;i<style_prop_labels.length();i++) {
                    s += style_prop_labels[i]+":"+style_prop_values[i]+";";
                }  
                s += "\""; 
            }
            ctx.sub().source().push(s);
        }

        std::string emit_inline_html(Context& ctx, Node node) {
            Node old_node = ctx.node();
            std::string old_source = ctx.source().to_std();
            ctx.source().col().clear();
            ctx.node(node);
            emit_inline_html(ctx);
            std::string to_reutrn = ctx.source().to_std();
            ctx.node(old_node);
            ctx.source() = old_source;
            return to_reutrn;
        }

       Node make_property(Node type, Node value, Node parent) {
            Node prop_node = make_node(property_id);
            prop_node.children().push(type);
            prop_node.children().push(value);
            //prop_node.quals() << copy_as_token(parent);
            return prop_node;
        }

        void standard_gather_from_scope(Context& ctx) {
            Node node = ctx.node();
            if(!node.scopes().empty()) {
                Node scope = node.scopes()[0];
                Node properties = make_node(properties_id);
                properties.mute(true);
                scope.quals().push(properties);
                for(int i = 0;i<scope.children().length();i++) {
                    Node c = scope.children()[i];
                    if(c.type()==func_call_id) { //Anything defined with a type and identifer becomes a function call when refrenced elsewhere
                        Node ref = c.value().type_scope();
                        if(ref.idx==0 || ref.owner().idx==0) {
                            print(red("gather_from_scope:func_call type_scope or owner is null"));
                            continue;
                        }
                        Value ref_v = ref.owner().value();
                        if(ref_v.type()==inlined_id) {
                            for(int q=0;q<ref.quals().length();q++) {
                                scope.quals().push(ref.quals()[q]);
                            }
                            // scope.quals() << copy_as_token(c);
                            scope.children().removeAt(i);
                            i--;
                        } else if(ref_v.type()==component_id) {
                            if(ref.owner().children().empty()) {
                                // scope.quals() << copy_as_token(c);
                                Node owner = ref.owner();
                                scope.children_col().set(i,(void*)&owner);
                            } else {
                                instantiate_template(c,ref.owner(),ctx);
                            }
                        } else {
                            print("ACTUAL FUNC CALL");
                            //Is an actual func call, handle as such
                        }
                    } else if(c.type() == func_decl_id) {

                    } else if(c.children().empty()) {
                        if(is_live(c.value())&&c.value().type()==inlined_id) {
                            //scope.quals() << copy_as_token(c);
                            for(int q=0;q<c.scopes()[0].quals().length();q++) {
                                scope.quals().push(c.scopes()[0].quals()[q]);
                            }
                            scope.children().removeAt(i);
                            i--;
                        }
                    } else if(c.children().length()==2&&c.type()==colon_id) {
                        properties.children().push(make_property(c.children()[0],c.children()[1],c));
                        for(int q=0;q<properties.children().last().quals().length();q++) {
                            scope.quals().push(properties.children().last().quals()[q]); //Stealing the tokens for ourselves
                        }
                        properties.children().last().quals_col().clear();
                        scope.children().removeAt(i);
                        i--;
                    }
                }
            }
        }

        Node webcorn_node_scan(const std::string& label, Node from) {
            //print("SEARCHING: ",node_info(from));
            if(!from.scopes().empty()) {
                //print("SEARCHING ",from.scopes()[0].quals().length()," QUALS");
                for(int q=0;q<from.scopes()[0].quals().length();q++) {
                    Node qual = from.scopes()[0].quals()[q];
                    //print("  LOOKING AT ",node_info(qual));
                    for(int i=0;i<qual.children().length();i++) {
                        Node c = qual.children()[i];
                        //print("   LOOKING AT ",node_to_string(c));
                        if(c.type()==property_id) {
                            std::string prop = "";
                            std::string val = "";
    
                            if(c.children()[0].value().type()==string_id) { //Figure out why the props for sheet aren't resolving so this workaround isnt' nessecary
                                if(!is_live(c.children()[0].value().data_ptr())) {
                                    process_node(c.children()[0],deadptr);
                                    if(!is_live(c.children()[0].value().data_ptr())) {
                                        continue;
                                    }
                                }
                                prop = string(*(Ptr*)c.children()[0].value().get()).to_std();
                            } else {
                                prop = c.children()[0].name().to_std();
                            }
    
                            if(c.children()[1].value().type()==string_id) {
                                if(!is_live(c.children()[1].value().data_ptr())) {
                                    process_node(c.children()[1],deadptr);
                                    if(!is_live(c.children()[1].value().data_ptr())) {
                                        continue;
                                    }
                                }
                                val = string(*(Ptr*)c.children()[1].value().get()).to_std();
                            } else {
                                val = c.children()[1].name().to_std();
                            }

                            //print("   ",prop,":",val);

                            if(prop=="id"&&val==label) return from;
                        }
                    }
                }
            } 
            for(int i=0;i<from.children().length();i++) {
                Node found = webcorn_node_scan(label,from.children()[i]);
                if(is_live(found)) {
                    return found;
                }
            }
            for(int i=0;i<from.scopes().length();i++) {
                if(from.scopes()[i].owner()==from) {
                    Node found = webcorn_node_scan(label,from.scopes()[i]);
                    if(is_live(found)) {
                        return found;
                    }
                }
            }
            return deadptr;
        }

        struct style_manager : public q_object {
            style_manager(Webcorn_Core* _unit) : unit(_unit) {}
            style_manager(Webcorn_Core* _unit, list<std::string> init) : unit(_unit) {
                for(auto s : init) add_prop(s);
            }
            Webcorn_Core* unit;
            list<Node> props;
            list<std::string> prop_names;

            void add_prop(const std::string& name, Node prop = deadptr) {
                props << prop;
                prop_names << name;
            }

            void match_prop(const std::string& name, Node prop) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name) {
                        props[i] = prop;
                        return;
                    }
                }
            }

            std::string resolve_prop(Context& ctx, const std::string& name) {
                for(int i=0;i<prop_names.length();i++) {
                    if(prop_names[i]==name&&is_live(props[i])) {
                        return unit->emit_inline_html(ctx,props[i]);
                    }
                }
                return "";
            }
        };

        std::string ColCol_to_Static(Context& ctx, Ptr ptr) {
            g_ptr<style_manager> styles = make<style_manager>(this);
            ColCol& t = resolve_to_pool(ptr);
            list<list<std::string>> lines = TypeCol_to_lines(t);

            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(auto& col : lines) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += col.empty() ? "" : col[0];
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(auto& col : lines) if(col.length() > max_rows) max_rows = col.length();
            
            for(int r = 1; r < max_rows; r++) {
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(auto& col : lines) {
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">";
                    out += r < col.length() ? col[r] : "";
                    out += "</td>";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        std::string ColCol_to_Form(Context& ctx, Ptr ptr) {
            g_ptr<style_manager> styles = make<style_manager>(this);
            ColCol& t = resolve_to_pool(ptr);
            list<list<std::string>> lines = TypeCol_to_lines(t);

            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            //out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(auto& col : lines) {
                out += "<th ";
                //out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += col.empty() ? "" : col[0];
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(auto& col : lines) if(col.length() > max_rows) max_rows = col.length();
            
            for(int r = 1; r < max_rows; r++) {
                ptr.sidx = r-1;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<lines.length();c++) {
                    list<std::string>& col = lines[c];
                    ptr.idx = c;
                    out += "<td ";
                    //out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    //out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+(r < col.length() ? col[r] : "")+"\""
                    + " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','("+Ptr_to_string(ptr)+").set('+this.value+')')\""
                    +"/>\n</td>\n";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        std::string ColColCol_to_DebugSheet(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            ColCol& rendersheet = resolve_to_pool(ptr);

            g_ptr<style_manager> styles = make<style_manager>(this);
            
            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<rendersheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += rendersheet[c].label.to_std();
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<rendersheet.length();c++) if(rendersheet[c].length() > max_rows) max_rows = rendersheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<rendersheet.length();c++) {
                    Col& col = rendersheet[c];
                    std::string tostr = "";
                    if(r<col.length()) {
                        if(rendersheet.tag==0) { //This sheet stores Ptrs
                            tostr = tag_to_str(col.tag,col[r]);
                        } else if(rendersheet.tag==1) { //This sheet stores direct values
                            Ptr p = *(Ptr*)col[r]; 
                            if(is_live(p)) {
                                p.unit = ptr.unit;
                                Col& vcol = resolve_to_col(p);
                                tostr = tag_to_str(vcol.tag,vcol[p.sidx]);
                            }
                        }
                    }
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\""
                    + " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\""
                    +"/>\n</td>\n";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }
        std::string ColColCol_to_Form(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            uint32_t target_pool = ptr.pool;
            ptr.pool = offset;   ColCol& sheetsheet    = resolve_to_pool(ptr);
            ptr.pool = offset+5; ColCol& datasheet     = resolve_to_pool(ptr);
            ptr.pool = offset+6; ColCol& metadatasheet = resolve_to_pool(ptr);
            ptr.pool = offset+7; ColCol& notesheet     = resolve_to_pool(ptr);
            ptr.pool = offset+8; ColCol& scriptsheet   = resolve_to_pool(ptr);
            ptr.pool = offset+9; ColCol& storesheet    = resolve_to_pool(ptr);
            ptr.pool = target_pool;
        
            g_ptr<style_manager> styles = make<style_manager>(this);
            std::string out = "<div id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";

            uint32_t max_rows = 0;
            for(int c = 0;c<sheetsheet.length();c++) if(sheetsheet[c].length() > max_rows) max_rows = sheetsheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                for(int c = 0; c < sheetsheet.length(); c++) {
                    Col& sheetcol = sheetsheet[c];
                    Col& datacol = datasheet[c];
                    Col& metacol = metadatasheet[c];
                    
                    std::string label = datacol.label.to_std();
        
                    std::string current = "";
                    if(datacol.length() > 0) {
                        Ptr p = *(Ptr*)datacol[r];
                        if(is_live(p)) {
                            Col& vcol = resolve_to_col(p);
                            current = value_as_string(vcol.tag, p);
                        }
                    }

                    Ptr metadata = *(Ptr*)metacol[r];
                }
            }
        
            // for(int c = 0; c < datasheet.length(); c++) {
            //     Col& sheetcol = sheetsheet[c];
            //     Col& datacol = datasheet[c];
            //     Col& metacol = metadatasheet[c];
        
            //     std::string label = datacol.label.to_std();
        
            //     std::string current = "";
            //     if(datacol.length() > 0) {
            //         Ptr p = *(Ptr*)datacol[0];
            //         if(is_live(p)) {
            //             Col& vcol = resolve_to_col(p);
            //             current = value_as_string(vcol.tag, p);
            //         }
            //     }
        
            //     // Read widget type from metadata col tag
            //     // 0 = auto (decide by row count), else explicit widget type
            //     uint32_t widget_type = metacol.tag;
        
            //     ptr.idx  = c;
            //     ptr.sidx = 0;
            //     std::string ptr_str = Ptr_to_string(ptr);
            //     std::string node_name = ctx.sub().node().name().to_std();
        
            //     out += "<div ";
            //     out += styles->resolve_prop(ctx, "field_style");
            //     out += ">\n<label ";
            //     out += styles->resolve_prop(ctx, label_sub+"label_style");
            //     out += ">" + label + "</label>\n";
        
            //     bool is_select = widget_type == select_widget_id 
            //                   || (widget_type == 0 && datacol.length() > 1);
        
            //     if(is_select) {
            //         out += "<select ";
            //         out += styles->resolve_prop(ctx, "select_style");
            //         out += " onchange=\"fragthree('"+node_name+"','setcell','"+ptr_str+"='+this.value)\">\n";
            //         // rows 1+ are options
            //         for(int r = 1; r < datacol.length(); r++) {
            //             Ptr op = *(Ptr*)datacol[r];
            //             std::string opt_val = "";
            //             if(is_live(op)) {
            //                 Col& ocol = resolve_to_col(op);
            //                 opt_val = value_as_string(ocol.tag, op);
            //             }
            //             std::string selected = (opt_val == current) ? " selected" : "";
            //             out += "<option value='"+opt_val+"'"+selected+">"+opt_val+"</option>\n";
            //         }
            //         out += "</select>\n";
            //     } else {
            //         out += "<input ";
            //         out += styles->resolve_prop(ctx, "form_input_style");
            //         out += " value=\""+current+"\"";
            //         out += " onchange=\"fragthree('"+node_name+"','setcell','"+ptr_str+"='+this.value)\"";
            //         out += "/>\n";
            //     }
        
            //     out += "</div>\n";
            // }
        
            out += "</div>";
            return out;
        }

        std::string ColColCol_to_Sheet(Context& ctx, Ptr ptr, uint32_t offset = 0) {
            uint32_t target_pool = ptr.pool;
            ptr.pool = offset;   ColCol& datasheet = resolve_to_pool(ptr);
            ptr.pool = offset+1; ColCol& metadatasheet = resolve_to_pool(ptr);
            ptr.pool = offset+2; ColCol& notesheet = resolve_to_pool(ptr);
            ptr.pool = offset+3; ColCol& scriptsheet = resolve_to_pool(ptr);
            ptr.pool = offset+4; ColCol& storesheet = resolve_to_pool(ptr);
            ptr.pool = target_pool;
            
            g_ptr<style_manager> styles = make<style_manager>(this);

            std::string out = "";
            out += "<table id='"+ctx.sub().node().name().to_std()+"' ";
            out += emit_inline_html(ctx, ctx.sub().node());
            if(!ctx.sub().node().scopes().empty()) {
                node_col props = ctx.sub().node().scopes()[0].children();
                for(int i=0;i<props.length();i++) {
                    styles->add_prop(props[i].name().to_std(),props[i].scopes()[0]);
                }    
            }
            out += ">\n";
            out+= "<tr "; 
            out+=styles->resolve_prop(ctx, "row_style"); 
            out+=">\n";
            for(int c = 0;c<datasheet.length();c++) {
                out += "<th ";
                out+=styles->resolve_prop(ctx, "header_style"); 
                out+=">";
                out += datasheet[c].label.to_std();
                out += "</th>";
            }
            out += "</tr>";
            
            uint32_t max_rows = 0;
            for(int c = 0;c<datasheet.length();c++) if(datasheet[c].length() > max_rows) max_rows = datasheet[c].length();
            
            for(int r = 0; r < max_rows; r++) {
                ptr.sidx = r;
                out += "<tr ";
                out+=styles->resolve_prop(ctx, "row_style"); 
                out+=">";
                for(int c = 0;c<datasheet.length();c++) {
                    Col& col = datasheet[c];
                    std::string tostr = "";
                    if(r<col.length()) {
                        Ptr p = *(Ptr*)col[r]; //Since the datasheet stores Ptrs
                        if(is_live(p)) {
                            Col& vcol = resolve_to_col(p);
                            tostr = value_as_string(vcol.tag,p);
                        }
                    }
                    ptr.idx = c;
                    out += "<td ";
                    out+=styles->resolve_prop(ctx, "column_style"); 
                    out+=">\n<input "; 
                    out+=styles->resolve_prop(ctx, "input_style"); 
                    out+=" value=\""+tostr+"\""
                    + " onchange=\"fragthree('"+ctx.sub().node().name().to_std()+"','setcell','"+Ptr_to_string(ptr)+"='+this.value)\""
                    +"/>\n</td>\n";
                }
                out += "</tr>";
            }
            
            out += "</table>";
            return out;
        }

        //render_sheet(sheetid, poolid, "render as")
        //Render as options: static, sheet, form
        uint32_t render_sheet_id = add_function("render_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            if(ctx.node().children().length()!=3) {print(red("Wrong number of arguments for render_sheet, expected 3")); return;}
            int sheetid = *(int*)ctx.node().children()[0].value().get();
            int poolid = *(int*)ctx.node().children()[1].value().get();
            string renderas = (string&)*(Ptr*)ctx.node().children()[2].value().get();
            print("RENDERING POOL: ",poolid);
            if(sheetid!=0) {
                Ptr ptr(poolid,0,0,sheetid);
                if(renderas.to_std()=="static") {
                    ctx.sub().source().push(ColCol_to_Static(ctx,ptr));
                } else if(renderas.to_std()=="sheet") {
                    ctx.sub().source().push(ColColCol_to_Sheet(ctx,ptr));
                } else if(renderas.to_std()=="form") {
                    ctx.sub().source().push(ColColCol_to_Form(ctx,ptr));
                } else if(renderas.to_std()=="debug") {
                    ctx.sub().source().push(ColColCol_to_DebugSheet(ctx,ptr));
                } else {
                    print(red("Unrecognized render type for render_sheet "),renderas);
                }
                // units[sheetid]->dump_unit(true);
            } else {
                ctx.sub().source().push("<table id='"+ctx.sub().node().name().to_std()+"'></table>");
            }
        });

        uint32_t create_sheet_id = add_function("create_sheet",[this](Context& ctx){
            ColColCol sheet;
            ColCol data_pool; data_pool.tag=1; sheet.push(data_pool); //Tag 1 means that everything here is a Ptr to something else
            ColCol metadata_pool; metadata_pool.tag=0; sheet.push(metadata_pool); //Tag 0 means that direct values are stored here
            ColCol notes_pool; notes_pool.tag=1; sheet.push(notes_pool);
            ColCol scripts_pool; scripts_pool.tag=1; sheet.push(scripts_pool);
            ColCol store_pool; store_pool.tag=0; sheet.push(store_pool);
            uint32_t sheetid = (uint32_t)make_unit(sheet);
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t load_sheet_id = add_function("load_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            string s(*(Ptr*)ctx.node().children()[0].value().get());
            uint32_t sheetid = 0;
            for(int u=0;u<units.length();u++) {
                if(units[u]->types.label==s.to_std()) {
                    sheetid = u; break;
                }   
            }
            if(sheetid==0) {
                auto in = openReadStream("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
                ColColCol sheet = read_TypeTypeCol(in);
                sheetid = (uint32_t)make_unit(sheet);

                for(int p = 0;p<sheet.length();p++) {
                    if(sheet[p].tag==1) { //Stores Ptrs, so it needs to be normalized
                        ColCol& pool = sheet[p];
                        for(int c=0;c<pool.length();c++) {
                            Col& col = pool[c];
                            for(int r=0;r<col.length();r++) {
                               Ptr ptr = *(Ptr*)col[r];
                               if(is_live(ptr)) {
                                    ptr.unit = sheetid;
                                    col.set(r,(void*)&ptr);
                               }
                            }
                        }   
                    }
                }

            }
            print("Rendering ",sheetid);
            units[sheetid]->dump_unit(true);
            print("Set and finished");
            ctx.node().value().set((void*)&sheetid);
        },4,int_id);
        uint32_t save_sheet_id = add_function("save_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            uint16_t sheetid = *(uint16_t*)ctx.node().children()[0].value().get();
            string s(*(Ptr*)ctx.node().children()[1].value().get());
            auto out = openWriteStream("mixos-acorn/web/thistle/users/fir/sheets/"+s.to_std());
            units[sheetid]->types.label = s.to_std();
            write_TypeTypeCol(out,units[sheetid]->types);
        });
        uint32_t add_column_id = add_function("add_column_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int offset = *(int*)ctx.node().children()[1].value().get();
            for(int o=0;o<4;o++) { //We're iterating over each of the diffrent pools in the sheet by offset
                Col ncol(sizeof(Ptr)); ncol.tag = ptr_id;
                if(!(*units[idx])[o+offset].empty()) { //We need to ensure there's always the same ammount of rows in each column
                    for(int i=0;i<((*units[idx])[o+offset][0].length());i++) {
                        ncol.push_default();
                    }
                }
                (*units[idx])[o+offset].push(ncol);
            }
        });
        uint32_t add_row_id = add_function("add_row_to_sheet",[this](Context& ctx){
            standard_sub_process(ctx);
            int idx = *(int*)ctx.node().children()[0].value().get();
            int offset = *(int*)ctx.node().children()[1].value().get();
            ColCol& data     = (*units[idx])[offset];
            ColCol& metadata = (*units[idx])[offset+1];
            ColCol& notes    = (*units[idx])[offset+2];
            ColCol& scripts  = (*units[idx])[offset+3];
            for(int i = 0; i < data.length(); i++) {
                data[i].push_default();
                metadata[i].push_default();
                notes[i].push_default();
                scripts[i].push_default();
            }
        });
        uint32_t setcell_id = add_function("setcell",[this](Context& ctx){
            standard_sub_process(ctx);
            string addr = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            list<std::string> terms = split_str(addr.to_std(),'=');
            Ptr cellptr = string_to_Ptr(terms[0]);

            uint32_t pooltag = resolve_to_pool(cellptr).tag;
            if(pooltag==0) { //The tag on the pool dictates how it's values are stored
                //Nothing here for now since we aren't allowing setcells on metadata and such just yet
            } else if(pooltag==1) {
                Ptr p = *(Ptr*)resolve_ptr(cellptr);
                Node literal = compile_literal(terms[1]);
                Value lv = literal.value();
                if(!is_live(p)) {
                    Ptr storeptr = cellptr;
                    storeptr.pool+=4; //To get to the store pool (this could be a bit fragile)
                    p = get_ticket(storeptr,lv.size(),lv.type());
                    resolve_to_col(cellptr).set(cellptr.sidx,(void*)&p);
                }

                void* data = lv.get();
                Col& col = resolve_to_col(p); //Where the value is stored in the store pool
                if(lv.type()==string_id) {
                    if(col.tag!=string_id||col.empty()) {
                        col.clear(); 
                        col.element_size = lv.size(); col.tag=lv.type();
                        Ptr storeptr = cellptr;
                        storeptr.pool+=4;
                        Ptr charp = get_ticket(storeptr,1,char_id); //Col is unsafe to use after this
                        string str = (string&)charp;
                        string lstr = (string&)*(Ptr*)lv.get();
                        str = lstr.to_std();
                        resolve_to_col(p).push((void*)&charp);
                        return;
                    } else {
                        data = col[p.sidx];
                        string str = (string&)*(Ptr*)col[p.sidx];
                        string lstr = (string&)*(Ptr*)lv.get();
                        str = lstr.to_std();
                    }
                } 

                if(col.element_size!=lv.size()||col.tag!=lv.type()) {
                    col.clear();
                    col.element_size = lv.size(); col.tag=lv.type();
                    col.push(data);
                } else if(col.empty()) {
                    col.push(data);
                } else {
                    col.set(p.sidx,data);
                }
            }
        });

        map<std::string,uint32_t> routes;
        map<uint32_t,Node> route_nodes;


        uint32_t div_id = make_tokenized_keyword("div");

        void init() override {
            set_binding_powers(colon_id,4,6);
            // register_type("div",component_id,0);
            register_type("inlined",inlined_id,0);
            register_type("invisible",invisible_id,0);

            n_handlers[div_id] = [this](Context& ctx){
                if(ctx.result().get(ctx.index()+1).type()!=lbrace_id) {
                    Node take = ctx.result().take(ctx.index()+1);
                    ctx.node().name(take.name().to_std()); 
                    for(int i=0;i<take.children().length();i++) {
                        ctx.node().children() << take.children()[i];
                    }
                }
            };
            t_handlers[div_id] = [this](Context& ctx) {
                Node node = ctx.node();
                ctx.node().value(make_value(component_id));
                node.scopes()[0].owner(node);
                node.scopes()[0].name(node.name().to_std());
                node.type(func_decl_id);
                node.scopes()[0] = distribute_node(node.in_scope(),node.name().to_std(),node.scopes()[0]);
                node.value().type_scope(node.scopes()[0]);
                node.value().sub_type(0);
                node.value(distribute_value(node.in_scope(),node.name().to_std(),node.value()));
                for(int c=0;c<node.children().length();c++) {
                    place_node_in_scope(node.children()[c],node.scopes()[0]);
                }
                ctx.node().type(func_decl_id);
            };

            r_handlers[func_decl_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                if(ctx.node().type()==func_call_id) {
                    if(ctx.node().value().type()!=component_id) {
                        //instantiate_template(ctx.node(),ctx.node().value().type_scope().owner(),ctx);
                        sync_args(ctx);
                    }
                } else {
                    Node scope = ctx.node().scopes()[0];
                    if(!is_live(scope.value())) {
                        scope.value(make_value()); 
                        scope.value().loc(0); //Set location for stack depth
                    }
                }
                if(ctx.node().value().type()==component_id) {
                    standard_gather_from_scope(ctx);    
                    if(!ctx.node().scopes().empty()) {
                        if(ctx.node().type()==func_decl_id&&!ctx.node().children().empty()) {
                            ctx.node().value().type(invisible_id); //For templates which we don't want to emit
                        }
                        for(int i=0;i<ctx.node().scopes().length();i++) {
                            Node s = ctx.node().scopes()[i];
                            if(ctx.node().value().type()==invisible_id) {
                                s.type(invisible_id);
                            // } else if(ctx.node()->value->type==foldable_id) {
                            //     s->type = foldable_id;
                            // } else if(ctx.node()->value->type==iframe_id) {
                            //     s->type = iframe_id;
                            } else {
                                s.type(component_id);
                            }
                        }
                    }
                }
            };
            r_handlers[func_call_id] = r_handlers[func_decl_id];

            x_handlers[make_tokenized_keyword("gather_props")] = [this](Context& ctx){
                ctx.node(ctx.sub().node());
                standard_gather_from_scope(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_inline_html")] = [this](Context& ctx){
                if(ctx.sub().node().scopes().empty()) return;
                ctx.node(ctx.sub().node().scopes()[0]);
                emit_inline_html(ctx);
            };

            x_handlers[make_tokenized_keyword("emit_contents")] = [this](Context& ctx){
                Node node = ctx.sub().node();
                if(node.scopes().length()>0) {
                    Node scope = node.scopes().get(0);
                    for(int i=0;i<scope.children().length();i++) {
                        Node child = scope.children().get(i);
                        start_stage(html_handlers);
                        process_node(ctx,child);
                        start_stage(x_handlers);
                    }
                }
            };

            ColCol t;
            for(int i=0;i<5;i++) {
                Col c;
                c.element_size = 4;
                c.tag = int_id;
                for(int n=0;n<8;n++) {
                    c.push((void*)&n);
                }
                t.push(c);
            }
            types.push(t);


            uint32_t display_node_id = make_tokenized_keyword("display_node");
            r_handlers[display_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                string contents(ticket);
                


                ctx.node().value().set((void*)&ticket);
            };
            x_handlers[display_node_id] = [this](Context& ctx){
                string addr(*(Ptr*)ctx.node().children()[0].value().get());
                string output(*(Ptr*)ctx.node().value().get());

                

                output = ("<p>"+addr.to_std()+"</p>");
            };

            r_handlers[find_node_id] = [this](Context& ctx){
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                resolve_overload(ctx);
            };
            x_handlers[find_node_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                std::string target = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                Node& from = (Node&)(*(Ptr*)ctx.node().children()[1].value().get());
                //print("TARGET: ",target," FROM: ",node_info(from));
                Node result = webcorn_node_scan(target,from);
                // if(!is_live(result)) {
                //     while(is_live(from.in_scope())&&is_live(from.in_scope().owner())&&from.in_scope().owner().type()==func_decl_id) {
                //         from = from.in_scope().owner();
                //     }
                //     print("NOW SEARCHING FROM: ",node_info(from));
                // }

                ctx.node().value().set((void*)&result);
                if(is_live(result)) {
                    //print("FOUND: ",node_to_string(result));
                } else {
                    print(red("COULD NOT FIND "+target));
                }
            };

            x_handlers[make_tokenized_keyword("webcorn")] = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
                int server_fd = 6;
                ctx.node().value().set((void*)&server_fd);
            };

            auto make_int_node = [this](Context& ctx){
                ctx.node().value(make_value(int_id,4));
            };

            x_handlers[make_tokenized_keyword("run_server")] = [this](Context& ctx){
                standard_sub_process(ctx);
                int server_fd = *(int*)ctx.node().children()[0].value().get();
                g_ptr<Server> new_server = make<Server>();
                new_server->fd = server_fd;
                new_server->thread = make<Thread>();
                new_server->unit = this;
                servers << new_server;
                
                if(!ctx.node().scopes().empty()) {
                    Node scope = ctx.node().scopes()[0];
                    new_server->thread->run_blocking([this, scope, ctx]() mutable {
                        standard_travel_pass(scope, ctx);
                    });
                }
            };
            uint32_t socket_id = make_tokenized_keyword("socket");
            r_handlers[socket_id] = make_int_node;
            x_handlers[socket_id] = [this](Context& ctx){
                #ifdef _WIN32
                    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
                #endif
                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                if(server_fd < 0) { print(red("socket() failed")); return; }
                ctx.node().value().set((void*)&server_fd);
            };
            
            uint32_t bind_id = make_tokenized_keyword("bind");
            r_handlers[bind_id] = make_int_node;
            x_handlers[bind_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int port = *(int*)ctx.node().children()[1].value().get();
                int opt = 1;
                setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                addr.sin_addr.s_addr = INADDR_ANY;
                int result = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
                ctx.node().value().set((void*)&result);
            };
            
            uint32_t listen_id = make_tokenized_keyword("listen");
            r_handlers[listen_id] = make_int_node;
            x_handlers[listen_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                int result = listen(fd, 10);
                ctx.node().value().set((void*)&result);
            };
            
            uint32_t accept_id = make_tokenized_keyword("accept");
            r_handlers[accept_id] = make_int_node;
            x_handlers[accept_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd == -1) { throw_error("accept failed"); return; }
                ctx.node().value().set((void*)&client_fd);
            };
            
            uint32_t read_id = make_tokenized_keyword("read");
            r_handlers[read_id] = [this](Context& ctx){
                ctx.node().value(make_value(string_id, sizeof(Ptr)));
            };
            x_handlers[read_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                char buffer[4096];
                std::string request;
                while(true) {
                    int bytes = READ_SOCKET(fd, buffer, sizeof(buffer)-1);
                    if(bytes <= 0) break;
                    buffer[bytes] = 0;
                    request += buffer;
                    if(bytes < (int)sizeof(buffer)-1) break;
                }
                Ptr ticket = get_ticket(name_store_id, 1, char_id);
                for(auto c : request) types[name_store_id][ticket.idx].push((void*)&c);
                ctx.node().value().set((void*)&ticket);
            };
            
            uint32_t write_id = make_tokenized_keyword("write");
            r_handlers[write_id] = make_int_node;
            x_handlers[write_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                Ptr strptr = *(Ptr*)ctx.node().children()[1].value().get();
                Col& col = types[strptr.pool][strptr.idx];
                WRITE_SOCKET(fd, (const char*)col.storage, col.size);
            };
            
            uint32_t close_id = make_tokenized_keyword("close");
            r_handlers[close_id] = make_int_node;
            x_handlers[close_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                int fd = *(int*)ctx.node().children()[0].value().get();
                CLOSE_SOCKET(fd);
            };

            // x_handlers[make_tokenized_keyword("respond")] = [this](Context& ctx){
            //     int fd = *(int*)ctx.node().children()[0].value().get();
            //     string str = *(Ptr*)ctx.node().children()[1].value().get();
            //     print("RESPONDING TO:\n",str.to_std());
            //     std::string body = "<html><body> <p> hello world </p>  <body></html>";
            //     std::string response = 
            //         "HTTP/1.1 200 OK\r\n"
            //         "Content-Type: text/html\r\n"
            //         "Content-Length: " + std::to_string(body.length()) + "\r\n"
            //         "\r\n" + body;
            //     print("Response:\n",response);
            //     if(::write(fd, response.c_str(), response.length()) < 0) {
            //         print(red("server_id::x_handler write() failed"));
            //     }
            // };


            x_handlers[make_tokenized_keyword("mem_test")] = [this](Context& ctx){
                uint32_t host_before = 0;
                uint32_t host_after = 0;
                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_before += types[t][c].size;
                    }
                }

                int iterations = 200;
                //readFile("mixos-acorn/web/webtest.gld");
                std::string sample = 
                //"int i = 5; print(i);"; 
                "Ptr Ptr Ptr int double_nested;\n"
                "Ptr Ptr int nested;\n"
                "Ptr int nums;\n"
                "nums.push(3);\n"
                "nums.push(8);\n"
                "nested.push(nums);\n"
                "Ptr int tums;\n"
                "tums.push(12);\n"
                "tums.push(14);\n"
                "nested.push(tums);\n"
                "double_nested.push(nested);\n"
                "print(double_nested.get(0).get(0).get(0));\n"
                "print(double_nested.get(0).get(0).get(1));\n"
                "print(double_nested.get(0).get(1).get(0));\n"
                "print(double_nested.get(0).get(1).get(1));\n";
            
                list<size_t> snapshots;
                
                for(int i = 0; i < iterations; i++) {
                    size_t before = current_memory_usage();
                    
                    Log::Line total; total.start();
                    Log::Line l; l.start();
                    g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                    print("INIT TIME: ",ftime(l.end())); l.start();
                    Node root = twig->process(sample);
                    print("PROCESS TIME: ",ftime(l.end())); l.start();
                    twig->compile(root);
                    print("COMPILE TIME: ",ftime(l.end())); l.start();
                    twig->start_stage(x_handlers);
                    twig->standard_travel_pass(root);
                    print("EXECUTE TIME: ",ftime(l.end())); l.start();
                    print("TOTAL TIME: ",ftime(total.end()));

                    units.removeAt(twig->uid);
                    twig->release();

                    size_t after = current_memory_usage();
                    snapshots << after;
                    print("iter ",i,": ",before," -> ",after," (delta: ",((int64_t)after-(int64_t)before),")");
                }

                for(int t = 0; t < types.length(); t++) {
                    for(int c = 0; c < types[t].length(); c++) {
                        host_after += types[t][c].size;
                    }
                }
                print("Host pool growth: ", (int)host_after - (int)host_before);
                
                // Print overall trend
                if(snapshots.length() > 1) {
                    int64_t total_growth = (int64_t)snapshots.last() - (int64_t)snapshots[0];
                    print("Total growth over ",iterations," iterations: ",total_growth," bytes");
                    print("Average per iteration: ",total_growth/iterations," bytes");
                }
            };

            x_handlers[make_tokenized_keyword("fragment_highlight")] = [this](Context& ctx) {
                std::string source = ctx.sub().source().to_std();
    
                size_t first = source.find(" ");
                size_t second = source.find(" ", first + 1);
                
                std::string target = source.substr(0, first);
                std::string instruction = source.substr(first + 1, second - first - 1);
                std::string content = source.substr(second + 1);

                print("TARGET: ",target);
                print("INSTRUCTION: ",instruction);
                print("CONTENT: ",content);

                print("MEMORY USED: ",current_memory_usage());

                std::string out = "";
                g_ptr<Webcorn_Core> twig = make_unit<Webcorn_Core>();
                if(instruction=="compile") {
                    Log::Line l; l.start();
                    Node root = twig->process(content);
                    twig->compile(root);
                    double a_time = l.end(); l.start();
                    out += fnodenet_to_string(root,Stamper{[this](Node n, list<int>& offsets){
                        std::string to_return = n.name().to_std();
                        if(n.type()!=0) {
                            std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
                            while((int)n.y()>=offsets.length()) {offsets<<0;}
                            n.x(n.x()+offsets[(int)n.y()]);
                            offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                            to_return = nreturn;
                        }
                        return to_return;
                    },[this](Node n){
                        list<Node> stamps;
                        map<uint64_t,bool> visited;
                        collect_stamps(n,stamps,visited);
                        return stamps;
                    }});
                    double b_time = l.end(); 
                    // l.start();
                    //print_root(root);
                    // double c_time = l.end();

                    print("A: ",ftime(a_time));
                    print("B: ",ftime(b_time));
                    //print("C: ",ftime(c_time));

                    // print(node_to_string(root));

                    // recycle_node(root); //Deal with memory managment later, like in the mem_test
                    // units.erase(twig);

                    print("POST TWIG: ",current_memory_usage());

                } else if(instruction=="end") {
                    // print("REQUEST TO END: ",target," OF ",servers.length());
                    // g_ptr<Server> to_end = get_server(target);
                    // if(to_end) {
                    //     ::close(to_end->fd); 
                    //     to_end->fd = -1;
                    //     to_end->thread->end();
                    //     servers.erase(to_end);
                    // } else {
                    //     print(red("Unable to find server "+target+" to end"));
                    // }
                } else if(instruction=="preview") {
                    Node root = twig->process(content);
                    twig->run(root);

                    int port_num = 8081;
                    // for(auto c : root->children) {
                    //     if(c->type==server_id) {
                    //         for(auto sc : c->scope()->children) {
                    //             if(sc->type==port_id) {
                    //                 port_num = sc->left()->value->get<int>();
                    //             }
                    //         }
                    //     }
                    // }
                    servers << twig->servers;
                    servers.last()->label = target;
                    print("SPINNING UP A NEW SERVER ON ",port_num," CALLED ",servers.last()->label);
                    out = std::to_string(port_num);
                } else if(instruction=="read") {
                    out = readFile(content);
                } else {
                    print(red("Unrecognized instruction for fragment: "+ctx.sub().source().to_std()));
                }
                ctx.sub().source() = out;
            };
            


        }
    };
}