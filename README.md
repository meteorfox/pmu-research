# pmu-research
Utilities to learn and explore CPU PMUs, and different libraries to interact with the PMUs.

## Installation

Pre-requisites:
* CMake 3.13+
* Recent GCC or Clang compiler with C11 and C++17 support

This project uses git submodules, make sure to pull submodules before running `cmake`.

```bash
git clone --recursive  https://github.com/meteorfox/pmu-research.git
cd pmu-research/
mkdir build && cd build/
cmake ..
```

## Usage

To use the tests that make use of libpfc library, load the 'pfc.ko' kernel module built under `build/third_party/libpfc/kmod`.

See [libpfc's documentation](https://github.com/obilaniu/libpfc#loading-the-kernel-module-pfcko
) for more information on how to load the kernel module.

### Simple test
```bash
$ ./hellopfc
Instructions Issued                  :           9000000213
Unhalted core cycles                 :           3000151518
Unhalted reference cycles            :           2684348892
uops_issued.any                      :           8000059100
uops_issued.any<1                    :           1001201011
uops_issued.any>=1                   :           2000021042
uops_issued.any>=2                   :           2000015375
...
```


## License
[MIT](https://choosealicense.com/licenses/mit/)