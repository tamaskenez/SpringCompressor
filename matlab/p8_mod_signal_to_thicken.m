% Create some control signal from a signal, which resembles the sawtooth signal that is
% produced in the level-detector of a compressor.
% Make it oscillate around 1 and multiply the signal with it.

% Evaluation: could work as an effect.
% Needs a number of filters at different frequencies. Take the min of all filters and modulate signal with that.
% Modulator can be HP-LP filtered.
% Reciprocal signal (capped) might accentuate transients.
% Intensity can be increased by feeding it into different functions: pow, exp (then db2mag)


fs = 48000;
N = fs;
t = [0:N-1]'/fs;
F0 = 100;
x = 10 * sin(2*pi*F0*t);

x = w;

ax = abs(x);
axx = ax.^2;
[b, a] = butter(1, 100/fs*2);
fax = filter(b, a, ax);
faxx = sqrt(filter(b, a, axx));
m = ((fax./faxx)-1) * 10 + 1;
y=x.*m;
y(isnan(y))=0;
y(isinf(y))=0;

player=audioplayer(y,48000);
play(player);


