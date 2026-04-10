% Coefficients for the first order digital low-pass filter, topology preserving realization.
% Wn is the cutoff frequency (0 < Wn < 1), in half-cycles-per-sample (MATLAB convention)
function g = tptlp1(Wn)
    g = tan(pi * Wn / 2);
end
