% Comparing the Hilbert RMS detector's ripple to non-hilbert

fs = 48000;
f1 = 500;
f2 = 1.5 * f1;
ns = [0:fs-1]';
x = sin(2*pi*f1/fs*ns) + 1 * sin(2*pi*f2/fs*ns);
x = x / sqrt(2);
x(1:fs/4) = 0;
x(3*fs/4:end) = 0;
x = x / sqrt(fs/4);
hx = hilbert(x)/sqrt(2);

plot(ns, pow2db(abs(fft(abs(x).^2))));

%                    REAL                     ANALTYTIC
% f1 only       -3 dB at 2 * f1                  -INF
% f1 + 2 * f1   -3 dB at f1, -6 at 2 * f1      -3 dB at f1
