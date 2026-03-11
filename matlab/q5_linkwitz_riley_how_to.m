% Try the Linkwitz-Riley crossover such that the sum of the resulting bands power
% will be 1

fs = 48000;
fc = 100;

order = 2;
twice = 0; % looks like we need only one butterworth HPF and LPF at the same freq

[bl, al] = butter(order, fc/fs*2, 'low');
[bh, ah] = butter(order, fc/fs*2, 'high');

if twice
    bl = conv(bl, bl);
    al = conv(al, al);
    bh = conv(bh, bh);
    ah = conv(ah, ah);
end

% Verify resulting power at different frequencies.

frs = logspace(log10(fc/4), log10(fc*4), 101)';

mags = zeros(size(frs));

NF = length(frs);

fr = frs(i);
pws = abs(freqz(bl, al, frs/fs*2*pi)).^2 + abs(freqz(bh, ah, frs/fs*2*pi)).^2;
mags = sqrt(pws);

plot(frs, mags), grid;

% Construct a full tree filter bank
fs = 48000;
f0 = 12000;
frs = f0;
per_octave = 1;
beta = 2^(1/per_octave);
while 1
    next_fr = frs(end) / beta;
    if next_fr < 20, break; end
    frs = [frs; next_fr];
end
NX = length(frs);
clear xos;
for i = 1:NX
    [lp_b, lp_a] = butter(order, frs(i)/fs*2, 'low');
    [hp_b, hp_a] = butter(order, frs(i)/fs*2, 'high');
    xos(i).hp_b = hp_b;
    xos(i).hp_a = hp_a;
    xos(i).lp_b = lp_b;
    xos(i).lp_a = lp_a;
end

length_K = 17;
K = 0.5;
fi = floor(NX/2);
test_freqs = [frs(fi) sqrt(frs(fi) * frs(fi+1))];
x = [];
for test_freq = test_freqs
    l = round(fs/test_freq*length_K);
    x = [x; rectwin(l) .* sin(2*pi*test_freq/fs*[0:l-1]')];
end

% Process with the bank

from_prev_lpf=x;
lx = length(x);
pwr = zeros(size(x));
pwrs = zeros(lx, NX);
for i=1:length(xos)
    hx = filter(xos(i).hp_b,xos(i).hp_a,from_prev_lpf);
    [b,a] = critdamplp2(frs(i)/fs*2/K);
    % [b,a] = butter(2, frs(i)/fs*2/K);
    lphx = filter(b,a,hx.^2);
    pwrs(:, i) = lphx;
    pwr = pwr + lphx;
    from_prev_lpf = filter(xos(i).lp_b,xos(i).lp_a,from_prev_lpf);
end
plot(0:lx-1,x,0:lx-1,pow2db(pwr)), axis([0 lx -7 1])
