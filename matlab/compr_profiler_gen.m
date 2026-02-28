% Generate a test signal to profile a compressor
function T = compr_profiler_gen(fs, fname)

run compr_profiler_consts

N = round(section_length_sec * fs);
B = round(section_break_sec * fs);

brk = zeros(B, 1);

% Sections
% - Uncompressed freq transfer
S1 = randn(N, 1) * db2mag(soft_dbfs);

% - Compressed freq transfer
S2 = randn(N, 1);

% LP/HP filter S1 and S2
[b, a] = butter(2, min_freq_hz/fs*2, 'high');
S1 = filter(b, a, S1);
S2 = filter(b, a, S2);

[b, a] = butter(1, max_freq_to_fs * 2);
S1 = filter(b, a, S1);
S2 = filter(b, a, S2);

S2 = S2 ./ max(abs(S2));

% - Static dB transfer, low and high crest factor

% Pick an F0 that results in integer period.
P0 = round(fs / F0);
F0 = fs / P0;
t = [0:2*N-1]' / fs;
dbs = (1 - abs([0:2*N-1]' / N - 1)) * (loud_dbfs - soft_dbfs) + soft_dbfs;
amps = db2mag(dbs);
N_harmonics = floor(max_freq_to_fs*fs/F0) - 1;
S3 = [];
for delta = [0.2 0.5]
    x = zeros(size(t));
    for n = 1:N_harmonics
        x = x + (4 / (n * pi)) * sin(n*pi*delta) * cos(2*pi*n*F0*t);
    end
    if isempty(S3)
        S3 = x .* amps;
    else
        S4 = x .* amps;
    end
end

% - Uncompressed step
t = [0:N-1]' / fs;
S5 = sin(2*pi*F0*t);
S6 = S5;
S7 = S5;
S5 = S5 * db2mag(soft_dbfs);
S6 = S6 * db2mag(soft_dbfs);
S7 = S7 * db2mag(loud_dbfs - step_db);
third = floor(N / P0 / 3) * P0;
R = 1 + third : 0 + 2 * third;
S5(R) = S5(R) * db2mag(step_db);
S6(R) = S6(R) * db2mag(loud_dbfs - soft_dbfs);
S7(R) = S7(R) * db2mag(step_db);

% - Compressed step around threshold
% - Compressed step above threshold

T = [S1; brk; S2; brk; S3; brk; S4; brk; S5; brk; S6; brk; S7];

if ~isempty(fname)
    audiowrite(fname, T, fs);
end
