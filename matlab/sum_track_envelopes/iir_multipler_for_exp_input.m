% Given a digital filter, driven by an exponential signal x[n] = A * exp(alpha * n),
% when it stabilizes the output y[n] converges to beta * x[n].
% This function returns beta for a given alpha.
function beta = iir_multipler_for_exp_input(b, a, alpha)

z0 = exp(alpha);
kb = 0:length(b)-1;
ka = 0:length(a)-1;
num = sum(b(:).' .* z0 .^ (-kb));
den = sum(a(:).' .* z0 .^ (-ka));
beta = num / den;

endfunction
