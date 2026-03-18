% Return the possible ks such that M and k are relative primes
% and T0 < M/k < T0+1
function ks = possible_ks(T0, M)
    ks = [];
    for k = floor(M/(T0+1))+1:ceil(M/T0)-1
        if gcd(M, k) == 1
            ks = [ks k];
        end
    end
end
