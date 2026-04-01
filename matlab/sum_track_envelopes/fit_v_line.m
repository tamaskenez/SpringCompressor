% LSQ fit a two-segment line to the data x, weighted by window w.
% Finds the optimal two-segment line with the first segment 1..c, the second c..length(x)
function [xhat, E] = fit_v_line(x, w, c)

assert(isvector(x));
assert(isvector(w));
N = length(x);
assert(length(w) == N);
assert(isscalar(c));
assert(1 <= c && c <= N);

x = x(:);
w = w(:);


WL = sum(w(1:c-1));
SL = sum(w(1:c-1) .* (0:c-2)');
QL = sum(w(1:c-1) .* ((0:c-2)'.^2));
XL = sum(w(1:c-1) .* x(1:c-1));
YL = sum(w(1:c-1) .* x(1:c-1) .* (0:c-2)');

WR = sum(w(c:N));
SR = sum(w(c:N) .* (0:N-c)');
QR = sum(w(c:N) .* ((0:N-c)'.^2));
XR = sum(w(c:N) .* x(c:N));
YR = sum(w(c:N) .* x(c:N) .* (0:N-c)');

ic = c - 1;
A = [
    QL + ic^2 * WR ic * SR SL + ic * WR
    ic * SR QR SR
    SL + ic * WR SR WL + WR
];
B = [
    YL + ic * XR
    YR
    XL + XR
];

switch rank(A)
case 2 % Find which 2 x 2 has rank 2
    AL = A([1 3], [1 3]);
    AR = A([2 3], [2 3]);
    if rank(AL) == 2
        BL = B([1 3]);
        a1_b1 = AL\BL;
        a1 = a1_b1(1);
        a2 = 0;
        b1 = a1_b1(2);
    elseif rank(AR) == 2
        BR = B([2 3]);
        a2_b1 = AR\BR;
        a1 = 0;
        a2 = a2_b1(1);
        b1 = a2_b1(2);
    else
        assert(false)
    end
case 3 %OK
    a1_a2_b1 = A\B;
    a1 = a1_a2_b1(1);
    a2 = a1_a2_b1(2);
    b1 = a1_a2_b1(3);
otherwise
    assert(false)
end

b2 = b1 + (a1 - a2) * ic;


xhat = zeros(N, 1);
xhat(1:c-1) = a1 * (0:c-2) + b1;
xhat(c:end) = a2 * (c-1:N-1) + b2;

E = sum(w .* (x - xhat).^2);

endfunction
