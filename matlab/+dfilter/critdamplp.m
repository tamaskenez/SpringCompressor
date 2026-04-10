% dfilter.critdamplp  Critically-damped low-pass filter returned as a dfilter.TF object.
%
%   f = dfilter.critdamplp(order, Wn, topology)
%
% Design a 1st or 2nd-order critically damped lowpass digital filter with normalized cutoff
% frequency Wn. Return the numerator and denominator coefficients of the filter transfer function.
% Wn is normalized to Nyquist, following the MATLAB convention.
%
% When order is 1, return a 1st order low pass with `butter`.
%
% topology can be 'tf' (default) or 'tpt'
%
% The magnitude response is -3 dB at Wn.
% The analog prototype is H(s) = wp^2 / (s + wp)^2 (double pole at -wp),
% where wp = wc*sqrt(sqrt(2)+1) and wc = 2*tan(pi*Wn/2) is the prewarped cutoff.
% The digital filter is obtained via bilinear transform s = 2*(z-1)/(z+1).
function f = critdamplp(order, Wn, topology)
    if nargin < 3
        topology = 'tf';
    end
    switch order
        case 1
            if strcmp(topology, 'tf')
                f = dfilter.butter(1, Wn);
            elseif strcmp(topology, 'tpt')
                f = dfilter.TPT1(Wn);
            else
                error('Unknown topology: %s', topology);
            end
        case 2
            if strcmp(topology, 'tf')
                % Prewarp the desired -3 dB cutoff to the analog domain.
                wc = 2 * tan(pi * Wn / 2);
                % Scale to the pole frequency so that |H(j*wc)| = -3 dB.
                % For H(s) = wp^2/(s+wp)^2: |H(j*wc)|^2 = 1/2 requires wp = wc*sqrt(sqrt(2)+1).
                wp = wc * sqrt(sqrt(2) + 1);
                r = (wp - 2) / (wp + 2);
                K = (wp / (wp + 2))^2;
                b = K * [1, 2, 1];
                a = [1, 2 * r, r^2];
                f = dfilter.TF(b, a);
            elseif strcmp(topology, 'tpt')
                f = dfilter.StateVarTPT(Wn, 0.5);
            else
                error('Unknown topology: %s', topology);
            end
        otherwise
            error('order must be 1 or 2');
    end
endfunction
