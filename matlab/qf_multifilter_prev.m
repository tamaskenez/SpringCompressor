% Use multiple filters to tell apart transients and interference ripple.

figure(2)

[x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
x = x(1.0853e+05:1.3502e+05,1);

% x = 10 * (sin(2*pi*100/fs*[0:fs-1]') + sin(2*pi*150/fs*[0:fs-1]'));

N = length(x);
ns = [0:N-1]';

ms = (ns)/fs*1000;
release_hz = 10;
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

y1 = zeros(size(phx));
y2 = zeros(size(phx));
s12_1 = [0 0];
s12_2 = [0 0];
o1 = y1;
o2 = y1;
s12_o1 = [0 0];
s12_o2 = [0 0];

for i = 2:length(phx)
    phxi = phx(i);
    [y_as_attack, s12_as_attack] = filter_statevartpt2(ghr2_attack, phxi, s12_1);
    [y_as_release, s12_as_release] = filter_statevartpt2(ghr2_release, phxi, s12_1);
    if y_as_attack > y_as_release
        y1(i) = y_as_attack;
        s12_1 = s12_as_attack;
    else
        y1(i) = y_as_release;
        s12_1 = s12_as_release;
    end
    [y_as_attack, s12_as_attack] = filter_statevartpt2(ghr2_attack, phxi, s12_2);
    [y_as_release, s12_as_release] = filter_statevartpt2(ghr2_release, phxi, s12_2);
    if y_as_attack < y_as_release
        y2(i) = y_as_attack;
        s12_2 = s12_as_attack;
    else
        y2(i) = y_as_release;
        s12_2 = s12_as_release;
    end
    ym = (y1(i) + y2(i)) / 2;
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
        if ym > (y1(i-1) + y2(i-1))/2
            [o1(i), s12_o1] = filter_statevartpt2(ghr2_attack, ym, s12_o1);
            o1(i) = ym;
        else
            [o1(i), s12_o1] = filter_statevartpt2(ghr2_release, phxi, s12_o1);
        end
    end
end
hold on

ym = (y1+y2)/2;
plot(ms, pow2db(y1), 'r', ms, pow2db(y2), 'r');

plot(ms, pow2db(ym), 'b:', ms, pow2db(o1), 'b');
%plot(ms, pow2db(o2), 'r:');
hold off


%plot(ms, pow2db(ym), 'b', ms, pow2db(rs(:,1)), 'b:');
