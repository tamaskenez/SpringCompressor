% With an upfront RMS envelope detection we still might need to filter the envelope itself.
% We can filter the envelope
% - as mag, as pow, as db
% - pre or post transfer
% We try out every possibilities here.



fs = 48000;
ns = [0:fs-1]';
fe = 100;
ff = 20;
A = db2mag(-3);
ratio = 8;
e0_pow = 1 + A * sin(2*pi*fe/fs*ns);
plot(ns, e0_pow);
[b, a] = critdamplp2(ff/fs*2);

for post_tc = [false true]
    for filterstage = [1 2 3]
        if post_tc
            switch filterstage
                case 1 % mag
                    rms_db = mag2db(1 - filter(b, a, 1 - sqrt(e0_pow)));
                case 2 % pow
                    rms_db = pow2db(1 - filter(b, a, 1 - e0_pow));
                case 3 % db
                    rms_db = filter(b, a, pow2db(e0_pow));
                otherwise
                    assert(false)
            end
            gr_db = rms_db / ratio - rms_db;
        else
            e0_db = pow2db(e0_pow);
            gr_db = e0_db / ratio - e0_db;
            switch filterstage
                case 1 % mag
                    gr_db = mag2db(1 - filter(b, a, 1 - db2mag(gr_db)));
                case 2 % pow
                    gr_db = pow2db(1 - filter(b, a, 1 - db2pow(gr_db)));
                case 3 % db
                    gr_db = filter(b, a, gr_db);
                otherwise
                    assert(false)
            end
        end
        gr_mag = db2mag(gr_db);
        fgr = mag2db(abs(fft(gr_mag)));
        fgr = fgr - fgr(1);
        R = 1:1000;
        plot(R-1, fgr(R)), grid, axis([ 0 1000 -120 0]);
        printf("post_tc: %d, filterstage: %d\n", post_tc, filterstage);
        pause
    end
end
