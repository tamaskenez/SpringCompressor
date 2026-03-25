% Apply asymmetric, first-order low-pass filter: no filter on attack, low-pass at f_hps on release.
function y = releasefilter(order, f_hps, x)
    assert(order == 1);
    assert(isvector(x));
    y = zeros(size(x));
    N = length(x);

    c = cos(pi * f_hps);
    tau = 2 - c - sqrt((c - 2).^2 - 1);
    s = 0;
    for i = 1:N
        if x(i) > s
            s = x(i);
        else
            s = tau * s + (1-tau) * x(i);
        end
        y(i) = s;
    end
end

