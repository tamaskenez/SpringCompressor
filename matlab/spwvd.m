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
%   timewin - the window used in the inner sum. The length must be odd.
%   freqwin - the window used in the outer sum. The length must be odd and less or equal to nfft.
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

half_t = (length(timewin) - 1) / 2;
half_f = (length(freqwin) - 1) / 2;

% Zero-pad x so that accesses near boundaries return zero.
pad      = half_t + half_f;
x_padded = [zeros(pad, 1); x; zeros(pad, 1)];

d = zeros(m_out, length(ts));

for t_idx = 1:length(ts)
    t = ts(t_idx);
    K = zeros(nfft, 1);

    for m = -half_t:half_t
        % Outer (time-smoothing) sum over shifts s.
        inner = 0;
        for s = -half_f:half_f
            ip = t + s + m + pad;   % x_padded index for x(t+s+m)
            im = t + s - m + pad;   % x_padded index for x(t+s-m)
            inner = inner + freqwin(s + half_f + 1) * x_padded(ip) * conj(x_padded(im));
        end

        % Place the weighted sample into the FFT-ordered kernel.
        % Negative m values wrap to the end of the array (standard FFT layout).
        K(mod(m, nfft) + 1) = K(mod(m, nfft) + 1) + timewin(m + half_t + 1) * inner;
    end

    col = fft(K);
    d(:, t_idx) = real(col(1:m_out));
end
