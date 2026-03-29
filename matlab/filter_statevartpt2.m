%
function [y, s12] = filter_statevartpt2(g_h_R2, x, s12)
    if nargin == 2
        s12 = [0 0];
    end
    assert(isvector(g_h_R2) && length(g_h_R2) == 3 && isvector(s12) && length(s12) == 2 && isvector(x));
    g = g_h_R2(1);
    h = g_h_R2(2);
    R2 = g_h_R2(3);
    s1 = s12(1);
    s2 = s12(2);
    y = zeros(size(x));
    for i = 1:length(x)
        yHP = h * (x(i) - (R2 + g) * s1 - s2);
        yBP = g * yHP + s1;
        s1 = 2 * yBP - s1;
        yLP = g * yBP + s2;
        s2 = 2 * yLP - s2;
        y(i) = yLP;
    end
    s12 = [s1 s2];
end
