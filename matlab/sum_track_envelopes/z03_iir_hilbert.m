[H1b, H1a, H2b, H2a] = hiir();
fs = 48000;
x = [1; zeros(fs-1,1)];
y1 = filter(H1b, H1a, x);
y2 = filter(H2b, H2a, x);
fy1 = fft(y1);
fy2 = fft(y2);
hzs = [0:50];
plot(hzs, rad2deg(angle(fy2(hzs+2) ./ fy1(hzs+1)))), grid;


