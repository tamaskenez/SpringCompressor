classdef AsymmetricCritDampLPF < handle
    % Two filters, switched on-the-fly depending whether samples are rising (attack)
    % or falling (decay/release)
    %
    % Usage:
    %
    %   f = AsymmetricCritDampLPF(attack_order, attack_freq_hps, release_order, release_freq_hps)
    %
    % For now, order must be 2.

    properties (Access = private)
        f_attack % attack filter
        f_release % release filter
    end

    methods

        function obj = AsymmetricCritDampLPF(attack_order, attack_freq_hps, release_order, release_freq_hps)
            assert(attack_order == 2);
            assert(release_order == 2);
            obj.f_attack = dfilter.critdamplp(attack_order, attack_freq_hps, 'tpt');
            obj.f_release = dfilter.critdamplp(release_order, release_freq_hps, 'tpt');
        end

        function y = filter(obj, x)
            assert(isvector(x));
            y = zeros(size(x));
            for i = 1:length(x)
                a = obj.f_attack.filter(x(i));
                r = obj.f_release.filter(x(i));
                if a > r
                    y(i) = a;
                    s = obj.f_attack.get_state();
                    obj.f_release.set_state([0 s(2)]);
                else
                    y(i) = r;
                    s = obj.f_release.get_state();
                    obj.f_attack.set_state([0 s(2)]);
                end
            end
        end

        function reset(obj)
            obj.f_attack.reset();
            obj.f_release.reset();
        end
    end
end
