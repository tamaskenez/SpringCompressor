% Return the possible M such that M and k are relative primes
% and T0 < M/k < T0+1
function Ms = possible_Ms(T0, k)
    Ms = [];
    for M = T0*k+1:(T0+1)*k-1
        if gcd(M, k) == 1
            Ms = [Ms M];
        end
    end
end
