% teager-kaiser energy operator
	
	N = 48000;
fs = 48000;

% sweep
	min_hz = 20;
max_hz = 16000;
hzs = logspace(log10(min_hz),log10(max_hz),N);
dangles = 2*pi*hzs/fs;
x = sin(cumsum(dangles));

orig_spec = abs(fft(x));
psi = x(2:end-1).^2 - x(1:end-2).*x(3:end);
plot(psi./((2*pi*hzs(2:end-1)/fs).^2));
