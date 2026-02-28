% Exploring IIR filterbank
	
min_hz = 20;
fs = 48000;
n_per_octave = 1;
q = 2^(1/n_per_octave);
fcs = min_hz / fs * 2;
while 1
	fnext = fcs(end) * q;
	if fnext*q >= 1, break; end
		fcs = [fcs; fnext];
end
fcs
	B = length(fcs);

clf;
if 0
sum_hh = []
for fc = fcs'
	assert(isscalar(fc));
	[b,a] = butter(1, [fc/q fc*q]);
	[h, w] = freqz(b, a);
	hh = abs(h).^2;
	if isempty(sum_hh)
		sum_hh = hh;
		semilogx(w/pi, pow2db(hh)), hold on;
		title(sprintf('%d per octave, order = 2 x 1', n_per_octave));
	else
		sum_hh = sum_hh + h;
		semilogx(w/pi, pow2db(hh));
	end
end
		semilogx(w/pi, pow2db(sum_hh), 'r');
end		


	% Test sound
		f_hz = 100;
		f = f_hz / fs * 2;
		[Y, I] = min(abs(fcs - f));
		N = round(fs / f_hz * 24);
		for inbetween = [0 1]
			if inbetween
				f = fcs(I);
			else
				f = sqrt(fcs(I) * fcs(I+1));
			end
			f_hz = f / 2 * fs;
			period = fs / f_hz;
			x = sin(2*pi*[0:N-1]'/period);
			total_power = zeros(N, 1);
			for b_ix = [1:B]
				fc = fcs(b_ix);
				% Bandpass
				[b,a] = butter(2, [fc/q fc*q]);
				band_power = filter(b, a, x).^2;
				% low-pass the band power
				[b,a] = butter(1, fc);
				smoothed_band_power = filter(b, a, band_power);
				total_power = total_power + smoothed_band_power;
			end
			if inbetween
				total_power_inbetween = total_power;
			else
				total_power_at = total_power;
			end
		end
	figure(1)
	period = 2 / fcs(I);
	plot([0:N-1]/period, pow2db(total_power_at), 'r', [0:N-1]/period, pow2db(total_power_inbetween), 'g'), grid;
	axis([0 7 -20 5]);
	
		