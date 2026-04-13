% Test filter overrides strategies for asymmetric envelope follower
% 1. TPT2, no update
% 2. TPT2, zero derivative
% 3. TPT2, update derivative (rescale with g)
% 4. 2 x TPT1 no update


fcs = 20;
fcf = 500;
fs = 4800;

NX = round(fs/30);
ns = (0:NX-1)';
x = -cos(2*pi*100/fs*ns) + 1;

df_tpt2_slow = dfilter.critdamplp(2, fcs * 2 / fs, 'tpt');
df_tpt2_fast = dfilter.critdamplp(2, fcf * 2 / fs, 'tpt');

df_tpt1_slow = dfilter.critdamplp(1, fcs * 2 / fs, 'tpt');
df_tpt1_fast = dfilter.critdamplp(1, fcf * 2 / fs, 'tpt');

y1 = zeros(size(x));
for i = 3:NX
    os = df_tpt2_slow.filter(x(i));
    of = df_tpt2_fast.filter(x(i));
    if of > os
        y1(i) = of;
        df_tpt2_slow.set_state(df_tpt2_fast.get_state());
    else
        y1(i) = os;
        df_tpt2_fast.set_state(df_tpt2_slow.get_state());
    end
end

acd = AsymmetricCritDampLPF(2, fcf * 2 / fs, 2, fcs * 2 / fs);
y2 = acd.filter(x);

df_tpt2_slow.reset();
df_tpt2_fast.reset();
y3 = zeros(size(x));
g_slow = df_tpt2_slow.get_g();
g_fast = df_tpt2_fast.get_g();
for i = 3:NX
    os = df_tpt2_slow.filter(x(i));
    of = df_tpt2_fast.filter(x(i));
    if of > os
        y3(i) = of;
        s = df_tpt2_fast.get_state();
        df_tpt2_slow.set_state([s(1) / g_fast * g_slow s(2)]);
    else
        y3(i) = os;
        s = df_tpt2_slow.get_state();
        df_tpt2_fast.set_state([s(1) / g_slow * g_fast s(2)]);
    end
end

g_slow = tptlp1(1.5 * fcs * 2 / fs);
g_fast = tptlp1(1.5 * fcf * 2 / fs);

s = [0 0];

y4 = zeros(size(x));
for i = 3:NX

    [os1, s1_slow] = filter_tpt1(g_slow, x(i), s(1));
    [os2, s2_slow] = filter_tpt1(g_slow, os1, s(2));
    [of1, s1_fast] = filter_tpt1(g_fast, x(i), s(1));
    [of2, s2_fast] = filter_tpt1(g_fast, of1, s(2));

    if of2 > os2
        y4(i) = of2;
        s = [s1_fast s2_fast];
    else
        y4(i) = os2;
        s = [s1_slow s2_slow];
    end
end

plot(
    ns, x,
    ns, y1,
    ns, y2,
    ns, y3,
    ns, y4
), grid, legend('x', 'tpt no-upd', 'tpt zero s(1)', 'tpt update s(1)', '2 x tpt1');
