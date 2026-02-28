% Use the result of compressor profiling
function compr_profiler_use(T, U, fs)

assert(isvector(T));

if ~isnumeric(U)
    printf('U is string\n');
    [U, fs2] = audioread(U);
    assert(fs2 == fs);
else
    printf('U is not string\n');
    assert(isvector(U));
end

run compr_profiler_consts

N = round(section_length_sec * fs);
B = round(section_break_sec * fs);

assert(length(T) == length(U));
assert(length(T) == 9 * N + 6 * B);

S1 = T(1 + 0 * N + 0 * B: 0 + 1 * N + 0 * B);
S2 = T(1 + 1 * N + 1 * B: 0 + 2 * N + 1 * B);
S3 = T(1 + 2 * N + 2 * B: 0 + 4 * N + 2 * B);
S4 = T(1 + 4 * N + 3 * B: 0 + 6 * N + 3 * B);
S5 = T(1 + 6 * N + 4 * B: 0 + 7 * N + 4 * B);
S6 = T(1 + 7 * N + 5 * B: 0 + 8 * N + 5 * B);
S7 = T(1 + 8 * N + 6 * B: 0 + 9 * N + 6 * B);

Y1 = U(1 + 0 * N + 0 * B: 0 + 1 * N + 0 * B);
Y2 = U(1 + 1 * N + 1 * B: 0 + 2 * N + 1 * B);
Y3 = U(1 + 2 * N + 2 * B: 0 + 4 * N + 2 * B);
Y4 = U(1 + 4 * N + 3 * B: 0 + 6 * N + 3 * B);
Y5 = U(1 + 6 * N + 4 * B: 0 + 7 * N + 4 * B);
Y6 = U(1 + 7 * N + 5 * B: 0 + 8 * N + 5 * B);
Y7 = U(1 + 8 * N + 6 * B: 0 + 9 * N + 6 * B);

third = round(N/3);

freq_transfer(S1(third:end), Y1(third:end), fs);
subplot(2,1,1);title('uncompressed freq transfer');

pause

freq_transfer(S2(third:end), Y2(third:end), fs);
subplot(2,1,1);title('compressed freq transfer');

pause

% Pick an F0 that results in integer period.
P0 = round(fs / F0);
S3 = S3(1:floor(length(S3) / P0) * P0);
S4 = S4(1:floor(length(S4) / P0) * P0);
Y3 = Y3(1:floor(length(Y3) / P0) * P0);
Y4 = Y4(1:floor(length(Y4) / P0) * P0);
S3 = reshape(S3, P0, length(S3) / P0);
S4 = reshape(S4, P0, length(S4) / P0);
Y3 = reshape(Y3, P0, length(Y3) / P0);
Y4 = reshape(Y4, P0, length(Y4) / P0);
S3_peaks_db = mag2db(max(abs(S3)))';
S4_peaks_db = mag2db(max(abs(S4)))';
Y3_peaks_db = mag2db(max(abs(Y3)))';
Y4_peaks_db = mag2db(max(abs(Y4)))';
S3_rms_db = pow2db(mean(S3.^2))';
S4_rms_db = pow2db(mean(S4.^2))';
Y3_rms_db = pow2db(mean(Y3.^2))';
Y4_rms_db = pow2db(mean(Y4.^2))';

subplot(2,1,1);
plot(S3_rms_db, Y3_rms_db, S4_rms_db, Y4_rms_db),grid;
title('50%, 10% square by RMS dB');

subplot(2,1,2);
plot(S3_peaks_db, Y3_peaks_db, S4_peaks_db, Y4_peaks_db),grid;
title('50%, 10% square by peak dB');

