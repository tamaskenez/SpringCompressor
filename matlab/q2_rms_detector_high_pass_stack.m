% Wide-band fast RMS detector using a filter bank

Q = 1;
beta=(1+sqrt(1+4*Q^2))/(2*Q);
bp_order = 1;
lp_order = 2;
lp_K = 1;
fs = 48000;
f0 = 20;
per_octave = 6;
% 2^(k/per_octave) < fs/(2*f0);
max_k = floor( per_octave * log2(fs/(2*f0)));
fcs = 2.^([0:max_k]'/per_octave) * f0;
while fcs(end) * beta >= fs / 2
    fcs = fcs(1:end - 1);
    max_k = max_k - 1;
end
bp_bs = zeros(2 * bp_order + 1, max_k + 1);
bp_as = zeros(2 * bp_order + 1, max_k + 1);
for k = 0:max_k
    [b, a] = butter(bp_order, [1/beta beta] * fcs(k+1)/fs*2, 'bandpass');
    bp_bs(:, k+1) = b';
    bp_as(:, k+1) = a';
end
test_fcs = 2.^([0:2*max_k]'/(2*per_octave)) * f0;
A = zeros(length(test_fcs), max_k + 1);
for k = 0:max_k
    h = abs(freqz(bp_bs(:,k+1), bp_as(:,k+1), test_fcs/fs*2*pi));
    A(:, k+1) = h;
end
B=ones(size(A,1),1);
X=A\B;
X2=A.^2\B;

G=sum(A(1:2:end,:)'.^2);
mean_G = mean(G);

lp_bs = zeros(lp_order + 1, max_k + 1);
lp_as = zeros(lp_order + 1, max_k + 1);
for k = 0:max_k
    [b, a] = butter(lp_order, fcs(k+1)/fs*2/lp_K);
    lp_bs(:, k+1) = b';
    lp_as(:, k+1) = a';
end

test_freqs = 320 * 2.^([0 1 2 3]/(4*per_octave));
N = fs;
ys = zeros(N, 4);
for i = 1:4
    test_f = test_freqs(i);
    x = cos(2*pi*test_f/fs*[0:N-1]');
    lp_bp_x = zeros(N, 1);
    for j = 1:max_k+1
        bp_x = filter(bp_bs(:,j), bp_as(:,j), x);
        lp_bp_x = lp_bp_x + filter(lp_bs(:,j), lp_as(:,j), bp_x.^2) / mean_G;
    end
    ys(:, i) = lp_bp_x;
end

figure(2);
plot(pow2db(ys) - pow2db(mean(x.^2)));

if 0
N = fs*4;
ns = [0:N-1]';
test_freqs = logspace(log10(fcs(1)),log10(fcs(end)),N)';
x = sin(cumsum(2*pi*test_freqs/fs));
test_f = test_freqs(i);
y = zeros(N, 1);
for j = 1:max_k+1
    bp_x = filter(bp_bs(:,j), bp_as(:,j), x);
    y = y + filter(lp_bs(:,j), lp_as(:,j), bp_x.^2) / mean_G;
end

figure(2);
plot(pow2db(y))
end

[x,fs] = audioread('bd.wav');
N=length(x);
y = zeros(N, 1);
mean_G = mean(G);
for j = 1:max_k+1
    bp_x = filter(bp_bs(:,j), bp_as(:,j), x);
    y = y + filter(lp_bs(:,j), lp_as(:,j), bp_x.^2) / mean_G;
end

figure(2);
m = 1 + 0.3 * ((abs(x)./sqrt(y) - 0.5));
plot(1:N,x,1:N, x ./ m);
