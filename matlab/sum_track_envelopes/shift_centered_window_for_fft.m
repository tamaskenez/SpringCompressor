function y = shift_centered_window_for_fft(w, N)
    w_is_row = isrow(w);
    if w_is_row
        w = w(:);
    end
    L = size(w, 1);
    assert(mod(L, 2) == 1);
    H = (L - 1) / 2;
    assert(H <= floor((N - 1) / 2));
    y = [w(H+1:end, :); zeros(N - L, size(w, 2)); w(1:H, :)];
    assert(size(y, 1) == N);
    if w_is_row
        y = y.';
    end
endfunction
