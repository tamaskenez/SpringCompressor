% LSQ fit a line to the data x, weighted by window w.
function [xhat, E] = fit_line(x, w)

assert(isvector(x));
assert(isvector(w));
N = length(x);
assert(length(w) == N);

x = x(:);
w = w(:);

WL = sum(w);
SL = sum(w .* (0:N-1)');
QL = sum(w .* ((0:N-1)'.^2));
XL = sum(w .* x);
YL = sum(w .* x .* (0:N-1)');

A = [
    QL SL
    SL WL
];
B = [
    YL
    XL
];

a1_b1 = A\B;
a1 = a1_b1(1);
b1 = a1_b1(2);


xhat = a1 * (0:N-1) + b1;

E = sum(w .* (x - xhat).^2);

endfunction
