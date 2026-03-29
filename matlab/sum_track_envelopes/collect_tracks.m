% Collect and analyze audio files of a multitrack arrangement from a directory.
%
% The function:
%
% - enumerates all the files in the directory `dir_path`
% - for each file, attempts to read as an audio file, ignores those which aren't audio files.
% - prints a line for each file containing the path, the sampling rate and the length in seconds
% - if not all files has the exact same sampling rate, prints an error and exists
% - otherwise, prints the minimum of the lengths and returns the list of file names and the minimum duration
%
function [track_files, min_duration_sec] = collect_tracks(dir_path)

entries = dir(dir_path);
track_files = {};
sample_rates = [];
durations_sec = [];

for i = 1:length(entries)
    if entries(i).isdir
        continue
    end
    fpath = fullfile(dir_path, entries(i).name);
    try
        info = audioinfo(fpath);
    catch
        continue
    end
    fs = info.SampleRate;
    dur = info.TotalSamples / fs;
    printf('%s  fs=%d Hz  ch=%d  duration=%.3f s\n', fpath, fs, info.NumChannels, dur);
    track_files{end+1} = fpath;
    sample_rates(end+1) = fs;
    durations_sec(end+1) = dur;
end

if isempty(track_files)
    min_duration_sec = 0;
    return
end

if any(sample_rates ~= sample_rates(1))
    printf('Error: not all files have the same sampling rate.\n');
    exit(1);
end

min_duration_sec = min(durations_sec);
printf('Minimum duration: %.3f s\n', min_duration_sec);

endfunction
