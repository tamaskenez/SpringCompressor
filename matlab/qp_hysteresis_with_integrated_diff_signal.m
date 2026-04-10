1;

% C is the integ_diff/slow_filter ratio (in dB) which saturates tau to 1. C = 0 dB is usually too much (need a very high
% integ_diff level to saturate), -3, -6 is more reasonable.
% Q is the amplifier (0 < Q), 1 dB increase in the integ_diff/slow_filter ratio results in Q dB increase in tau. So the
% higher the more sensitive. Lower ratios will contribute more if Q is higher. Q needs to be lower to suppress
% ripples when integ_diff is low. Good values are around 0.5
function y = integ_diff_db_to_tau(db, Q, C)
    y = db2pow(min(0, (db - C) / Q));
endfunction

% Shift a value [-inf, C] to [-inf, 0] and scales it with F
function y = shift_and_scale_below(db, C, F)
    y = min(0, (db - C) * F);
endfunction

% Shift a value [C, inf] to [0, inf] and scales it with F
function y = shift_and_scale_above(db, C, F)
    y = max(0, (db - C) * F);
endfunction

fc1 = 7;
fc2 = 200;
fc_peak = 7;

if false
    [x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
    x = x(30001:60000);
else
    [x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
    x = x(1.0853e+05:1.3502e+05,1);
end

hx = hilbert(x)/sqrt(2);
hxx = abs(hx).^2;

if false
    amp_db = 2;
    level1_db = 5;
    level2_db = 20;
    peak1 = db2mag(amp_db + level1_db);
    through1 = db2mag(-amp_db + level1_db);
    peak2 = db2mag(amp_db + level2_db);
    through2 = db2mag(-amp_db + level2_db);
    x = [
        (peak1 + through1) / 2 + (peak1 - through1) / 2 * sin(2*pi*50/fs*[0:fs-1]');
        (peak2 + through2) / 2 + (peak2 - through2) / 2 * sin(2*pi*50/fs*[0:fs-1]')
    ];
    hx = x;
end

NX = length(x);
hxx = abs(hx).^2;

[b1, a1] = critdamplp(2, fc1 * 2 / fs);
[b2, a2] = critdamplp(2, fc2 * 2 / fs);
[b_integ, a_integ] = butter(1, fc1 * 2 / fs);

y1 = filter(b1, a1, hxx);
y2 = filter(b2, a2, hxx);
ns = 0:NX-1;

y12_diff = y2 - y1;
y12_diff_integ = filter(b_integ, a_integ, y12_diff);

% We construct the final output by starting from the difference signal:
% The difference signal should be slow-filtered with potential immediate peak-following, both positive and negative,
% regulated by the integrated diff signal:
% 1. Jump-to-peak is allowed only in the direction of the integrated diff signal.
% 2. The amount of the diff signal we can jump to is given as a function of the abs value of the integrated diff signal
%    with a function that is
%    f(0) = 0, df/dx >= 0, df/dx@0 = 0 or low, f(x) = 1 when x >= some limit

diff_integ_to_slow_db = pow2db(abs(y12_diff_integ) ./ y1);

taus = integ_diff_db_to_tau(diff_integ_to_slow_db, 0.125, -3);


if false
    % peak filter is order 1

    g_peak = tptlp1(fc_peak * 2 / fs);
    s_peak = [0];

    y_peak = zeros(size(x));
    Ds = zeros(size(x));
    M = 10;
    for i = 2:NX
        [y_peak(i), s_peak] = filter_tpt1(g_peak, 1 * y12_diff(i), s_peak);
        jdv_prev = taus(i-1) * y12_diff(i-1) + (1 - taus(i-1)) * y_peak(i-1);
        jdv = taus(i) * y12_diff(i) + (1 - taus(i)) * y_peak(i);
        d_jdv_per_d_sample = jdv - jdv_prev;
        if y12_diff_integ(i) > 0
            % See if y12_diff would pull it up.
            if y_peak(i) < jdv
                y_peak(i) = jdv;
                s_peak = jdv;
            elseif jdv < y_peak(i) && 0 < d_jdv_per_d_sample
                % jdv is going up, when does it meet y_peak(i)?
                meet_in = (y_peak(i) - jdv) / d_jdv_per_d_sample;
                assert(meet_in > 0);
                % We'd like to have d_y_peak_per_d_sample to reach d_jdv_per_d_sample in meet_in samples
                D = min(d_jdv_per_d_sample * exp(-meet_in / M), y_peak(i) - jdv);
                y_peak(i) = y_peak(i) + D;
                s_peak = s_peak + D;
                Ds(i) = D;
            end
        elseif y12_diff_integ(i) < 0
            % See if y12_diff would pull it down.
            if jdv < y_peak(i)
                y_peak(i) = jdv;
                s_peak = jdv;
            elseif y_peak(i) < jdv && d_jdv_per_d_sample < 0
                % jdv is going down, when does it meet y_peak(i)?
                meet_in = (y_peak(i) - jdv) / d_jdv_per_d_sample;
                assert(meet_in > 0);
                % We'd like to have d_y_peak_per_d_sample to reach d_jdv_per_d_sample in meet_in samples
                D = max(d_jdv_per_d_sample * exp(-meet_in / M), y_peak(i) - jdv);
                y_peak(i) = y_peak(i) + D;
                s_peak = s_peak + D;
                Ds(i) = D;
            end
        end
    end
elseif false
    % peak filter is order 2

    ghr_peak = statevartpt2(fc_peak * 2 / fs, 0.5);
    s_peak = [0 0];

    y_peak = zeros(size(x));
    Ds = zeros(size(x));
    M = 10;
    for i = 2:NX
        [y_peak(i), s_peak] = filter_statevartpt2(ghr_peak, 0 * y12_diff(i), s_peak);
        jdv_prev = taus(i-1) * y12_diff(i-1) + (1 - taus(i-1)) * y_peak(i-1);
        jdv = taus(i) * y12_diff(i) + (1 - taus(i)) * y_peak(i);
        d_jdv_per_d_sample = jdv - jdv_prev;
        if y12_diff_integ(i) > 0
            % See if y12_diff would pull it up.
            if y_peak(i) < jdv
                y_peak(i) = jdv;
                s_peak = [0 jdv];
            elseif jdv < y_peak(i) && 0 < d_jdv_per_d_sample
                % jdv is going up, when does it meet y_peak(i)?
                meet_in = (y_peak(i) - jdv) / d_jdv_per_d_sample;
                assert(meet_in > 0);
                % We'd like to have d_y_peak_per_d_sample to reach d_jdv_per_d_sample in meet_in samples
                D = min(d_jdv_per_d_sample * exp(-meet_in / M), y_peak(i) - jdv);
                y_peak(i) = y_peak(i) + D;
                s_peak = [s_peak(1) s_peak(2) + D];
                Ds(i) = D;
            end
        elseif y12_diff_integ(i) < 0
            % See if y12_diff would pull it down.
            if jdv < y_peak(i)
                y_peak(i) = jdv;
                s_peak = [0 jdv];
            elseif y_peak(i) < jdv && d_jdv_per_d_sample < 0
                % jdv is going down, when does it meet y_peak(i)?
                meet_in = (y_peak(i) - jdv) / d_jdv_per_d_sample;
                assert(meet_in > 0);
                % We'd like to have d_y_peak_per_d_sample to reach d_jdv_per_d_sample in meet_in samples
                D = max(d_jdv_per_d_sample * exp(-meet_in / M), y_peak(i) - jdv);
                y_peak(i) = y_peak(i) + D;
                s_peak = [s_peak(1) s_peak(2) + D];
                Ds(i) = D;
            end
        end
    end
else
    % We're creating y_peak as the estimate for the difference between y1 and which is as smooth
    % as y1 but as quick as y2.
    % It is calculated from 2 sources:
    % - it evolves just like LP(y12_diff) by default
    % - depending on a 2D condition map, we might need to pull it to y12_diff or to y12_diff_integ:
    %
    %                                                   y1_diff_integ to y1
    %                         negligible      low           mid          high (0 dB)
    % y12_diff to y1
    %       negligible       pull to integ
    %       low              pull to integ             pull to integ
    %       mid              pull to integ                 LP              LP          pull to
    %       high             pull to integ  pull to diff    pull to diff    pull to diff
    %
    % When y1_diff_integ to y1
    % - negligible: pull to integ
    % - low (kicks in around -6 dB): if y12/2 is higher then y_peak and maybe y12_diff/y1 is at least around 0 dB,
    %   then pull, otherwise filter
    % - from mid: if y12/2 is higher then pull, otherwise filter

    % peak filter is order 1, go up only to half diff

    g_peak = tptlp1(fc_peak * 2 / fs);
    s_peak = [0];

    y_peak = zeros(size(x));
    for i = 2:NX
        y12d = y12_diff(i);
        [y_peak(i), s_peak] = filter_tpt1(g_peak, y12d, s_peak);
        ypi = y_peak(i);
        if y12_diff_integ(i) > 0
            % See if y12_diff would pull it up.
            % We're aiming for 0.5 * y12d
            if 0 < ypi && ypi < y12d / 2
                D = y12d / 2 - ypi;
                y_peak(i) = y_peak(i) + D;
                s_peak = s_peak + D;
            end
        elseif y12_diff_integ(i) < 0
            % See if y12_diff would pull it down.
            if y12d / 2 < ypi && ypi < 0
                D = y12d / 2 - ypi;
                y_peak(i) = y_peak(i) + D;
                s_peak = s_peak + D;
            end
        end
        if y12_diff_integ(i) < 0
            if y_peak(i) > y12_diff_integ(i)
                y_peak(i) = y12_diff_integ(i);
                s_peak = y12_diff_integ(i);
            end
        else
            if y_peak(i) < y12_diff_integ(i)
                y_peak(i) = y12_diff_integ(i);
                s_peak = y12_diff_integ(i);
            end
        endif
        % Create a dB value from y12_diff_integ:
        db_in = pow2db(abs(y12_diff_integ(i)) / y1(i));
        C = -20;
        % db_in < C -> 0
        % C < db_in -> decreases
        db_out = -shift_and_scale_above(db_in, C, 2);
        tau = db2pow(db_out);
        Ds(i) = tau;
        D = tau * y12_diff_integ(i) + (1 - tau) * y_peak(i) - y_peak(i);
        y_peak(i) = y_peak(i) + D;
        s_peak = s_peak + D;
    end
end

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
plot(ns, taus), grid, legend('taus')


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
    ns, y12_diff
), grid, legend('diff_integ', 'peak', 'diff')

assert(all(y1 + y_peak >= 0));