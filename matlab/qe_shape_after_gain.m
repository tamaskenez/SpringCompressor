% How does the shape of a decaying sinus + base level changes when the base level and the transient changes?
% In what domains are the shape kept?

% Result:
% We have steady sine wave with amplitude A, an exponentially decaying sine wave with amplitude B.
%
% 1. Setup: A is fixed, we visualize the shape with different B amplitudes
%
% - the shape of the transient in linear (mag) and power (pow) domains is the same, independent of B
% - the shape is different in dB domain. It is linear only if A is negligible, and the curve is different
%.  with different B amplitudes.
%
% 2. Setup: A is changed, B is fixed.

fs = 48000;
f1 = 312;
f2 = 823;
ns = [0:fs-1]';

A = 1;
B = 1;

x1 = sin(2*pi*f1*ns/fs);
x2 = sin(2*pi*f2*ns/fs);

    w = blackman(251);
    w = w / sum(w);

clf;
for C = [0.1 1 10]
    x = C * A * x1;
    x(12001:36000) = x(12001:36000) + C * B * x2(1:24000) .* exp(-0.0001*ns(1:24000));
    hx = hilbert(x);
    phx = abs(hx).^2;
    phx = conv(phx, w, 'same');
    % hold on, plot((pow2db(phx)-mag2db(A))), hold off
    hold on, plot((phx - (C*A)^2)/(C*B)^2), hold off
end
