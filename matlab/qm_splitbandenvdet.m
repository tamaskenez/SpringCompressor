%[x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
%x = x(1.0853e+05:1.3502e+05,1);

if false
    x = zeros(size(x));
    if false
        sx = [];
        for n = round([1 2 4 8 16 32 64 128 256 512]/10000*fs/2)*2+1
            w = blackman(n) .* cos(2*pi*10000/fs*(-(n-1)/2:(n-1)/2)')
            sx = [sx; w; zeros(7 * n, 1)];
        end
    elseif false
        sx = cos(2*pi*1000/fs*(0:round(fs/20))');
    endif
    x(9000:9000+length(sx)-1) = sx;
    x = x + randn(size(x))/10;
end

NX = length(x);
ns = (0:NX-1)';

f1 = 7;
f2 = 70;
order = 1;

switch order
case 1
    [b1, a1] = butter(1, f1*2/fs);
    [b2, a2] = butter(1, f2*2/fs);
case 2
    [b1, a1] = critdamplp2(f1*2/fs);
    [b2, a2] = critdamplp2(f2*2/fs);
otherwise
    assert(false);
end

pwr = x.^2;
pwr = abs(hilbert(x)/sqrt(2)).^2;
DC = mean(pwr);


R = 9000:14000;

figure(1), clf
rb2 = filter(b2, a2, pwr);
if false
    rb1 = filter(b1, a1, pwr);
else
    rb1 = filter(b1, a1, rb2);
end


function t = ease(distance, limit)
    if distance >= limit
        t = 1;
    else
        x = distance / limit;
        t = ((-cos(x * pi / 2) + 1) .^ 4);
    end
end

[b,a] = butter(1,f1*2/fs);
rd = filter(b, a, rb2 - rb1) / sum(b);
rx = rb2 - rb1;
DT = -rd ./ rx;
DT(DT<0) = nan;
DT(DT + ns > ns(end)) = nan;

NX = length(x);
y = zeros(size(x));
for i = 2:NX
    in = rb2(i-1:i);
    y(i) = b1(1) * in(1) + b1(2) * in(2) - a1(2) * y(i-1);
    r = rb2(i);
    if ...
        rd(i) > 0 && y(i) < r ... % Pull up
        || rd(i) < 0 && y(i) > r  % Pull down
        d = abs(pow2db(r/y(i)));
        t = ease(d, 6);
        % printf("[%d] pull %f to %f, d: %f, t: %f\n", i, pow2db(y(i)), pow2db(r), d, t);
        y(i) = db2pow((1-t) * pow2db(y(i)) + t * pow2db(r));
    end
end

h = zeros(size(x));
n0 = 0;
for i = 2:NX
    if sign(rd(i-1)) ~= sign(rd(i))
        n0 = i - rd(i) / (rd(i) - rd(i-1));
    end
    dn = i - n0;
    h(i) = rd(i) / dn;
end

subplot(2,1,1);plot(ns, pow2db(rb1), ns, pow2db(rb2), ns, pow2db(y), ns, pow2db(rb1 + h)), grid;
subplot(2,1,2);plot(ns, rb1, ns, rb2, ns, y, ns, rb1 + h), grid;

figure(2);
subplot(2,1,1);plot(ns, rd), grid
subplot(2,1,2);plot(ns, DT + ns), grid
