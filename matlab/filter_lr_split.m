% Split the signal in `x` into low and high frequency bands, using
% Linkwitz-Riley filters at the cutoff frequency `fc`
%
% order: 1, 2 or 4. The order 1 is not technically a Linkwitz-Riley filter.
%    fc: normalized cutoff frequency (half-cycles per sample)
%     x: input signal
%
% The magnitudes of frequencies in `xlo + xhi` is the same as in `x` for all orders. However
% the phase varies with the order:
%
%      order: |      1      |     2     |        4
% ------------+-------------+-----------+-------------------
%   xlo phase |   0 -> -90  | 0 -> -180 | 0 -> -180 -> -360
%   xhi phase |  90 ->  0   | 0 -> -180 | 0 -> -180 -> -360
%   sum phase |   flat 0    | 0 -> -180 | 0 -> -180 -> -360

function [xlo, xhi] = filter_lr_split(order, fc, x)

assert(0 < fc && fc < 1);
hi_coeff = 1;
switch order
case 1
    [bl, al] = butter(1, fc);
    [bh, ah] = butter(1, fc, 'high');
case 2
    [bl, al] = butter(1, fc);
    [bh, ah] = butter(1, fc, 'high');
    bl = conv(bl, bl);
    al = conv(al, al);
    bh = conv(bh, bh);
    ah = conv(ah, ah);
    hi_coeff = -1;
case 4
    [bl, al] = butter(2, fc);
    [bh, ah] = butter(2, fc, 'high');
    bl = conv(bl, bl);
    al = conv(al, al);
    bh = conv(bh, bh);
    ah = conv(ah, ah);
otherwise
    assert(false);
end

xlo = filter(bl, al, x);
xhi = hi_coeff * filter(bh, ah, x);

end