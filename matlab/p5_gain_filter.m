% Different gain filter methods:

% 1. filter input level as rms, ms, db, calculate gr
% 2. calculate gr, filter gr as rms, ms, db
% 3. calculate gr, slew-rate limit gr

threshold_db = -60;
ratio = 4;

calc_gr1 = @(in_db) (in_db - threshold_db) / ratio + threshold_db - in_db;
calc_gr = @(in_db) (calc_gr1(in_db) - abs(calc_gr1(in_db)))/2;

fs = 1000;
N = 1000;
% input signal db

dbs = [-20 0 -20 -60 -40 -60 -70 -50 -70];
dbs = repmat(dbs, N, 1);
x_db = dbs(:);
x_mag = db2mag(x_db);
x_pow = db2pow(x_db);
tc = 0.4;
[b,a] = butter(1, 1/tc/fs*2);
x_mag_f = filter(b, a, x_mag);
x_pow_f = filter(b, a, x_pow);
x_mag_f_gr_db = calc_gr(mag2db(x_mag_f));
x_pow_f_gr_db = calc_gr(pow2db(x_pow_f));
y_mag_f_gr_db = x_db + x_mag_f_gr_db;
y_pow_f_gr_db = x_db + x_pow_f_gr_db;
x_gr_db = calc_gr(x_db);
x_gr_db_f = filter(b, a, x_gr_db);
x_gr_mag_f = filter(b, a, db2mag(x_gr_db)-1)+1;
x_gr_pow_f = filter(b, a, db2pow(x_gr_db)-1)+1;
y_gr_db_f_db = x_db + x_gr_db_f;
y_gr_mag_f_db = x_db + mag2db(x_gr_mag_f);
y_gr_pow_f_db = x_db + pow2db(x_gr_pow_f);
ms = [0:length(x_db)-1]/fs*1000;
plot(ms, x_db, ...
    ms, y_mag_f_gr_db, ...
    ms, y_pow_f_gr_db, ...
    ms, y_gr_db_f_db, ...
    ms, y_gr_mag_f_db, ...
    ms, y_gr_pow_f_db), grid;
legend('input', ...
    'mag f gr', ...
    'pow f gr', ...
    'gr db f', ...
    'gr mag f', ...
    'gr pow f' ...
);
