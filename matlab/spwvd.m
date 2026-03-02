% FUNCTION NAME:
%   spwvd
%
% DESCRIPTION:
%   Compute the smoothed pseudo Wigner-Ville distribution of a signal.
%   The computation will be performed around the indices specified in the ts argument.
%   In the inner sum of the SPWVD computation only even lags will be considered so we don't
%   need to interpolate between samples.
%
% INPUT:
%   x       - vector, the input signal
%   ts      - vector of the indices in x where the distribution will be calculated at
%   nfft    - can be a single number, the size of the FFT.
%             It can also be 2 numbers: [nfft m] where m specifies the number of FFT coefficients to be returned (1:m)
%   timewin - time-smoothing window applied along time shifts. Controls time resolution. The length must be odd.
%   freqwin - frequency-smoothing window applied along lags. Controls frequency resolution. The length must be odd and less or equal to nfft.
%
% OUTPUT:
%   d - matrix of the computed distribution, rows are the frequency axis and columns is the time axis
%       the size of the matrix is [nfft length(ts)] or [m length(ts)] if nfft is a 2-element vector
%
function d = spwvd(x, ts, nfft, timewin, freqwin)

if numel(nfft) == 2
    m_out = nfft(2);
    nfft  = nfft(1);
else
    m_out = nfft;
end

x       = x(:);
timewin = timewin(:);
freqwin = freqwin(:);

half_lag  = (length(freqwin) - 1) / 2;  % freqwin applied along lags  → controls frequency resolution
half_time = (length(timewin) - 1) / 2;  % timewin applied along time shifts → controls time resolution

% Zero-pad x so that accesses near boundaries return zero.
pad      = half_lag + half_time;
x_padded = [zeros(pad, 1); x; zeros(pad, 1)];

d = zeros(m_out, length(ts));

for t_idx = 1:length(ts)
    t = ts(t_idx);
    K = zeros(nfft, 1);

    for m = -half_lag:half_lag
        % Time-smoothing sum over shifts s, vectorised.
        base_plus  = t + m + pad;
        base_minus = t - m + pad;
        seg_plus  = x_padded((base_plus  - half_time):(base_plus  + half_time));
        seg_minus = x_padded((base_minus - half_time):(base_minus + half_time));
        inner = sum(timewin .* seg_plus .* conj(seg_minus));

        % Place the lag-windowed sample into the FFT-ordered kernel.
        % Negative m values wrap to the end of the array (standard FFT layout).
        K(mod(m, nfft) + 1) = K(mod(m, nfft) + 1) + freqwin(m + half_lag + 1) * inner;
    end

    col = fft(K);
    d(:, t_idx) = real(col(1:m_out));
end
