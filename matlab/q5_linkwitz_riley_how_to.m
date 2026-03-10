fs = 48000;
fc = 100;

order = 2;
twice = 0;

[bl, al] = butter(order, fc/fs*2, 'low');
[bh, ah] = butter(order, fc/fs*2, 'high');

if twice
    bl = conv(bl, bl);
    al = conv(al, al);
    bh = conv(bh, bh);
    ah = conv(ah, ah);
end

frs = logspace(log10(fc/4), log10(fc*4), 101)';

mags = zeros(size(frs));

NF = length(frs);

fr = frs(i);
pws = abs(freqz(bl, al, frs/fs*2*pi)).^2 + abs(freqz(bh, ah, frs/fs*2*pi)).^2;
mags = sqrt(pws);

plot(frs, mags), grid;
