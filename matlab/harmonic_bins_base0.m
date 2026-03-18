% For an M-point FFT, we have a fundamental frequency in the `k` bin (base-0)
% Return the base-0 bins of all the harmonics.
function bins = harmonic_bins_base0(k, M, only_positive)
    middle_bin = M/2;
    max_harmonics = floor(M/2/k);
    h = (1:max_harmonics)';
    bins = h * k;
    if nargin == 2 && ~only_positive
        bins = unique([bins sort(M - bins)]);
    end
end
