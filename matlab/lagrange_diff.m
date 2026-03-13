function filter_coefficient=lagrange_diff(filter_length, fs)
% LAGRANGE_DIFF
% - filter_length >= 2, integer
% - fs is optional, if specified the filter will be scaled to yield the correct value for dx/dt
% The digital differentiator from the Lagrange interpolation.
% It is equivalent to the maximally flat low-pass digital differentiator,
% or the degenerated form of Savitzky-Golay digital differentiator.
%
% Author:
% Jianwen Luo <luojw@ieee.org> 2006-06-17
% Department of Biomedical Engineering,
% Tsinghua University, Beijing 100084, P. R. China
%
% References:
% [1]	Carlsson B.
% Maximum Flat Digital Differentiator,
% Electron. Lett. 1991, 27(8): 675-677.
% [2]	Kumar B, Roy S C D.
% Coefficients of Maximally Linear, Fir Digital Differentiators for Low-Frequencies,
% Electron. Lett. 1988, 24(9): 563-565.
% [3]	Selesnick I W.
% Maximally flat low-pass digital differentiators,
% IEEE Trans. Circuits Syst. II-Analog Digit. Signal Process. 2002, 49(3): 219-223.
% [4]	Khan I R, Okuda M, Ohba R.
% Design of FIR digital differentiators using maximal linearity constraints,
% IEICE Trans. Fundam. Electron. Commun. Comput. Sci. 2004, E87A(8): 2010-2017.
% [5]	Luo J W, Ying K, He P, Bai J.
% Properties of Savitzky-Golay digital differentiators,
% Digit. Signal Prog. 2005, 15(2): 122-136.
% [6]	Luo J W, Ying K, Bai J.
% Savitzky-Golay smoothing and differentiation filter for even number da,
% Signal Process. 2005, 85(7): 1429-1434
if filter_length>=2 && floor(filter_length)==ceil(filter_length)
    if mod(filter_length,2)==1
        m=(filter_length-1)/2;
        h=zeros(1,m);
        for k=1:m
            h(k)=(-1).^(k+1)*(prod(1:m))^2/k/prod(1:m-k)/prod(1:m+k);
        end
        filter_coefficient=[h(end:-1:1) 0 -h];
    else
        m=filter_length/2;
        h=zeros(1,m);
        for k=1:m;
            h(k)=(-1)^(k+1)*(prod(2*m-1:-2:1))^2/2^(2*m)/(k-1/2)^2/prod(1:m-k)/prod(1:m+k-1);
        end
        filter_coefficient=[h(end:-1:1) -h];
    end
else
    error('The input parameter (filter_length) should be a positive integer larger no less than 2.');
end

if nargin == 2
    filter_coefficient = filter_coefficient * fs;
end
