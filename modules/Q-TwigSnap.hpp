#pragma once

#include "../modules/Q-HTML.hpp"

namespace GDSL {
    struct TwigSnap_Unit : public HTML_Unit {
        TwigSnap_Unit() { init(); }

        std::string default_button_style = 
            "padding:8px 16px;"
            "background-color:#3b82f6;"
            "color:white;"
            "border:none;"
            "border-radius:4px;"
            "cursor:pointer;"
            "font-size:14px;";

        g_ptr<Node> make_property(const std::string& type, const std::string& value) {
            g_ptr<Node> prop_node = make<Node>(property_id);
            prop_node->name = type;
            prop_node->opt_str = value;
            return prop_node;
        }

        g_ptr<Node> make_button(const std::string& id, const std::string& label, std::string action = "") {
            g_ptr<Node> btn = make<Node>(button_id);
            if(!action.empty()) 
                btn->name = "id=\""+id+"\" onclick=\""+action+"\"";
            
            g_ptr<Node> sty = make<Node>(style_id);
            sty->children << make_property("cursor", "pointer");
            sty->children << make_property("padding", "8px 16px");
            sty->children << make_property("background-color", "#374151");
            sty->children << make_property("color", "white");
            sty->children << make_property("border", "none");
            sty->children << make_property("border-radius", "4px");
            sty->children << make_property("font-size", "14px");
            btn->quals << sty;
            
            btn->children << make<Node>(text_id, span_subtype, label);
            return btn;
        }
    };
}