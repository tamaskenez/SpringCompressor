% How does inharmonic distortion arise

% Parameters
fs = 44100;              % Sample rate
f0 = fs/34.5;               % Input frequency (1.3 kHz)
duration = 1.0;          % Seconds
t = 0:1/fs:duration-(1/fs);

% 1. Generate Sine Wave
x = sin(2 * pi * f0 * t);

% 2. Apply Nonlinearity (Half-Wave Rectifier)
% In the analog world, this creates harmonics at 0Hz, 1.3kHz, 2.6kHz, 3.9kHz...
y = max(0, x);

% 3. Analyze Spectrum
L = length(y);
Y = fft(y .* blackman(length(y))');
P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);
freqs = fs*(0:(L/2))/L;

% 4. Plot
figure;
semilogx(freqs, 20*log10(P1));
grid on;
title(['Spectrum of Half-Wave Rectified ', num2str(f0), 'Hz Sine']);
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
axis([20 22050 -100 0]);

% Highlight the first few "Valid" Harmonics
fprintf('Expected Harmonics: %d, %d, %d, %d...\n', f0, f0*2, f0*3, f0*4);

% f0, 2*f0, 3*fn, n*f0 > fs/2
% n*f0 -> fs - n*f0
% (fs - n * f0) / f0 = fs/f0 - n