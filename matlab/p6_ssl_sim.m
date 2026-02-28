% Try recreating the SLL Bus Compressor attack-relase logic

% The test and result signals must be in T and U.
% fs is the sampling rate.

% Result: Looks like the attack/release filtering is done on the GR signal in dB.
% The SSL and UAD implementation agree more or less.

opt_for = 5;

switch opt_for
case 1
    attack_ms = 1;
    release_ms = 40;
    threshold_db = -19.15;
    ratio = 4.5;
case 2
    attack_ms = 1;
    release_ms = 25;
    threshold_db = -19.125;
    ratio = 4.5;
case 3
    attack_ms = 1;
    release_ms = 65;
    threshold_db = -18.75;
    ratio = 5.25;
case 4
    attack_ms = 1;
    release_ms = 85;
    threshold_db = -18.5;
    ratio = 6.5;
case 5
    if 0
        % SSL
        attack_ms = 1.35;
        release_ms = 50;
        threshold_db = -18.9;
        ratio = 5;
    else
        % UAD
        attack_ms = 1.35;
        release_ms = 50;
        threshold_db = -25;
        ratio = 3;
    end
otherwise
    attack_ms = 1.25;
    release_ms = 50;
    threshold_db = -18.75;
    ratio = 4.7;
end

run compr_profiler_consts
N = round(section_length_sec * fs);
B = round(section_break_sec * fs);
S7 = T(1 + 8 * N + 6 * B: 0 + 9 * N + 6 * B);
Y7 = U(1 + 8 * N + 6 * B: 0 + 9 * N + 6 * B);

attack_c = exp(-1000 / (fs * attack_ms));
release_c = exp(-1000 / (fs * release_ms));

levels = repmat([0 0]', 1, N+1);
grs = repmat([1 1 0]', 1, N+1);
in = S7;
for i = 1:N
    abs_ini = abs(in(i));
    in_db = mag2db(abs_ini);
    gr_db = 0;
    if in_db > threshold_db
        gr_db = (in_db - threshold_db) / ratio + threshold_db - in_db;
    end
    gr = [db2mag(gr_db) db2pow(gr_db) gr_db]';
    l = [abs_ini abs_ini^2]';

    mask = l > levels(:, i);
    c = ones(2, 1) * release_c;
    c(mask) = attack_c;

    levels(:, i + 1) = c .* levels(:, i) + (1 - c) .* l;

    mask = gr <= grs(:, i);
    c = ones(3, 1) * release_c;
    c(mask) = attack_c;

    grs(:, i + 1) = c .* grs(:, i) + (1 - c) .* gr;
end
levels = levels(:, 2:end)';
grs = grs(:, 2:end)';
levels_db = zeros(size(levels));
levels_db(:, 1) = mag2db(levels(:, 1));
levels_db(:, 2) = pow2db(levels(:, 2));
out_db = levels_db;
above = out_db >= threshold_db;
out_db(above) = (levels_db(above) - threshold_db) / ratio + threshold_db;
gr_db = zeros(size(out_db));
gr_db(above) = out_db(above) - levels_db(above);
out = repmat(in, 1, 2) .* db2mag(gr_db);

gr2_db = grs;
gr2_db(:, 1) = mag2db(grs(:, 1));
gr2_db(:, 2) = pow2db(grs(:, 2));
out = [out repmat(in, 1, 3) .* db2mag(gr2_db)];

P0 = round(fs / F0);
M = floor(N / P0);
K = M * P0;
y = max(reshape(Y7(1:K), P0, M))';
o1 = max(reshape(out(1:K, 1), P0, M))';
o2 = max(reshape(out(1:K, 2), P0, M))';
o3 = max(reshape(out(1:K, 3), P0, M))';
o4 = max(reshape(out(1:K, 4), P0, M))';
o5 = max(reshape(out(1:K, 5), P0, M))';
o = mag2db([o1 o2 o3 o4 o5]);
figure(6)
plot(1:M, mag2db(y), 1:M, mag2db(o1), 1:M, mag2db(o2), 1:M, mag2db(o3), 1:M, mag2db(o4), 1:M, mag2db(o5));
figure(opt_for);
plot(1:M, mag2db(y), 1:M, o(:, opt_for));
