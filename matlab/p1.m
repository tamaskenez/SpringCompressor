pkg load signal
	
[x, fs] = audioread('acguitar.wav');
n = [0:length(x)-1]';

xx=x.^2;
desired_xx = xx;
knee = db2pow(-60);
as_db = pow2db((desired_xx(desired_xx > knee) / knee));
as_db = as_db / 4;
as_pow = db2pow(as_db);
desired_xx(desired_xx > knee) =  as_pow * knee;


gain2 = desired_xx ./ xx;
gain2(gain2>1)=1;
gain2(isnan(gain2))=1;
gain = sqrt(gain2);
plot(n,gain);
[b,a] = butter(2,20/(fs/2));
fgain = filter(b, a, gain);
plot(n,mag2db(fgain));
gx = fgain .* x;
plot(n,x,n,gx);



if 0
player = audioplayer(x, fs);
playblocking(player);
player = audioplayer(3*y, fs);
playblocking(player);
endif
	