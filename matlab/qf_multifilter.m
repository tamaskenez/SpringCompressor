% Use multiple filters to tell apart transients and interference ripple.

figure(1)

[x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
x = 10 * x(1.0853e+05:1.3502e+05,1);

% x = (sin(2*pi*100/fs*[0:fs-1]') + sin(2*pi*150/fs*[0:fs-1]'));

N = length(x);
ns = [0:N-1]';

ms = (ns)/fs*1000;
release_hz = 7;
attack_hz = 80;
frs = [release_hz attack_hz];
NF = length(frs);
phx = abs(hilbert(x)/sqrt(2)).^2;
clf,grid, hold on;
plot(ms, pow2db(phx), 'y');
rs = zeros(length(phx), NF);
for i = 1:NF
    [b,a] = critdamplp2(frs(i)/fs*2);
    r1 = filter(b, a, phx);
    plot(ms, pow2db(r1), 'k');
    rs(:, i) = r1;
end
hold off

ghr2_attack = statevartpt2(attack_hz*2/fs, 0.5);
ghr2_release = statevartpt2(release_hz*2/fs, 0.5);

[b_attack, a_attack] = butter(1, attack_hz*2/fs);
[b_release, a_release] = butter(1, release_hz*2/fs);

y_ub = zeros(size(phx)); % Upper bound.
y_lb = zeros(size(phx)); % Lower bound.
s_ub = [0 0]; % Filter state.
s_lb = [0 0];
ar_ub = 2; % Current attack/release selector (which time constant is currently used)
ar_lb = 2;

o1 = y_ub;
o2 = y_ub;
s12_o1 = [0 0];
s12_o2 = [0 0];

function y = filter_1storder(b, a, x, x_prev, y_prev)
    assert(a(1) == 1);
    y = b(1) * x + b(2) * x_prev - a(2) * y_prev;
end

for i = 2:length(phx)
    phxi = phx(i);
    phxim1 = phx(i-1);

    if true
        if rs(i, 2) > s_ub(2)
            y_ub(i) = rs(i, 2);
            s_ub = [0 rs(i, 2)];
        else
            [y_ub(i), s_ub] = filter_statevartpt2(ghr2_release, rs(i, 2), s_ub);
        end
        if rs(i, 2) < s_lb(2)
            y_lb(i) = rs(i, 2);
            s_lb = [0 rs(i, 2)];
        else
            [y_lb(i), s_lb] = filter_statevartpt2(ghr2_release, rs(i, 2), s_lb);
        end
    elseif true
        [y_attack, s_attack] = filter_statevartpt2(ghr2_attack, rs(i, 2), s_ub);
        [y_release, s_release] = filter_statevartpt2(ghr2_release, rs(i, 2), s_ub);
        if y_attack > y_release
            y_ub(i) = y_attack;
            s_ub = s_attack;
            ar_ub = 1;
        else
            y_ub(i) = y_release;
            s_ub = s_release;
            ar_ub = 2;
        end

        [y_attack, s_attack] = filter_statevartpt2(ghr2_attack, rs(i, 2), s_lb);
        [y_release, s_release] = filter_statevartpt2(ghr2_release, rs(i, 2), s_lb);
        if y_attack < y_release
            y_lb(i) = y_attack;
            s_lb = s_attack;
            ar_lb = 1;
        else
            y_lb(i) = y_release;
            s_lb = s_release;
            ar_lb = 2;
        end
    elseif false
        y_attack = filter_1storder(b_attack, a_attack, phxi, phxim1, y_ub(i-1));
        y_release = filter_1storder(b_release, a_release, phxi, phxim1, y_ub(i-1));
        y_ub(i) = max(y_attack, y_release);
        y_attack = filter_1storder(b_attack, a_attack, phxi, phxim1, y_lb(i-1));
        y_release = filter_1storder(b_release, a_release, phxi, phxim1, y_lb(i-1));
        y_lb(i) = min(y_attack, y_release);
    else
        y = filter_1storder(b_release, a_release, rs(i, 2), rs(i-1, 2), y_ub(i-1));
        y_ub(i) = max(rs(i, 2), y);
        y = filter_1storder(b_release, a_release, rs(i, 2), rs(i-1, 2), y_lb(i-1));
        y_lb(i) = min(rs(i, 2), y);
    end

    ym = (y_ub(i) + y_lb(i)) / 2;
    rs1_db = pow2db(rs(i,1));
    if pow2db(ym) < rs1_db - 2
        % mean is well below release-filtered signal
        [o1(i), s12_o1] = filter_statevartpt2(ghr2_attack, ym, s12_o1);
        o1(i) = ym;
    elseif pow2db(ym) < rs1_db + 2
        % mean is around release filtered signal
        [o1(i), s12_o1] = filter_statevartpt2(ghr2_release, phxi, s12_o1);
    else
        % mean is well above release filtered signal
        ym_prev = (y_ub(i-1) + y_lb(i-1)) / 2;
        if ym > ym_prev
            [o1(i), s12_o1] = filter_statevartpt2(ghr2_attack, ym, s12_o1);
            o1(i) = ym;
        else
            [o1(i), s12_o1] = filter_statevartpt2(ghr2_release, phxi, s12_o1);
        end
    end
end
hold on

ym = (y_ub + y_lb) / 2;
plot(ms, pow2db(y_ub), 'r', ms, pow2db(y_lb), 'r');

plot(ms, pow2db(ym), 'b:');
%plot(ms, pow2db(o1), 'b');
%plot(ms, pow2db(o2), 'r:');
hold off


%plot(ms, pow2db(ym), 'b', ms, pow2db(rs(:,1)), 'b:');

[b,a] = critdamplp2(frs(1)/fs*2);

th = -52;
ratio = 2;
in_db = pow2db(rs(:, 1));
gr_db = (in_db - th) / ratio + th - in_db;
gr_db(in_db < th) = 0;


% gr_db = filter(b, a, gr_db);
gr = db2mag(gr_db);
% gr = filter(b, a, gr);

figure(2)
grw = gr(1500:12500);
f = mag2db(abs(fft(grw.*blackman(length(grw)),fs)));
f = f-f(1);
plot(0:100,f(1:101)),grid;
axis([0 100 -70 0])
