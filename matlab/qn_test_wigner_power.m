fs = 10000;
f1 = 1000;
S = 1000;
x = [zeros(S, 1); exp(j*2*pi*f1/fs*[0:S-1]'); zeros(S, 1)];
d=wigner_power(x,blackman(101),blackman(1001));
subplot(2,1,1);plot(real(d));
subplot(2,1,2);plot(imag(d));
