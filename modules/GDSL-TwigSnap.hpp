#pragma once

#include "../modules/Q-TwigSnap.hpp"
#include "../modules/GDSL-Starter.hpp"

namespace GDSL {
    struct TwigSnap_DSL_Frontend : public virtual TwigSnap_Unit, public virtual Starter_DSL_Frontend {
        TwigSnap_DSL_Frontend() { init(); }

        void init() override {
            TwigSnap_Unit::init();
            Starter_DSL_Frontend::init();

            scope_link_handlers[to_unary_id(slash_id)] = [](g_ptr<Node> new_scope, g_ptr<Node> current_scope, g_ptr<Node> owner_node) {
                new_scope->owner = owner_node.getPtr();
                owner_node->scopes << new_scope.getPtr();
                for(auto c : owner_node->children) {
                    c->place_in_scope(new_scope.getPtr());
                }
                owner_node->name = "/"+(owner_node->left()?owner_node->left()->name:"");
                new_scope->name = owner_node->name;

            };
            discard_types.push_if_absent(rbrace_id);
        }
    };
}