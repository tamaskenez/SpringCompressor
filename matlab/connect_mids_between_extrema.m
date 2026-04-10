% This function:
% - Takes the input vector `x` which represents a continuous curve
% - Finds minima and maxima (assumes there are no adjacent equal elements)
% - Finds the points between subsequent minima and maxima where the curve equals to the midpoint between
%   the minimum and maximum (with linear interpolation)
% - Outputs a `y`, a vector of the same size as `x` which contains a polyline which is the the midpoints
%   connected by lines. The part of the curve before the first midpoint and after the midpoint is copied
%   over from `x`.
function y = connect_mids_between_extrema(x)
    n = length(x);
    y = x;

    % Find indices of local extrema (interior points only)
    extrema_idx = [];
    for i = 2:n-1
        if (x(i) > x(i-1) && x(i) > x(i+1)) || (x(i) < x(i-1) && x(i) < x(i+1))
            extrema_idx(end+1) = i;
        end
    end

    if length(extrema_idx) < 2
        return;
    end

    % For each pair of adjacent extrema, find the fractional index where x equals the midpoint value.
    % The segment between two adjacent extrema is monotone, so there is exactly one such crossing.
    num_pairs = length(extrema_idx) - 1;
    mid_frac = zeros(1, num_pairs);
    mid_val = zeros(1, num_pairs);

    for k = 1:num_pairs
        i1 = extrema_idx(k);
        i2 = extrema_idx(k+1);
        target = (x(i1) + x(i2)) / 2;
        mid_val(k) = target;

        % Walk from i1 toward i2 to find the bracket
        for i = i1:i2-1
            if (x(i) <= target && target <= x(i+1)) || (x(i) >= target && target >= x(i+1))
                % Linear interpolation for fractional index
                if x(i+1) == x(i)
                    mid_frac(k) = i;
                else
                    mid_frac(k) = i + (target - x(i)) / (x(i+1) - x(i));
                end
                break;
            end
        end
    end

    % Build the polyline: between consecutive midpoints, linearly interpolate.
    % Before the first midpoint and after the last midpoint, y = x (already set).
    for k = 1:num_pairs-1
        f1 = mid_frac(k);
        f2 = mid_frac(k+1);
        v1 = mid_val(k);
        v2 = mid_val(k+1);
        for i = ceil(f1):floor(f2)
            t = (i - f1) / (f2 - f1);
            y(i) = v1 + t * (v2 - v1);
        end
    end
endfunction
