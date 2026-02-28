% Process kick-snare with RMS detector filterbank
	% instead of low-pass, with postprocess with Teager-Kaiser Energy Operator
	
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
			y = filter(b, a, x);
			% Discrete Teager-Kaiser
			psi = [0; y(2:end-1).^2 - y(1:end-2).*y(3:end); 0];
			% Convert to power
			band_power = psi ./ ((pi*fc).^2);
			% low-pass the band power
			total_power = total_power + band_power;
		end

		msecs = [0:N-1]/fs*1000;
	plot(...
		msecs, pow2db(bd.^2), 'k',...
		msecs, pow2db(total_power), 'r'...
		), grid;
	axis([0 msecs(end) -20 5]);
	
		