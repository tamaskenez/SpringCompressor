% Apply the 1-pole TPT filter on the the input vector x.
% g - filter coefficient
% x - input signal (vector)
% s - (optional) input filter state, zero if omitted
% Return the filtered result `y` and the ending state `s`.
function [y, s] = filter_tpt1(g, x, s)

if nargin < 3
    s = 0;
end

N = length(x);
y = zeros(size(x));
g1 = 1 / (1 + g);

for n = 1:N
    v = (x(n) - s) * g1;
    lp = v * g + s;
    s = lp + v * g;
    y(n) = lp;
end

end
