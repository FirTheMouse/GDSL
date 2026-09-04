#include "../mixos-acorn/Acorn-Script.hpp"

namespace Acorn {
    void Acorn_Script::YAPA_init(Ptr push_to, Node node, bool is_indirect) {
        for(int i=0;i<node.children().length();i++) {
            Node prop = node.children()[i];  
            Node c = prop; 
            Ptr label = deadptr;
            if(c.type()==init_property_id) {
                if(is_live(prop.left().value())&&prop.left().value().type()==string_id){label = prop.left().getPtr();} else {label = prop.left().name_ptr();}
                c = prop.right();
            } 
            CHECK_ERROR("Error while initilizing YAPA prop ",node_basic_info_with_children_and_position(prop));
            //This logic is not sacred: if you're trying to understand it a week later I wrote this at 11pm
            Ptr dataptr = deadptr;
            Ptr ticket = deadptr;
            Ptr subticket = deadptr;
            void* data = nullptr;
            if(c.type()==group_id) {
                if(!c.children().empty()) {
                    ticket = get_ticket(push_to,c.c0().value().size(),c.c0().value().type());
                    data = (void*)&ticket;
                    if(c.c0().type()!=c.c0().value().type()) { //Check if it's just a type identifer like [char] vs a list of elements
                        for(int e=0;e<c.children().length();e++) {
                            resolve_to_col(ticket).push(c.get(e));
                        }
                    }
                    if(is_indirect) {
                        Ptr overticket = get_ticket(push_to,sizeof(Ptr),ptr_id);
                        resolve_to_col(overticket).push((void*)&ticket);
                        ticket = overticket;
                        data = (void*)&ticket;
                    }
                }
            } else if(is_live(c.value())) {
                if(is_indirect) {
                    dataptr = get_ticket(push_to,c.value().size(),c.value().type());
                    data = (void*)&dataptr;
                    if(c.value().type()==string_id) {
                        ticket = get_ticket(push_to,1,char_id);
                        resolve_to_col(dataptr).push((void*)&ticket);
                    }
                } else {
                    dataptr = push_to;
                    dataptr.sidx = resolve_to_col(push_to).length();
                    if(c.value().type()==string_id) {
                        subticket = get_ticket(push_to,1,char_id);
                        data = (void*)&subticket;
                    } else {
                        data = c.get();
                    }
                }
            }
            CHECK_ERROR("Error while initilizing YAPA prop data ",node_basic_info_with_children_and_position(prop));
            if(data) {
                if(is_live(label)) {
                    resolve_to_col(push_to).qput(data,resolve_to_col(label).storage,resolve_to_col(label).length(),string_id);
                } else {
                    resolve_to_col(push_to).push(data);
                }
            }
            if(is_live(dataptr)) {
                if(c.value().type()==string_id) {
                    (*(string*)resolve_ptr(dataptr)) = c.getString().to_std();
                } else {
                    Value puppet = make_value(c.value().type(),c.value().size());
                    puppet.data_ptr(dataptr);
                    assign(puppet,c.value());
                    recycle_value(puppet,false);
                }
            }
            CHECK_ERROR("Error while assigning inital YAPA prop ",node_basic_info_with_children_and_position(prop));
        }
    };

