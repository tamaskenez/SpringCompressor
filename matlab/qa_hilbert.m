% test if hilbert helps with faster envelope detection

[x, fs] = audioread('bd.wav');
N = length(x);
ns = [0:N-1]';

fc = 100;
K = 0.25;
J = 2;

[b,a] = butter(2, fc/fs*2, 'high');
x0=x;
x = filter(b, a, x0);

[b,a] = butter(2, fc/fs*2, 'low');
xlo = filter(b, a, x0);

[b,a] = critdamplp2(fc/K/fs*2);
r1 = sqrt(filter(b, a, x.^2));
[b,a] = critdamplp2(fc/K/fs*2);
r2 = sqrt(filter(b, a, abs(hilbert(x)).^2)/2);

% plot(ns,x,ns,r1,ns,r2);
R = 1001:4001;
nsr = ns(R);

r1 = releasefilter(1, fc/J/fs*2, r1);
r2 = releasefilter(1, fc/J/fs*2, r2);
if 1
    plot(nsr,(abs(x0(R))),'c',nsr,(abs(x(R))),'g',nsr,(abs(r1(R))),'k',nsr,(abs(r2(R))),'r');
    %axis([1000 4000 -60 0]);
else
    plot(nsr,mag2db(abs(x0(R))),'c',nsr,mag2db(abs(x(R))),'g',nsr,mag2db(abs(r1(R))),'k',nsr,mag2db(abs(r2(R))),'r');
    axis([1000 4000 -60 0]);
end
grid;
legend;




x=sin(2*pi*100/fs*R) + sin(2*2*pi*100/fs*R);
plot(R,abs(x),R,abs(hilbert(x)));