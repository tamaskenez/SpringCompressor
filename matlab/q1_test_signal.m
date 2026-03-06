f0 = 1000;
fs = 48000;
n = [0:4*fs-1]';
A0 = -60;
A1 = 0;
a = [A0*ones(1,fs) linspace(A0,A1,fs) A1*ones(1,fs) linspace(A1,A0,fs)]';
y = db2mag(a) .* sin(2*pi*f0/fs*n);
plot(mag2db(abs(y)));
audiowrite('test_signal_sine_1k_amp_60_0_db.wav', y, fs);
