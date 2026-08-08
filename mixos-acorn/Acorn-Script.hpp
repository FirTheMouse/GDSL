#pragma once

#include "../mixos-acorn/Acorn-Compiler.hpp"
#include "../ext/g_lib/core/thread.hpp"


#define LOBOTOMIZE_M_STAGE 1

namespace Acorn {
    struct Acorn_Script : public virtual Compiler_Unit {
        Acorn_Script(uint16_t _uid) : Unit(_uid) {init();}
        Acorn_Script() {init();}

        //For my sanity
        string resolve_string_ticket(Node n) {
            if(is_live(n.value())) {
                if(is_live(n.getPtr())) {
                    return n.getString();
                } else {
                    Ptr p = get_ticket(name_store_id,1,char_id); 
                    n.value().set((void*)&p);
                    return (string&)p;
                }
            }
            return deadptr;
        }

        Ptr resolve_ticket_by_ptr(Ptr p, Col& col, uint32_t index, uint32_t width, uint32_t size, uint32_t type) {
            if(is_live(p)) {
                return p;
            } else {
                p = get_ticket(data_store_id,size,type); 
                col.qset(index, (void*)&p, width);
                return p;
            }
        }

        Ptr resolve_ticket(Node n,  uint32_t size, uint32_t type) {
            if(is_live(n.value())) {
                if(is_live(*(Ptr*)n.value().get())) {
                    return *(Ptr*)n.value().get();
                } else {
                    Ptr p = get_ticket(data_store_id,size,type); 
                    n.value().set((void*)&p);
                    return p;
                }
            }
            return deadptr;
        }

        uint32_t test_id = make_tokenized_keyword("test");
        Stage& n_handlers = reg_stage("naming"); 
        
        uint32_t query_id = make_type("Query",sizeof(Ptr));

        uint32_t labels_id = make_tokenized_keyword("labels");

        uint32_t node_block_id = reg_id("node_block");
        uint32_t invoke_stage_id = make_keyword("invoke_stage");
        uint32_t in_id = make_keyword("in");
        uint32_t on_id = make_tokenized_keyword("on");
        uint32_t precompiling_id = reg_id("PRECOMPILING");

        uint32_t ctx_id = make_tokenized_keyword("ctx");
        uint32_t lctx_id = make_tokenized_keyword("lctx");

        uint32_t while_id = make_tokenized_keyword("while");
        uint32_t for_id = make_tokenized_keyword("for");
        uint32_t if_id = make_tokenized_keyword("if");
        uint32_t else_id = make_tokenized_keyword("else");

        uint32_t read_file_id = make_tokenized_keyword("read_file");
        uint32_t write_file_id = add_function("write_file",[this](Context& ctx){
            standard_sub_process(ctx);
            string path = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            string contents = (string&)*(Ptr*)ctx.node().children()[1].value().get();
            DEBUG_ONLY(if(ERROR_FLAG){return;})
            writeFile(path.to_std(),contents.to_std());
        });
        uint32_t compile_id = make_tokenized_keyword("compile");

        uint32_t live_qual = register_qual_ids("live");
        uint32_t gatekeeper_qual = register_qual_ids("gatekeeper");
        uint32_t assigned_qual = register_qual_ids("assigned");
        uint32_t constant_qual = register_qual_ids("constant");

        uint32_t to_string_id = make_tokenized_keyword("to_string");
        uint32_t from_string_id = add_function("from_string",[this](Context& ctx){
            standard_sub_process(ctx);
            string str = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            Node n = compile_literal(str.to_std());
            ctx.node().value(n.value());
        },0,duck_id);
        uint32_t to_type_id = make_tokenized_keyword("to_type");
        uint32_t DEBUG_ROOT_id = make_tokenized_keyword("DEBUG_ROOT");

        uint32_t get_ticket_id = add_function("get_ticket",[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ticket = deadptr;
            uint32_t pool = ctx.node().getInt(0);
            if(ctx.node().children().length()==2) {
                Value v = ctx.node().children()[1].value();
                ticket = get_ticket(pool,v.size(),v.type());
                if(is_live(v.data_ptr())) {
                    Col& c = resolve_to_col(ticket);
                    CHECK_ERROR("Ticket did not resolve to a valid col: ",Ptr_as_string(ticket));
                    c.push(v.get());
                }
            } else if(ctx.node().children().length()==3) {
                uint32_t size = ctx.node().getInt(1);
                uint32_t type = ctx.node().getInt(2);
                ticket = get_ticket(pool,size,type);
            }
            ctx.node().value().set((void*)&ticket);
        },sizeof(Ptr),ptr_id);
        uint32_t get_ticket_of_type_id = add_function("get_ticket_of_type",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t pool = *(uint32_t*)ctx.node().children()[0].value().get();
            Value v = ctx.node().children()[1].value();
            Ptr ticket = get_ticket(pool,v.size(),v.type());
            ctx.node().value().set((void*)&ticket);
        },sizeof(Ptr),ptr_id);

        void script_general_get(Ptr& ptr, Col& search, uint32_t field, Node key, bool error_on_key_not_found = true) {
            uint32_t right_arg_lookup_type = key.value().type();
            if(right_arg_lookup_type==int_id) {
                int index = key.getInt();
                // if(!error_on_key_not_found) {
                //     while(search.length()>=index) {
                //         search.push_default();
                //     }
                // }
                if(index<search.length()) {
                    ptr[field] = index;
                } else {
                    throw_error("script:script_general_get Index ",index,+" out of bounds on ",Ptr_as_string(ptr));
                    ptr.specialization = _DEADSPEC;
                }
            } else if(is_ptr_alias(right_arg_lookup_type)) {
                Col& ccol = resolve_to_col(key.getPtr());
                if(!search.hasKey(ccol.storage,ccol.size)) {
                    if(error_on_key_not_found) {
                        throw_error("script:script_general_get Key not found");
                        ptr.specialization = _DEADSPEC;
                        return;
                    } else {
                        search.qput(malloc(search.element_size),ccol.storage,ccol.size,right_arg_lookup_type);
                    }
                }
                ptr[field] = search.getidx(ccol.storage,ccol.size);
            }
        };

        inline Col& resolve_YAPA_ptr(Ptr ptr, uint32_t YAPA_level) {
            switch(YAPA_level) {
                case 1: return resolve_to_col(ptr);
                case 2: return (Col&)resolve_to_pool(ptr);
                case 3: return (Col&)resolve_to_subunit(ptr);
                case 4: return (Col&)resolve_to_unit(ptr);
                default: return col1_ref;
            }   
        }

