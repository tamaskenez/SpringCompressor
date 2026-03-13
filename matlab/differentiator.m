function d = differentiator(k, f)
%differentiator(k)
%returns differentiator filter of order k, frame size f
%f must be odd

if mod(f, 2) ~= 1, error('f must be odd'); end

d = zeros(f, 1);
half = (f-1) / 2;
if k == 0
    d(half+1) = 1;
    return
end


x = -half:half;
for i = -half:half
    y = zeros(1, f);
    y(i+half+1) = 1;
    p = polyfit(x, y, f-1);
    d(-half-i+f) = p(end-k) * factorial(k);
end
