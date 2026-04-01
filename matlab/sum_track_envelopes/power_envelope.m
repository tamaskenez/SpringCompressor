% Return the power envelope of 1-channel audio signal `x`.
% It will be a low-pass filtered version of `x.^2`, with the low-pass filter
% tailored to the frequencies in `x`. The low-pass filtering has a linear phase, zero-delay,
function p = power_envelope(x, fs)
    w = gausswin(round(fs/80/2)*2+1);
    w = w / sum(w);
    p = conv(abs(hilbert(x)/sqrt(2)).^2, w, 'same');
endfunction
