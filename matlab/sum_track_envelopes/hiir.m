% Return one of the HIIR filters: two all-pass filters, the phase difference between them is about 90 degs
% in a wide frequency range.
% The phase difference starts to decrease about 13 Hz at 48000 sample rate.
% 11 Hz: 70°
% 15 Hz: 80°
% Source: https://yehar.com/blog/?p=368
function [H1b, H1a, H2b, H2a] = hiir()

[b1, a1] = H_sect(0.6923878);
[b2, a2] = H_sect(0.9360654322959);
[b3, a3] = H_sect(0.9882295226860);
[b4, a4] = H_sect(0.9987488452737);
H1b = [0 conv(b1, conv(b2, conv(b3, b4)))];
H1a = conv(a1, conv(a2, conv(a3, a4)));

[b1, a1] = H_sect(0.4021921162426);
[b2, a2] = H_sect(0.8561710882420);
[b3, a3] = H_sect(0.9722909545651);
[b4, a4] = H_sect(0.9952884791278);
H2b = conv(b1, conv(b2, conv(b3, b4)));
H2a = conv(a1, conv(a2, conv(a3, a4)));

endfunction
