#pragma once

enum class GeneratorStatus {
    Idle,
    TransmittingId,
    Connected // In this state we might be executing the current_command.
};
