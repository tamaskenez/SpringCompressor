% try what happens if we don't compress just modulate

[x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
x=x(:,1);
lpf1_order = 1;
lpf1_freq = 10;
lpf2_order = 1;
lpf2_freq = 100;
intensity = -.5;
normalize = true;
th = 2;
[b1,a1] = butter(lpf1_order, lpf1_freq/fs*2);
[b2,a2] = butter(lpf2_order, lpf2_freq/fs*2);

rms = filter(b1,a1,x.^2);
m = x.^2 - rms;
if normalize
    m = m ./ rms;
end
m(isnan(m)) = 0;
m2 = filter(b2, a2, tanh(m * th));
y = x .* (1 + m2 * intensity);
p=audioplayer(y, fs);play(p);
