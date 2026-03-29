% Recalculates 2nd order state variable TPT function state on coefficient change
function s_after = convert_statevartpt2_state(ghr2_before, ghr2_after, s_before)
    ga = ghr2_before(1);
    gb = ghr2_after(1);

    s_after = [s_before(1) * (ga / gb) s_before(2)];
end
