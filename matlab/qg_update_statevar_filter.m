% Perform a cutoff freq sweep wit low pss

fs = 48000;

N = fs;
ns = [0:fs-1]';
s = [0 0];
fsignal = 500;
fm = 4;
frs = fsignal * 2.^(sin(2*pi*fm/fs*ns));

update_hz = 10;
update_n = round(fs / update_hz);

x = sin(2*pi*fsignal/fs*ns - 1.2);
% x = ns * 0.01;
y = zeros(N, 1);
for i = 0:N-1
    if i == 0
        ghr2 = statevartpt2(frs(i+1)/fs*2, 0.8);
    elseif mod(i, update_n) == 0
        g = ghr2(1);
        h = ghr2(2);
        R2 = ghr2(3);
        yLP_from_s = s(2) - g * s(1);
        xi_from_s = s(2) - (g - R2) * s(1);
        D_from_s = 2 * g * s(1);

        ghr2_before = ghr2;
        ghr2 = statevartpt2(frs(i+1)/fs*2, 0.8);
        s_new_steady = convert_statevartpt2_state(ghr2_before, ghr2, s);

        g = ghr2(1);
        h = ghr2(2);
        R2 = ghr2(3);

        s2_new = (g^2 * h * xi_from_s + (g - g^2 * h * (R2 + g)) * s(1) + (g^2 * h - 1) * D_from_s - yLP_from_s) / (g^2 * h - 1);

        s(1) = s_new_steady(1);
        s = s_new_steady;
        s(2) = s(2);
    end
    [yi, s] = filter_statevartpt2(ghr2, x(i+1), s);
    y(i+1) = yi;
    % printf("[%d]: s1: %f\n", i, s(1));
    if i > 10000, break; end
end

plot(y(4780:4920), 's-');
dy = diff(y(1:10000));
plot(dy(4780:4920), 's-');