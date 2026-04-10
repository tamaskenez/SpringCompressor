classdef TPT1 < dfilter.Interface
    % dfilter.TPT1  First-order topology-preserving transform (TPT) low-pass filter.
    %
    % Topology-preserving discretisation of the RC low-pass. The integrator
    % state s corresponds directly to the low-pass output in steady state,
    % which makes override_output exact.
    %
    % Usage:
    %
    %   f = dfilter.TPT1(Wn)
    %
    %   Wn - cutoff frequency in half-cycles-per-sample (0 < Wn < 1)

    properties (Access = private)
        g   % filter coefficient: tan(pi * Wn / 2)
        g1  % precomputed 1 / (1 + g)
        s   % integrator state (scalar)
    end

    methods

        function obj = TPT1(Wn)
            obj.g  = tan(pi * Wn / 2);
            obj.g1 = 1 / (1 + obj.g);
            obj.s  = 0;
        end

        function y = filter(obj, x)
            n  = numel(x);
            y  = zeros(size(x));
            g  = obj.g;
            g1 = obj.g1;
            s  = obj.s;
            for k = 1:n
                v    = (x(k) - s) * g1;
                lp   = v * g + s;
                s    = lp + v * g;
                y(k) = lp;
            end
            obj.s = s;
        end

        function reset(obj)
            obj.s = 0;
        end

        function override_output(obj, y_out)
            % In steady state the integrator state equals the LP output,
            % so setting s = y_out is exact.
            obj.s = y_out;
        end

    end

end
