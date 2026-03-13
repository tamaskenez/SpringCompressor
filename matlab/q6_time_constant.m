% Figure out attack/release time constant definition.attack

fs = 48000;
N = 5000;
tc_sec = 0.01;
tc_samples = fs * tc_sec
fc_hz = 1/(2*pi*tc_sec);
fc_hps = fc_hz/fs*2;
threshold_db = -10;
threshold = db2mag(threshold_db);

compressed_one = db2mag(-threshold_db / ratio + threshold_db);

x = [threshold * ones(N, 1); ones(N,1); threshold * ones(N,1)];
y0 = [threshold * ones(N, 1); compressed_one * ones(N,1); threshold * ones(N,1)];
[b1,a1] = butter(1, fc_hps);
[b2,a2] = butter(2, fc_hps);
[b3,a3] = critdamplp2(fc_hps);
ratio = 4;
ns = [0:length(x)-1]';
y1=filter(b1,a1,x);
y2=filter(b2,a2,x);
y3=filter(b3,a3,x);

figure(1)

subplot(2,1,1);
plot(ns, y1, ns, y2, ns, y3), grid;
legend('b1','b2','crit');
subplot(2,1,2);
plot(ns, mag2db(y1), ns, mag2db(y2), ns, mag2db(y3)), grid;
legend('b1','b2','crit');

function [y, gain_db] = process(x, b, a, method, threshold_db, ratio)
    if strcmp(method, "input_amp")
        level_db = mag2db(filter(b, a, abs(x)));
    elseif strcmp(method, "input_pow")
        level_db = pow2db(filter(b, a, x.^2));
    elseif strcmp(method, "gr_db") || strcmp(method, "gr_mag")
        level_db = mag2db(abs(x));
    else
        assert(0);
    end
    above_threshold = level_db > threshold_db;
    gain_db = zeros(size(x));
    gain_db(above_threshold) = (level_db(above_threshold) - threshold_db) / ratio + threshold_db - level_db(above_threshold);
    if strcmp(method, "gr_db")
        gain_db = filter(b, a, gain_db);
    elseif strcmp(method, "gr_mag")
        gain_db = mag2db(1 - filter(b, a, 1 - db2mag(gain_db)));
    end
    y = x .* db2mag(gain_db);
end

figure(2), clf;

methods = {"input_amp", "input_pow", "gr_db", "gr_mag"};
NM = length(methods);
ys = []; gr_dbs = [];
fcs = [0.394 0.22 0.695 1] * fc_hps;
for i=1:NM
    fc = fcs(i);
    [b1,a1] = butter(1, fc);
    [b2,a2] = butter(2, fc);
    [b3,a3] = critdamplp2(fc);
    [y, gr_db] = process(x, b3, a3, methods{i}, threshold_db, ratio);
    ys = [ys y];
    gr_dbs = [gr_dbs gr_db];
end
subplot(2,1,1);
R=[5000:7000]';
plot(repmat(R, 1, NM), (ys(R,:)-repmat(y0(R),1,NM)).^2), grid, legend(methods);
subplot(2,1,2);
plot(repmat(ns, 1, NM), gr_dbs), hold on;
set(gca, 'ColorOrderIndex', 1);
plot(repmat(ns, 1, NM), mag2db(ys)), hold off;
grid, legend(methods);
sum(ys(1:2*N,:).^2)