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
    virtual void on_role_selected_by_user(Role role) = 0;
    virtual void on_mode_changed(Mode::E mode) = 0;

    // Return generator status and current command text.
    virtual std::pair<GeneratorStatus, std::optional<std::string>> get_generator_status() const = 0;
    virtual const ProbeRoleState* get_probe_state() const = 0;

protected:
    ~ProcessorInterface() = default;
};
