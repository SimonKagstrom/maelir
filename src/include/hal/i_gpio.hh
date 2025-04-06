#pragma once

#include "../listener_cookie.hh"

#include <cstdint>
#include <functional>
#include <memory>

namespace hal
{

class IGpio
{
public:
    virtual ~IGpio() = default;

    virtual bool GetState() const = 0;

    virtual void SetState(bool state) = 0;

    virtual std::unique_ptr<ListenerCookie>
    AttachIrqListener(std::function<void(bool)> on_state_change) = 0;
};

} // namespace hal