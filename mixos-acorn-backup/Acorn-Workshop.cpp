#include "../mixos-acorn/Acorn-Workshop.hpp"

namespace Acorn {
    
    void Workshop_Unit::sub_init() {
        m_handlers[equals_rangle_id] = [this](Context& ctx){
            if(is_live(ctx.node().scope())) {
               if(ctx.node().scope().children().length()==1) {
                    ctx.node().value(ctx.node().scope().children()[0].value());
               }
            }
            m_handlers.default_function(ctx);
        };
    }
}