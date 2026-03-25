% First we compute RMS on hilbert, then asymmetrical post filtering fixes first filter's ripple

% test if hilbert helps with faster envelope detection

[x, fs] = audioread('bd.wav');
N = length(x);
ns = [0:N-1]';

f1 = 2000;
f2 = 50;

[b,a] = critdamplp2(f1/fs*2);
r1 = filter(b, a, abs(hilbert(x)).^2);
r2 = releasefilter(1, f2/fs*2, r1);
[b,a] = critdamplp2(f1/fs*2);
r3 = filter(b, a, r2);

% plot(ns,x,ns,r1,ns,r2);
R = 1001:4001;
nsr = ns(R);

plot(nsr,abs(hilbert(x(R))).^2,'c',nsr,(abs(r1(R))),'k',nsr,(abs(r2(R))),'r',nsr,(abs(r3(R))),'r');
