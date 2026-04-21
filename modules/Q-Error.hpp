#pragma once

#include "../core/GDSL.hpp"

namespace GDSL {
    struct Error_unit : public virtual Unit {
        Error_unit() { init(); }

        size_t error_id = reg_id("ERROR");
        size_t trivial_error = reg_id("TRIVIAL");
        size_t minor_error = reg_id("MINOR");
        size_t normal_error = reg_id("NORMAL");
        size_t major_error = reg_id("MAJOR");
        size_t severe_error = reg_id("SEVERE");
        size_t fatal_error = reg_id("FATAL");

        void attach_error(g_ptr<Node> to, size_t level, const std::string& message) {
            g_ptr<Node> error = make<Node>(error_id);
            error->opt_str = message;
            error->mute = true;
            error->sub_type = level;
            to->quals << error;
        };

        void init() override {

        }
    };
}