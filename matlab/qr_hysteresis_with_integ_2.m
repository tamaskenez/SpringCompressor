1;

% Affine map C0 -> 0, CM3 -> -3, everything from C0, on the opposite side of CM3 is also 0.
function y = affine_map_to_0_and_minus3(db, C0, CM3)
    if C0 < CM3
        saturated = db <= C0;
    else
        saturated = C0 <= db;
    end
    y = zeros(size(db));
    y(~saturated) = (db(~saturated) - C0) * (3 / (C0 - CM3));
endfunction

% Affine map C0 -> 0, CD -> D, everything from C0, on the opposite side of CD is also 0.
% D must be < 0
function y = saturated_affine_map(db, C0, CD, D)
    assert(D < 0);
    if C0 < CD
        saturated = db <= C0;
    else
        saturated = C0 <= db;
    end
    y = zeros(size(db));
    y(~saturated) = (db(~saturated) - C0) * (D / (CD - C0));
endfunction

function y = lerp(a, b, t)
    y = a + t .* (b - a);
endfunction

fc1 = 7;
fc2 = 200;
% df_integ and df_peak should be the same, probably they should be the same as fc1
fc_integ = 7;

if false
    [x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
    x = x(30001:60000);
else
    [x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
    x = x(1.0853e+05:1.3502e+05,1);
end

hx = hilbert(x) / sqrt(2);
hxx = abs(hx).^2;
NX = length(x);

df1 = dfilter.critdamplp(2, fc1 * 2 / fs);
df2 = dfilter.critdamplp(2, fc2 * 2 / fs);
df_integ = dfilter.critdamplp(2, fc_integ * 2 / fs);
df_peak = dfilter.critdamplp(2, fc_integ * 2 / fs);
df_peak2 = dfilter.critdamplp(2, fc_integ * 2 / fs);
dfu = dfilter.critdamplp(2, fc_integ * 2 / fs);
dfd = dfilter.critdamplp(2, fc_integ * 2 / fs);

y1 = df1.filter(hxx);
y2 = df2.filter(hxx);
ns = 0:NX-1;

y12_diff = y2 - y1;
y12_diff_integ = df_integ.filter(y12_diff);

% We construct the final output by starting from the difference signal:
% The difference signal should be slow-filtered with potential immediate peak-following, both positive and negative,
% regulated by the integrated diff signal:
% 1. Jump-to-peak is allowed only in the direction of the integrated diff signal.
% 2. The amount of the diff signal we can jump to is given as a function of the abs value of the integrated diff signal
%    with a function that is
%    f(0) = 0, df/dx >= 0, df/dx@0 = 0 or low, f(x) = 1 when x >= some limit

diff_integ_to_slow_db = pow2db(abs(y12_diff_integ) ./ y1);
diff_to_slow_db = pow2db(abs(y12_diff) ./ y1);
diff_on_same_side = sign(y12_diff) == sign(y12_diff_integ);
diff_to_slow_db(~diff_on_same_side) = -inf;
diff_to_slow_db(diff_to_slow_db <= diff_integ_to_slow_db + pow2db(2)) = -inf;
y_peak = zeros(size(x));
yu = zeros(size(x));
yd = zeros(size(x));
Ds = zeros(size(x));
y_peak2 = zeros(size(x));

for i = 2:NX
    y12d = y12_diff(i);
    y12di = y12_diff_integ(i);

    yu(i) = dfu.filter(y12d);
    yd(i) = dfd.filter(y12d);
    jump = false;
    if y12d > yu(i)
        yu(i) = y12d;
        dfu.override_output(y12d);
        jump = true;
    end
    if y12d < yd(i)
        yd(i) = y12d;
        dfd.override_output(y12d);
        jump = true;
    end

    M = (yu(i) + yd(i)) / 2;
    y_peak2(i) = df_peak2.filter(y12d + 2 * (M - y_peak2(i - 1)));
    if ~jump && pow2db(abs(M - y_peak2(i)) / y1(i)) > 7
        y_peak2(i) = M;
        df_peak2.override_output(M);
    end


    ypi = df_peak.filter(y12d);
    override = false;

    % Fix ypi first, it must be on the correct side, abs value not less then y12di.
    if y12di < 0
        if ypi > y12di
            ypi = y12di;
            override = true;
        end
    else
        if ypi < y12di
            ypi = y12di;
            override = true;
        end
    endif

    if ypi > 0
        if ypi < y12d / 2
            ypi = y12d / 2;
            override = true;
        end
    elseif ypi < 0
        if y12d / 2 < ypi
            ypi = y12d / 2;
            override = true;
        end
    end

    y_peak(i) = ypi;
    if override
        df_peak.override_output(ypi);
    end
end

% y_peak = lerp(y_peak, y12_diff_integ, db2pow(saturated_affine_map(diff_integ_to_slow_db, -8, -2, -12)));

w = blackman(55);
w = w / sum(w);
figure(1)
subplot(2,1,1);
plot(...
    ns, pow2db(conv(x.^2, w, 'same')),
    ns, pow2db(conv(hxx, w, 'same')),
    ns, pow2db(y1),
    ns, pow2db(y2),
    ns, pow2db(y1 + y12_diff_integ),
    ns, pow2db(max(0, y1 + y_peak)), '.'
)
grid
legend('raw pow', 'raw hxx', 'slow pow', 'fast pow', 'slow + diff-integ', 'slow + peak');
subplot(2,1,2);
plot(ns, diff_integ_to_slow_db, ns, diff_to_slow_db), grid, legend('integ to slow', 'diff to slow');


figure(2),clf
subplot(2,1,1);
plot(...
    ns, y1,
    ns, y2,
    ns, y1 + y_peak, '.'
)
grid
legend('slow pow', 'fast pow', 'slow + peak');
subplot(2,1,2)
plot(
    ns, y12_diff_integ,
    ns, y_peak,
    ns, y12_diff,
    ns, (yu + yd) / 2,
    ns, y_peak2, '.'
), grid, legend('diff_integ', 'peak', 'diff', '(yu + yd)/2', 'y_peak2')

assert(all(y1 + y_peak >= 0));
