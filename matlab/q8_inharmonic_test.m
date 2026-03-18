% Measure harmonic and inharmonic distortion.
% Testing whether T = T0 + 0.5 is fine or we need a longer, coherent sampling.


% M is the entire FFT
% which contains k periods of a signal with period = M / k

M = 97;
k = 13;
T = M/k;

assert(gcd(M,k)==1);

h_ixs = harmonic_bins_base0(k, M, true) + 1;
harmonic_and_dc_mask = false(floor(M/2)+1, 1);
harmonic_and_dc_mask(1) = true;
harmonic_and_dc_mask(h_ixs) = true;
inh_ixs = find(~harmonic_and_dc_mask);

ns = [0:M-1]';
x=cos(2*pi*ns/T);

% Distortion
if 1
    Q = 0;
    if Q > 0
        xd = tanh(x*Q)/Q;
    end
else
    xd = max(x,0);
end
plot(ns,x,ns,xd);

fx = abs(fft(x));
fxd = abs(fft(xd));

F = fxd(1:floor(M/2)+1)/M/sqrt(2);
bins_to_double = 2:ceil(M/2);
F(bins_to_double) = 2 * F(bins_to_double);
printf("DC: %f\n", F(1)^2);
sum_f0 = F(k+1)^2;
printf("F0: %f\n", sum_f0);
sum_hs = sum(F(h_ixs).^2);
printf("hs: %f\n", sum_hs);
sum_ihs = sum(F(inh_ixs).^2);
printf("ihs: %f\n", sum_ihs);
printf("THD: %f dB\n", pow2db((sum_hs - sum_f0) / sum_f0));
printf("TIHD: %f dB\n", pow2db(sum_ihs / sum_f0));
