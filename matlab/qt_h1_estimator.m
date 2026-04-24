% System identification: we have two, time domain signals, the input and output of a linear system.
% What's the transfer function?

fs = 48000;

% The linear system
fc = 40;

% input signal
f1 = 10;
f2 = 1000;
N = fs;
x = zeros(N, 1);
frs = zeros(N, 1);
for i = 0:N-1
    tau = 1 - abs(i/N * 2 - 1);
    frs(i + 1) = exp(log(f1) + tau * (log(f2) - log(f1)));
end
a = 0;
for i = 0:N-1
    x(i + 1) = cos(a);
    a = a + 2 * pi * frs(i + 1) / fs;
end

[b, a] = butter(2, 40*2/fs);
y = filter(b, a, [x;x;x]);
delay = 1000;
y = y(N+1-delay:2*N-delay);
y = y + db2mag(-20) * randn(size(y));

figure(1);
H = fft(y) ./ fft(x);
subplot(2,1,1);semilogx(1:N/2-1, mag2db(abs(H(2:N/2)))), grid
w = blackman(15);
w = w / sum(w);
H = cconv(H, w);
ah = unwrap(angle(H));
dah = [0; (ah(3:N) - ah(1:N-2))/2; 0];
subplot(2,1,2);semilogx(1:N/2-1, dah(2:N/2)/(2*pi)*fs), axis([1 N/2-1 -2000 0]); grid


