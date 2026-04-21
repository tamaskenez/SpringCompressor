#pragma once

#include "Command.h"
#include "Modes.h"

class Pipe;

struct ProbeRoleState {
    std::unique_ptr<Pipe> pipe;
    optional<Command> pending_command; // Command that has been sent but no response received.

    int next_command_index = 1;

    ~ProbeRoleState();
};
