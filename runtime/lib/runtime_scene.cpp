#include "atlas/runtime/context.h"
#include "atlas/runtime/scripting.h"
#include "atlas/window.h"

void RuntimeScene::update(Window &window) {
    auto runtimeContext = context.lock();
    if (runtimeContext == nullptr || runtimeContext->camera == nullptr ||
        !runtimeContext->cameraAutomaticMoving) {
        if (runtimeContext != nullptr && runtimeContext->context != nullptr) {
            runtime::scripting::dispatchInteractiveFrame(
                runtimeContext->context, runtimeContext->scriptHost, window,
                window.getDeltaTime());
        }
        return;
    }

    static const std::string emptyAction;
    const auto actionAt = [&](std::size_t index) -> const std::string & {
        return index < runtimeContext->cameraActions.size()
                   ? runtimeContext->cameraActions[index]
                   : emptyAction;
    };
    runtimeContext->camera->updateWithActions(window, actionAt(0), actionAt(1),
                                              actionAt(2));

    if (runtimeContext->context != nullptr) {
        runtime::scripting::dispatchInteractiveFrame(
            runtimeContext->context, runtimeContext->scriptHost, window,
            window.getDeltaTime());
    }
}

void RuntimeScene::onMouseMove(Window &window, Movement2d movement) {
    auto runtimeContext = context.lock();
    if (runtimeContext != nullptr && runtimeContext->context != nullptr) {
        const auto [x, y] = window.getCursorPosition();
        MousePacket packet;
        packet.xpos = static_cast<float>(x);
        packet.ypos = static_cast<float>(y);
        packet.xoffset = movement.x;
        packet.yoffset = movement.y;
        packet.constrainPitch = true;
        packet.firstMouse = runtimeContext->scriptHost.interactiveFirstMouse;
        runtime::scripting::dispatchInteractiveMouseMove(
            runtimeContext->context, runtimeContext->scriptHost, window, packet,
            window.getDeltaTime());
    }
}

void RuntimeScene::onMouseScroll(Window &window, Movement2d offset) {
    auto runtimeContext = context.lock();
    if (runtimeContext != nullptr && runtimeContext->context != nullptr) {
        MouseScrollPacket packet{offset.x, offset.y};
        runtime::scripting::dispatchInteractiveMouseScroll(
            runtimeContext->context, runtimeContext->scriptHost, packet,
            window.getDeltaTime());
    }
}
