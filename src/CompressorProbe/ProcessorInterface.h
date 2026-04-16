#pragma once

#include "GeneratorStatus.h"
#include "Role.h"

#include <optional>
#include <string>
#include <utility>

class ProcessorInterface
{
public:
    virtual void on_role_selected_by_user(Role role) = 0;

    // Return generator status and current command text.
    virtual std::pair<GeneratorStatus, std::optional<std::string>> get_generator_status() const = 0;

protected:
    ~ProcessorInterface() = default;
};
