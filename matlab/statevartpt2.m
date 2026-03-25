% Second order state variable TPT filter coefficients, for now, only low-pass.
function g_h_R2 = statevartpt2(freq_hps, Q)
    a = 2.0 - 1.0 / Q^2;
    omega_ratio = sqrt((a + sqrt(a^2 + 4)) / 2);
    g = tan(pi * freq_hps / 2 / omega_ratio);
    R2 = 1 / Q;
    h = 1 / (1 + (R2 + g) * g);
    g_h_R2 = [g h R2];
end
