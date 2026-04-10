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

    % Octave does not support methods (Abstract) in classdef; using concrete
    % methods that throw instead.  Subclasses must override all three.
    methods

        % filter  Process input samples through the filter.
        %
        %   y = obj.filter(x)
        %
        %   x - scalar or vector of input samples
        %   y - output samples, same size as x
        %
        % The filter state is updated in place after each call.
        function y = filter(obj, x)
            error('dfilter:notImplemented', ...
                  'filter() not implemented by subclass %s', class(obj));
        end

        % reset  Reset the filter state to zero initial conditions.
        %
        %   obj.reset()
        %
        % After reset(), the filter behaves as if it has seen no prior input.
        % Filter parameters (coefficients, matrices) are not affected.
        function reset(obj)
            error('dfilter:notImplemented', ...
                  'reset() not implemented by subclass %s', class(obj));
        end

        % override_output  Force the last output to a given value.
        %
        %   obj.override_output(y)
        %
        %   y - scalar, the value to treat as the most recent filter output
        %
        % Updates the internal state so that the next call to filter()
        % continues smoothly from y, avoiding discontinuities. The exact
        % state update depends on the filter topology (see subclasses).
        function override_output(obj, y)
            error('dfilter:notImplemented', ...
                  'override_output() not implemented by subclass %s', class(obj));
        end

    end

end
