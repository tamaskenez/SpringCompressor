% Process kick-snare with RMS detector filterbank
	
[bd, fs] = audioread('bd.wav');
min_hz = 20;
n_per_octave = 1;
q = 2^(1/n_per_octave);
fcs = min_hz / fs * 2;
while 1
	fnext = fcs(end) * q;
	if fnext*q >= 1, break; end
		fcs = [fcs; fnext];
end
B = length(fcs);

	% Test sound
		N = length(bd)
		x = bd;
		total_power = zeros(N, 1);
		for b_ix = [1:B]
			fc = fcs(b_ix);
			% Bandpass
			[b,a] = butter(1, [fc/q fc*q]);
			band_power = filter(b, a, x).^2;
			% low-pass the band power
			[b,a] = butter(2, fc/2);
			smoothed_band_power = filter(b, a, band_power);
			total_power = total_power + smoothed_band_power;
		end

		msecs = [0:N-1]/fs*1000;
	plot(...
		msecs, pow2db(bd.^2), 'k',...
		msecs, pow2db(total_power), 'r'...
		), grid;
	axis([0 msecs(end) -20 5]);
	
		