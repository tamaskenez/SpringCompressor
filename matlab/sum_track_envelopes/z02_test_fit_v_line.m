
N = 101;
c = 32;
a1 = 2;
b1 = 3;
a2 = -5;
b2 = b1 + (a1 - a2) * (c-1);

is = (0:N-1)';
xexp1 = a1 * is + b1;
xexp2 = a2 * is + b2;
assert(xexp1(c) == xexp2(c));
x = zeros(N, 1);
x(1:c-1) = xexp1(1:c-1);
x(c:end) = xexp2(c:end);
w = gausswin(N);
[xhat, E] = fit_v_line(x, w, c);
plot(0:N-1,x,0:N-1,xhat);

