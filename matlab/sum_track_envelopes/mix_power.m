function [mix, fs] = mix_power(paths, duration_sec)

fs = 0;
mix = [];
had_stereo = false;

for i = 1:length(paths)
    [data, file_fs] = audioread(paths{i});
    num_channels = size(data, 2);

    if num_channels < 1 || num_channels > 2
        printf('Error: %s has %d channels, only 1 or 2 are supported.\n', paths{i}, num_channels);
        exit(1);
    end

    if fs == 0
        fs = file_fs;
    elseif file_fs ~= fs
        printf('Error: sample rate mismatch: %s has %d Hz, expected %d Hz.\n', paths{i}, file_fs, fs);
        exit(1);
    end

    num_samples = min(size(data, 1), round(duration_sec * fs));
    data = data(1:num_samples, :);

    % Expand mono to stereo
    if num_channels == 1
        data = power_envelope(data, fs);
        data = [data, data];
    else
        had_stereo = true;
        data = [power_envelope(data(:, 1), fs) power_envelope(data(:, 2), fs)];
    end

    if isempty(mix)
        mix = data;
    else
        n = min(size(mix, 1), size(data, 1));
        mix = mix(1:n, :) + data(1:n, :);
    end
end

if ~had_stereo
    mix = mix(:, 1);
end

endfunction
