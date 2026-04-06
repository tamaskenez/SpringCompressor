% All methods try to model `x` with 1 or 2 log-ramps `db2pow(a * x + b)`, residual weighted by `w`.`
%
% Methods:
% 1: model 1 log-ramp, identify with w (0th moment) and  w .* (0:N-1) (first moment)
% 2: model 1 log-ramp, identify with the first 2 dpss
% 3: model 1 log-ramp, identify with the 0th, 1st and 2nd moments
% 4: model 2 log-ramps, identify with LSQ
% 5: model 1 log-ramp, identity with LSQ
% 6: model 1 log-ramp, identify with L1-norm
% 7: like 1. but without the window_len / 2 lookahead
% 8: peak detector on pow, quick lp1 fed by raw data, slow lp1 peak tracks quick lp1
% 9: peak detector on pow, lp1 fed by raw data
%10: like 8 but lp2
%11: like 9 but lp2
%12: like 1. but with some lookahead 0 < .. < window_len / 2
function y = multiwindow_power_filter(x, wlen, method)

assert(mod(wlen, 2) == 1);

pwr0 = abs(hilbert(x)/sqrt(2)).^2;
% pwr0 = x.^2;

switch method
case {1, 7, 12} % 0th and 1st moment
    windows = zeros(wlen, 2);
    w1 = gausswin(wlen);
    w1 = w1 / sum(w1);
    windows(:, 1) = w1;
    windows(:, 2) = linspace(-1, 1, wlen)' .* w1;
case 2 % first 2 dpss
    [windows, _] = dpss(wlen, 4, 2);
    sw1 = sum(windows(:, 1));
    windows = windows / sw1;
case 3 % 0th, 1st, 2nd moment
    windows = zeros(wlen, 3);
    w1 = gausswin(wlen);
    w1 = w1 / sum(w1);
    windows(:, 1) = w1;
    windows(:, 2) = linspace(-1, 1, wlen)' .* w1;
    windows(:, 3) = linspace(-1, 1, wlen)'.^2 .* w1;
case {4, 5, 6} % fit on-line or two-line
    w1 = gausswin(wlen);
    w1 = w1 / sum(w1);
case {8, 9, 10, 11}
    % Nothing to do.
otherwise
    assert(false)
end

NX = length(x);
y = zeros(size(x));

switch method
    case {1, 2, 3}
        NW = size(windows, 2);
        dbs = (-90:0.1:90)';
        NDBS = length(dbs);
        cb = (wlen + 1) / 2;
        cs = zeros(NDBS, NW);
        for i = 1:NDBS
            tx = db2pow(linspace(0, dbs(i), wlen))';
            tx = tx / tx(cb);
            for j = 1:NW
                cs(i, j) = sum(tx .* windows(:, j));
            end
        end
        ds = zeros(NX, NW);

        for i = 1:NW
            ds(:, i) = conv(pwr0, windows(:, i), 'same');
        end

        y = zeros(size(x));

        cs_2_o_1 = cs(:, 2) ./ cs(:, 1);
        x_2_o_1 = ds(:, 2) ./ ds(:, 1);

        switch NW
            case 2
                for i = 1:NX
                    [Y, I] = min(abs(cs_2_o_1 - x_2_o_1(i)));
                    y(i) = ds(i, 1) / cs(I, 1);
                end
            case 3
                cs_3_o_1 = cs(:, 3) ./ cs(:, 1);
                x_3_o_1 = ds(:, 3) ./ ds(:, 1);
                for i = 1:NX
                    e = (cs_2_o_1 - x_2_o_1(i)).^2 + (cs_3_o_1 - x_3_o_1(i)).^2;
                    [Y, I] = min(e);
                    y(i) = ds(i, 1) / cs(I, 1);
                end
            otherwise
                assert(false)
        end
    case 7
        NW = size(windows, 2);
        dbs = (-1000:0.1:1000)';
        NDBS = length(dbs);
        cs = zeros(NDBS, NW);
        for i = 1:NDBS
            tx = db2pow(linspace(0, dbs(i), wlen))';
            tx = tx / tx(1);
            for j = 1:NW
                cs(i, j) = sum(tx .* windows(:, j));
            end
        end
        ds = zeros(NX, NW);

        for i = 1:NW
            c = conv(pwr0, windows(:, i));
            ds(:, i) = c(1:NX);
        end

        y = zeros(size(x));

        cs_2_o_1 = cs(:, 2) ./ cs(:, 1);
        x_2_o_1 = ds(:, 2) ./ ds(:, 1);

        assert(NW == 2);
        for i = 1:NX
            [Y, I] = min(abs(cs_2_o_1 - x_2_o_1(i)));
            % assert(I ~= 1 && I ~= length(cs_2_o_1));
            y(i) = ds(i, 1) / sqrt(cs(I, 1));
        end
    case 12
        hw = (wlen - 1) / 2;
        lookahead = round(0.5 * hw); % 0 <= lookahead <= hw
        NW = size(windows, 2);
        dbs = (-1000:0.1:1000)';
        NDBS = length(dbs);
        cs = zeros(NDBS, NW);
        for i = 1:NDBS
            tx = db2pow(linspace(0, dbs(i), wlen))';
            tx = tx / tx(1 + lookahead);
            for j = 1:NW
                cs(i, j) = sum(tx .* windows(:, j));
            end
        end
        ds = zeros(NX, NW);

        for i = 1:NW
            c = conv(pwr0, windows(:, i));
            ds(:, i) = c(1 + lookahead:NX + lookahead);
        end

        y = zeros(size(x));

        cs_2_o_1 = cs(:, 2) ./ cs(:, 1);
        x_2_o_1 = ds(:, 2) ./ ds(:, 1);

        assert(NW == 2);
        for i = 1:NX
            [Y, I] = min(abs(cs_2_o_1 - x_2_o_1(i)));
            % assert(I ~= 1 && I ~= length(cs_2_o_1));
            y(i) = ds(i, 1) / cs(I, 1);
        end
    case 4
        num_splits = 10;
        splits = round(linspace(1, wlen, num_splits))';
        pwr0_padded = [zeros(wlen, 1); pwr0; zeros(wlen, 1)];
        wlen_half = (wlen - 1) / 2;
        middle_bins = zeros(num_splits, 1);
        Es = zeros(num_splits, 1);
        for i = 1:NX
            pwr0_segment = pwr0_padded(i - wlen_half + wlen:i + wlen_half + wlen);
            if mod(i, 100) == 0
                printf("i: %d/%d\n", i, NX);
            end
            for j = 1:num_splits
                [xhat, E] = fit_v_line(pow2db(pwr0_segment), w1, splits(j));
                middle_bins(j) = xhat(wlen_half + 1);
                Es(j) = E;
            end
            [Y, I] = min(Es);
            y(i) = db2pow(middle_bins(I));
        end
    case 5
        pwr0_padded = [zeros(wlen, 1); pwr0; zeros(wlen, 1)];
        wlen_half = (wlen - 1) / 2;
        for i = 1:NX
            if mod(i, 100) == 0
                printf("i: %d/%d\n", i, NX);
            end
            pwr0_segment = pwr0_padded(i - wlen_half + wlen:i + wlen_half + wlen);
            [xhat, E] = fit_line(pow2db(pwr0_segment), w1);
            y(i) = db2pow(xhat(wlen_half + 1));
        end
    case 6
        pwr0_padded = [pwr0(1) * ones(wlen, 1); pwr0; pwr0(end) * ones(wlen, 1)];
        wlen_half = (wlen - 1) / 2;
        for i = 1:NX
            if mod(i, 100) == 0
                printf("i: %d/%d\n", i, NX);
            end
            pwr0_segment = pwr0_padded(i - wlen_half + wlen:i + wlen_half + wlen);
            [a, b, E] = fit_line_l1(pow2db(pwr0_segment), w1);
            y(i) = db2pow(a * wlen_half + b);
        end
    case 8
        [b, a] = butter(1, 4 * 2/wlen);
        y1 = filter(b, a, pwr0);
        [b, a] = butter(1, 2/wlen);
        assert(a(1) == 1);
        for i = 2:NX
            y(i) = b(1) * y1(i) + b(2) * y1(i - 1) - a(2) * y(i - 1);
            if y(i) < y1(i)
                y(i) = y1(i);
            end
        end
    case 9
        [b, a] = butter(1, 2/wlen);
        assert(a(1) == 1);
        for i = 2:NX
            y(i) = b(1) * pwr0(i) + b(2) * pwr0(i - 1) - a(2) * y(i - 1);
            if y(i) < pwr0(i)
                y(i) = pwr0(i);
            end
        end
    case 10
        [b, a] = critdamplp2(4 * 2/wlen);
        y1 = filter(b, a, pwr0);
        g_h_R2 = statevartpt2(2/wlen, 0.5);
        s = [0 0];
        for i = 1:NX
            [y(i), s] = filter_statevartpt2(g_h_R2, y1(i), s);
            if y(i) < y1(i)
                y(i) = y1(i);
                s(2) = y(i);
            end
        end
    case 11
        g_h_R2 = statevartpt2(2/wlen, 0.5);
        s = [0 0];
        for i = 1:NX
            [y(i), s] = filter_statevartpt2(g_h_R2, pwr0(i), s);
            if y(i) < pwr0(i)
                y(i) = pwr0(i);
                s(2) = y(i);
            end
        end
    otherwise
        assert(false);
end

endfunction
