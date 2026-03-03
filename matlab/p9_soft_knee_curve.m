% Find a good curve for a soft knee transfer function.
% The idea is that below threshold dy/dx = 1
% Above a certain value dy/dx = 1/ratio
% We need to get from 1 to 1/ratio either linearly or exponentially.

R = 10;
x0 = -50; % threshold
x1 = -10; % where we reach the nominal ratio

% Linear transition:
a = (1-R) / (2 * R * (x1 - x0));
b = 1 - 2 * a * x0;
c = x0 - a * x0^2 - b*x0;

p = [a b c];

x = -60:0;
xt = x0:x1;
yt = polyval(p, xt);
xf = x1:0;
figure(1);
plot(x, x, xt, yt, xf, (xf - x1)/R + polyval(p, x1)), grid;


% Exponential case.
w = x1 - x0;
B = log(1/R)/w;
A = 1 / B;
C = x0 - A;

hf = @(xx) A * exp(B * (xx - x0)) + C;

figure(2);
plot(x, x, xt, hf(xt), xf, (xf - x1)/R + hf(x1)), grid;
