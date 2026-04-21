#pragma once

#include "GeneratorStatus.h"
#include "Modes.h"
#include "Role.h"

#include <optional>
#include <string>
#include <utility>

struct ProbeRoleState;

class ProcessorInterface
{
public:
    virtual void on_role_selected_by_user_mt(Role role) = 0;

protected:
    ~ProcessorInterface() = default;
};
