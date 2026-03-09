[y,fs] = audioread('sine_100_1000_ssl.wav');
plot(y);
x = y(fs+(6000:48000),1);
N=length(x);
%x = sin(2*pi*100*[0:N-1]/fs)';
fy=mag2db(abs(fft(x.*blackman(N),fs)));
plot((0:12000)',fy(1:12001)), grid;
