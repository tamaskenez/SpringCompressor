function freq_transfer(S1, Y, fs)

assert(isvector(S1));
assert(isvector(Y));

N=length(S1);

assert(length(Y) == N);

subplot(2,1,1)
[h1, f1] = cpsd(S1,Y,[],[],fs);
[h2, f2] = cpsd(S1,S1,[],[],fs);
h = h1 ./ h2;
semilogx(f1*fs, mag2db(abs(h))), grid

subplot(2,1,2)
[m, f] = mscohere(S1, Y, [], [], fs);
semilogx(f*fs, m), grid
