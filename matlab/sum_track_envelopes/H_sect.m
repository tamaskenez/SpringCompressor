function [b, a] = H_sect(alpha)
    b = [alpha^2 0 -1];
    a = [1 0 -alpha^2];
endfunction
