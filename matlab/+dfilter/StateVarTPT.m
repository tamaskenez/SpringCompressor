classdef StateVarTPT < dfilter.Interface
    % dfilter.StateVarTPT  Second-order state-variable TPT filter (SVF).
    %
    % Topology-preserving discretisation of the RLC state-variable filter.
    % Outputs the low-pass response. The two integrator states correspond
    % directly to the BP and LP outputs in steady state, which makes
    % override_output exact.
    %
    % Usage:
    %
    %   f = dfilter.StateVarTPT(freq_hps, Q)
    %
    %   freq_hps - cutoff frequency in half-cycles-per-sample (0 < freq_hps < 1)
    %   Q        - quality factor (Q = 1/sqrt(2) for Butterworth)

    properties (Access = private)
        g   % integrator coefficient
        h   % high-pass gain factor: 1 / (1 + (R2 + g) * g)
        R2  % damping coefficient: 1/Q
        s1  % BP integrator state
        s2  % LP integrator state
    end

    methods

        function obj = StateVarTPT(freq_hps, Q)
            a              = 2.0 - 1.0 / Q^2;
            omega_ratio    = sqrt((a + sqrt(a^2 + 4)) / 2);
            obj.g          = tan(pi * freq_hps / 2 / omega_ratio);
            obj.R2         = 1 / Q;
            obj.h          = 1 / (1 + (obj.R2 + obj.g) * obj.g);
            obj.s1         = 0;
            obj.s2         = 0;
        end

        function y = filter(obj, x)
            g  = obj.g;
            h  = obj.h;
            R2 = obj.R2;
            s1 = obj.s1;
            s2 = obj.s2;
            y  = zeros(size(x));
            for k = 1:numel(x)
                yHP   = h * (x(k) - (R2 + g) * s1 - s2);
                yBP   = g * yHP + s1;
                s1    = 2 * yBP - s1;
                yLP   = g * yBP + s2;
                s2    = 2 * yLP - s2;
                y(k)  = yLP;
            end
            obj.s1 = s1;
            obj.s2 = s2;
        end

        function reset(obj)
            obj.s1 = 0;
            obj.s2 = 0;
        end

        function override_output(obj, y_out)
            % In steady state the BP integrator state is zero (bandpass
            % rejects DC) and the LP integrator state equals the LP output,
            % so setting s1 = 0, s2 = y_out is exact.
            obj.s1 = 0;
            obj.s2 = y_out;
        end

    end

end
