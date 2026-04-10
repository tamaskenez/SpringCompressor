classdef Interface < handle
    % dfilter.Interface  Abstract base class for digital filters.
    %
    % Represents a particular filter implementation together with its
    % parameters and internal state. Subclasses are responsible for
    % storing both the filter coefficients/matrices and the runtime state.
    %
    % Planned subclasses:
    %   - Transfer function filter (numerator/denominator polynomials, any order)
    %   - First-order topology-preserving (TPT) filter
    %   - Second-order state-variable (SVF) topology-preserving filter
    %   - State-space filter (A, B, C, D matrices)
    %
    % All subclasses are handle classes (stateful, modified in place).

    methods (Abstract)

        % filter  Process input samples through the filter.
        %
        %   y = obj.filter(x)
        %
        %   x - scalar or column vector of input samples
        %   y - scalar or column vector of output samples, same size as x
        %
        % The filter state is updated in place after each call.
        y = filter(obj, x)

        % reset  Reset the filter state to zero initial conditions.
        %
        %   obj.reset()
        %
        % After reset(), the filter behaves as if it has seen no prior input.
        % Filter parameters (coefficients, matrices) are not affected.
        reset(obj)

        % override_output  Force the last output to a given value.
        %
        %   obj.override_output(y)
        %
        %   y - scalar, the value to treat as the most recent filter output
        %
        % Updates the internal state so that the next call to filter()
        % continues smoothly from y, avoiding discontinuities. The exact
        % state update depends on the filter topology (see subclasses).
        override_output(obj, y)

    end

end
