fs = 48000;
fc = 100;
order1 = 1;
order2 = 1;

if order1 == 1
    [b1, a1] = butter(1, fc*2/fs);
else
    assert(order1 == 2);
    [b1, a1] = critdamplp2(fc*2/fs);
end

N = round(32*fs/fc);
ns = [0:N-1]';

fa = fc/4;
K = 10;

function ym = lpf_peakthrough_mean(f1, f2, x, filter_x)
    [b1, a1] = butter(1, f1);
    [b2, a2] = butter(1, f2);
    N = length(x);
    if filter_x
        fx = filter(b1, a1, x);
    else
        fx = x;
    end

    y_ub = zeros(N, 1);
    y_lb = zeros(N, 1);

    for i = 2:N
        if fx(i) > y_ub(i-1)
            y_ub(i) = fx(i);
        else
            y_ub(i) = b2(1) * fx(i) + b2(2) * fx(i-1) - a2(2) * y_ub(i-1);
        end
        if fx(i) < y_lb(i-1)
            y_lb(i) = fx(i);
        else
            y_lb(i) = b2(1) * fx(i) + b2(2) * fx(i-1) - a2(2) * y_lb(i-1);
        end
    end

    ym = (y_ub + y_lb) / 2;
endfunction

%for fa = [1/8 1/4 1/2 1 2 4 8] * fc
for K = [1 2 4 8 16]
    x = sin(2*pi*fa/fs*ns);
    x(N/2:end) = x(N/2:end) * 2;
    fx = filter(b1, a1, x.^2);

    ym1 = lpf_peakthrough_mean(fc*2/fs,fc*2/fs/K, x.^2, true);
    ym2 = lpf_peakthrough_mean(fc*2/fs,fc*2/fs/K, ym1, false);
    ym3 = lpf_peakthrough_mean(fc*2/fs,fc*2/fs/K, ym2, false);

    [b,a] = critdamplp2(fc/8*2/fs);
    fx = filter(b,a,x.^2);
    plot(ns, pow2db(x.^2), 'k', ns, pow2db(ym1), 'r', ns, pow2db(ym3), 'r', ns, pow2db(fx));
    a = axis;
    a(4) = max(pow2db(x.^2));
    a(3) = a(4) - 40;
    axis(a), grid;
    pause
end
