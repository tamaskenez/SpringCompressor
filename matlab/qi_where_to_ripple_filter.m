% We have a low level A1 and high level A2 sounds

A1 = 1;
A2 = 10;

% Ripple from RMS detector is proportional to the amplitude. The power signal vibrates between L and H

power_ripple_amplitude_db = -12;

PRA1 = A1^2 * db2mag(power_ripple_amplitude_db);
L1 = A1^2 - PRA1;
H1 = A1^2 + PRA1;

PRA2 = A2^2 * db2mag(power_ripple_amplitude_db);
L2 = A2^2 - PRA2;
H2 = A2^2 + PRA2;

% This ripple can be filtered in power domain
ripple_rejection_db = -6;

FPRA1 = (H1 - L1)/2*db2mag(ripple_rejection_db);
FPRA2 = (H2 - L2)/2*db2mag(ripple_rejection_db);

% Which results in a modulating signal

ratio = 4000;

L1_db = pow2db(A1^2 - FPRA1);
H1_db = pow2db(A1^2 + FPRA1);

L2_db = pow2db(A2^2 - FPRA2);
H2_db = pow2db(A2^2 + FPRA2);

th = -10;
assert(th < L1_db && th < L2_db);

gr1_lo = db2mag((L1_db - th)/ratio + th - L1_db);
gr1_hi = db2mag((H1_db - th)/ratio + th - H1_db);
gr2_lo = db2mag((L2_db - th)/ratio + th - L2_db);
gr2_hi = db2mag((H2_db - th)/ratio + th - H2_db);

% Modulation amplitude:

mod_amp_1 = (gr1_lo - gr1_hi) / 2
mod_amp_2 = (gr2_lo - gr2_hi) / 2


% What if filtering takes place in db domain before TC
LH1_db = pow2db([L1 H1]);
LH2_db = pow2db([L2 H2]);
DBRA1 = diff(LH1_db)/2 * db2mag(ripple_rejection_db);
DBRA2 = diff(LH2_db)/2 * db2mag(ripple_rejection_db);
FLH1_db = mean(LH1_db) + [-1 1] * DBRA1;
FLH2_db = mean(LH2_db) + [-1 1] * DBRA2;
gr1_lh = db2mag((FLH1_db - th)/ratio + th + FLH1_db);
gr2_lh = db2mag((FLH2_db - th)/ratio + th + FLH2_db);

mod_amp1 = diff(gr1_lh)/2
mod_amp2 = diff(gr2_lh)/2



% What if filtering takes place in db domain after TC
LH1_db = pow2db([L1 H1]);
LH2_db = pow2db([L2 H2]);
LH1_db = (LH1_db - th)/ratio + th + LH1_db;
LH2_db = (LH2_db - th)/ratio + th + LH2_db;
DBRA1 = diff(LH1_db)/2 * db2mag(ripple_rejection_db);
DBRA2 = diff(LH2_db)/2 * db2mag(ripple_rejection_db);
FLH1_db = mean(LH1_db) + [-1 1] * DBRA1;
FLH2_db = mean(LH2_db) + [-1 1] * DBRA2;

mod_amp1 = diff(db2mag(FLH1_db))/2
mod_amp2 = diff(db2mag(FLH2_db))/2
