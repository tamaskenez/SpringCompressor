#!/bin/bash -e

if [[ $(uname -s) == "Darwin" ]]; then
	cppstd=gnu23
else
	cppstd=23
fi

for build_type in Debug Release; do
	conan install conanfile.txt \
		-b missing \
		-pr:b default \
		-of _d/cmake \
		-s build_type=${build_type} \
		-s compiler.cppstd=${cppstd}
done
