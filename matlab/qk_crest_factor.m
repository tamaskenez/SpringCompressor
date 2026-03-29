[x, fs] = audioread('AngelaThomasWade_MilkCowBlues.wav');
x = x(1.0853e+05:1.3502e+05,1);

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

figure(2)


fcs = 2.^[0:0.5:10];
NF = length(fcs);
fc = 10;
rs = zeros(NX, NF);
drs = zeros(NX, NF);
order = 1;
pwr = x.^2;
pwr = abs(hilbert(x)/sqrt(2)).^2;
DC = mean(pwr);
for i = 1:length(fcs)
    switch order
    case 2
        [b,a] = critdamplp2(fcs(i)*2/fs);
    case 1
        [b,a] = butter(1, fcs(i)*2/fs);
    otherwise
        assert(false);
    end
    rs(:, i) = filter(b,a,pwr-DC)+DC;
    if order == 1
        A = filter(b, a, 1);
        drs(1:end-1, i) = diff(mag2db(rs(:, i)));
    end
end

M = zeros(NX, NF);
for i = 2:NF
    M(:, i+1) = pow2db(rs(:, i)) - pow2db(rs(:, i-1));
end
MP=M;
MN=M;
MP(MP < 0) = 0;
MN(MN > 0) = 0;
MN=-MN;

subplot(2,1,1);plot(pow2db(rs));
w = blackman(1001);w = w / sum(w);
subplot(2,1,2);
%plot(drs);
plot(pow2db(conv(pwr, w, 'same')));

figure(1);
subplot(2,1,1);imagesc(MP(9000:14000,:)')
subplot(2,1,2);imagesc(MN(9000:14000,:)')

figure(3);
R = 9000:14000;
R = 9000+(3000:7000);
% mesh(MP(R,:)')


% MDN = [zeros(1, size(M, 2)); (M(3:end, :) - M(1:end-2, :)) / 2; zeros(1, size(M, 2))];
% MDF = [zeros(size(M, 1), 1) (M(:, 3:end) - M(:, 1:end-2)) / 2 zeros(size(M, 1), 1)];
mesh(M(R,:)')

figure(4);
mesh(pow2db(rs(R,:))');