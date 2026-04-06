figure(3);
fs = 48000;
wl = round(fs/20/2) * 2 + 1;
hwl = (wl - 1) / 2;

iir = false;
if false
    [windows, lambdas] = dpss(wl, 4, 2);
elseif false
    gw = gausswin(wl);
    windows = [gw gw .* linspace(-1, 1, wl)'];
else
    [b1, a1] = critdamplp2(13.7*2/fs);
    [b2, a2] = critdamplp2(30*2/fs);
    x = [1; zeros(fs - 1, 1)];
    fx1 = filter(b1, a1, x);
    fx2 = filter(b2, a2, x);
    windows = [fx1 fx1 - fx2];
    iir = true;
end

windows = windows / sum(windows(:, 1));

if iir
    fws = fft(windows);
else
    fws = fft(shift_centered_window_for_fft(windows, fs));
end

fws = fws ./ max(abs(fws));

subplot(2,1,1);
F = round(fs / wl) * 10;
a = repmat((0:F)', 1, size(fws, 2));
subplot(2,1,1);
plot(a, mag2db(abs(fws(1:F+1, :)))), axis([0 F -50 0]), grid;
subplot(2,1,2);
plot(a, rad2deg(angle(fws(1:F+1, :))));
