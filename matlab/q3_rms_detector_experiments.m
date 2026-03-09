clear all
[y, fs] = audioread('bd.wav');
f = figure(1);

function redraw_y(freq, fs, y, order)
    order = round(order);           % butter needs an integer
    order = max(order, 1);          % safety clamp
    N = length(y);
    ns = (0:N-1)';
    [b, a] = butter(order, freq/fs*2);


    wn = 2 * pi * freq;
    num = [wn^2];
    den = [1, 2*wn, wn^2];
    H = tf(num, den);
    Hd = c2d(H, 1/fs, 'tustin');
    [num_d, den_d] = tfdata(Hd, 'v');

    plot(ns, y, ns, sqrt(filter(b, a, y.^2)),ns, sqrt(filter(num_d, den_d, y.^2))), grid;
    fprintf("order: %d, freq: %f\n", order, freq);
end

% Create both sliders first, with dummy callbacks
o = uicontrol(f, 'Style', 'slider', ...
    'Min', 1, 'Max', 6, 'Value', 1, ...
    'Position', [20 20 60 20]);

sl = uicontrol(f, 'Style', 'slider', ...
    'Min', 1, 'Max', 100, 'Value', 50, ...
    'Position', [120 20 60 20]);

% Now assign callbacks — both handles exist, so closures capture valid objects
set(o,  'Callback', @(src, ~) redraw_y(get(sl,'Value'), fs, y, get(src,'Value')));
set(sl, 'Callback', @(src, ~) redraw_y(get(src,'Value'), fs, y, get(o,'Value')));

redraw_y(get(sl,'Value'), fs, y, get(o,'Value'));
