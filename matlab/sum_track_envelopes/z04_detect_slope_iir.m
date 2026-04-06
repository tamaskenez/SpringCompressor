fs = 48000;
fc1 = 13.7;
fc2 = 30;
prime_sec = 0.25;
slope_db_per_sec = 2400;
slope_duration = 0.022;
x = [
    db2pow(0) * ones(prime_sec * fs, 1);
    db2pow(linspace(0, slope_duration * slope_db_per_sec, slope_duration * fs)');
    db2pow(slope_duration * slope_db_per_sec) * ones(prime_sec * fs, 1);
    db2pow(linspace(slope_duration * slope_db_per_sec, 0, slope_duration * fs)');
    db2pow(0) * ones(prime_sec * fs, 1);
];


if false
    [b1, a1] = butter(2, fc1*2/fs);
    [b2, a2] = butter(2, fc2*2/fs);
else
    [b1, a1] = critdamplp2(fc1*2/fs);
    [b2, a2] = critdamplp2(fc2*2/fs);
end

D = 0;

x1 = filter(b1, a1, x);
x2 = filter(b2, a2, [zeros(D,1); x(1:end-D)]);
d = x2 - x1;
d1 = filter(b1, a1, d);
d2 = filter(b2, a2, d);

ns = [0:length(x)-1]';
plot(d./x1, x1./x, '.')
%plot(ns, d);

D=0;
R=5000:35000;
subplot(2,1,1);
plot(R, x1(R), R, d(R), R, d1(R), R, d2(R)), grid, legend('x1', 'd', 'd1', 'd2');
subplot(2,1,2);
plot(R, d(R)./x1(R), R, pow2db(x1(R + D) ./ x(R + D)), '.'), grid, legend('d/x1', 'x1/x');


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