        uint32_t register_YAPA_type(const std::string& label, uint32_t YAPA_level, uint32_t id) {
            Value get_value = deadptr;
            if(YAPA_level==1) get_value = make_value(0);
            else if(YAPA_level==2) get_value = make_value(ptr_id,sizeof(Ptr));
            else if(YAPA_level==3) get_value = make_value(colcol_id,sizeof(Ptr));     
            else if(YAPA_level==4) get_value = make_value(colcolcol_id,sizeof(Ptr));  

            overload_type(id,".\"getOrPut\"",label+"_GETORPUT",get_value,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" getOrPut");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(YAPA_level==1&&ctx.node().value().type()==0) {
                    ctx.node().value().type(col.tag);
                    ctx.node().value().size(col.element_size);
                }
                script_general_get(ptr,col,YAPA_level,ctx.node().right().c0(),false);
                if(YAPA_level>1) {
                    ctx.node().value().set((void*)&ptr);
                } else {
                    ctx.node().value().data_ptr(ptr);
                }
            });
            overload_type(id,"[any]",label+"_IDXGET",get_value,[this,id,YAPA_level](Context& ctx){
                // if(!ctx.node().value().has_qual(static_qual)) { An attempt to stop collections from breaking when used with functions
                //     ctx.node().value().quals().push(make_node(static_qual));
                // }
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" idxget");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(YAPA_level==1&&ctx.node().value().type()==0) {
                    ctx.node().value().type(col.tag);
                    ctx.node().value().size(col.element_size);
                }
                script_general_get(ptr,col,YAPA_level,ctx.node().right(),false);
                if(YAPA_level>1) {
                    ctx.node().value().set((void*)&ptr);
                } else {
                    ctx.node().value().data_ptr(ptr);
                }
            });
            overload_type(id,".\"get\"",label+"_GET",get_value,[this,id,YAPA_level](Context& ctx){
                // if(!ctx.node().value().has_qual(static_qual)) {
                //     ctx.node().value().quals().push(make_node(static_qual));
                // }
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" get");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(YAPA_level==1&&ctx.node().value().type()==0) {
                    ctx.node().value().type(col.tag);
                    ctx.node().value().size(col.element_size);
                }
                script_general_get(ptr,col,YAPA_level,ctx.node().right().c0());
                if(YAPA_level>1) {
                    ctx.node().value().set((void*)&ptr);
                } else {
                    ctx.node().value().data_ptr(ptr);
                }
            });
            overload_type(id,".\"take\"",label+"_TAKE",get_value,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" take");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                Value arg = ctx.node().right().c0().value();
                if(YAPA_level==1&&ctx.node().value().type()==0) {
                    ctx.node().value().type(col.tag);
                    ctx.node().value().size(col.element_size);
                }
                if(arg.type()==int_id) {
                    int index = *(int*)arg.get();
                    ctx.node().value().set(col.get((uint32_t)index));
                    col.removeAt(index);
                }
            });
            overload_type(id,".\"length\"",label+"_LENGTH",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" length");
                int len = resolve_YAPA_ptr(ptr,YAPA_level).length();
                ctx.node().value().set((void*)&len);
            });
            overload_type(id,".\"has\"",label+"_HAS",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" has");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                Value keyv = ctx.node().right().c0().value();
                CHECK_ERROR("Invalid argument for "+labels[id]+" has");
                if(is_ptr_alias(keyv.type())) {
                    Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                    bool has_thing = col.hasKey(keycol.storage,keycol.size);
                    ctx.node().value().set((void*)&has_thing);
                } else {
                    throw_error("script:has Type ",labels[keyv.type()]," is not a valid key type for has");
                }
            });
            overload_type(id,".\"indexof\"",label+"_INDEXOF",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" length");
                Col& owner = resolve_YAPA_ptr(ptr,YAPA_level);
                Value key = ctx.node().right().c0().value();
                int index = 0;
                if(key.type()==string_id) {
                    std::string label = ctx.node().right().getString(0).to_std();
                    for(int i=0;i<owner.length();i++) {
                        if(((ColCol&)owner).get(i).label==label) {
                            index = i; break;
                        }
                    }
                } else if(is_ptr_alias(key.type())) {
                    Col& col = resolve_YAPA_ptr(ctx.node().right().getPtr(0),YAPA_level-1);
                    index = owner.indexof(&col);
                }
                ctx.node().value().set((void*)&index);
            });

            overload_type(id,".\"put\"",label+"_PUT",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" put");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                Value keyv = ctx.node().right().c0().value();
                Value elementv = ctx.node().right().c1().value();
                CHECK_ERROR("Invalid arguments for "+labels[id]+" put");
                if(is_ptr_alias(keyv.type())) {
                    Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                    col.qput(elementv.get(),keycol.storage,keycol.size,keyv.type());
                } else {
                    throw_error("script:put Type ",labels[keyv.type()]," is not a valid key type for put");
                }
            });

            overload_type(id,".\"push\"",label+"_PUSH",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" push");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(YAPA_level>1) {
                    Col& pushcol = resolve_YAPA_ptr(ctx.node().right().getPtr(0),YAPA_level-1);
                    switch(YAPA_level) {
                        case 4:{ColColCol* copycol = new ColColCol(pushcol); ((PtrColColCol&)col).push(copycol); }break;
                        case 3:{ColCol copycol = (ColCol&)pushcol; ((ColColCol&)col).push(copycol); }break;
                        case 2:{Col copycol = pushcol; ((ColCol&)col).push(copycol); }break;
                        default: break;
                    }
                } else {
                    if(col.tag==char_id) { //Improve later when I have a proper coercsion system
                        col.push(ctx.node().right().getString(0).col()[0]);
                    } else {
                        col.push(ctx.node().right().c0().value().get());
                    }
                }
            });
            overload_type(id,"<<any",label+"_PUSH_OP",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" push");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(YAPA_level>1) {
                    Col& pushcol = resolve_YAPA_ptr(ctx.node().right().getPtr(),YAPA_level-1);
                    switch(YAPA_level) {
                        case 4:{ColColCol* copycol = new ColColCol(pushcol); ((PtrColColCol&)col).push(copycol); }break;
                        case 3:{ColCol copycol = (ColCol&)pushcol; ((ColColCol&)col).push(copycol); }break;
                        case 2:{Col copycol = pushcol; ((ColCol&)col).push(copycol); }break;
                        default: break;
                    }
                } else {
                    col.push(ctx.node().right().value().get());
                }
            });
            overload_type(id,".\"push_default\"",label+"_PUSH_DEFAULT",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" push_default");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                col.push_default();
            });

            overload_type(id,".\"qset\"",label+"_QSET",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" qset");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                void* data = ctx.node().right().get(0);
                int width = ctx.node().right().getInt(1);
                CHECK_ERROR("Invalid arguments for "+labels[id]+" qset");
                col.qset(ptr.sidx,data,width);
            });
            overload_type(id,".\"set\"",label+"_SET",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" set");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                Value keyv = ctx.node().right().c0().value();
                void* data = ctx.node().right().get(1);
                CHECK_ERROR("Invalid arguments for "+labels[id]+" set");
                if(keyv.type()==int_id) {
                    uint32_t index = *(int*)keyv.get();
                    if(index>=col.length()) {
                        throw_error("script:set Index "+std::to_string(index)+" out of bounds on "+Ptr_as_string(ptr));
                        return;
                    }
                    col.set(index,data);
                } else if(is_ptr_alias(keyv.type())) {
                    Col& keycol = resolve_to_col(*(Ptr*)keyv.get());
                    uint32_t index = col.getidx(keycol.storage,keycol.size);
                    CHECK_ERROR("Key not found in set");
                    col.set(index,data);
                }
            });

            overload_type(id,".\"label\"",label+"_LABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" label");
                if(!ctx.node().right().children().empty()) {
                    string label = ctx.node().right().getString(0);
                    resolve_YAPA_ptr(p,YAPA_level).label = label.to_std();
                } else {
                    string output = resolve_string_ticket(ctx.node());
                    output = resolve_YAPA_ptr(p,YAPA_level).label.to_std();
                }
            });
            overload_type(id,".\"tag\"",label+"_TAG",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" tag");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(!ctx.node().right().children().empty()) {
                    uint32_t tag = ctx.node().right().getInt(0);
                    col.tag = tag;
                } else {
                    uint32_t tag = col.tag;
                    ctx.node().value().set((void*)&tag);
                }
            });
            overload_type(id,".\"element_size\"",label+"_ELEMENT_SIZE",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" element_size");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                if(!ctx.node().right().children().empty()) {
                    uint32_t element_size = ctx.node().right().getInt(0);
                    col.element_size = element_size;
                } else {
                    uint32_t element_size = col.element_size;
                    ctx.node().value().set((void*)&element_size);
                }
            });

            overload_type(id,".\"celllabel\"",label+"_CELLLABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = ctx.node().getPtr(0);
                CHECK_ERROR("Left arg of cellabel is invalid");

                if(!ctx.node().right().children().empty()) {
                    string label = ctx.node().right().getString(0);
                    Col& cellcol = resolve_YAPA_ptr(p,YAPA_level);
                    CHECK_ERROR("Unable to resolve some elmenets of celllabel, label pooltag or cellcol");
                     //We only suppourt string keys for now
                    while(cellcol.cells.count()<=p.sidx) {
                        CCol c; //Temporary filler
                        char defc = ' ';
                        c.element_size = 1; 
                        c.tag = string_id;
                        c.hash = hashBytes((void*)&defc, 1);
                        c.index = cellcol.cells.count();
                        c.push((void*)&defc);
                        cellcol.cells.scan_for_slot(c);
                    }
                    CCol* cellptr = cellcol.cells.find_cell(p.sidx);
                    if(!cellptr) {
                        throw_error("No cell was found for sidx ",p.sidx," in celllabel of count ",cellcol.cells.count());
                        return;
                    }
                    CCol& cell = *cellptr;
                    cell.clear();
                    cell.element_size = label.length();
                    cell.hash = hashBytes(resolve_ptr(label), label.length());
                    cell.index = p.sidx;
                    cell.push(resolve_ptr(label)); 
                } else {
                    string output = resolve_string_ticket(ctx.node());
                    Col& cellcol = resolve_YAPA_ptr(p,YAPA_level);
                    CCol* cell = cellcol.cells.find_cell(p.sidx);
                    if(cell) {
                        output = ((QString&)*cell).to_std();
                    } else {
                        output = "";
                    }
                }
            });

            overload_type(id,".\"clear\"",label+"_CLEAR",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                col.clear();
            });

            overload_type(id,".\"unlock\"",label+"_UNLOCK",deadptr,[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" unlock");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                col.unlock();
            });
            overload_type(id,".\"try_lock\"",label+"_TRY_LOCK",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                bool b = false;
                if(ctx.node().right().children().empty()) {
                    b = col.try_lock();
                } else {
                    float wait = ctx.node().right().getFloat(0);
                    b = col.try_lock(wait);
                }
                ctx.node().value().set((void*)&b);
            });
            overload_type(id,".\"try_lock_forever\"",label+"_TRY_LOCK_FOREVER",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock_forever");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                bool b = col.try_lock_forever();
                ctx.node().value().set((void*)&b);
            });

            if(YAPA_level==3) {
                overload_type(id,".\"acquire\"",label+"_ACQUIRE",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" acquire");
                    ColColCol& subunit = resolve_to_subunit(ptr);
                    CHECK_ERROR("Failed to resolve Ptr to subunit in acquire");
                    bool b = acquire_subunit(&subunit);
                    ctx.node().value().set((void*)&b);
                });

                overload_type(id,".\"save\"",label+"_SAVE",deadptr,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" save");
                    ColColCol& subunit = resolve_to_subunit(ptr);
                    CHECK_ERROR("Failed to resolve Ptr to subunit in save");
                    save_subunit(&subunit);
                });
                overload_type(id,".\"bounce\"",label+"_BOUNCE",deadptr,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" bounce");
                    ColColCol& subunit = resolve_to_subunit(ptr);
                    CHECK_ERROR("Failed to resolve Ptr to subunit in bounce");
                    bounce_subunit(&subunit);
                });
                overload_type(id,".\"load\"",label+"_LOAD",deadptr,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" load");
                    ColColCol& subunit = resolve_to_subunit(ptr);
                    CHECK_ERROR("Failed to resolve Ptr to subunit in load");
                    load_subunit(&subunit);
                });
            }

            if(YAPA_level>1) {
                overload_type(id,".\"getByTag\"",label+"_GETBYTAG",get_value,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" getByTag");
                    int tag = ctx.node().right().getInt(0);
                    CHECK_ERROR("Invalid tag argument");
                    Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                    for(int at=0;at<col.length();at++) {
                        int i = (ptr[YAPA_level] + at) % col.length();
                        int coltag = -1;
                        switch(YAPA_level) {
                            case 4: coltag = ((PtrColColCol&)col)[i]->tag; break;
                            case 3: coltag = ((ColColCol&)col)[i].tag; break;
                            case 2: coltag = ((ColCol&)col)[i].tag; break;
                            default: break;
                        }
                        if(coltag==tag) {
                            ptr[YAPA_level] = i;
                            ctx.node().value().set((void*)&ptr);
                            return;
                        }
                    }
                    ctx.node().value().set((void*)&deadptr);
                });
                overload_type(id,".\"getByLabel\"",label+"_GETBYLABEL",get_value,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" getByLabel");
                    string qlabel = ctx.node().right().getString(0);
                    CHECK_ERROR("Invalid label argument");
                    std::string label = qlabel.to_std();
                    Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                    for(int i=0;i<col.length();i++) {
                        std::string collabel = "";
                        switch(YAPA_level) {
                            case 4: collabel = ((PtrColColCol&)col)[i]->label.to_std(); break;
                            case 3: collabel = ((ColColCol&)col)[i].label.to_std(); break;
                            case 2: collabel = ((ColCol&)col)[i].label.to_std(); break;
                            default: break;
                        }
                        if(collabel==label) {
                            ptr[YAPA_level] = i;
                            ctx.node().value().set((void*)&ptr);
                            return;
                        }
                    }
                    throw_error("Label not found ",label," in pool ",Ptr_as_string(ptr));
                });
                overload_type(id,".\"add\"",label+"_ADD",get_value,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
                    Col& col = resolve_YAPA_ptr(ptr,YAPA_level);

                    //Infer the label and type
                    Node args = ctx.node().right();
                    std::string label = "";
                    Value typeval = deadptr;
                    uint32_t tag = 0;
                    for(int i=0;i<2;i++) {
                        if(args.children().length()<=i) break;
                        Node arg = args.children()[i];
                        if(arg.type()==arg.value().type()) { 
                            typeval = arg.value();
                        } else {
                            if(arg.value().type()==string_id) {
                                label = arg.getString().to_std();
                            } else if(arg.value().type()==int_id) {
                                tag = arg.getInt();
                            }
                        }
                    }
                    CHECK_ERROR("Bad args for add in "+labels[id]+" add");

                    uint32_t index = 0;
                    Ptr p = deadptr;
                    switch(YAPA_level) {
                        case 4: {
                            index  = col.length();
                            ColColCol* new_subunit = ((PtrColColCol&)col).create(label);
                            new_subunit->tag = tag;
                            p = Ptr(new_subunit,0,0,0);
                        } break;
                        case 3: {
                            index  = col.length();
                            p = Ptr(&resolve_to_subunit(ptr),index,0,0);
                            ColCol new_col; new_col.tag = tag; ((ColColCol&)col).push(new_col);
                        } break;
                        case 2: {
                            index = col.length();
                            ColColCol& subunit = resolve_to_subunit(ptr);
                            p = Ptr(&subunit,subunit.indexof(&col),index,0);
                            Col new_col; 
                            new_col.tag = tag;
                            if(is_live(typeval)) {
                                new_col.tag = typeval.type();
                                new_col.element_size = typeval.size();
                            }
                            ((ColCol&)col).push(new_col);
                        } break;
                        default: break;
                    }
                    CHECK_ERROR("Error while adding in "+labels[id]+" add");

                    if(!label.empty()) {
                        switch(YAPA_level) {
                            case 3: {
                                col.addcell(index,label.data(),label.size(),string_id);
                                ((ColColCol&)col).get(index).label = label;
                            } break;
                            case 2: {
                                col.addcell(index,label.data(),label.size(),string_id);
                                ((ColCol&)col).get(index).label = label;
                            } break;
                            default: break;
                        }
                    }
                    ctx.node().value().set((void*)&p);
                });
            } else {
                overload_type(id,".\"retype\"",label+"_RETYPE",deadptr,[this,id,YAPA_level](Context& ctx){
                    standard_sub_process(ctx);
                    Ptr ptr = ctx.node().getPtr(0);
                    CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
                    Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                    Value val = ctx.node().right().c0().value();
                    col.tag = val.type();
                    col.element_size = val.size();
                });
            }

            return id;
        }

        uint32_t make_YAPA_type(const std::string& label, uint32_t YAPA_level) {
            uint32_t id = make_type(label,sizeof(Ptr));
            register_ptr_alias(id);
            register_YAPA_type(label,YAPA_level,id);
            return id;
        }


        uint32_t ptr_as_YAPA_id = register_YAPA_type("Ptr",1,ptr_id);
        uint32_t colcol_as_YAPA_id = register_YAPA_type("ColCol",2,colcol_id);
        uint32_t colcolcol_as_YAPA_id = register_YAPA_type("ColColCol",3,colcolcol_id);
        uint32_t ptrcolcolcol_as_YAPA_id = register_YAPA_type("PtrColColCol",4,subunit_id);
        

        uint32_t string_as_YAPA_id = register_YAPA_type("string",1,string_id);
        uint32_t header_as_YAPA_id = register_YAPA_type("Header",2,header_id);

        uint32_t c3_header_id = overload_type(colcolcol_id,".\"header\"","ColColCol_HEADER",make_value(header_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            ColColCol& col = resolve_to_subunit(ptr);

            int nth = 0;
            if(ctx.node().right().children().length()==1) {
                nth = ctx.node().right().getInt(0);
            }

            int pool_at = getpool(col,headerpool_id,nth);
            if(pool_at==-1) {
                ColCol header; header.tag = headerpool_id;
                if(col.empty()) {
                    ptr.pool = 0;
                    col.push(header);
                } else {
                    ptr.pool = 1;
                    insert_pools(col,{&header},1);
                }
            } else {
                ptr.pool = pool_at;
            }
            if(pool_at==-1) { //If newely created give it a ribbon
                Header header(ptr);
                header.add_ribbon();
            }
            ctx.node().value().set((void*)&ptr);
        });

        // uint32_t string_append_id = reg_id("STRING_APPEND");
        uint32_t string_substr_id = reg_id("STRING_SUBSTR");
        uint32_t string_slice_id = reg_id("STRING_SLICE");
        uint32_t string_find_id = reg_id("STRING_FIND");
        uint32_t string_find_from_id = reg_id("STRING_FIND_FROM");


        uint32_t utypes_id = add_function("utypes",[this](Context& ctx){
            Ptr p(&types,0,0,0); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcolcol_id);
        uint32_t usubunits_id = add_function("usubunits",[this](Context& ctx){
            Ptr p((uint32_t)uid,0,0,0,0); ctx.node().value().set((void*)&p);
        },sizeof(Ptr),subunit_id);
        uint32_t create_pool_id = add_function("create_pool",[this](Context& ctx){
            Ptr p(uid,(uint32_t)types.length(),0,0);
            ColCol new_pool; types.push(new_pool);
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcol_id);
        uint32_t create_pool_at_id = add_function("create_pool_at",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t at = ctx.node().getInt(0);
            Ptr p(uid,at,0,0);
            ColCol new_pool; insert_pools(types,{&new_pool},at);
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcol_id);

        uint32_t load_subunit_id = add_function("load_subunit",[this](Context& ctx){
            standard_sub_process(ctx);
            string label = ctx.node().getString(0);
            CHECK_ERROR("No label provided for load_subunit");
            Ptr p = load_subunit(label.to_std());
            CHECK_ERROR("Error while trying to load subunit");
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcolcol_id);
        uint32_t save_subunit_id = add_function("save_subunit",[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr p = ctx.node().getPtr(0);
            ColColCol& sub = resolve_to_subunit(p);
            save_subunit(&sub);
        });
        uint32_t acquire_id = add_function("acquire",[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("No subunit provided for acquire");
            if(is_live(ctx.node().scope())) {
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in acquire");
                if(acquire_subunit(&subunit)) {
                    standard_travel_pass(ctx.node().scope(),ctx.sub());
                    subunit.unlock();
                }
            } else {
                throw_error("script:acquire was not provided with any scope");
            }   
        });


        uint32_t break_id = add_function("break",[this](Context& ctx){
            ctx.state(2);
        });
        uint32_t continue_id = add_function("continue",[this](Context& ctx){
            ctx.state(3);
        });

        uint32_t suspend_unit_id = add_function("suspend_unit",[this](Context& ctx){print("Suspended unit"); suspend();});
        uint32_t thread_sleep_id = add_function("thread_sleep",[this](Context& ctx){std::this_thread::sleep_for(std::chrono::milliseconds(100));});
        uint32_t unit_sleep_id = add_function("unit_sleep",[this](Context& ctx){types.live = false;});
        uint32_t unit_wake_id = add_function("unit_wake",[this](Context& ctx){types.live = true;});
        uint32_t unit_index_id = add_function("unit_index",[this](Context& ctx){ctx.node().value().set((void*)&types.index);},4,int_id);
        uint32_t unit_uid_id = add_function("unit_uid",[this](Context& ctx){uint32_t tuid = (uint32_t)uid; ctx.node().value().set((void*)&tuid);},4,int_id);
        uint32_t unit_setindex_id = add_function("unit_setindex",[this](Context& ctx){int idx = *(int*)ctx.node().children()[0].value().get(); types.index=idx;});
        uint32_t unit_label_id = add_function("unit_label",[this](Context& ctx){string s = resolve_string_ticket(ctx.node()); s = types.label.to_std();},sizeof(Ptr),string_id);

        uint32_t unit_date_id = add_function("unit_date",[this](Context& ctx){
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
            string s = resolve_string_ticket(ctx.node());
            s = std::string(buf);
        },sizeof(Ptr),string_id);
        
        uint32_t unit_time_id = add_function("unit_time",[this](Context& ctx){
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
            string s = resolve_string_ticket(ctx.node());
            s = std::string(buf);
        },sizeof(Ptr),string_id);

        uint32_t unit_standard_time_id = add_function("unit_standard_time",[this](Context& ctx){
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%a, %b %e, %I:%M %p", tm);
            std::string result(buf);
            auto pos = result.find("  ");
            if(pos != std::string::npos) result.erase(pos, 1);
            string s = resolve_string_ticket(ctx.node());
            s = result;
        },sizeof(Ptr),string_id);
        
        uint32_t unit_time_precise_id = add_function("unit_time_precise",[this](Context& ctx){
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03lld",
                tm->tm_hour, tm->tm_min, tm->tm_sec, (long long)ms.count());
            string s = resolve_string_ticket(ctx.node());
            s = std::string(buf);
        },sizeof(Ptr),string_id);

        uint32_t unit_timestamp_id = add_function("unit_timestamp",[this](Context& ctx){
            uint32_t ts = (uint32_t)std::time(nullptr);
            string s = resolve_string_ticket(ctx.node());
            s = std::to_string(ts);
        },sizeof(Ptr),string_id);

        uint32_t header_ribbonT_id = overload_type(header_id,".\"ribbonT\"","HEADER_RIBBONT",make_value(header_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header ribbon");
            std::string ribbon_label = "";
            if(!ctx.node().right().children().empty()) {
                string gotstr = ctx.node().right().getString(0);
                CHECK_ERROR("Invalid string in header ribbon");
                ribbon_label = gotstr.to_std();
            }
            if(ribbon_label.empty()) {
                ptr.idx = 0;
            } else {
                Header header(ptr); 
                if(header.col().hasKey(ribbon_label)) {
                    ptr.idx = header.col().getidx(ribbon_label.data(),ribbon_label.size());
                } else {
                    ptr.idx = header.add_ribbon(ribbon_label);
                }
            }
            Header header(ptr);
            Node scope = ctx.node().right().scope();
            if(!is_live(scope)) {
                scope = ctx.node().scope();
            }
            if(is_live(scope)) {
                standard_travel_pass(scope,ctx.sub());
                std::string label = "";
                for(int i=0;i<scope.children().length();i++) {
                    Node c = scope.children()[i];
                    if(i%2==0) {label = c.getString().to_std();}
                    else {
                        if(c.value().type()==string_id) {
                            header.putString(label,c.getString().to_std());  
                        } else if(c.type()==group_id) {
                            if(!c.children().empty()) {
                                Ptr ticket = get_ticket(ptr,c.c0().value().size(),c.c0().value().type());
                                if(c.c0().type()!=c.c0().value().type()) { //Check if it's just a type identifer like [char] vs a list of elements
                                    for(int e=0;e<c.children().length();e++) {
                                        resolve_to_col(ticket).push(c.get(e));
                                    }
                                }
                                resolve_to_col(ptr).put(label,(void*)&ticket,string_id);
                            }
                        }
                    }                 
                }
            }
            ctx.node().value().set((void*)&ptr);
        });

        uint32_t header_ribbon_id = overload_type(header_id,".\"ribbon\"","HEADER_RIBBON",make_value(header_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header ribbon");
            std::string ribbon_label = "";
            if(!ctx.node().right().children().empty()) {
                string gotstr = ctx.node().right().getString(0);
                CHECK_ERROR("Invalid string in header ribbon");
                ribbon_label = gotstr.to_std();
            }
            if(ribbon_label.empty()) {
                ptr.idx = 0;
            } else {
                Header header(ptr); 
                if(header.col().hasKey(ribbon_label)) {
                    ptr.idx = header.col().getidx(ribbon_label.data(),ribbon_label.size());
                } else {
                    ptr.idx = header.add_ribbon(ribbon_label);
                }
            }
            ctx.node().value().set((void*)&ptr);
        });
        uint32_t header_ribbons_id = overload_type(header_id,".\"ribbons\"","HEADER_RIBBONS",make_value(ptr_id,sizeof(Ptr),0,ptr_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header ribbons");
            if(ctx.node().value().data_col().empty()||!is_live(ctx.node().getPtr())) {
                Ptr ticket = get_ticket(ptr,sizeof(Ptr),ptr_id);
                ctx.node().value().set((void*)&ticket);
            }
            Col& data = resolve_to_col(ctx.node().getPtr());
            data.clear(); data.cells.clear(); //Later I can do some caching, but for now it's not really going to save any performance over correctness

            QCellCol& cells = resolve_to_pool(ptr).cells;
            for(int i=0;i<cells.length();i++) {
                CCol& cell = cells.get(i);
                if(cell.storage) {
                    Ptr rbn_ptr = ptr;
                    rbn_ptr.idx = cell.index;
                    std::string tpr((const char*)cell.storage,cell.size);
                    data.put(tpr,(void*)&rbn_ptr,string_id);
                }
            }
        });

        uint32_t header_col_id = overload_type(header_id,".\"col\"","HEADER_COL",make_value(colcol_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header col");
            ctx.node().value().set((void*)&ptr);
        });
        uint32_t header_getString_id = overload_type(header_id,".\"getString\"","HEADER_GETSTRING",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header getString");
            std::string label = "";
            if(!ctx.node().right().children().empty()) {
                string gotstr = ctx.node().right().getString(0);
                CHECK_ERROR("Invalid first string in header getString");
                label = gotstr.to_std();
            }
            std::string ribbon_label = "";
            if(ctx.node().right().children().length()==2) {
                string gotstr = ctx.node().right().getString(1);
                CHECK_ERROR("Invalid second string in header getString");
                ribbon_label = gotstr.to_std();
            }
            Header header(ptr); 
            string data = header.getString(label,ribbon_label);
            CHECK_ERROR("Key not found ",label," in header getString");
            string output = resolve_string_ticket(ctx.node());
            output = data.to_std();
        });
        uint32_t header_putString_id = overload_type(header_id,".\"putString\"","HEADER_PUTSTRING",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid ptr in header putString");
            std::string label = "";
            if(!ctx.node().right().children().empty()) {
                string gotstr = ctx.node().right().getString(0);
                CHECK_ERROR("Invalid first string in header putString");
                label = gotstr.to_std();
            }
            std::string valstr = "";
            if(ctx.node().right().children().length()>1) {
                string gotstr = ctx.node().right().getString(1);
                CHECK_ERROR("Invalid second string in header putString");
                valstr = gotstr.to_std();
            }
            std::string ribbon_label = "";
            if(ctx.node().right().children().length()==3) {
                string gotstr = ctx.node().right().getString(2);
                CHECK_ERROR("Invalid third string in header putString");
                ribbon_label = gotstr.to_std();
            }
            Header header(ptr); 
            header.putString(label,valstr,ribbon_label);
        });

        uint32_t check_messages_id = add_function("check_messages",[this](Context& ctx){
            if(ctx.node().children().empty()) {
                throw_error("script:check_messages no header argument was provided");
                return;
            }
            standard_sub_process(ctx);
            bool b = false;
            ColColCol msgpools = check_messages();
            if(msgpools.empty()) {
                ctx.node().value().set((void*)&b);
            } else {
                adopt_ptrs(msgpools,&types);
                uint32_t at = types.length();
                push_pools(types,msgpools);
                Header header = makePtr(types,at);
                print("Returning a found header: ",Ptr_as_string(header));
                ctx.node().getPtr(0) = header;
                b = true;
                ctx.node().value().set((void*)&b);
            }
        },1,bool_id);
        uint32_t discard_message = add_function("discard_message",[this](Context& ctx){
            if(ctx.node().children().empty()) {
                throw_error("script:discard_message no header argument was provided");
                return;
            }
            standard_sub_process(ctx);
            Ptr p = ctx.node().getPtr(0);
            types.removeAt(p.pool);
        });


        uint32_t save_pool_id = add_function("save_pool",[this](Context& ctx){
            standard_sub_process(ctx);
            ColCol& col = resolve_to_pool(ctx.node().getPtr(0));
            string path = ctx.node().getString(1);
            auto out = openWriteStream(path.to_std());
            snapshot_colcol(out,col);
        });

        ColCol func_load_pool(Context& ctx) {
            standard_sub_process(ctx);
            string path = ctx.node().getString(0);
            auto in = openReadStream(path.to_std());
            return load_snapshot_colcol(in);
        }
        uint32_t load_pool_id = add_function("load_pool",[this](Context& ctx){
            Ptr p(uid,(uint32_t)types.length(),0,0);
            ColCol col = func_load_pool(ctx);
            types.push(col);
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcol_id);
        uint32_t load_pool_at_id = add_function("load_pool_at",[this](Context& ctx){
            ColCol col = func_load_pool(ctx);
            uint32_t at = ctx.node().getInt(1);
            Ptr p(uid,at,0,0);
            insert_pools(types,{&col},at);
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),colcol_id);


        uint32_t check_equality_int = overload_type(int_id,"==int","CHECK_EQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()==*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_noequality_int = overload_type(int_id,"!=int","CHECK_NOEQUALITY_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()!=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_equality_node = overload_type(node_id,"==node","CHECK_EQUALITY_NODE",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (ctx.node().getNode(0)==ctx.node().getNode(1));
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_noequality_node = overload_type(node_id,"!=node","CHECK_NOEQUALITY_NODE",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (ctx.node().getNode(0)!=ctx.node().getNode(1));
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_equality_ptr = overload_type(ptr_id,"==ptr","CHECK_EQUALITY_PTR",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (ctx.node().getPtr(0)==ctx.node().getPtr(1));
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_noequality_ptr = overload_type(ptr_id,"!=ptr","CHECK_NOEQUALITY_PTR",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (ctx.node().getPtr(0)!=ctx.node().getPtr(1));
            ctx.node().value().set((void*)&result);
        });

        uint32_t check_equality_string = overload_type(string_id,"==string","CHECK_EQUALITY_STRING",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            bool result = false;
            if(l.length()!=r.length()) {ctx.node().value().set((void*)&result); return;}
            for(int i=0;i<l.length();i++) {
                if(l.at(i)!=r.at(i)) {
                    ctx.node().value().set((void*)&result);
                    return;
                }
            }
            result = true;
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_noequality_string = overload_type(string_id,"!=string","CHECK_NOEQUALITY_STRING",make_value(bool_id,1),[this](Context& ctx){
            x_handlers.run(check_equality_string)(ctx);
            bool b = !(*(bool*)ctx.node().value().get());
            ctx.node().value().set((void*)&b);
        });

        uint32_t check_lessthan_or_equalsto_int = overload_type(int_id,"<=int","CHECK_LEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()<=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_or_equalsto_int = overload_type(int_id,">=int","CHECK_GEQ_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            bool result = (*(int*)ctx.node().children()[0].value().get()>=*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_lessthan_int = overload_type(int_id,"<int","CHECK_LT_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = (*(int*)ctx.node().children()[0].value().get()<*(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });
        uint32_t check_greaterthan_int = overload_type(int_id,">int","CHECK_GT_INT",make_value(bool_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            DEBUG_ONLY(if(ERROR_FLAG) {return;});
            bool result = (*(int*)ctx.node().children()[0].value().get() > *(int*)ctx.node().children()[1].value().get());
            ctx.node().value().set((void*)&result);
        });

        uint32_t increment_int = overload_type(int_id,"++int","INCREMENT_INT",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node().value(ctx.node().children()[0].value());
            int inced = *(int*)ctx.node().value().get()+1;
            if(ctx.node().value().data_col().heterogenous) {
                ctx.node().value().data_col().qset(ctx.node().value().data_ptr().sidx,(void*)&inced,ctx.node().value().size());
            } else {
                ctx.node().value().set((void*)&inced);
            }
        });


        uint32_t valueGet_id = overload_type(value_id,".\"get\"","VALUE_GET",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t valueGetStr_id = overload_type(value_id,".\"getStr\"","VALUE_GETSTR",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t valueGetNode_id = overload_type(value_id,".\"getNode\"","VALUE_GETNODE",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
            standard_sub_process(ctx);
            Node v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t valueGetInt_id = overload_type(value_id,".\"getInt\"","VALUE_GETINT",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            ctx.node().value(v);
        });
        uint32_t value_set_id = overload_type(value_id,".\"set\"","VALUE_SET",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            Value v = (Value&)*(Ptr*)ctx.node().children()[0].value().get();
            void* d = ctx.node().children()[1].children()[0].value().get();
            v.set(d);
        });
        uint32_t give_value_id = add_function("give_value",[this](Context& ctx){
            standard_sub_process(ctx);
            ctx.node().getNode(0).value(ctx.node().getValue(1));
        });

        uint32_t node_asid_id = overload_type(node_id,".\"asID\"","NODE_ASID",make_value(int_id,4),[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t id = (*(Ptr*)ctx.node().children()[0].value().get()).idx;
            ctx.node().value().set((void*)&id);
        });

        uint32_t std_sub_proccess_id = add_function("subprocess",[this](Context& ctx){
            standard_sub_process(ctx);
            Context c = (Context&)*(Ptr*)ctx.node().children()[0].value().get();
            standard_sub_process(c);
        });

        //Investigate why this isn't working later
        // uint32_t context_doat = overload_type(context_id,".\"p\"","DOAT",deadptr,[this](Context& ctx){
        //     standard_sub_process(ctx);
        //     Context context = (Context&)*(Ptr*)ctx.node().children()[0].value().get();
        //     print("Source ptr of ",Ptr_as_string(context),": ",Ptr_as_string(context.source_ptr()));
        // });


        uint32_t string_equals_id = overload_type(string_id,"=string","STRING_EQUALS",deadptr,[this](Context& ctx){
            standard_sub_process(ctx);
            string l = resolve_string_ticket(ctx.node().children()[0]);
            string r = resolve_string_ticket(ctx.node().children()[1]);
            l = r;
        });

        uint32_t string_append_id = overload_type(string_id,"+string","STRING_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l = resolve_string_ticket(ctx.node().children()[0]);
            string r = resolve_string_ticket(ctx.node().children()[1]);
            string output = resolve_string_ticket(ctx.node());
            output.col().clear(); output.push(l.to_std()); output.push(r.to_std());
        });

        uint32_t string_func_append_id = overload_type(string_id,".\"append\"","STRING_FUNC_APPEND",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            Node left = ctx.node().children()[0]; Node right = ctx.node().children()[1];
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            Node right_child = ctx.node().children()[1].children()[0];
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            void* lv = left.value().get(); void* rv = right_child.value().get();
            DEBUG_ONLY(if(ERROR_FLAG) return;)
            string l(*(Ptr*)lv);
            string r(*(Ptr*)rv);
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });        

        uint32_t string_append_eq_id = overload_type(string_id,"+=string","STRING_APPEND_EQ",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
            standard_sub_process(ctx);
            string l(*(Ptr*)ctx.node().children()[0].value().get());
            string r(*(Ptr*)ctx.node().children()[1].value().get());
            l.push(r.to_std());
            ctx.node().value(ctx.node().children()[0].value());
        });

        uint32_t split_str_id = add_function("split_str",[this](Context& ctx){
            standard_sub_process(ctx);
            std::string str = ((string&)(*(Ptr*)ctx.node().children()[0].value().get())).to_std();
            std::string delimiter = ((string&)(*(Ptr*)ctx.node().children()[1].value().get())).to_std();
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            list<std::string> split = split_str(str, delimiter.at(0));
            while(data.length() > split.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<split.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = split[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = split[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);


        uint32_t colsize_id = add_function("_colsize",[this](Context& ctx){
            Node left = ctx.node().children()[0];
            
        },4,int_id);

        uint32_t ptr_size_id = add_function("ptr_size",[this](Context& ctx){
            uint32_t s = sizeof(Ptr);
            ctx.node().value().set((void*)&s);
        },4,int_id);

        uint32_t make_value_id = add_function("make_value",[this](Context& ctx){
            Value v = make_value();
            if(ctx.node().children().length()==2) {
                standard_sub_process(ctx);
                int type = *(int*)ctx.node().children()[0].value().get();
                int size = *(int*)ctx.node().children()[1].value().get();
                v.type(type); v.size(size);
                v.init_data();
            }
            ctx.node().value().set((void*)&v);
        },sizeof(Ptr),value_id);

        uint32_t recycle_id = add_function("recycle",[this](Context& ctx){
            standard_sub_process(ctx);
            Node to_recycle = (Node&)(*(Ptr*)ctx.node().children()[0].value().get());
            recycle_node(to_recycle);
        });

        uint32_t stoi_id = add_function("stoi",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            int stoid = std::stoi(s.to_std());
            ctx.node().value().set((void*)&stoid);
        },4,int_id);

        uint32_t string_to_Ptr_id = add_function("string_to_Ptr",[this](Context& ctx){
            standard_sub_process(ctx);
            string s = (string&)*(Ptr*)ctx.node().children()[0].value().get();
            Ptr p = string_to_Ptr(s.to_std());
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);


        uint32_t make_unit_id = add_function("make_unit",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t unitid = (uint32_t)make_unit<Unit>()->uid;
            ctx.node().value().set((void*)&unitid);
        },4,int_id);

        uint32_t randi_id = add_function("randi",[this](Context& ctx){
            standard_sub_process(ctx);
            int min = *(int*)ctx.node().children()[0].value().get();
            int max = *(int*)ctx.node().children()[1].value().get();
            int rand = randi(min,max);
            ctx.node().value().set((void*)&rand);
        },4,int_id);

        uint32_t unit_error_id = add_function("unit_error",[this](Context& ctx){
            ctx.node().set((void*)&UERROR_FLAG);
        },1,bool_id);
        uint32_t unit_errors_id = add_function("unit_errors",[this](Context& ctx){
            Ptr p = resolve_ticket(ctx.node(),sizeof(Ptr),string_id);
            Col& data = resolve_to_col(p);
            while(data.length() > UERRORS.length()) {
                recycle_column(*(Ptr*)data.last());
                data.removeAt(data.length()-1);
            }
            for(int i=0;i<UERRORS.length();i++) {
                if(i<data.length()) {
                    string s = (string&)*(Ptr*)data[i];
                    s = UERRORS[i];
                } else {
                    string s = get_ticket(name_store_id,1,char_id);
                    s = UERRORS[i];
                    data.push((void*)&s);
                }
            }
            ctx.node().value().set((void*)&p);
        },sizeof(Ptr),ptr_id);


        uint32_t gather_pools_id = add_function("gather_pools",[this](Context& ctx){

        },sizeof(Ptr),ptr_id);

        void e_stage_assignment_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage assignment handler because one of the values is missing"));
                return;
            }
            ctx.node().quals().push((make_node(live_qual))); //This liveness thing is kludgy but I'm tired and just trying to get ctx.source = "wub" to work
            process_node(ctx,left);
            if(left.has_qual(live_qual)) { //Transfer liveness
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
            } else {
                ctx.node().quals().pop(); //Kill it
            }

            int const_at = left.value().find_qual(constant_qual);
            if(const_at==-1) { //A value is constant if it's only asigned once
                if(!left.has_qual(assigned_qual)) {
                    left.value().quals().push(make_node(constant_qual));
                }
            } else {
                left.value().quals().removeAt(const_at);
            }

            if(!left.has_qual(assigned_qual)) {
                left.value().quals().push(make_node(assigned_qual));
            }
        };
        void e_stage_binary_handler(Context& ctx) {
            Node left = ctx.node().children()[0];
            Node right = ctx.node().children()[1];
            if(!is_live(left.value())||!is_live(right.value())) {
                log(red("Unable to do e stage binary handler because one of the values is missing"));
                return;
            }
            if(ctx.node().has_qual(live_qual)) {
                if(!right.has_qual(live_qual)) {
                    right.value().quals().push(make_node(live_qual));
                }
                if(!left.has_qual(live_qual)) {
                    left.value().quals().push(make_node(live_qual));
                }
            } else {
                if(left.has_qual(live_qual)) { //Transfer liveness
                    ctx.node().quals().push((make_node(live_qual)));
                    if(!right.has_qual(live_qual)) {
                        right.value().quals().push(make_node(live_qual));
                    }
                } else if(right.has_qual(live_qual)) {
                    ctx.node().quals().push((make_node(live_qual)));
                    left.value().quals().push(make_node(live_qual));
                }
            }
        };
        void e_scoped_handler(Context& ctx) {
            if(!ctx.node().scopes().empty()) {
                Node scope = ctx.node().scopes()[0];
                for(int i=0;i<scope.children().length();i++) {
                    if(scope.children()[i].has_qual(live_qual)) {
                        ctx.node().quals().push(make_node(live_qual));
                        break;
                    }
                }
            }
            if(ctx.node().has_qual(live_qual)) {
                for(int i=0;i<ctx.node().children().length();i++) {
                    Node c = ctx.node().children().get(i);
                    if(is_live(c.value())&&!c.has_qual(live_qual)) {
                        c.value().quals().push(make_node(live_qual));
                    } else if(!is_live(c.value())) {
                        c.quals().push(make_node(live_qual));
                    }
                }
            }
            standard_sub_process(ctx);
        }

        void e_pass_prune_dead(node_col nodes) {
            for(int i=nodes.length()-1;i>=0;i--) {
                Node node = nodes[i];
                log("Pruning: ",node_info(node));
                e_pass_prune_dead(node.children());
                for(int s = 0;s<node.scopes().length();s++) {
                    if(node.scopes()[s].owner()==node)
                        e_pass_prune_dead(node.scopes()[s].children());
                }
                if(!node.has_qual(live_qual)) {
                    log(yellow("Recycled: "+Ptr_as_string(nodes.get(i))));
                    recycle_node(nodes.get(i));
                    nodes.removeAt(i);
                }
            }
        };

        uint32_t bumpid = add_function("BUMP",[](Context& ctx){});

        void m_stage_assignment_handler(Context& ctx) {
            standard_sub_process(ctx); //Not sure what to do with this yet
        };
        bool should_allocate_data(Value v) {
            return (is_live(v)&&!is_live(v.data_ptr())&&v.type()!=0);
        }
        bool should_recycle_data(Value v) {
            return (is_live(v)&&is_live(v.data_ptr())&&v.type()!=0);
        }
        void allocate_data(Value v) {
            v.data_ptr(get_ticket(data_store_id,v.size(),v.type()));
            if(v.data_col().empty()) {
                v.data_col().push_default();
            }
        }

        uint32_t precompile_brace = add_token_combo("precompile_brace",'#','#');
        uint32_t comment_brace = add_token_combo("comment_brace",'/','/');

        void init() override {
            register_type("list",ptr_id,sizeof(Ptr));


            overload_type(string_id,".\"substr\"",string_substr_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"slice\"",string_slice_id,make_value(string_id,sizeof(Ptr),0,char_id,1));
            overload_type(string_id,".\"find\"",string_find_id,make_value(int_id,4));

            overload_type(string_id,"|*^+int",reg_id("THRONGLIZE"),make_value(ptr_id,sizeof(Ptr),0,int_id,4));

            x_handlers[string_substr_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                string str = ctx.node().getString(0);
                int from = ctx.node().right().getInt(0);
                int to = str.length()-from;
                if(ctx.node().right().children().length()>1) {
                    to = ctx.node().right().getInt(1);
                }
                output.col().clear();
                for(int i=from;i<from+to;i++) {
                    output.push(str.at(i));
                }
            };
            x_handlers[string_slice_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                if(!is_live(ctx.node().value().data_ptr())||!is_live(*(Ptr*)ctx.node().value().get())) {
                    Ptr ticket = get_ticket(data_store_id,1,char_id);
                    ctx.node().value().set((void*)&ticket);
                }
                string target(*(Ptr*)ctx.node().value().get());
                Ptr ptr = *(Ptr*)left.value().get();
                int from = 0;
                int to = 0; //Add some deffensive checking here later
                if(right.children()[0].value().type()==string_id) {
                    string refstr(*(Ptr*)right.children()[0].value().get());
                    string fromstr(ptr);
                    from = fromstr.find(refstr,0,*(int*)right.children()[1].value().get())+1;
                    to = fromstr.find(refstr,0,*(int*)right.children()[2].value().get());
                } else {
                    from = *(int*)right.children()[0].value().get();
                    to = *(int*)right.children()[1].value().get();
                }
                target.col().clear();
                for(int i=from;i<to;i++) {
                    target.push(*(char*)resolve_to_col(ptr)[i]);
                }
            };
            x_handlers[string_find_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                Node right = ctx.node().children()[1];
                Ptr ptr = *(Ptr*)left.value().get();
                Col& tcol = resolve_to_col(ptr);
                string refstr(*(Ptr*)right.children()[0].value().get());
                int start_at = 0;
                if(right.children().length()>1) {
                    start_at = *(int*)right.children()[1].value().get();
                }
                int nth_of = 1;
                if(right.children().length()>2) {
                    nth_of = *(int*)right.children()[2].value().get();
                }
                int found_id = string(ptr).find(refstr,start_at,nth_of);
                ctx.node().value().set((void*)&found_id);
            };


            add_function("makePtr3",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().value().get();
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t idx = *(int*)ctx.node().children()[1].value().get();
                uint32_t sidx = *(int*)ctx.node().children()[2].value().get();
                p.pool = pool; p.idx = idx; p.sidx = sidx;
                p.cachelevel = 3; p.cache = &types; p.specialization = 2;
            },sizeof(Ptr),ptr_id);

            // add_function("makeptr",[this](Context& ctx){
            //     standard_sub_process(ctx);
            //     Ptr p;
            //     uint32_t len = ctx.node().children().length();
            //     if(len==2) {
            //         p = Ptr(ctx.node().getInt(0),ctx.node().getInt(1));
            //     } else if(len==3) {
            //         p = Ptr(ctx.node().getInt(0),ctx.node().getInt(1),ctx.node().getInt(2));
            //     }
            //     // uint32_t pool = *(int*)ctx.node().children()[0].value().get();
            //     // uint32_t idx = *(int*)ctx.node().children()[1].value().get();
            //     // uint32_t sidx = *(int*)ctx.node().children()[2].value().get();
            //     // p.pool = pool; p.idx = idx; p.sidx = sidx;
            //     // p.cachelevel = 3; p.cache = &types; p.specialization = 2;
            // },sizeof(Ptr),ptr_id);


            //Temporary kludge methods until Ptrs become proper heterogenous systems
            add_function("getpool",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.pool);
            },4,int_id);
            add_function("setpool",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.pool = val;
            });
            add_function("getidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.idx);
            },4,int_id);
            add_function("setidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.idx = val;
            });
            add_function("getsidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set((void*)&p.sidx);
            },4,int_id);
            add_function("setsidx",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr& p = *(Ptr*)ctx.node().children()[0].value().get();
                int val = *(int*)ctx.node().children()[1].value().get();
                p.sidx = val;
            });

            uint32_t ptr_setsidx_id = add_binding_token_combo("PTR_SETSIDX_OP",8,2,'|','S','=');
            r_handlers[ptr_setsidx_id] = [this](Context& ctx){standard_sub_process(ctx); resolve_overload(ctx);};
            overload_type(ptr_id, "|S=int", "PTR_SETSIDX", make_value(ptr_id, sizeof(Ptr)), [this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid ptr in ptr_setsidx");
                int val = ctx.node().getInt(1);
                CHECK_ERROR("Invalid val in ptr_setsidx");
                p.sidx = val;
                ctx.node().value().set((void*)&p);
            });

            add_function("plen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                Ptr pptr(&types,pool,0,0);
                uint32_t plen = resolve_to_pool(pptr).length();
                ctx.node().value().set((void*)&plen);
            },4,int_id);

            add_function("clen",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = *(int*)ctx.node().children()[0].value().get();
                uint32_t col = *(int*)ctx.node().children()[1].value().get();
                Ptr cptr(&types,pool,col,0);
                uint32_t clen = resolve_to_col(cptr).length();
                ctx.node().value().set((void*)&clen);
            },4,int_id);

            add_function("clabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = 0;
                uint32_t col = 0;
                if(ctx.node().children()[0].value().type()==ptr_id) {
                    Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                    pool = p.pool; col = p.idx;
                } else {
                    pool = *(int*)ctx.node().children()[0].value().get();
                    col = *(int*)ctx.node().children()[1].value().get();
                }
                Ptr cptr(&types,pool,col,0);
                string output = resolve_string_ticket(ctx.node());
                output = resolve_to_col(cptr).label.to_std();
            },sizeof(Ptr),string_id);

            add_function("csetlabel",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t pool = 0;
                uint32_t col = 0;
                std::string label = "";
                if(ctx.node().children()[0].value().type()==ptr_id) {
                    Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                    pool = p.pool; col = p.idx;
                    label = ((string&)*(Ptr*)ctx.node().children()[1].value().get()).to_std();
                } else {
                    pool = *(int*)ctx.node().children()[0].value().get();
                    col = *(int*)ctx.node().children()[1].value().get();
                    label = ((string&)*(Ptr*)ctx.node().children()[2].value().get()).to_std();
                }
                Ptr cptr(&types,pool,col,0);
                resolve_to_col(cptr).label = label;
                resolve_to_pool(cptr).addcell(col,label.data(),label.size(),string_id);
            });

            add_function("resolve_as_Ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value().set(resolve_ptr(*(Ptr*)ctx.node().children()[0].value().get()));
            },sizeof(Ptr),ptr_id);

            add_function("resolve_as_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr resolved = *(Ptr*)ctx.node().children()[0].value().get();
                ctx.node().value().set(resolve_ptr(resolved));
            },sizeof(Ptr),node_id);

            add_function("cast_to_Node",[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                if(n.cachelevel==3) n.cache = &types; //Figure out later why this isnt' working with string_to_Ptr
                ctx.node().value().set((void*)&n);
            },sizeof(Ptr),node_id);
            add_function("cast_to_Ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = *(Ptr*)ctx.node().children()[0].value().get();
                if(p.cachelevel==3) p.cache = &types; //Figure out later why this isnt' working with string_to_Ptr
                ctx.node().value().set((void*)&p);
            },sizeof(Ptr),ptr_id);


            add_function("value_as_string",[this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                output = value_as_string(*(Ptr*)ctx.node().children()[0].value().get());
            },sizeof(Ptr),string_id);

            add_function("is_live",[this](Context& ctx){
                standard_sub_process(ctx);
                bool b = false;
                Ptr p = ctx.node().getPtr(0);
                // print(Ptr_to_string(p,p.cachelevel),"[",(int)p.specialization,"]");
                b = is_live(p);
                // print(b?"Y":"N");
                ctx.node().value().set((void*)&b);
            },1,bool_id);

            add_function("call_owner_as_string",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Ptr ownerptr = n.scopes()[0].owner();
                string output = resolve_string_ticket(ctx.node());
                output = Ptr_to_string(ownerptr,ownerptr.cachelevel);
            },sizeof(Ptr),string_id);

            add_function("call_owner",[this](Context& ctx){
                //We *don't* standard sub process because we don't want to emit to source
                Node n = ctx.node().children()[0];
                Node owner = n.scopes()[0].owner();
                ctx.node().value().set((void*)&owner);
            },sizeof(Ptr),node_id);

            add_function("cast_to",[this](Context& ctx){
                standard_sub_process(ctx);
                int type = ctx.node().getInt(0);
                ctx.node().value().size(ctx.node().c1().value().size());
                ctx.node().value().type(type);
                ctx.node().value().data_ptr(ctx.node().c1().value().data_ptr());
            });
            

            Handler discard = [this](Context& ctx){
                if(ctx.index()>0) {
                    ctx.result().get(ctx.index()-1).quals() << turn_into_token(ctx.result().take(ctx.index()));
                    ctx.index()--;
                } else if(is_live(ctx.root())) {
                    ctx.root().quals() << turn_into_token(ctx.result().take(ctx.index()));
                    if(!ctx.result().empty()) {
                        ctx.index()--;
                    }
                }
            };
            t_handlers[end_id] = discard;
            t_handlers[comma_id] = discard;
            t_handlers[comment_id] = discard;

            a_handlers[make_tokenized_keyword("register")] = [this](Context& ctx){
                ctx.node().quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                make_tokenized_keyword(ctx.node().quals().last().name().to_std());
            };
            a_handlers[make_tokenized_keyword("newstage")] = [this](Context& ctx){
                ctx.node().quals() << turn_into_token(ctx.result().take(ctx.index()+1));
                reg_stage(ctx.node().quals().last().name().to_std());
            };

            add_function("set_binding_powers",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t type = *(int*)ctx.node().children()[0].value().get();
                int lbp = *(int*)ctx.node().children()[1].value().get();
                int rbp = *(int*)ctx.node().children()[2].value().get();
                set_binding_powers(type,lbp,rbp);
                print("Set the binding powers for ",labels[type]," to ",lbp," ",rbp);
            });

            //Revist this idea later once we have proper closures
            add_function("add_function",[this](Context& ctx){
                standard_sub_process(ctx);
                string label = resolve_string_ticket(ctx.node().children()[0]);
                Node n = (Node&)*(Ptr*)ctx.node().children()[1].value().get();
                add_function(label.to_std(),[this,n](Context& ctx) mutable {
                    // g_ptr<Stage> old_stage = active_stage;

                    // start_stage(x_handlers);
                    // standard_travel_pass(this_node.scopes()[0],ctx);
                    // start_stage(old_stage);
                    
                });
            });

            add_function("capture_ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                Ptr p = ctx.node().getPtr(0);
                output = capture_ptr(p);
            },sizeof(Ptr),string_id);
            add_function("retrive_ptr",[this](Context& ctx){
                standard_sub_process(ctx);
                string s = ctx.node().getString(0);
                Ptr p = string_to_Ptr(s.to_std());
                ctx.node().value().set((void*)&p);
            },sizeof(Ptr),ptr_id);

            add_function("C0",[this](Context& ctx){
                Node c0 = ctx.sub().node().children()[0];
                ctx.node().value().set((void*)&c0);
            },sizeof(Ptr),node_id);
            add_function("C1",[this](Context& ctx){
                Node c1 = ctx.sub().node().children()[1];
                ctx.node().value().set((void*)&c1);
            },sizeof(Ptr),node_id);
    
            overload_type(node_id,".\"c0\"","NODE_c0",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                Node c0 = n.children()[0];
                ctx.node().value().set((void*)&c0);
            });
            overload_type(node_id,".\"c1\"","NODE_c1",make_value(node_id,sizeof(Ptr)),[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = (Node&)*(Ptr*)ctx.node().children()[0].value().get();
                Node c1 = n.children()[1];
                ctx.node().value().set((void*)&c1);
            });

            r_handlers[qmark_id] = [this](Context& ctx){
                standard_sub_process(ctx); //Because the second child is a : so  we just grab it's left value, so that other things know how to resolve against the qmark
                Value lv;
                if(ctx.node().children().length()==3) { //Case where the children are flat
                    lv = ctx.node().children()[1].value();
                } else if(ctx.node().children().length()==2) { //Case where we're using a : or other grouper
                    lv = ctx.node().children()[1].children()[0].value();
                }
                ctx.node().value(lv); //Just the same for now, in the future we can be a bit fancier
                resolve_overload(ctx);
            };
            x_handlers[qmark_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                if(*(bool*)ctx.node().children()[0].value().get()) {
                    if(ctx.node().children().length()==3) { //Case where the children are flat
                        ctx.node().value(ctx.node().children()[1].value());
                    } else if(ctx.node().children().length()==2) { //Case where we're using a : or other grouper
                        ctx.node().value(ctx.node().children()[1].children()[0].value());
                    }
                }
                else {
                    if(ctx.node().children().length()==3) { //Case where the children are flat
                        ctx.node().value(ctx.node().children()[2].value());
                    } else if(ctx.node().children().length()==2) { //Case where we're using a : or other grouper
                        ctx.node().value(ctx.node().children()[1].children()[1].value());
                    }
                }
            };



            add_function("dump_unit",[this](Context& ctx){
                standard_sub_process(ctx);
                bool clear = ctx.node().getBool(0);
                dump_unit(clear);
            });
            add_function("dump_subunit",[this](Context& ctx){
                standard_sub_process(ctx);
                bool clear = ctx.node().getBool(1);
                dump_subunit(resolve_to_subunit(ctx.node().getPtr(0)),clear);
            });
            add_function("dump_pool",[this](Context& ctx){
                standard_sub_process(ctx);
                ColCol* pool = nullptr;
                int poolid = 0;
                if(ctx.node().left().value().type()==int_id) {
                    poolid = ctx.node().getInt(0);
                    CHECK_ERROR("Unable to get poolid from left");
                    pool = &types[poolid];
                } else if(ctx.node().left().value().type()==colcol_id) {
                    Ptr p = ctx.node().getPtr(0);
                    CHECK_ERROR("Unable to get pool from left");
                    pool = &resolve_to_pool(p);
                }
                bool clear = ctx.node().getBool(1);
                CHECK_ERROR("Expected a bool as second arg in dump_pool");
                dump_pool(*pool,poolid,clear);
            });
            add_function("dump_pool_range",[this](Context& ctx){
                standard_sub_process(ctx);
                int from = ctx.node().getInt(0);
                int to = ctx.node().getInt(1);
                bool clear = ctx.node().getBool(2);
                for(int i=from;i<to;i++) {
                    dump_pool(types[i],i,(i==0?clear:false));
                }
            });

            
            add_function("take_left",[this](Context& ctx){
                standard_sub_process(ctx); 
                take_left(ctx.sub(),ctx.node().getInt(0));
            });
            add_function("take_right",[this](Context& ctx){
                standard_sub_process(ctx); 
                take_right(ctx.sub(),ctx.node().getInt(0));
            });

            add_function("col_by_label",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t poolidx = ctx.node().getInt(0);
                std::string label = ctx.node().getString(1).to_std();
                ColCol& pool = types[poolidx];
                for(int i=0;i<pool.length();i++) {
                    if(pool[i].label==label) {
                        Ptr p(&types,poolidx,i,0);
                        ctx.node().value().set((void*)&p);
                        return;
                    }
                }
            },sizeof(Ptr),ptr_id);


            //An experiment
                // uint32_t query_id = make_registering_tokenized_keyword("query");
                // uint32_t queryend_id = make_tokenized_keyword("endquery");
                // uint32_t query_for_id = make_tokenized_keyword("for",reg_id("query_for"),query_id);
                // uint32_t query_in_id = make_tokenized_keyword("in",reg_id("query_in"),query_id);
                
                // tokenizer_state_functions[queryend_id] = [this](Context& ctx){
                //     tokenized_keywords.clear(); 
                //     for(auto& f : token_registers[0]) {f();}
                //     ctx.state(0); at_x-=1.0f; --ctx.index();
                // };
            
            //The new form
                register_ptr_alias(query_id);
                add_function("makeQuery",[this](Context& ctx){
                    standard_sub_process(ctx);
                    Col& sortcol = ctx.node().value().data_col();
                    sortcol.clear();
                    Ptr target = ctx.node().getPtr(0);
                    // print("Making a query to ",Ptr_to_string(target,3));
                    Col& datacol = resolve_to_col(target); 
                    uint32_t datatype = datacol.tag;
                    for(int i=0;i<datacol.length();i++) {
                        Ptr sortptr = target; sortptr.sidx = i;
                        sortcol.push((void*)&sortptr);
                    }
                    ctx.node().value().sub_type(datatype);
                },sizeof(Ptr),query_id);

                overload_type(query_id,".\"sort\"","QUERY_SORT",make_value(query_id,sizeof(Ptr)),[this](Context& ctx){
                    standard_sub_process(ctx);
                    uint32_t datatype = ctx.node().left().value().sub_type();
                    if(datatype==0) { //Kludge
                        Col& datacol = resolve_to_col(ctx.node().left().getPtr()); 
                        datatype = datacol.tag;
                    }

                    Col& sortcol = ctx.node().left().value().data_col();
                    if(sortcol.heterogenous) {sortcol.heterogenous = false;} //No idea how this is happening when we refrence query in overloads, investigate and fix later
                    //Okay, overloads are just broken for query, when I actually need them I'll do a deeper dive.
                    std::string sortkind = ctx.node().right().getString(0).to_std();

                    ctx.node().value(ctx.node().left().value());
                    
                    std::function<bool(Ptr&,Ptr&)> func = nullptr;

                    if(sortkind=="asc"){
                        if(datatype==int_id) {
                            func = [this](Ptr& a, Ptr& b){
                                return *(int*)resolve_ptr(a) < *(int*)resolve_ptr(b);
                            };
                        } else if(datatype==float_id) {
                            func = [this](Ptr& a, Ptr& b){
                                return *(float*)resolve_ptr(a) < *(float*)resolve_ptr(b);
                            };
                        } else if(datatype==string_id) {
                            func = [this](Ptr& a, Ptr& b){
                                Ptr a1 = *(Ptr*)resolve_ptr(a);
                                Ptr b1 = *(Ptr*)resolve_ptr(b);
                                return strcmp(
                                    (const char*)resolve_to_col(a1).sget(a1.sidx),
                                    (const char*)resolve_to_col(b1).sget(b1.sidx)
                                ) < 0;
                            };
                        }
                    }
                    else if(sortkind=="desc"){
                        if(datatype==int_id) {
                            func = [this](Ptr& a, Ptr& b){
                                return *(int*)resolve_ptr(a) > *(int*)resolve_ptr(b);
                            };
                        } else if(datatype==float_id) {
                            func = [this](Ptr& a, Ptr& b){
                                return *(float*)resolve_ptr(a) > *(float*)resolve_ptr(b);
                            };
                        } else if(datatype==string_id) {
                            func = [this](Ptr& a, Ptr& b){
                                Ptr a1 = *(Ptr*)resolve_ptr(a);
                                Ptr b1 = *(Ptr*)resolve_ptr(b);
                                return strcmp(
                                    (const char*)resolve_to_col(a1).sget(a1.sidx),
                                    (const char*)resolve_to_col(b1).sget(b1.sidx)
                                ) > 0;
                            };
                        }
                    } else if(sortkind=="none") {
                        func = [this](Ptr& a, Ptr& b){
                            return false;
                        };
                    }

                    std::sort((Ptr*)sortcol.storage, (Ptr*)sortcol.storage + sortcol.length(),
                    [func](Ptr& a, Ptr& b){
                        return func(a,b);
                    });

                });

                overload_type(query_id,".\"asIndexString\"","QUERY_ASINDEXSTRING",make_value(string_id,sizeof(Ptr),0,char_id,1),[this](Context& ctx){
                    standard_sub_process(ctx);
                    Col& sortcol = ctx.node().left().value().data_col();
                    string output = resolve_string_ticket(ctx.node());
                    output.col().clear();
                    output.push("[");
                    for(int i=0;i<sortcol.length();i++) {
                        output.push(std::to_string((*(Ptr*)sortcol[i]).sidx));
                        if(i<sortcol.length()-1) output.push(",");
                        else output.push("]");
                    }
                });

            add_function("recycle_node",[this](Context& ctx){
                standard_sub_process(ctx);
                Node n = ctx.node().getNode(0);
                recycle_node(n);
            });

            add_function("newline",[this](Context& ctx){
                uspan->newline(children_to_string(ctx,ctx.node().children()));
            });
            add_function("log",[this](Context& ctx){
                uspan->log(children_to_string(ctx,ctx.node().children()));
            });
            add_function("udump",[this](Context& ctx){
                uspan->print_all();
            });
            add_function("uprofilers",[this](Context& ctx){
                uspan->print_all_profilers();
            });
            add_function("ureset_profilers",[this](Context& ctx){
                uspan->profilers.clear();
            });
            add_function("ussw",[this](Context& ctx){ //uspan_setup_standard_watcher
                setup_uspan_standard_watchers();
            });
            add_function("ursw",[this](Context& ctx){ //uspan_remove_standard_watcher
                for(int i=0;i<watchers.length();i++) {if(watchers[i].label=="uspan_core") watchers.removeAt(i); break;}
            });
            add_function("ures",[this](Context& ctx){ //uspan_restart
                uspan->restart();
            });
            add_function("utg",[this](Context& ctx){ //uspan_time_get
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->get_root_time());
            },sizeof(Ptr),string_id);
            add_function("uts",[this](Context& ctx){ //uspan_time_start
                uspan->restart_root_time();
            });
            add_function("ute",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->end_root_time());
            },sizeof(Ptr),string_id);

            add_function("tstart",[this](Context& ctx){ //uspan_time_start
                uspan->start_timer(children_to_string(ctx,ctx.node().children()));
            });
            add_function("ttime",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->get_time(children_to_string(ctx,ctx.node().children())));
            },sizeof(Ptr),string_id);
            add_function("tstop",[this](Context& ctx){ //uspan_time_end
                string output = resolve_string_ticket(ctx.node());
                output = ftime(uspan->end_timer(children_to_string(ctx,ctx.node().children())));
            },sizeof(Ptr),string_id);

            add_function("unit_has_arg",[this](Context& ctx){
                standard_sub_process(ctx);
                std::string arg = ctx.node().getString(0).to_std();
                bool b = uargs.has(arg);
                ctx.node().set((void*)&b);
            },1,bool_id);

            add_function("unit_get_arg",[this](Context& ctx){
                standard_sub_process(ctx);
                std::string arg = ctx.node().getString(0).to_std();
                Value n = ctx.node();
                string output = resolve_string_ticket(ctx.node());
                for(std::string& a : uargs) {
                    if(a.find(arg) == 0) {
                        output = a.substr(arg.length()); // everything after the prefix
                        break;
                    }
                }
            },sizeof(Ptr),string_id);

            tokenizer_state_functions[comment_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());
                if(c == '/' && (ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='/')) {
                    ctx.node().type(comment_id);
                    ctx.state(0);
                    ctx.index()++;
                    at_x += 1.0f;
                    ctx.node().name().push("//");
                } else if(c == '\n') {
                    at_y += 1.0f;
                    at_x = -1.0f;
                    ctx.node().type(comment_id);
                    ctx.state(0);
                } else {
                    ctx.node().name().push(c);
                }
            };


            tokenizer_state_functions[precompile_brace] = [this](Context& ctx) {
                char c = ctx.source().at(ctx.index());

                if(ctx.node().quals().empty()) {
                    Node open_token = copy_as_token(ctx.node());
                    ctx.node().quals() << open_token;
                    ctx.node().type(precompiling_id);
                    ctx.node().name().col().clear();
                    ctx.node().x(at_x);
                    ctx.node().y(at_y);
                }

                if(c == '#'&&(ctx.index()+1<ctx.source().length()&&ctx.source().at(ctx.index()+1)=='#')) {
                    Node closer = copy_as_token(ctx.node().quals()[0]);
                    closer.x(at_x); closer.y(at_y);
                    ctx.node().quals() << closer;

                    ctx.state(0);
                    //ctx.result().removeAt(ctx.index());
                    ctx.index()++;
                    at_x+=1.0f;

                    std::string oldsrc = ctx.source().to_std(); //Remember to just fix the source in context (when I'm not trying to ship a prototype)

                    // list<Watcher> watcher_daycare; //We don't log things like precompiling stages (for now)
                    // watcher_daycare << watchers;
                    // watchers.clear();

                    std::string oldlabel = unit_label;
                    unit_label = oldlabel+"pc";

                    float old_at_x = at_x; float old_at_y = at_y;
                    at_x = ctx.node().quals()[0].x(); at_y = ctx.node().quals()[0].y(); at_z = ctx.node().quals()[0].z();
                    Node root = process(ctx.node().name().to_std());
                    ctx.node().name().col().clear(); //To avoid stinking up the nodenet and memory dump
                    compile(root,false);
                    at_x = old_at_x; at_y = old_at_y;
                    start_logged_stage(x_handlers);
                    standard_travel_pass(root);
                    end_logged_stage();

                    unit_label = oldlabel;
                    //watchers << watcher_daycare; //Restore watchers

                    ctx.node().scopes() << root;

                    ctx.source(oldsrc);
                } else if(c=='\n') {
                    at_y += 1.0f; at_x = -1.0f;
                    ctx.node().name().push(c);
                }
                else {
                    ctx.node().name().push(c);
                }
            };


            


            x_handlers[make_tokenized_keyword("as_data")] = [this](Context& ctx){
                Value rv = ctx.node().children()[0].value();
                ctx.node().value(make_value(ptr_id,sizeof(Ptr)));
                ctx.node().value().set((void*)&rv.data_ptr());
            };

            r_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[to_unary_id(star_id)] = [this](Context& ctx){ //To get size data and such from the type
                standard_sub_process(ctx);
                if(is_live(ctx.node().value())) {
                    fire_quals(ctx,ctx.node().value());
                }
                resolve_overload(ctx);
            };


            r_handlers[to_decl_id(star_id)] = [this](Context& ctx){
                if(ctx.node().value().type()!=ptr_id) {
                    ctx.node().value().type(ptr_id);
                    ctx.node().value().size(sizeof(Ptr));
                    ctx.node().value().quals().insert(0,make_node(ptr_id,"Ptr",make_value(ptr_id,sizeof(Ptr)),ctx.node().in_scope()));
                    fire_quals(ctx,ctx.node().value());
                    standard_sub_process(ctx);
                    resolve_overload(ctx);
                }
            };
            x_handlers[to_decl_id(star_id)] = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    Node child = ctx.node().children()[0];
                    Ptr p = *(Ptr*)child.value().get();
                    ctx.node().value().set((void*)&p);
                }
            };
            r_handlers[to_unary_id(amp_id)] = [this](Context& ctx){
                if(ctx.node().children().length()!=1) return;
                if(ctx.node().value().type()!=ptr_id) {
                    ctx.node().value().type(ptr_id);
                    ctx.node().value().size(sizeof(Ptr));
                    ctx.node().value().quals().insert(0,make_node(ptr_id,"Ptr",make_value(ptr_id,sizeof(Ptr)),ctx.node().in_scope()));
                    fire_quals(ctx,ctx.node().value());
                    standard_sub_process(ctx);
                    resolve_overload(ctx);
                }
            };

            x_handlers[to_unary_id(amp_id)] = [this](Context& ctx){
                if(ctx.node().children().length()!=1) return;
                standard_sub_process(ctx);
                Node child = ctx.node().children()[0];
                Ptr p = child.value().data_ptr();
                ctx.node().value().set((void*)&p);
            };
            x_handlers[to_unary_id(star_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                Ptr p = ctx.node().getPtr(0);
                ctx.node().value().data_ptr(p);
                ctx.node().value().type(resolve_to_col(p).tag);
                ctx.node().value().size(resolve_to_col(p).element_size);
            };


            x_handlers[var_decl_id] = [this](Context& ctx){
                fire_quals(ctx,ctx.node().value());
            };
            r_handlers[to_prefix_id(ptr_id)] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    if(ctx.value().quals().length()>1) {
                        int i = 0;
                        while(i<ctx.value().quals().length()) {
                            Node q = ctx.value().quals()[i];
                            if(q.type()==ptr_id&&i<ctx.value().quals().length()-1) {
                                Node left = ctx.value().quals()[i+1];
                                ctx.value().sub_type(left.value().type());
                                ctx.value().sub_size(left.value().size());
                                break;
                            }
                            i++;
                        }
                    } else {
                        //It's just a normal Ptr
                        // print(red("prefix_ptr_id::r_handler missing type it points to!"));
                        // print(node_to_string(ctx.node()));
                    }
                }
            };
            x_handlers[to_prefix_id(ptr_id)] = [this](Context& ctx){
                if(is_live(ctx.value())) {
                    if(ctx.value().sub_type()!=0) {
                        Ptr ticket = get_ticket(data_store_id,ctx.value().sub_size(),ctx.value().sub_type());
                        ctx.value().set((void*)&ticket);
                    }
                }
            };
            x_handlers[to_prefix_id(string_id)] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Ptr ticket = get_ticket(name_store_id,1,char_id);
                    ctx.value().set((void*)&ticket);
                }
            };
            x_handlers[prefix_node_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()&&!is_live(ctx.value().data_ptr())) {
                    Node n = make_node();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&n);
                }
            };
            x_handlers[prefix_value_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Value v = make_value();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&v);
                }
            };
            x_handlers[prefix_context_id] = [this](Context& ctx){
                if(is_live(ctx.value())&&ctx.value().quals()[0]==ctx.qual()) {
                    Context c = make_context();
                    ctx.node().value().init_data();
                    ctx.value().set((void*)&c);
                }
            };

            x_handlers[literal_id] = [this](Context& ctx){
                std::string name = ctx.node().name().to_std();
                uint32_t vtype = ctx.node().value().type();
                if(vtype==int_id) {
                    int i = std::stoi(name); ctx.node().value().set((void*)&i);
                } else if(vtype==float_id) {
                    float f = std::stof(name); ctx.node().value().set((void*)&f);
                } else if(vtype==ptr_id) {
                    Ptr p = string_to_Ptr(name); 
                    if(p.cachelevel==3&&!p.cache) {p.cache = &types;} //Add fillins for other caches somehow later
                    ctx.node().value().set((void*)&p);
                } else if(vtype==string_id) { //This is a race condition
                    Ptr p = get_ticket(name_store_id,1,char_id); string s(p); s = name; ctx.node().value().set((void*)&p);
                } else if(vtype==bool_id) {
                    bool b = (name=="true"||name=="1");
                    ctx.node().value().set((void*)&b);
                } else if(vtype==char_id) {
                    char c = name.empty() ? '\0' : name[0];
                    ctx.node().value().set((void*)&c);
                } else if(vtype==null_id) { //For future use
                    Ptr dead = deadptr;
                    ctx.node().value().set((void*)&dead);
                } else {
                    throw_error("No way to handle value type ",labels[vtype],"!");
                }
            };


            uint32_t include_id = make_tokenized_keyword("include");
            tokenizer_state_functions[include_id] = [this](Context& ctx){
                char c = ctx.source().at(ctx.index());
                if(c=='\n'||c==';') {
                    std::string path = ctx.node().name().to_std().substr(8);
                    try {
                        std::string source = readFile(path);
                        float old_at_x = at_x; float old_at_y = at_y; float old_at_z = at_z;
                        at_x = 0.0f; at_y = 0.0f; last_z++; at_z = last_z;
                        Node root = process(source);
                        for(int i=0;i<root.children().length();i++) {
                            ctx.result().push(root.children()[i]);
                        }
                        at_x = old_at_x; at_y = old_at_y; at_z = old_at_z;
                    } catch(std::exception& e) {
                        throw_error("Bad path in include: ",path);
                    }
                    ctx.index()--;
                    ctx.state(0);
                } else { //Consume the path
                    ctx.node().name().push(c);
                }
            };

            r_handlers[to_unary_id(hash_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[to_unary_id(hash_id)] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                std::string search_for = c.name().to_std();
                uint32_t type = 0;
                type = labels_lookup.getOrDefault(c.name().to_std(),type);
                ctx.node().value().set((void*)&type);
            };

            int l_lbp = 5; int l_rbp = 3;
            set_binding_powers(hash_id,l_lbp,l_rbp);
            //Double check what the precedence for these should be later
            uint32_t label_type_combo = add_binding_token_combo("LABEL_TYPE",l_lbp,l_rbp,'L','#');
            r_handlers[label_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = *(int*)c.value().get();
                output = labels[type];
            };

            uint32_t node_type_combo = add_binding_token_combo("NODE_TYPE",l_lbp,l_rbp,'N','#');
            r_handlers[node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node c = ctx.node().children()[0];
                uint32_t type = c.type();
                ctx.node().value().set((void*)&type);
            };
            uint32_t label_node_type_combo = add_binding_token_combo("LABEL_NODE_TYPE",l_lbp,l_rbp,'S','N','#');
            r_handlers[label_node_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_node_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = c.type();
                output = labels[type];
            };
            uint32_t value_type_combo = add_binding_token_combo("VALUE_TYPE",l_lbp,l_rbp,'V','#');
            r_handlers[value_type_combo] = [this](Context& ctx){                 
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                resolve_overload(ctx);
            };
            x_handlers[value_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                uint32_t type = 0;
                if(is_live(c.value())) {
                    type = c.value().type();
                }
                ctx.node().value().set((void*)&type);
            };
            uint32_t label_value_type_combo = add_binding_token_combo("LABEL_VALUE_TYPE",l_lbp,l_rbp,'S','V','#');
            r_handlers[label_value_type_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[label_value_type_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                uint32_t type = 0;
                if(is_live(c.value())) {
                    type = c.value().type();
                }
                output = labels[type];
            };

            uint32_t info_value_combo = add_binding_token_combo("INFO_VALUE",l_lbp,l_rbp,'I','V','#');
            r_handlers[info_value_combo] = [this](Context& ctx){
                standard_sub_process(ctx); 
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));  
                resolve_overload(ctx); 
            };
            x_handlers[info_value_combo] = [this](Context& ctx){
                Node c = ctx.node().children()[0];
                string output = resolve_string_ticket(ctx.node());
                output = value_info(c.value());
            };

            add_function("deadptr",[this](Context& ctx){
                ctx.node().value().set((void*)&deadptr);
            },sizeof(Ptr),ptr_id);
            add_function("deadnode",[this](Context& ctx){
                ctx.node().value().set((void*)&deadptr);
            },sizeof(Ptr),node_id);
            add_function("pool_info",[this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t poolid = *(int*)ctx.node().children()[0].value().get();
                string output = resolve_string_ticket(ctx.node());
                output = pool_info(poolid);
            },sizeof(Ptr),string_id);
            add_function("unit_info",[this](Context& ctx){
                standard_sub_process(ctx);
                string output = resolve_string_ticket(ctx.node());
                output = unit_info();
            },sizeof(Ptr),string_id);

            x_handlers[to_prefix_id(global_qual)] = [this](Context& ctx){
                if(ctx.node().type()==var_decl_id) {
                    value_col vcol(uid,unitdata_col,global_value_table_idx);
                    vcol.put(ctx.node().name().to_std(),ctx.value());
                } else if(ctx.node().type()==func_decl_id) {
                    node_col ncol(uid,unitdata_col,global_node_table_idx);
                    ncol.put(ctx.node().name().to_std(),ctx.node().scopes()[0]);
                    value_col vcol(uid,unitdata_col,global_value_table_idx);
                    vcol.put(ctx.node().name().to_std(),ctx.value());
                }
            };
           
            r_handlers[ctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[ctx_id] = [this](Context& ctx){
                if(is_live(ctx.sub())) {
                    Context sctx = ctx.sub();
                    ctx.node().value().set((void*)&sctx);
                } else {
                    ctx.node().value().set((void*)&ctx);
                }
            };
            r_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value(make_value(context_id,sizeof(Ptr)));
                ctx.node().value().quals().push(make_node(live_qual));
            };
            x_handlers[lctx_id] = [this](Context& ctx){
                ctx.node().value().set((void*)&ctx);
            };

            r_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr)));
                ctx.node().value().init_data();
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[to_string_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                string str(*(Ptr*)ctx.node().value().get());
                str = value_as_string(ctx.node().children()[0].value());
            };

            r_handlers[labels_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(data_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[labels_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    string label(*(Ptr*)ctx.node().value().get());
                    uint32_t p = *(uint32_t*)ctx.node().children()[0].value().get();
                    label = labels[p]; 
                }
            };

            r_handlers[to_type_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(int_id,4));
                ctx.node().value().init_data();
                resolve_overload(ctx);
            };
            x_handlers[to_type_id] = [this](Context& ctx){
                //Add caching for this later
                standard_sub_process(ctx);
                std::string search_for = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                for(auto e : labels.entrySet()) {
                    if(e.value == search_for) {
                        ctx.node().value().set((void*)&e.key);
                        return;
                    }
                }
            };

            // x_handlers[make_tokenized_keyword("make_value")] = [this](Context& ctx){
            //     standard_sub_process(ctx);
            //     int type = *(int*)ctx.node().children()[0].value().get();
            //     int size = *(int*)ctx.node().children()[1].value().get();
            //     ctx.sub().node().value(make_value(type,size));
            //     ctx.sub().node().value().init_data();
            // };


            s_handlers[string_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];
                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                    ctx.node().quals() << copy_as_token(ctx.node().children().last());
                    ctx.node().type(node_block_id);
                    ctx.node().children().last().name(ctx.node().name().to_std());
                    ctx.node().children().last().x(-1.0f); ctx.node().children().last().y(-1.0f);
                }
            };
            t_handlers[node_block_id] = [this](Context& ctx){
                for(auto e : labels.entrySet()) {
                    if(e.value==ctx.node().name().to_std()) {
                        ctx.node().sub_type(e.key);
                        break;
                    }
                }
                if(ctx.node().sub_type()==0) {
                    print(red("node_block:t_handler unrecognized node type: "+ctx.node().name().to_std()));
                }
            };
            x_handlers[node_block_id] = [this](Context& ctx){
                ctx.state(standard_travel_pass(ctx.node().scopes()[0]));
            };

 
            r_handlers[in_id] = [this](Context& ctx){
                if(!ctx.node().children().empty()&&is_live(ctx.node().in_scope())&&is_live(ctx.node().in_scope().owner())) {
                    ctx.node().quals() << copy_as_token(ctx.node());
                    ctx.node().x(-1.0f); ctx.node().y(-1.0f);
                    ctx.node().name("in "+ctx.node().children()[0].name().to_std()+" "+labels[ctx.node().in_scope().owner().sub_type()]);
                    if(!ctx.node().scopes().empty()) {
                        ctx.node().scopes()[0].name(ctx.node().name().to_std());
                    }
                }
            };
            x_handlers[in_id] = [this](Context& ctx){
                Node this_node = ctx.node();
                uint32_t target_type = ctx.node().in_scope().owner().sub_type();
                std::string stage_name = ctx.node().children()[0].name().to_std();
                if(!stages.hasKey(stage_name)) {
                    print(red("in_id:x_handler unknown stage "+stage_name));
                    return;
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                (*stage)[target_type] = [this,this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;

                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0],ctx);
                    start_stage(old_stage);
                    
                };

                uint32_t stage_id = *(uint32_t*)types[handler_type_id][stages_id].get(stage_name);
                while(types[handler_type_id][target_type].length()<=stage_id) types[handler_type_id][target_type].push_default();
                Node target_scope = this_node.scopes()[0];
                types[handler_type_id][target_type].set(stage_id,(void*)&target_scope);
            };

            n_handlers[on_id] = [this](Context& ctx){ //Add a proper n_take_right later, like we had in GDSL
                ctx.index()++;
                while(ctx.result().get(ctx.index()).type()!=lbrace_id) {
                    ctx.node().children().push(ctx.result().take(ctx.index()));
                }
            };
            x_handlers[on_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                uint32_t target_type = ctx.node().in_scope().owner().sub_type();
                string instr = resolve_string_ticket(ctx.node().children()[0]);

                Value v = deadptr;
                if(ctx.node().children().length()>1) {
                    v = ctx.node().children()[1].value();
                }

                Node this_node = ctx.node();
                overload_type(target_type, instr.to_std(), instr.to_std()+"_overload", v, [this, this_node](Context& ctx) mutable {
                    g_ptr<Stage> old_stage = active_stage;
                    start_stage(x_handlers);
                    standard_travel_pass(this_node.scopes()[0], ctx);
                    start_stage(old_stage);
                });
            };


            s_handlers[if_id] = [this](Context& ctx){
                if(ctx.index()+1>=ctx.result().length()) return;

                Node right = ctx.result()[ctx.index()+1];

                if(right.type()==lbrace_id) {
                    ctx.node().children() << ctx.result().take(ctx.index()+1);
                } else if(is_live(right)) {
                    ctx.index()++;
                    process_node(ctx,right);
                    ctx.index()--;
                    Node newscope = make_node(scope_id,ctx.node().name().to_std(),deadptr,deadptr);
                    ctx.node().scopes() << newscope;
                    newscope.owner(ctx.node());
                    place_node_in_scope(right,newscope);
                    newscope.children() << ctx.result().take(ctx.index()+1);
                }
            };
            s_handlers[else_id] = s_handlers[if_id];

            x_handlers[if_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                DEBUG_ONLY(if(ERROR_FLAG) {return;});
                if(*(bool*)ctx.node().children()[0].value().get()) {
                    ctx.state(standard_travel_pass(ctx.node().scopes()[0],ctx.sub()));
                }
                else if(ctx.node().scopes().length()>1) {
                    ctx.state(standard_travel_pass(ctx.node().scopes()[1],ctx.sub()));
                }
            };
            t_handlers[else_id] = [this](Context& ctx) {
                if(ctx.index()>0) {
                    Node left_if = deadptr;      
                    if(ctx.left().type()==if_id) {
                        left_if = ctx.left();
                        while(left_if.scopes().length()==2) { //To chain else ifs
                            Node lscope = left_if.scopes()[1];
                            if(!lscope.children().empty()) {
                                if(lscope.children()[0].type()==if_id) {
                                    left_if = lscope.children()[0];
                                }
                            }
                        }
                    }
                    if(is_live(left_if)) {
                        ctx.node().scopes()[0].owner(left_if);
                        left_if.scopes() << ctx.node().scopes()[0];
                        left_if.quals() << turn_into_token(ctx.node());
                        ctx.result().removeAt(ctx.index());
                        ctx.index()--;
                    } 
                }
            };

            uint32_t catch_id = add_function("catch",[this](Context& ctx){
                if(ERROR_FLAG) {
                    catch_pass_error(ctx,false);
                }
                if(UERROR_FLAG) {
                    ctx.state(standard_travel_pass(ctx.node().scopes()[0],ctx.sub()));
                }
            });
            s_handlers[catch_id] = s_handlers[if_id];

            r_handlers[while_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().value(make_value());
            };
            x_handlers[while_id] = [this](Context& ctx) {
                while(true) {
                    DEBUG_ONLY(if(ERROR_FLAG){log(red("Attempted to execute while while another error was flagged")); return;})
                    process_node(ctx, ctx.node().children()[0]);
                    if(!(*(bool*)ctx.node().children()[0].value().get()))break;
                    uint32_t result = standard_travel_pass(ctx.node().scopes()[0], ctx.sub());
                    if(result > 0) {
                        uint32_t kind = result % 4;
                        if(kind == 2 || kind == 3) {//Break or continue
                            if(result>=4) {
                                result -= 4;//Consume one magnitude
                                ctx.state(result);//And propagate up
                            } 
                            if(kind == 2) {break;} //Otherwise break away
                            //Continue will fall through here, after consuming the magnitude
                        } else { //If it's a return, pass it up
                            ctx.state(result);
                            break;
                        }
                    }
                }
            };         

            t_handlers[for_id] = [this](Context& ctx){
                for(int i=0;i<ctx.node().children().length();i++) {
                    place_node_in_scope(ctx.node().children()[i],ctx.node().scopes()[0]);
                }
            };
            r_handlers[for_id] = [this](Context& ctx) {
                standard_sub_process(ctx);
                ctx.node().scopes()[0].value(make_value());
            };
            x_handlers[for_id] = [this](Context& ctx) {
                process_node(ctx, ctx.node().children()[0]);
                Node scope = ctx.node().scopes()[0];
                scope.value().loc(0);
                while(true) {
                    process_node(ctx, ctx.node().children()[1]);
                    DEBUG_ONLY(if(ERROR_FLAG) {return;})
                    if(!(*(bool*)ctx.node().children()[1].value().get()))break;
                    scope.value().loc(scope.value().loc()+1); //This starts at 1 because function calls also start it at one
                    uint32_t result = standard_travel_pass(scope, ctx.sub());
                    process_node(ctx, ctx.node().children()[2]);
                    if(result > 0) {
                        uint32_t kind = result % 4;
                        if(kind == 2 || kind == 3) {//Break or continue
                            if(result>=4) {
                                result -= 4;//Consume one magnitude
                                ctx.state(result);//And propagate up
                            } 
                            if(kind == 2) {break;} //Otherwise break away
                            continue;
                        } else { //If it's a return, pass it up
                            ctx.state(result);
                            break;
                        }
                    }
                }
            };  

            r_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(string_id,sizeof(Ptr),0,char_id,1));
                Ptr ticket = get_ticket(name_store_id,1,char_id);
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[read_file_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Ptr ptr = *(Ptr*)ctx.node().value().get();
                string output(ptr);
                Ptr cptr = *(Ptr*)ctx.node().children()[0].value().get();
                DEBUG_ONLY(if(ERROR_FLAG){return;})
                output = readFile(string(cptr).to_std());
            };

            r_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                ctx.node().value(make_value(node_id,sizeof(Ptr)));
                Ptr ticket = make_node();
                ctx.node().value().set((void*)&ticket);
                resolve_overload(ctx);
            };
            x_handlers[compile_id] = [this](Context& ctx){
                standard_sub_process(ctx);
                Node& node = (Node&)(*(Ptr*)ctx.node().value().get());
                std::string source = string(*(Ptr*)ctx.node().children()[0].value().get()).to_std();
                float old_at_x = at_x; float old_at_y = at_y; float old_at_z = at_z;
                at_x = 0.0f; at_y = 0.0f; last_z++; at_z = last_z;
                Node root = process(source);
                root.z(at_z);
                at_x = old_at_x; at_y = old_at_y; at_z = old_at_z;
                compile(root);
                start_logged_stage(x_handlers);
                node.copy(root);
                // ctx.node().scopes() << root;
                // root.owner(ctx.node());
                end_logged_stage();
            };

            // t_handlers[precompiling_id] = [this](Context& ctx){
            //     Node croot = ctx.node().scopes()[0];
            //     Col& vt = croot.value_table().col();
            //     for(int i=0;i<vt.length();i++) {
            //         ctx.root().value_table().put(((QString&)vt.cells[i]).to_std(),*(Value*)vt[i]);
            //     }
            // };
            m_handlers[precompiling_id] = [this](Context& ctx){ctx.node().scopes()[0].owner(ctx.node());}; //To restore visibility

            n_handlers.default_function = [this](Context& ctx){standard_sub_process(ctx);};

            e_handlers.default_function = [this](Context& ctx){
                if(!ctx.node().scopes().empty()&&ctx.node().scopes()[0].owner()==ctx.node()) {
                    e_scoped_handler(ctx);
                    return;
                }

                if(!is_live(ctx.node().value())) {ctx.node().quals().push(make_node(live_qual));} //For debug things and such

                if(is_node_opperator(ctx.node())) {
                    if(ctx.node().children().length()==2) {
                        e_stage_binary_handler(ctx);
                    } else if(ctx.node().children().length()==1) {
                        ctx.node().value((ctx.node().children()[0].value())); 
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[var_decl_id] = [this](Context& ctx){}; //Doing nothing
            e_handlers[equals_id] = [this](Context& ctx){
                e_stage_assignment_handler(ctx);
                standard_sub_process(ctx);
            };
            e_handlers[func_call_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[return_id] = [this](Context& ctx){
                if(ctx.node().has_qual(live_qual)) {
                    for(int i=0;i<ctx.node().children().length();i++) {
                        Node c = ctx.node().children().get(i);
                        if(is_live(c.value())&&!c.has_qual(live_qual)) {
                            c.value().quals().push(make_node(live_qual));
                        }
                    }
                }
                standard_sub_process(ctx);
            };
            e_handlers[print_id] = [this](Context& ctx){
                ctx.node().value().quals().push(make_node(gatekeeper_qual));
                ctx.node().value().quals().push(make_node(live_qual));
                value_col subvals = ctx.node().value().sub_values();
                gather_all_values_in_scope(subvals,ctx.node());
                fire_quals(ctx,ctx.node().value());
                standard_sub_process(ctx);
            };
            e_handlers[to_prefix_id(gatekeeper_qual)] = [this](Context& ctx){
                for(int i=0;i<ctx.value().sub_values().length();i++) {
                    Value v = ctx.value().sub_values().get(i);
                    if(!v.has_qual(live_qual)) {v.quals().push(make_node(live_qual));}
                }
            };

            m_handlers.default_function = [this](Context& ctx){
                if(is_live(ctx.node().value())) {
                    if(should_allocate_data(ctx.node().value())) {
                        allocate_data(ctx.node().value());
                    }
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(m_handlers.has(child.type())) {
                            process_node(ctx,child);
                        } else {
                            if(is_live(child.value())) {
                                if(should_allocate_data(child.value())) {
                                    allocate_data(child.value());
                                }
                            }
                            process_node(ctx,child);
                        }
                    }
                    #if !LOBOTOMIZE_M_STAGE
                    for(int i=ctx.node().children().length()-1;i>=0;i--) {
                        Node child = ctx.node().children()[i];
                        if(!m_handlers.has(child.type())) {
                            if(should_recycle_data(child.value())) {
                                recycle_column(child.value().data_ptr());
                            }
                        }
                    }
                    #endif
                } else {
                    backwards_sub_process(ctx);
                }
            };
            m_handlers[var_decl_id] = [this](Context& ctx){
                #if !LOBOTOMIZE_M_STAGE
                if(should_recycle_data(ctx.node().value())) {
                    recycle_column(ctx.node().value().data_ptr());
                }
                #endif
            };
            m_handlers[identifier_id] = [this](Context& ctx){
                if(should_allocate_data(ctx.node().value())) {
                    allocate_data(ctx.node().value());
                }
            };

            // r_handlers[test_id] = [this](Context& ctx) {
            //     ctx.node().value(make_value(int_id,4));
            // };
            // a_handlers[test_id] = [this](Context& ctx){
            //     print("I'm a test during ",active_stage->label,", looking at my context I see: ");
            //     print(context_trace_to_string(ctx));
            // };
            // x_handlers[test_id] = a_handlers[test_id];
            x_handlers[test_id] = [this](Context& ctx){
                Node node = ctx.left();
                print(node_info(node,1));
                if(is_live(node.value())) {
                    for(int q=0;q<node.value().quals().length();q++) {
                        Node qual = node.value().quals()[q];
                        print("  ",q,": ",node_info(qual,1)," | ",is_true_type.getOrDefault(qual.type(),false)?"Y":"N");
                    }
                }
            };


            a_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==A STAGE==");
                print(node_to_string(ctx.root()));
            };
            n_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==N STAGE==");
                print(node_to_string(ctx.root()));
            };
            t_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==T STAGE==");
                print(node_to_string(ctx.root()));
            };
            r_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==R STAGE==");
                print(node_to_string(ctx.root()));
            };
            e_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==E STAGE==");
                print(node_to_string(ctx.root()));
            };
            m_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==M STAGE==");
                print(node_to_string(ctx.root()));
            };
            x_handlers[DEBUG_ROOT_id] = [this](Context& ctx){
                print("==X STAGE==");
                print(node_to_string(ctx.node().in_scope()));
            };

            add_function("DEBUG_ROOT_X_BRIEF",[this](Context& ctx){
                print(node_to_string(ctx.node().in_scope(),0,0,0));
            });
            add_function("DEBUG_ROOT_X_VERBOSE",[this](Context& ctx){
                print(node_to_string(ctx.node().in_scope(),0,0,2));
            });

            x_handlers[make_tokenized_keyword("MISTAKE")] = [this](Context& ctx) {
                standard_sub_process(ctx);
                Node node = make_node();
                int val = ctx.node().children().empty() ? 0 : ctx.node().getInt(0);
                switch (val) {
                    case 0: {
                        print("Live Node, dead Value.");
                        string value = node.getString();
                        break;
                    }
        
                    case 1: {
                        print("Missing child.");
                        string value = node.getString(1);
                        break;
                    }
        
                    case 2: {
                        print("Child exists, but has no Value.");
                        Node child = make_node();
                        node.children().push(child);
                        string value = node.getString(0);
                        break;
                    }
        
                    case 3: {
                        print("Value exists, but has no data allocation.");
                        node.value(make_value(string_id, sizeof(Ptr)));
                        string value = node.getString();
                        break;
                    }
        
                    case 4: {
                        print("Data column exists but contains zero elements.");
                        Value value = make_value(string_id, sizeof(Ptr));
                        value.init_data();
                        node.value(value);
                        string result = node.getString();
                        break;
                    }
                }
            };

            add_function("this_scope",[this](Context& ctx){
                Node inscope = ctx.node().in_scope();
                ctx.node().value().set((void*)&inscope);
            },sizeof(Ptr),node_id);

            add_function("STAMP_SOURCE",[this](Context& ctx){

                Node node = ctx.node().in_scope();
                float old_at_z = at_z;
                if(ctx.node().children().length()==1) {
                    standard_sub_process(ctx);
                    node = ctx.node().getNode(0);
                    if(node.z()>0) {
                        at_z = node.z();
                    }
                }

                writeFile("printout.txt",fnodenet_to_string(node,Stamper{[this](Node n, list<int>& offsets){
                    std::string to_return = n.name().to_std();
                    if(n.type()!=0) {
                        list<char> extra_escapes; 
                        if(is_live(n.value())&&n.value().type()==string_id) {
                            if(!n.quals().empty()&&n.quals()[0].name().to_std()=="'") {
                                extra_escapes << '\'';
                            } else {
                                extra_escapes << '"';
                            }
                        }
                        std::string nreturn = escape_string(to_return,false,extra_escapes);
                        //std::string nreturn = "<span class='"+labels[n.type()]+"'>"+to_return+"</span>";
                        while((int)n.y()>=offsets.length()) {offsets<<0;}
                        n.x(n.x()+offsets[(int)n.y()]);
                        offsets[(int)n.y()]+=nreturn.length()-to_return.length();
                        to_return = nreturn;
                    }
                    return to_return;
                },[this](Node n){
                    list<Node> stamps;
                    map<uint64_t,bool> visited;
                    Log::Line l; l.start();
                    collect_stamps(n,stamps,visited,false);
                    stamps.sort([](Node a, Node b){
                        int ay = (int)a.y(), by = (int)b.y();
                        if(ay != by) return ay < by;
                        return (int)a.x() < (int)b.x();
                    });
                    return stamps;
                }}));
                at_z = old_at_z;
            });

            e_handlers[make_tokenized_keyword("LBF_E")] = [this](Context& ctx){print("Launching blackfeather in e stage"); launch_blackfeather(unit_root);};
            x_handlers[make_tokenized_keyword("LBF_X")] = [this](Context& ctx){print("Launching blackfeather in x stage"); launch_blackfeather(unit_root);};


            x_handlers[invoke_stage_id] = [this](Context& ctx){
                std::string stage_name = ctx.node().name().to_std();
                if(!stages.hasKey(stage_name)) {
                    if(!stages.hasKey(stage_name+"ing")) {
                        print(red("invoke_stage_id:x_handler unknown stage "+stage_name));
                        return;
                    } else {
                        stage_name+="ing";
                    }
                }
                g_ptr<Stage> stage = stages.get(stage_name);
                standard_sub_process(ctx);
                Node left = ctx.node().children()[0];
                if(left.value().type()==node_id) {
                    left = Node(*(Ptr*)left.value().get());
                }
                ctx.node(left);
                stage->run(left.type())(ctx);
            };
        }

        void lemmatize_stages() {
            for(auto e : stages.entrySet()) {
                std::string label = e.key;
                if(e.key.length()>3 && e.key.substr(e.key.length()-3)=="ing") {
                    label = e.key.substr(0,e.key.length()-3);
                }
                if(!keywords.hasKey(label)) { //to stop double registraion
                    add_alias(label,invoke_stage_id);
                }
            }
        }

        virtual Node process(std::string path) override {
            //Log::Line l; l.start();
            Node root = tokenize(path);
            //print("[",at_z,"] Tokenize: ",ftime(l.end()));
            unit_root = root;
            return root;
        }

        bool post_mortem_printed = false;
        void post_mortem(Node root) {
            if(post_mortem_printed) return; 
            else post_mortem_printed = true;

            print(red("UNIT "+std::to_string(uid)+" FINISHED WITH ERROR IN "+active_stage->label+": "),ERROR_MSG);
            ERROR_FLAG = false;
            launch_blackfeather(root);
            ERROR_FLAG = true;
        }

        void deresolve_nodes(Node root) {
            walk_nodenet(root,[](Node n){n.resolved(false);});
        }

        Node compile_literal(const std::string& literal) {
            float old_at_x = at_x; float old_at_y = at_y;
            at_x = 0.0f; at_y =0.0f;
            Node root = tokenize(literal);
            at_x = old_at_x; at_y = old_at_y;
            Node n = root.children()[0];
            if(root.children().length()>1) {
                n.type(string_id);
                n.name(literal);
            } else {
                if(n.type()==identifier_id) {
                    n.type(string_id);
                }
            }
            Context ctx = make_context(); ctx.node(n);
            t_handlers.run(n.type())(ctx);
            m_handlers.run(n.type())(ctx);
            x_handlers.run(n.type())(ctx);
            return n;
        }

        void print_stage_header(const std::string& label) {print_and_pause(0.7f,"\n\n\n\n\n\n\n\n=="+label+" STAGE==\n\n\n\n\n\n\n\n");} 

        void compile(Node root, bool prune = true) {
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //Log::Line l; l.start();
            start_logged_stage(a_handlers);
            standard_direct_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();
            
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            lemmatize_stages();
            newline("Resolving keywords");
                a_pass_resolve_keywords(root.children());
            endline();
            for(int i=0;i<root.children().length();i++) {
                place_node_in_scope(root.children()[i],root);
            }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(n_handlers);
            standard_direct_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(s_handlers);
            standard_direct_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();
            
            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(t_handlers);
            standard_resolving_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(r_handlers);
            standard_resolving_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();

            //No E stage for now while testing
            // DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            // start_logged_stage(e_handlers);
            // memory_backwards_pass(root);
            // endline();

            // if(prune) {
            //     newline("Pruning dead nodes");
            //         e_pass_prune_dead(root.children());
            //     endline();
            // }

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            start_logged_stage(m_handlers);
            memory_backwards_pass(root);
            //print("[",at_z,"] ",active_stage->label,": ",ftime(l.end())); l.start();
            end_logged_stage();
        }
    


        virtual void run(Node root) override {
            compile(root);

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //print(node_to_string(root,0,0,true));

            start_logged_stage(x_handlers);
            standard_travel_pass(root);
            end_logged_stage();

            // ColCol& npool = types[types.length()-1];
            // auto out = openWriteStream("savetest");
            // snapshot_colcol(out,npool);
            // out.close();
            // auto in = openReadStream("savetest");
            // ColCol newpool = load_snapshot_colcol(in);
            // in.close();
            // dump_pool(newpool,0,false);

            DEBUG_ONLY(if(ERROR_FLAG){post_mortem(root); return;})
            //dump_unit(true);

            //launch_blackfeather(root);
        }


    };
}