% Design an 2nd-order critically damped lowpass digital filter with normalized cutoff
% frequency Wn. Return the numerator and denominator coefficients of the filter transfer function.
% Wn is normalized to Nyquist, following the MATLAB convention.
%
% The magnitude response is -3 dB at Wn.
% The analog prototype is H(s) = wp^2 / (s + wp)^2 (double pole at -wp),
% where wp = wc*sqrt(sqrt(2)+1) and wc = 2*tan(pi*Wn/2) is the prewarped cutoff.
% The digital filter is obtained via bilinear transform s = 2*(z-1)/(z+1).
function [b, a] = critdamplp2(Wn)
  % Prewarp the desired -3 dB cutoff to the analog domain.
  wc = 2 * tan(pi * Wn / 2);
  % Scale to the pole frequency so that |H(j*wc)| = -3 dB.
  % For H(s) = wp^2/(s+wp)^2: |H(j*wc)|^2 = 1/2 requires wp = wc*sqrt(sqrt(2)+1).
  wp = wc * sqrt(sqrt(2) + 1);
  r = (wp - 2) / (wp + 2);
  K = (wp / (wp + 2))^2;
  b = K * [1, 2, 1];
  a = [1, 2*r, r^2];
end
