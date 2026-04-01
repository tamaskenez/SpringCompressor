[x, fs] = audioread('/Users/tamas/Documents/REAPER Media/AngelaThomasWade_MilkCowBlues/Media/01_Kick.wav');
x = x(30001:60000);
plot(x);
hx = hilbert(x)/sqrt(2);

shortest_window_sec = 0.001;
wlens = round(((shortest_window_sec*fs*2.^[0:0.25:7])/2))*2+1;
NW = length(wlens);
NX = length(x);

Z = zeros(NX, NW);

pwr0 = abs(hx).^2;

for i = 1:NW
    w = gausswin(wlens(i));
    w = w / sum(w);
    Z(:, i) = conv(pwr0, w, 'same');
end

plot(pow2db(Z)), grid;

F = zeros(size(Z));
for i = 2:NW-1
    fold_mask = (Z(:,i - 1) < Z(:,i) & Z(:,i) > Z(:,i + 1)) ...
    | (Z(:,i - 1) > Z(:,i) & Z(:,i) < Z(:,i + 1));
    F(fold_mask, i) = Z(fold_mask, i);
end

ww = 101;
w1 = gausswin(ww);
w1 = w1 / sum(w1);
w2 = linspace(0, 1, ww)' .* w1;
dbs = 0:0.1:30;
NDB = length(dbs);
rs = zeros(NDB, 1);
for i = 1:NDB
    sx = db2pow(linspace(0, dbs(i), ww))';
    s1 = w1' * sx;
    s2 = w2' * sx;
    rs(i) = s1/s2;
end
plot(dbs, rs);

