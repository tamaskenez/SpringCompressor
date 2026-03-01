format long

function print_as_std_vector(x)
    assert(isrow(x));
    printf("    std::vector({");
    printf("%.15e,", x);
    printf("}),\n");
endfunction

printf("const std::vector<std::vector<double>> butter_test_data = {\n");
for N = 1:12
    order = floor(rand(1) * 4) + 1;
    lohi = sort([rand() rand()]);
    printf("    std::vector({%.1f, %.15e, %.15e}),\n", order, lohi(1), lohi(2));
    [b, a] = butter(order, lohi(1));
    print_as_std_vector([b a]);
    [b, a] = butter(order, lohi(1), 'high');
    print_as_std_vector([b a]);
    [b, a] = butter(order, lohi, 'bandpass');
    print_as_std_vector([b a]);
    [b, a] = butter(order, lohi, 'stop');
    print_as_std_vector([b a]);
end
printf("};\n");
