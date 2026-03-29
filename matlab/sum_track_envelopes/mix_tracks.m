% Read audio files and sum them.
%
% The `paths` parameter contains a list of audio files (tracks). The function reads all the files
% and sums their first `duration_sec` seconds and returns the mix and the sampling rate.
%
% The function accepts only 1 or 2 channel tracks. It exits with an error if it encounters anything else.
% It always mixes in stereo (2 channels). Mono tracks are simply duplicated to left and right.
% At the end of the function if there were at least one 2-channel track, the stereo mix is returned.
% If all tracks were mono, only the first channel is returned.
% All the files must have the same sample rate, otherwise the function exits with an error message.
%
% The `command` parameter is optional. The only permitted value is the string 'play' in which case
% the function plays the final mix with the `audioplayer`.
%
function [mix, fs] = mix_tracks(paths, duration_sec, command)

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
        data = [data, data];
    else
        had_stereo = true;
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

if nargin >= 3
    if ~strcmp(command, 'play')
        printf('Error: unknown command "%s".\n', command);
        exit(1);
    end
    M = max(max(abs(mix)));
    if M < 1, M = 1; end
    player = audioplayer(mix, fs);
    playblocking(player);
end

endfunction
