

fs = 48000;
fc = 200;
[bl, al] = butter(1, 0.2);
[bh, ah] = butter(1, 0.2, 'high');

% bl = conv(bl,bl);
% al = conv(al,al);
% bh = conv(bh,bh);
% ah = conv(ah,ah);

figure(1);
freqz(bl, al);
figure(2);
freqz(bh, ah);

figure(3),clf
[hl, wl] = freqz(bl, al);
[hh, wh] = freqz(bh, ah);
plot(mag2db(abs(hl+hh)))