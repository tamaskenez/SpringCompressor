% TODO
% - subtle chorus effect test: modulate signal by itself
% - check UAD SSL

% Module 1 or more sine waves with (1 + sine), sqrt(1 + sine), db2mag(1 + sine) and check spectrum

F0 = 1000; % Carrier frequency.
G0 = 100; % Modulation frequency.
A = 0.2; % Modulation amplitude.

fs = 48000;
N = fs;
t = [0:N-1]'/fs;
x = sin(2*pi*F0*t);
m = sin(2*pi*G0*t);
% sqrt(1 + Bup) = 1 + A
% sqrt(1 + Bdn) = 1 - A
Bup = (1 + A)^2 - 1;
Bdn = (1 - A)^2 - 1;
B0 = 1 + (Bup + Bdn) / 2;
B = (Bup - Bdn) / 2;
second_modulation = sqrt(B0 + B * m);
if 1
    % db2mag(1 + Cup) = 1 + A
    % db2mag(1 + Cdn) = 1 - A
    Cup = mag2db(1 + A) - 1;
    Cdn = mag2db(1 - A) - 1;
    C0 = 1 + (Cup + Cdn) / 2;
    C = (Cup - Cdn) / 2;
    third_modulation = db2mag(C0 + C * m);
else
    % square(1 + Cup) = 1 + A
    % square(1 + Cdn) = 1 - A
    Cup = sqrt(1 + A) - 1;
    Cdn = sqrt(1 - A) - 1;
    C0 = 1 + (Cup + Cdn) / 2;
    C = (Cup - Cdn) / 2;
    third_modulation = (C0 + C * m).^2;
end

ms = [1 + A * m second_modulation third_modulation];
ys = repmat(x, 1, 3) .* ms;
% Normalize power
amps = sqrt(mean(ys.^2));
amps = amps / amps(1);
ys = ys ./ repmat(amps, N, 1);
fys = abs(fft(ys .* repmat(gausswin(N), 1, 3)));
mfys = mag2db(fys);
R = 500:1500;
plot(R, mfys(R, 1), R+10, mfys(R, 2), R+20, mfys(R, 3));
legend('linear', 'power', 'dB')
