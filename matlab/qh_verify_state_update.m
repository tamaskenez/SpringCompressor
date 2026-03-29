fs = 48000;
fc1 = 200; Q1 = 0.4;
fc2 = 700; Q = 0.95;

ghr1 = statevartpt2(fc1/fs*2, Q1);
ghr2 = statevartpt2(fc2/fs*2, Q1);

g1 = ghr1(1);
h1 = ghr1(2);
r1 = ghr1(3);
g2 = ghr2(1);
h2 = ghr2(2);
r2 = ghr2(3);

D = 4;
x = [0:fs-1]' * D;
xi = x(end);

[y1, s_before_i] = filter_statevartpt2(ghr1, x(1:end-1));
[yLP, s] = filter_statevartpt2(ghr1, xi, s_before_i);

% Last iteration:

s_exp = [0 0];
g = g1;
h = h1;
R2 = r1;
s1p = s_before_i(1);
s2p = s_before_i(2);
s_exp(1) = 2 * g * h * xi + (1 - 2 * g * h * (R2 + g)) * s1p - 2 * g * h * s2p;
yLP_expect = g^2 * h * xi + (g - g^2 * h * (R2 + g)) * s1p + (1 - g^2 * h) * s2p;
s_exp(2) = 2 * g^2 * h * xi + (2 * g - 2 * g^2 * h * (R2 + g)) * s1p + (1 - 2 * g^2 * h) * s2p;

printf("actual yLP, s1, s2: %f %f %f\n", yLP, s(1), s(2));
printf("expect yLP, s1, s2: %f %f %f\n", yLP_expect, s_exp(1), s_exp(2));

% Steady state:

s1_exp = [D / (2 * g) yLP + D / 2];

printf("Steady state actual: %f %f\n", s(1), s(2));
printf("Steady state expect: %f %f\n", s1_exp(1), s1_exp(2));


% Compute yLP, D from state:
printf("Actual yLP, D: %f %f\n", yLP, D);
printf("Expect yLP, D: %f %f\n", s(2) - g1 * s(1), 2 * g1 * s(1));

s_before_conv = convert_statevartpt2_state(ghr1, ghr2, s_before);


% [y1, s] = filter_statevartpt2(ghr1, x);
% filter_statevartpt2(ghr1, xi

% yLP = s2n - g * s1;
% D = 2 * g * s1;
%
%
% s1n = 2 * g * h * xi + (1 - 2 * g * h * (R2 + g)) * s1p - 2 * g * h * s2p;
% yLP = g^2 * h * xi + (g - g^2 * h * (R2 + g)) * s1p + (1 - g^2 * h) * s2p;
% s2n = 2 * g^2 * h * xi + (2 * g - 2 * g^2 * h * (R2 + g)) * s1p + (1 - 2 * g^2 * h) * s2p;
%
%         y(i) = yLP;
