if false
    [x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
    x = x(30001:60000);
else
    [x, fs] = audioread('../AngelaThomasWade_MilkCowBlues.wav');
    x = x(1.0853e+05:1.3502e+05,1);
end

% x = [sin(2*pi*.01*(1:3000)'); db2mag(10) * sin(2*pi*.01*(1:3000)')];
if false
    B = 5000;
    x = [
        linspace(db2mag(-60),db2mag(0),B)';
        linspace(db2mag(0),db2mag(10),B)';
        linspace(db2mag(10),db2mag(30),B)';
        linspace(db2mag(30),db2mag(0),B)';
        linspace(db2mag(0),db2mag(-60),B)'
    ];
end

% [0:length(x)-1]

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

sw = gausswin(301);
sw = sw / sum(sw);


y1 = multiwindow_power_filter(x, 251, 1);
y2 = multiwindow_power_filter(x, 1101, 1);
% y2 = multiwindow_power_filter(x, 2001, 2);
% y2 = multiwindow_power_filter(x, 1601, 3);
% y2 = multiwindow_power_filter(x, 2101, 9);
% y2 = multiwindow_power_filter(x, 2001, 4);
% y2 = multiwindow_power_filter(x, 1101, 6);
% y1 = multiwindow_power_filter(x, 5001, 8);
% y2 = multiwindow_power_filter(x, 5001, 10);
ns = 0:NX-1;
[b, a] = critdamplp2(2/2101);
plot(...
    % ns, mag2db(abs(x)),'y',
    ns, pow2db(conv(pwr0, sw, 'same')),'g',
    ns, pow2db(y1),'r',
    ns, pow2db(y2),'m',
    % ns, pow2db(conv(x.^2, w, 'same')), 'b',
    ns, pow2db(filter(b, a, pwr0)), 'b',
    ns, pow2db(conv(pwr0, w, 'same')), 'b'
)
grid
