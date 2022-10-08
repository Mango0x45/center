# center

`center(1)` is a very basic UNIX utility to… well… center text.  The
documentation for the tool can be found [here][1].  If you would like to
contribute to the project, a list of tasks that need completing can be found in
the [TODO][2] file in the repo.  This utility should be as simple as possible,
accepting input from stdin and printing output to stdout.  This should be the
best utility possible to center text.

## Installation

If you're on an arch-based Linux distribution, then you can install `center`
from the aur:

```sh
paru -S center-git
```

Otherwise if you would like to build from source, simply run:

```sh
git clone git://git.thomasvoss.com/center.git
cd center
make
make install
```

[1]: https://thomasvoss.com/man/center.1.html
[2]: https://git.thomasvoss.com/center/tree/TODO
