classdef TF < dfilter.Interface
    % dfilter.TF  Transfer-function digital filter.
    %
    % Wraps MATLAB/Octave's built-in filter() using Direct Form II Transposed
    % (DFII-T), so behaviour is identical to:
    %
    %   [y, z] = filter(b, a, x, z)
    %
    % Usage:
    %
    %   f = dfilter.TF(b, a)
    %
    %   b - numerator polynomial coefficients   [b0, b1, b2, ...]
    %   a - denominator polynomial coefficients [a0, a1, a2, ...]
    %
    % a(1) need not equal 1; the constructor normalises b and a by a(1).
    % The filter order is max(length(b), length(a)) - 1.

    properties (Access = private)
        b   % normalised numerator (column vector, length N)
        a   % normalised denominator (column vector, length N)
        z   % DFII-T state (column vector, length N-1)
    end

    methods

        function obj = TF(b, a)
            scale  = a(1);
            obj.b  = b(:) / scale;
            obj.a  = a(:) / scale;
            n_state = max(length(obj.b), length(obj.a)) - 1;
            obj.z  = zeros(n_state, 1);
        end

        function y = filter(obj, x)
            % Use builtin() to avoid shadowing by this method's own name.
            [y_flat, obj.z] = builtin('filter', obj.b, obj.a, x(:), obj.z);
            y = reshape(y_flat, size(x));
        end

        function reset(obj)
            obj.z(:) = 0;
        end

        function override_output(obj, y_out)
            % Set state to the DFII-T steady state consistent with constant
            % output y_out.  The implied constant input is derived from the
            % DC gain: x_ss = y_out * sum(a) / sum(b).
            %
            % Derivation: in steady state z(k) = z_prev(k) for all k, so
            % the DFII-T recurrence gives:
            %
            %   z_ss(k) = sum(b_ext(k+1:N)) * x_ss
            %            - sum(a_ext(k+1:N)) * y_ss,   k = 1 .. N-1
            %
            % where b_ext and a_ext are b, a padded to length N = n_state+1.
            % After the next call to filter(x_ss) the state is unchanged,
            % and the next filter(x) will continue smoothly from y_out.

            if isempty(obj.z)
                return;
            end

            N     = length(obj.z) + 1;
            b_ext = [obj.b; zeros(N - length(obj.b), 1)];
            a_ext = [obj.a; zeros(N - length(obj.a), 1)];

            dc_b = sum(b_ext);

            if abs(dc_b) < eps
                % Filter has zero DC gain: no consistent steady-state input
                % exists, so fall back to zeroing the state.
                obj.z(:) = 0;
                return;
            end

            x_ss = y_out * sum(a_ext) / dc_b;

            % Suffix sums: suffix(k) = sum(v(k+1:N)), vectorised via cumsum.
            b_suffix = flipud(cumsum(flipud(b_ext(2:end))));
            a_suffix = flipud(cumsum(flipud(a_ext(2:end))));

            obj.z = b_suffix * x_ss - a_suffix * y_out;
        end

    end

end
