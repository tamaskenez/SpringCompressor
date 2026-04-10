% We compare two filter methods
% 1. filtering the power signal (fc1pow)
% 2. filtering the power signal (fc2pow) then the db signal (fc2db)

switch 4
    case 1
        fc1pow = 20;
        fc2pow = 37.5;
        fc2db = fc2pow;
        order = 1;
    case 2
        fc1pow = 20;
        fc2pow = 34;
        fc2db = fc2pow;
        order = 2;
    case 3
        fc1pow = 20;
        fc2pow = 29;
        fc2db = 40;
        order = 3;
    case 4
        fc1pow = 20;
        fc2pow = 31;
        fc2db = 31;
        order = 4;
    otherwise
        assert(false)
end

if false
    [x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
    x = x(30001:60000);
else
    [x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
    x = x(1.0853e+05:1.3502e+05,1);
end

hx = hilbert(x)/sqrt(2);
hxx = abs(hx).^2;

if true
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

switch order
    case 1
        [b1pow, a1pow] = butter(1, fc1pow*2/fs);
        [b2pow, a2pow] = butter(1, fc2pow*2/fs);
        [b2db, a2db] = butter(1, fc2db*2/fs);
    case 2
        [b1pow, a1pow] = critdamplp2(fc1pow*2/fs);
        [b2pow, a2pow] = critdamplp2(fc2pow*2/fs);
        [b2db, a2db] =   critdamplp2(fc2db*2/fs);
    case 3
        [b1pow, a1pow] = critdamplp2(fc1pow*2/fs);
        [b2pow, a2pow] = critdamplp2(fc2pow*2/fs);
        [b2db, a2db] =   butter(1, fc2db*2/fs);
    case 4
        [b1pow, a1pow] = critdamplp2(fc1pow*2/fs);
        [b2pow, a2pow] = butter(1, fc2pow*2/fs);
        [b2db, a2db] =   butter(1, fc2db*2/fs);
    otherwise
        assert(false)
end

y1 = pow2db(filter(b1pow, a1pow, hxx));
y2 = filter(b2db, a2db, pow2db(filter(b2pow, a2pow, hxx)));
ns = 0:NX-1;
plot(...
    ns, pow2db(hxx),
    ns, y1,
    ns, y2
)
grid
legend('unfiltered pow', 'filter pow', 'filter pow + db')

