
#ifndef THREEPP_MOUSELISTENER_HPP
#define THREEPP_MOUSELISTENER_HPP

#include "threepp/math/Vector2.hpp"
#include "threepp/utils/uuid.hpp"

#include <functional>
#include <utility>

namespace threepp {

    struct MouseEvent {

        const int button;
        const int action;
        const int mods;

        MouseEvent(const int button, const int action, const int mods)
            : button(button), action(action), mods(mods) {}
    };

    struct MouseListener {

        const std::string uuid = utils::generateUUID();

        virtual void onMouseDown(int button, Vector2 pos) {}
        virtual void onMouseUp(int button, Vector2 pos) {}
        virtual void onMouseMove(Vector2 pos) {}
        virtual void onMouseWheel(Vector2 delta) {}

        virtual ~MouseListener() = default;

    };

    struct MouseMoveListener: MouseListener {

        explicit MouseMoveListener(std::function<void(Vector2)> f)
            : f_(std::move(f)) {}

        void onMouseMove(Vector2 pos) override {
            f_(pos);
        }

    private:
        std::function<void(Vector2)> f_;

    };

}// namespace threepp


#endif//THREEPP_MOUSELISTENER_HPP
