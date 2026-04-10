fc = 7;
fs = 4800;
tau = 0.995;

N = round(fs/30);
x = zeros(N, 1);
y1 = zeros(N, 1);
y2 = zeros(N, 1);
ns = (0:N-1)';
x = cos(2*pi*70/fs*ns) - 1;
x(1:round(0.5*fs/70)) = -2;
df1 = dfilter.critdamplp(2, fc*2/fs, 'tpt');
g_h_r2 = statevartpt2(fc*2/fs, 0.5);
s = [0 0];
for i = 1:N
    y1(i) = df1.filter(-1);
    if y1(i) < x(i)
        y1(i) = x(i);
        df1.override_output(x(i));
    end

    [y2(i), s] = filter_statevartpt2(g_h_r2, -1, s);
    if y2(i) < x(i)
        y2(i) = x(i);
        s = [0 x(i)];
    end
end

plot(ns, y1, 'x', ns, y2, 's', ns, x), grid, legend('y1', 'y2', 'x');
axis([0 N-1 -0.5 0.5]);
