# Third-party provenance

## Reference HomebrewUpdate project

VitaHomebrewUpdate is an independent behavioral reimplementation inspired by
the reference HomebrewUpdate project. The reference source code was not
available and was not copied or used. The implementation was recreated from
visual and on-device observation of behavior and publicly visible interfaces
and artifacts, then independently extended. We respectfully acknowledge the
reference project for demonstrating the LiveArea homebrew-update workflow.
This repository is not affiliated with or endorsed by that project and does
not claim exact internal equivalence.

## PKGj background-download adapter

`common/bgdl.c` is a C adaptation of PKGj's `src/bgdl.cpp` native
background-download adapter:

- Project: <https://github.com/blastrock/pkgj>
- Copyright 2018-2019 Philippe Daouadi
- Copyright 2019-2020 Asakura Reiko
- License: BSD-2-Clause

The reverse-engineering credit retained by PKGj names dots_tb, CelesteBlue123,
SilicaDevs, possvkey, and the NPS Team.

## BearSSL

TLS and certificate verification use BearSSL 0.6:

- Project: <https://www.bearssl.org/>
- Copyright 2016 Thomas Pornin
- License: MIT

The complete upstream license is retained at
`third_party/BearSSL-0.6/LICENSE.txt`.

No repository-wide license has been granted beyond these component licenses.