    uint32_t Acorn_Script::register_YAPA_type(const std::string& label, uint32_t YAPA_level, uint32_t id, bool is_indirect) {
        Value get_value = deadptr;
        YAPAs[YAPA_level].push(id);
        is_YAPA_indirect.put(id,is_indirect);
        if(YAPA_level==1) get_value = make_value(0);
        else if(YAPA_level==2) get_value = make_value(ptr_id,sizeof(Ptr));
        else if(YAPA_level==3) get_value = make_value(colcol_id,sizeof(Ptr));     
        else if(YAPA_level==4) get_value = make_value(colcolcol_id,sizeof(Ptr));  

       

        overload_type(id,".'as'(any)",label+"_AS",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" as");
            Node arg = ctx.node().right().c0();
            if(arg.type()==arg.value().type()) { 
                ctx.node().value().type(arg.value().type());
            } else {
                if(arg.value().type()==int_id) {
                    ctx.node().value().type(arg.getInt());
                }
            }
            ctx.node().value().set((void*)&ptr);
            if(resolve_overload(ctx.root())) {
                mark_and_skip(ctx);
            }
        });


        auto YAPA_add = [this,YAPA_level,id,is_indirect](Context& ctx, Ptr ptr, Ptr& p, uint32_t tag, string label, Value typeval){
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t index = 0;
            switch(YAPA_level) {
                case 4: {
                    index  = col.length();
                    ColColCol* new_subunit = ((PtrColColCol&)col).create(label.to_std());
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
                case 1: {
                    index = col.length();
                    if(is_indirect) {
                        Ptr ticket = get_ticket(ptr,0,duck_id);
                        col.push((void*)&ticket);
                    } else {
                        col.push_default();
                        p = ptr;
                    }
                    p.sidx = index;
                } break;
                default: break;
            }
            CHECK_ERROR("Error while adding in "+labels[id]+" add");

            if(is_live(label)&&label.length()>0) {
                switch(YAPA_level) {
                    case 3: {
                        col.addcell(index,label.col().storage,label.length(),string_id);
                        ((ColColCol&)col).get(index).label = label.to_std();
                    } break;
                    case 2: {
                        col.addcell(index,label.col().storage,label.length(),string_id);
                        ((ColCol&)col).get(index).label = label.to_std();
                    } break;
                    case 1: {
                        col.addcell(col.length()-1,label.col().storage,label.length(),string_id);
                    } break;
                    default: break;
                }
            }
        };

        auto YAPA_get = [this,YAPA_level,id,is_indirect,YAPA_add](Context& ctx, bool key_on_right, bool error_on_key_not_found) {
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" get");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);

            Node key = key_on_right?ctx.node().right():ctx.node().right().c0();

            void* data = nullptr;
            uint32_t size = 0;
            uint32_t lookup_type = 0;
            if(is_live(key.value())&&key.value().type()!=0) {
                lookup_type = key.value().type();
                if(is_ptr_alias(lookup_type)) {
                    Col& ccol = resolve_to_col(key.getPtr());
                    data = ccol.storage;
                    size = ccol.size;
                } else {
                    data = key.value().get();
                    size = key.value().size();
                }
            } else {
                data = key.name().col().storage;
                size = key.name().length();
                lookup_type = identifier_id;
            }
            if(lookup_type==int_id) {
                int index = key.getInt();
                if(index<col.length()) {
                    ptr[YAPA_level] = index;
                } else {
                    throw_error(labels[id]+" get, index ",index,+" out of bounds on ",Ptr_as_string(ptr));
                    ptr.specialization = _DEADSPEC;
                    return;
                }
            } else {
                if(col.hasKey(data,size)) {
                    ptr[YAPA_level] = col.getidx(data,size);
                } else {
                    if(error_on_key_not_found) {
                        throw_error(labels[id]+" get, Key not found");
                        ptr.specialization = _DEADSPEC;
                        return; 
                    } else {
                        YAPA_add(ctx,ptr,ptr,YAPA_level>1?0:duck_id,key.value().type()==string_id?key.getString():key.name(),deadptr);
                    }
                }
            }

            if(YAPA_level>1) {
                ctx.node().value().set((void*)&ptr);
            } else if(is_indirect) {
                Ptr inner = *(Ptr*)resolve_ptr(ptr);
                Col& innercol = resolve_to_col(inner);
                if(innercol.tag==function_id&&key.has_qual(lparen_id)) {
                    key.type(lambda_call_id);
                    key.value().data_ptr(inner);
                    key.value().type(innercol.tag); key.value().size(innercol.element_size);
                    if(resolve_overload(ctx.node())) {
                        standard_process(ctx);
                    }
                } else {
                    ctx.node().value().data_ptr(inner);
                    sync_value(ctx);
                }
            } else {
                ctx.node().value().data_ptr(ptr);
                sync_value(ctx);
            }
            CHECK_ERROR("Error during "+labels[id]+" get");
        };
      
        overload_type(id,list<std::string>{".UNDEFINED",".UNDEFINED(...)"},label+"_DUCK_GET",get_value,[this,YAPA_get](Context& ctx){
            YAPA_get(ctx,true,false);
        });
        uint32_t idxget_id = overload_type(id,"[any]",label+"_IDXGET",get_value,[this,YAPA_get](Context& ctx){
            YAPA_get(ctx,true,false);
        });
        // r_handlers[idxget_id] = [this](Context& ctx){
        //     fire_quals(ctx,ctx.node().right().value());
        // };
        overload_type(id,".'get'(any)",label+"_GET",get_value,[this,YAPA_get,id,YAPA_level](Context& ctx){
            YAPA_get(ctx,false,true);
        });
        overload_type(id,".'getOrPut'(any)",label+"_GETORPUT",get_value,[this,YAPA_get](Context& ctx){
            YAPA_get(ctx,false,false);
        });

        overload_type(id,list<std::string>{".LAMBDA_CALL",".LAMBDA_CALL(...)"},label+"_DOT_CALL",get_value,[this,YAPA_get](Context& ctx){
            process_node(ctx,ctx.node().right());
        });


        overload_type(id,".'init'(...)",label+"_INIT",make_value(id,sizeof(Ptr)),[this,id,YAPA_level,is_indirect](Context& ctx){
            uint32_t old_type = ctx.node().type();
            for(int i=0;i<ctx.node().right().children().length();i++) {
                if(ctx.node().right().children()[i].type()==property_id) {ctx.node().right().children()[i].type(init_property_id);}
            }
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            YAPA_init(ptr,ctx.node().right(),is_indirect);
            ctx.node().value().set((void*)&ptr);

            fire_quals(ctx,ctx.node().left().value());
        });

        x_handlers[to_prefix_id(id)] = [this,id,YAPA_level,is_indirect](Context& ctx){
            if(is_first_type_qual(ctx)&&ctx.node().type()==var_decl_id||ctx.node().type()==to_decl_id(amp_id)) {
                if(!ctx.node().children().empty()&&is_live(ctx.node().value().data_ptr())) {
                    ctx.node().value().data_col().set(0,&deadptr);
                }
                Ptr push_to = deadptr;
                if(is_indirect) {
                    push_to = resolve_ticket(ctx.node(),sizeof(Ptr),id);
                } else {
                    if(ctx.node().value().sub_type()!=0) {
                        push_to = resolve_ticket(ctx.node(),ctx.node().value().sub_size(),ctx.node().value().sub_type());
                    } else if(ctx.node().value().type()==string_id) {
                        push_to = resolve_ticket(ctx.node(),1,char_id);
                    } 
                    
                    // else {
                    //     push_to = resolve_ticket(ctx.node(),0,duck_id);
                    // }
                }
                if(!ctx.node().children().empty()) {
                    standard_sub_process(ctx);
                    YAPA_init(push_to,ctx.node(),is_indirect);
                }
            }
        };

        overload_type(id,".'register'(string)",label+"_REGISTER",make_value(id,sizeof(Ptr)),[this,id,YAPA_level,is_indirect](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            std::string label = ctx.node().right().getString(0).to_std();
            switch(YAPA_level) {
                case 3:
                    subunits.addcell(ptr.pool,label.data(),label.length(),string_id);
                    resolve_to_subunit(ptr).label = label;
                break;
                case 2:
                    resolve_to_subunit(ptr).addcell(ptr.pool,label.data(),label.length(),string_id);
                    resolve_to_pool(ptr).label = label;
                break;
                case 1:
                    resolve_to_pool(ptr).addcell(ptr.idx,label.data(),label.length(),string_id);
                    resolve_to_col(ptr).label = label;
                break;
                default: break;
            }
            ctx.node().value().set((void*)&ptr);
        });

        overload_type(id,".'take'(any)",label+"_TAKE",get_value,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" take");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            Value arg = ctx.node().right().c0().value();
            if(arg.type()==int_id) {
                int index = *(int*)arg.get();
                sync_value(ctx,&col);
                ctx.node().value().set(col.get((uint32_t)index));
                col.removeAt(index);
            }
        });
        overload_type(id,".'pop'",label+"_POP",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" pop");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            col.removeAt(col.length()-1);
            // if(YAPA_level==1&&ctx.node().value().type()==0) {
            //     ctx.node().value().type(col.tag);
            //     ctx.node().value().size(col.element_size);
            //     resolve_overload(ctx,false);
            // }
            // void* data = malloc(col.element_size);
            // col.pop(data);
            //ctx.node().value().set(data);
        });
        uint32_t length_overload_id = overload_type(id,".'length'",label+"_LENGTH",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" length");
            int len = resolve_YAPA_ptr(ptr,YAPA_level).length();
            ctx.node().value().set((void*)&len);
        });
        // r_handlers[length_overload_id] = [this](Context& ctx){
        //     if(ctx.root().type()==equals_id) {
        //         Node new_qual = make_node(length_relation_qual);
        //         new_qual.value(ctx.node().left().value());
        //         ctx.root().left().value().quals().push(new_qual);
        //     }
        // };


        auto YAPA_has = [this,YAPA_level,id,is_indirect,YAPA_add](Context& ctx, bool key_on_right) {
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" has");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);

            Node key = key_on_right?ctx.node().right():ctx.node().right().c0();
            void* data = nullptr;
            uint32_t size = 0;
            uint32_t lookup_type = 0;
            if(is_live(key.value())&&key.value().type()!=0) {
                lookup_type = key.value().type();
                if(is_ptr_alias(lookup_type)) {
                    Col& ccol = resolve_to_col(key.getPtr());
                    data = ccol.storage;
                    size = ccol.size;
                } else {
                    data = key.value().get();
                    size = key.value().size();
                }
            } else {
                data = key.name().col().storage;
                size = key.name().length();
                lookup_type = identifier_id;
            }
            bool has_thing = col.hasKey(data,size);
            ctx.node().value().set((void*)&has_thing);
        };
        overload_type(id,".'has'(any)",label+"_HAS",make_value(bool_id,1),[this,YAPA_has](Context& ctx){
            YAPA_has(ctx,false);
        });
        overload_type(id,".?any",label+"_DOT_HAS",make_value(bool_id,1),[this,YAPA_has](Context& ctx){
            YAPA_has(ctx,true);
        });
        overload_type(id,".'indexof'(any)",label+"_INDEXOF",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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

        overload_type(id,list<std::string>{".'put'(any,any)","<<[any,any]"},label+"_PUT",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" put");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);

            Node key = ctx.node().right().c0();
            void* data = nullptr;
            uint32_t size = 0;
            uint32_t lookup_type = 0;
            if(is_live(key.value())&&key.value().type()!=0) {
                lookup_type = key.value().type();
                if(is_ptr_alias(lookup_type)) {
                    Col& ccol = resolve_to_col(key.getPtr());
                    data = ccol.storage;
                    size = ccol.size;
                } else {
                    data = key.value().get();
                    size = key.value().size();
                }
            } else {
                data = key.name().col().storage;
                size = key.name().length();
                lookup_type = identifier_id;
            }
            col.qput(ctx.node().right().c1().get(),data,size,lookup_type);
            ctx.node().value().set((void*)&ptr);
        });

        overload_type(id,".'push'(any)",label+"_PUSH",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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
                    string str = ctx.node().right().getString(0);
                    col.reserve(col.length()+str.length());
                    for(int i=0;i<str.length();i++) {
                        char c = str.at(i);
                        col.push((void*)&c);
                    }
                } else {
                    col.push(ctx.node().right().c0().value().get());
                }
            }
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,"<<any",label+"_PUSH_OP",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,".'push_default'",label+"_PUSH_DEFAULT",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" push_default");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            col.push_default();
        });

        overload_type(id,".'qset'(any,any)",label+"_QSET",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" qset");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            void* data = ctx.node().right().get(0);
            int width = ctx.node().right().getInt(1);
            CHECK_ERROR("Invalid arguments for "+labels[id]+" qset");
            col.qset(ptr.sidx,data,width);
        });
        overload_type(id,".'set'(any,any)",label+"_SET",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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

        overload_type(id,".'label'",label+"_LABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr p = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" label");
            string output = resolve_string_ticket(ctx.node());
            output = resolve_YAPA_ptr(p,YAPA_level).label.to_std();
        });
        overload_type(id,".'label'(string)",label+"_LABEL_SET",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr p = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" label set");
            string label = ctx.node().right().getString(0);
            resolve_YAPA_ptr(p,YAPA_level).label = label.to_std();
        });
        overload_type(id,".'tag'",label+"_TAG",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" tag");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t tag = col.tag;
            ctx.node().value().set((void*)&tag);
        });
        overload_type(id,".'tag'(int)",label+"_TAG_SET",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" tag set");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t tag = ctx.node().right().getInt(0);
            col.tag = tag;
        });
        overload_type(id,".'element_size'",label+"_ELEMENT_SIZE",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" element_size");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t element_size = col.element_size;
            ctx.node().value().set((void*)&element_size);
        });
        overload_type(id,".'element_size'(int)",label+"_ELEMENT_SIZE_SET",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" element_size set");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t element_size = ctx.node().right().getInt(0);
            col.element_size = element_size;
        });

        overload_type(id,".'index'",label+"_INDEX",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" index");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t index = col.index;
            ctx.node().value().set((void*)&index);
        });
        overload_type(id,".'index'(int)",label+"_INDEX_SET",make_value(int_id,4),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" index set");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            uint32_t index = ctx.node().right().getInt(0);
            col.index = index;
        });

        overload_type(id,".'celllabel'",label+"_CELLLABEL",make_value(string_id,sizeof(Ptr),0,char_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr p = ctx.node().getPtr(0);
            CHECK_ERROR("Left arg of cellabel is invalid");
            string output = resolve_string_ticket(ctx.node());
            Col& cellcol = resolve_YAPA_ptr(p,YAPA_level);
            CCol* cell = cellcol.cells.find_cell(p.sidx);
            if(cell) {
                output = ((QString&)*cell).to_std();
            } else {
                output = "";
            }
        });
        overload_type(id,".'celllabel'(string)",label+"_CELLLABEL_SET",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr p = ctx.node().getPtr(0);
            CHECK_ERROR("Left arg of cellabel is invalid");
            string label = ctx.node().right().getString(0);
            Col& cellcol = resolve_YAPA_ptr(p,YAPA_level);
            CHECK_ERROR("Unable to resolve some elmenets of celllabel, label pooltag or cellcol");
            CCol* cellptr = cellcol.cells.find_cell(p.sidx);
            if(!cellptr) {
                cellcol.addcell(p.sidx,resolve_ptr(label), label.length(), string_id);
            } else {
                CCol& cell = *cellptr;
                cell.clear();
                cell.element_size = label.length();
                cell.hash = hashBytes(resolve_ptr(label), label.length());
                cell.index = p.sidx;
                cell.push(resolve_ptr(label)); 
            }
            ctx.node().value().set((void*)&p);
        });

        auto do_iter = [this,YAPA_level,is_indirect](Context& ctx, Ptr& ptr, Node body, Node element, Node index){
            Col& col = resolve_YAPA_ptr(ptr, YAPA_level);
            uint32_t len = col.length();
                        
            uint32_t i = 0;
            uint32_t* i_ptr = &i;
            if(is_live(index)) {
                if(index.value().type()==duck_id) {
                    index.value().data_col().tag = int_id; index.value().data_col().element_size = 4;
                    index.value().data_col().push_default();
                    index.value().type(int_id); index.value().size(4);   
                    index.value().set((void*)&i);
                }
                i_ptr = (uint32_t*)index.value().get();
                *i_ptr = 0;
            }
            while(*i_ptr < len) {
                ptr[YAPA_level] = *i_ptr;
                if(is_live(element)) {
                    if(is_indirect) {
                        element.value().data_ptr(*(Ptr*)resolve_ptr(ptr));
                    } else {
                        element.value().data_ptr(ptr);
                    }
                    element.value().type(element.value().data_col().tag);
                    element.value().size(element.value().data_col().element_size);
                }
                standard_travel_pass(body, ctx.sub());
                (*i_ptr)++;
            }
        };
        overload_type(id,list<std::string>{".'iter'([](any))",".'iter'(function)"}, label+"_ITER", make_value(id,sizeof(Ptr)), [this,do_iter,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" iter");
            do_iter(ctx,ptr,ctx.node().right().getNode(0).scope(),ctx.node().right().getNode(0).children().last().c0(),deadptr);
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,list<std::string>{".'iter'([](any,any))",".'iter'(function)"}, label+"_ITER_IDX", make_value(id,sizeof(Ptr)), [this,do_iter,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" iter idx");
            do_iter(ctx,ptr,ctx.node().right().getNode(0).scope(),ctx.node().right().getNode(0).children().last().c0(),ctx.node().right().getNode(0).children().last().c1());
            ctx.node().value().set((void*)&ptr);
        });

        overload_type(id,list<std::string>{".'iter'(=>)",".'do'(=>)",".'forEach'(=>)",".'values'(=>)"}, label+"_ITER_EXPR", make_value(id,sizeof(Ptr)), [this,do_iter,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" iter expr");
            Node arg = ctx.node().right().c0().left();
            do_iter(ctx,ptr,ctx.node().right().c0().scope(),arg.c0(),(arg.children().length()>1?arg.c1():deadptr));
            ctx.node().value().set((void*)&ptr);
        });


        auto do_cells = [this,YAPA_level,is_indirect](Context& ctx, Ptr& ptr, Node body, Node key, Node element, Node index){
            Col& col = resolve_YAPA_ptr(ptr, YAPA_level);
            
            uint32_t i = 0;
            uint32_t* i_ptr = &i;
            if(is_live(index)) {
                if(index.value().type()==duck_id) {
                    index.value().data_col().tag = int_id; index.value().data_col().element_size = 4;
                    index.value().data_col().push_default();
                    index.value().type(int_id); index.value().size(4);   
                    index.value().set((void*)&i);
                }
                i_ptr = (uint32_t*)index.value().get();
                *i_ptr = 0;
            }
            list<CCol*> cells = col.allCells();
            while(*i_ptr < cells.length()) {
                CCol* cell = cells[*i_ptr];
                ptr[YAPA_level] = cell->index;
                if(is_live(key)) {
                    if(key.value().type()==string_id) {
                        resolve_string_ticket(key) = ((QString&)*cell).to_std();
                    } else {
                        key.value().data_col().tag = string_id; key.value().data_col().element_size = sizeof(Ptr);
                        key.value().data_col().push_default();
                        key.value().type(string_id); key.value().size(sizeof(Ptr));
                        key.value().data_ptr().sidx=0; //Kludge because it's 10:30pm and I have no clue whose descending it!
                        resolve_string_ticket(key) = ((QString&)*cell).to_std();
                    }   
                }
                if(is_live(element)) {
                    if(is_indirect) {
                        element.value().data_ptr(*(Ptr*)resolve_ptr(ptr));
                    } else {
                        element.value().data_ptr(ptr);
                    }
                    element.value().type(element.value().data_col().tag);
                    element.value().size(element.value().data_col().element_size);
                }
                standard_travel_pass(body, ctx.sub());
                (*i_ptr)++;
            }
        };
        
        overload_type(id,list<std::string>{".'cells'([](any))",".'cells'(function)"}, label+"_CELLS", make_value(id,sizeof(Ptr)), [this,do_cells,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" cells");
            Node lambda = ctx.node().right().getNode(0);
            Node args = lambda.children().last();
            do_cells(ctx,ptr,lambda.scope(),deadptr,args.c0(),deadptr);
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,list<std::string>{".'cells'([](any,any))",".'cells'(function)"}, label+"_CELLS_KEY", make_value(id,sizeof(Ptr)), [this,do_cells,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" cells key");
            Node lambda = ctx.node().right().getNode(0);
            Node args = lambda.children().last();
            do_cells(ctx,ptr,lambda.scope(),args.c0(),args.c1(),deadptr);
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,list<std::string>{".'cells'([](any,any,any))",".'cells'(function)"}, label+"_CELLS_KEY_IDX", make_value(id,sizeof(Ptr)), [this,do_cells,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" cells key idx");
            Node lambda = ctx.node().right().getNode(0);
            Node args = lambda.children().last();
            do_cells(ctx,ptr,lambda.scope(),args.c0(),args.c1(),args.children()[2]);
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,list<std::string>{".'cells'(=>)",".'forEachCell'(=>)",".'eachCell'(=>)"}, label+"_CELLS_EXPR", make_value(id,sizeof(Ptr)), [this,do_cells,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" cells expr");
            Node args = ctx.node().right().c0().left();
            do_cells(ctx,ptr,ctx.node().right().c0().scope(),args.c0(),(args.children().length()>1?args.c1():deadptr),(args.children().length()>2?args.children()[2]:deadptr));
            ctx.node().value().set((void*)&ptr);
        });
        overload_type(id,list<std::string>{".'keys'(=>)"}, label+"_KEYS_EXPR", make_value(id,sizeof(Ptr)), [this,do_cells,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr& ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" cells expr");
            Node args = ctx.node().right().c0().left();
            do_cells(ctx,ptr,ctx.node().right().c0().scope(),args.c0(),deadptr,(args.children().length()>1?args.c1():deadptr));
            ctx.node().value().set((void*)&ptr);
        });
        
        overload_type(id,".'clear'",label+"_CLEAR",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            col.clear(); col.cells.clear();
        });

        overload_type(id,".'print_lock_state'",label+"_PRINT_LOCK_STATE",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            print("Lock state: ",(uint32_t)col.live.load());
        });
        overload_type(id,".'clear_lock'",label+"_CLEAR_LOCK",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            col.live.store(0);
        });

        overload_type(id,".'unlock'",label+"_UNLOCK",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" unlock");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            col.unlock();
        });
        overload_type(id,".'try_lock'",label+"_TRY_LOCK",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = false;
            b = col.try_lock();
            ctx.node().value().set((void*)&b);
        });
        overload_type(id,".'try_lock'(float)",label+"_TRY_LOCK_TIME",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock time");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = false;
            float wait = ctx.node().right().getFloat(0);
            b = col.try_lock(wait);
            ctx.node().value().set((void*)&b);
        });
        overload_type(id,".'try_lock_forever'",label+"_TRY_LOCK_FOREVER",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock_forever");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = col.try_lock_forever();
            ctx.node().value().set((void*)&b);
        });

        overload_type(id,".'try_lock_then'",label+"_TRY_LOCK_THEN",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            if(is_live(ctx.node().right().scope())) {
                if(col.try_lock()) {
                    standard_travel_pass(ctx.node().right().scope(),ctx.sub());
                    col.unlock();
                }
            } else {
                throw_error("Try lock then was not provided with any scope");
            }   
        });
        overload_type(id,".'try_lock_forever_then'",label+"_TRY_LOCK_FORVER_THEN",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock_forever");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            if(is_live(ctx.node().right().scope())) {
                if(col.try_lock_forever()) {
                    standard_travel_pass(ctx.node().right().scope(),ctx.sub());
                    col.unlock();
                }
            } else {
                throw_error("Try lock forever then was not provided with any scope");
            }   
        });
        overload_type(id,".'try_read_then'",label+"_TRY_READ_THEN",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_red");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            if(is_live(ctx.node().right().scope())) {
                if(col.try_read()) {
                    standard_travel_pass(ctx.node().right().scope(),ctx.sub());
                    col.unlock();
                }
            } else {
                throw_error("Try read then was not provided with any scope");
            }   
        });
        overload_type(id,".'try_read_forever_then'",label+"_TRY_READ_FORVER_THEN",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_lock_forever");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            if(is_live(ctx.node().right().scope())) {
                if(col.try_read_forever()) {
                    standard_travel_pass(ctx.node().right().scope(),ctx.sub());
                    col.unlock();
                }
            } else {
                throw_error("Try read forever then was not provided with any scope");
            }   
        });

        overload_type(id,".'try_read'",label+"_TRY_READ",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_read");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = false;
            b = col.try_read();
            ctx.node().value().set((void*)&b);
        });
        overload_type(id,".'try_read'(float)",label+"_TRY_READ_TIME",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_read");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = false;
            float wait = ctx.node().right().getFloat(0);
            b = col.try_read(wait);
            ctx.node().value().set((void*)&b);
        });
        overload_type(id,".'try_read_forever'",label+"_TRY_READ_FOREVER",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" try_red_forever");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            bool b = col.try_read_forever();
            ctx.node().value().set((void*)&b);
        });

        if(YAPA_level==3) {
            overload_type(id,".'acquire'",label+"_ACQUIRE",make_value(bool_id,1),[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" acquire");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in acquire");
                bool b = acquire_subunit(&subunit);
                ctx.node().value().set((void*)&b);
            });

            overload_type(id,".'save'",label+"_SAVE",deadptr,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" save");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in save");
                save_subunit(&subunit);
            });
            overload_type(id,".'bounce'",label+"_BOUNCE",deadptr,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" bounce");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in bounce");
                bounce_subunit(&subunit);
            });
            overload_type(id,".'load'",label+"_LOAD",deadptr,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" load");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in load");
                load_subunit(&subunit);
            });
            overload_type(id,".'reload'",label+"_RELOAD",deadptr,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" reload");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in reload");
                load_subunit(&subunit,true);
            });
            overload_type(id,".'adopt'(any)",label+"_ADOPT",deadptr,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" adopt");
                ColColCol& subunit = resolve_to_subunit(ptr);
                CHECK_ERROR("Failed to resolve Ptr to subunit in adopt");
                Ptr& adoptee = ctx.node().right().getPtr(0);
                CHECK_ERROR("Failed to resolve right Ptr to subunit in adopt");
                if(adoptee.cachelevel==3) adoptee.cache  = &subunit;
            });
        }

        if(YAPA_level>1) {
            overload_type(id,".'getByTag'(int)",label+"_GETBYTAG",get_value,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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
            overload_type(id,".'getByLabel'(string)",label+"_GETBYLABEL",get_value,[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
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

            overload_type(id,list<std::string>{".'add'", ".'add'(...)"},label+"_ADD",get_value,[this,id,YAPA_level,YAPA_add](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);

                //Infer the label and type
                Node args = ctx.node().right();
                Ptr label = deadptr;
                Value typeval = deadptr;
                uint32_t tag = 0;
                for(int i=0;i<2;i++) {
                    if(args.children().length()<=i) break;
                    Node arg = args.children()[i];
                    if(arg.type()==arg.value().type()) { 
                        typeval = arg.value();
                    } else {
                        if(arg.value().type()==string_id) {
                            label = arg.getPtr();
                        } else if(arg.value().type()==int_id) {
                            tag = arg.getInt();
                        }
                    }
                }
                CHECK_ERROR("Bad args for add in "+labels[id]+" add");
                Ptr p = deadptr;
                YAPA_add(ctx,ptr,p,tag,label,typeval);
                ctx.node().value().set((void*)&p);
            });
        } else {
            overload_type(id,".'retype'(any)",label+"_RETYPE",make_value(id,sizeof(Ptr)),[this,id,YAPA_level](Context& ctx){
                uint32_t old_type = ctx.node().type();
                standard_sub_process(ctx);
                if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
                Ptr ptr = ctx.node().getPtr(0);
                CHECK_ERROR("Invalid Ptr for "+labels[id]+" retype");
                Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
                Value val = ctx.node().right().c0().value();
                col.tag = val.type();
                col.element_size = val.size();
                ctx.node().value().set((void*)&ptr);
            });
        }


        overload_type(id,".'validate_offests_and_lock'(any)",label+"_EXPR_A",deadptr,[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            if(col.tag!=int_id) {
                throw_error("A validator must be a col of int offsets");
            } else {
                Col& scol = resolve_to_col(ctx.node().right().getPtr(0));
                for(int i=0;i<col.length();i++) {
                    int e = *(int*)col[i];
                    if(e<0||e>=scol.length()) {
                        throw_error("Element ",i," of value ",e," is out of bounds as an offset for column of length ",scol.length());
                        return;
                    }
                }
                if(scol.live.load()==1) {
                    scol.unlock();
                }
                if(!scol.try_lock()) {
                    throw_error("Unable to acquire a lock in validate_offsets_and_lock");
                    return;
                }
                ctx.node().left().value().store(ctx.node().right().getPtr(0));
                ctx.node().value(ctx.node().left().value());
            }
        });
        overload_type(id,".'store_get'(int)",label+"_EXPR_B",make_value(duck_id,0),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            Ptr ptr = ctx.node().getPtr(0);
            CHECK_ERROR("Invalid Ptr for "+labels[id]+" add");
            Col& col = resolve_YAPA_ptr(ptr,YAPA_level);
            int idx = *(int*)col[ctx.node().right().getInt(0)];
            Col& scol = ctx.node().left().value().store_col();

            if(idx<scol.length()) {
                Ptr sptr = ctx.node().left().value().store_ptr();
                sptr.sidx = idx;
                ctx.node().value().data_ptr(sptr);
                sync_value(ctx);
            } else {
                throw_error("Adjancet index is out of bounds");
            }
        });

        uint32_t bind_by_id = overload_type(id,".'bind_by'(any,any)",label+"_EXPR_C",make_value(duck_id,0),[this,id,YAPA_level](Context& ctx){
            uint32_t old_type = ctx.node().type();
            standard_sub_process(ctx);
            if(ctx.node().type()!=old_type) {standard_process(ctx); return;}
            
            uint32_t type_tag = ctx.node().right().c0().type();
            Ptr bind_to = ctx.node().right().getPtr(1);

            Value lval = ctx.node().left().value();
            if(lval.has_qual(type_tag)) {
                if(lval.get_qual(type_tag).getPtr()==bind_to) {
                    return;
                } else {
                    lval.get_qual(type_tag).value().set((void*)&bind_to);
                }
            }
        });
        r_handlers[bind_by_id] = [this,YAPA_level,id](Context& ctx){
            uint32_t type_tag = ctx.node().right().c0().type();
            Value lval = ctx.node().left().value();
            Node new_qual =  make_node(type_tag);
            new_qual.value(make_value(ptr_id,sizeof(Ptr)));
            new_qual.value().set((void*)&deadptr);
            lval.quals().push(new_qual);
        };

        uint32_t assert_bound_by_id = overload_type(id,".'assert_bound_by'(any)",label+"_EXPR_D",deadptr,[this,id,YAPA_level](Context& ctx){
        });
        r_handlers[assert_bound_by_id] = [this,YAPA_level,id](Context& ctx){
            uint32_t type_tag = ctx.node().right().c0().type();
            if(!ctx.node().left().has_qual(type_tag)) {
                throw_error("Node ",node_info(ctx.left(),1)," must be bound by ",labels[type_tag]);
            }
        };

        return id;
    }

    uint32_t Acorn_Script::make_YAPA_type(const std::string& label, uint32_t YAPA_level, bool is_indirect) {
        uint32_t id = make_type(label,sizeof(Ptr));
        register_ptr_alias(id);
        value_printers[id] = [this](Context& ctx){Ptr p = *(Ptr*)ctx.value().get(); ctx.source(Ptr_to_string(p,p.cachelevel));};
        register_YAPA_type(label,YAPA_level,id,is_indirect);
        return id;
    }

    uint32_t Acorn_Script::init_adjacency_type() {
        uint32_t id = make_YAPA_type("Adjacency",1,false);

        add_function("add_binding_qual",[this](Context& ctx){
            standard_sub_process(ctx);
            uint32_t qid = add_qual(ctx.node().getString(0).to_std());
            x_handlers[to_prefix_id(qid)] = [this](Context& ctx){
                standard_process(ctx,ctx.qual().type());
            };
        });

        uint32_t rprint_id = add_function("rprint",[this](Context& ctx){});
        r_handlers[rprint_id] = [this](Context& ctx){
            print(ctx.node().c0().name());
        };

        r_handlers[to_prefix_id(length_relation_qual)] = [this](Context& ctx){
            if(ctx.node().type()==labels_lookup["Ptr_IDXGET"]) {
                if(ctx.qual().value()==ctx.node().left().value()) {
                    throw_error("Violated length relation, ",node_info(ctx.node())," is defined by the length of the container it is attempting to index");
                }
            }
        };

        x_handlers[to_prefix_id(offset_qual)] = [this](Context& ctx){
            if(ctx.node().type()==labels_lookup["Ptr_GET"]) {
                if(ctx.node().getInt()<0||ctx.node().getInt()>=resolve_to_col(ctx.qual().getPtr()).length()) {
                    throw_error("Retrived offset is out of bounds for offset qual");
                }
            }
            if(ctx.node().type()==labels_lookup["Ptr_INIT"]) {
                Col& nums = resolve_to_col(ctx.node().getPtr());
                for(int i=0;i<nums.length();i++) {
                    int num = *(int*)nums[i];
                    if(num<0||num>=resolve_to_col(ctx.qual().getPtr()).length()) {
                        throw_error("Retrived offset ",num," is out of bounds for offset qual during initilization");
                    }
                }
            }
        };
        return id;
    }
}