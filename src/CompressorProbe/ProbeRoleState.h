#pragma once

#include "Command.h"
#include "Modes.h"

class Pipe;
struct ProbeRoleState {
    std::unique_ptr<Pipe> pipe;
    optional<Command> pending_command; // Command that has been sent but no response received.
    struct Modes {
        Mode::Bypass bypass;
        Mode::DecibelCycle decibel_cycle;
    } modes;
    Mode::E current_mode = Mode::E::Bypass;

    ~ProbeRoleState();
};
