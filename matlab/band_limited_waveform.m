% Synthesize band limited waveform as a sum of harmonics up to Nyquist. Return the vector
% with the synthesized period.
%
% - period_samples: length of the period in samples
% - even_harmonics: true or false
% - amp_exp: the k-th harmonics has an amplitude of k^amp_exp
% - dphase: the k-th harmonics has a phase of (k - 1) * dphase radians (meaning the fundamental, the 1st harmonics is cosine)
function y = band_limited_waveform(period_samples, even_harmonics, amp_exp, dphase)
    N = period_samples;
    n = (0:N-1)';
    k_max = floor(N / 2);

    if even_harmonics
        harmonics = 1:k_max;
    else
        harmonics = 1:2:k_max;
    end

    y = zeros(N, 1);
    for k = harmonics
        phase = (k - 1) * dphase;
        y = y + k^amp_exp * cos(2*pi * k * n / N + phase);
    end
end
