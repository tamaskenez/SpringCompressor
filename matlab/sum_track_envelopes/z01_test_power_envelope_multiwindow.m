[x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
x = x(30001:60000);

% x = [sin(2*pi*.01*(1:3000)'); db2mag(10) * sin(2*pi*.01*(1:3000)')];
[0:length(x)-1]

hx = hilbert(x)/sqrt(2);

if false
    R = 2500:13000;
    x = x(R);
    hx = hx(R);
end

NX = length(x);

pwr0 = abs(hx).^2;

w = gausswin(1001);
w = w / sum(w);

y1 = multiwindow_power_filter(x, 1101, 1);
% y2 = multiwindow_power_filter(x, 2001, 2);
% y2 = multiwindow_power_filter(x, 1601, 3);
y2 = multiwindow_power_filter(x, 1101, 5);
% y2 = multiwindow_power_filter(x, 2001, 4);
% y2 = multiwindow_power_filter(x, 1101, 6);
ns = 0:NX-1;
plot(...
    ns, mag2db(abs(x)),'y', ...
    ns, pow2db(pwr0),'y', ...
    ns, pow2db(y1),'r',
    ns, pow2db(y2),'m',
    ns, pow2db(conv(x.^2, w, 'same')), 'b',
    ns, pow2db(conv(pwr0, w, 'same')), 'b'
)
