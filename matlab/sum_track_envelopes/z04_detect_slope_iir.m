fs = 48000;
fc1 = 20;
fc2 = 80;
slope_hz = 40;
slope_per_sec = 2 * pi * slope_hz;
slope_per_sample = slope_per_sec / fs;
slope_duration = 0.5;
x = [0:slope_duration*fs-1]' * slope_per_sample;
x = [x; x(end) * ones(slope_duration*fs, 1); flipud(x); zeros(size(x))];

[H1b, H1a, H2b, H2a] = hiir();
[b1, a1] = butter(1, fc1*2/fs);
[b2, a2] = butter(1, fc2*2/fs);

b1 = conv(b1, H2b);
a1 = conv(a1, H2a);
b2 = conv(b2, H1b);
a2 = conv(a2, H1a);

D = 0;

x1 = filter(b1, a1, x);
x2 = filter(b2, a2, [zeros(D,1); x(1:end-D)]);
d = x2 - x1;

ns = [0:length(x)-1]';
%plot(ns, d);

slope_hzs = 5*2.^(0:0.1:7);
slope_hzs = [20 40 80];
NS = length(slope_hzs);
i1 = round(slope_duration/2*fs);
i2 = round(5*slope_duration/2*fs);
x2_to_x1 = zeros(2 * length(slope_hzs), 1);
x_to_x1 = zeros(2 * length(slope_hzs), 1);
figure(1), clf;
for i = 1:NS
    slope_per_sample = 2 * pi * slope_hzs(i) / fs;
    x = [0:slope_duration*fs-1]' * slope_per_sample;
    x = [x; x(end) * ones(slope_duration*fs, 1); flipud(x); zeros(size(x))];
    x1 = filter(b1, a1, x);
    x2 = filter(b2, a2, [zeros(D,1); x(1:end-D)]);
    d = x2 - x1;
    subplot(2,1,1);
    hold on, plot(ns, d), hold off
    subplot(2,1,2);
    hold on, plot(ns, x1,ns,x2,ns,x,'.'), hold off
    x2_to_x1(2*i) = d(i1);
    x_to_x1(2*i) = x(i1)-x1(i1);
    x2_to_x1(2*i+1) = d(i2);
    x_to_x1(2*i+1) = x(i2)-x1(i2);
end
figure(2);
plot(x2_to_x1./x_to_x1, '.');