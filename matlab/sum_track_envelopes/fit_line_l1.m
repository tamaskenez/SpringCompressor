% Fit a line on the input data x such that it minimizes the sum of weighted L1 residuals.
%
% If
%
%     N = length(x);
%     ns = (0:N-1);
%     xhat = a * ns + b;
%
% the function returns a, b and r such that
%
%     r = sum(w .* abs(x - xhat));
%
% is minimal.
%
function [a, b, r] = fit_line_l1(x, w)

x = x(:);
w = w(:);
N = length(x);
ns = (0:N-1)';

% Variables: z = [a; b; t_1; ...; t_N]
% Minimize c'*z = sum(w .* t)
c = [0; 0; w];

% |x_i - (a*ns_i + b)| <= t_i  rewrites as two linear constraints per sample:
%   a*ns_i + b - t_i <=  x_i
%  -a*ns_i - b - t_i <= -x_i
A = [ ns,  ones(N,1), -eye(N);
     -ns, -ones(N,1), -eye(N)];
rhs = [x; -x];

% a and b are free; t_i >= 0
lb = [-Inf; -Inf; zeros(N, 1)];
ub = [];
ctype = repmat('U', 1, 2*N);   % all <= constraints
vartype = repmat('C', 1, 2+N); % all continuous

[z, fval, status] = glpk(c, A, rhs, lb, ub, ctype, vartype);

if status ~= 0
    printf('glpk did not find an optimal solution (errnum %d)\n', status);
    a = nan;
    b = nan;
    r = inf;
else
    a = z(1);
    b = z(2);
    r = fval;
end

endfunction
