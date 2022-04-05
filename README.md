# [Riivolution](https://wiibrew.org/wiki/Riivolution) and [RawkSD](https://wiibrew.org/wiki/RawkSD)

Information archives:
- [Riivolution Wiki](https://aerialx.github.io/rvlution.net/wiki/Main_Page/)
- [RawkSD beta](https://www.japaneatahand.com/rawksd/beta.htm)

## Dependencies

- [devkitPPC and devkitARM](https://devkitpro.org/wiki/Getting_Started)
	- `dkp-pacman -S wii-dev ppc-portlibs wii-portlibs devkitARM`
- a build environment for your host with GNU Make, some coreutils, and a C++ compiler
	- on Windows, devkitPro supplies and requires this via [MSYS2](https://www.msys2.org/)
- Python 3.x and python-yaml or pyyaml (only for rawksd)
- `curl`, `unzip`, and i386 multilibs (for dollz3)
- `zip` (only for packaging the build result)

## Building

With the required dependencies installed, just run `make` in the project's directory:

```shell
make -j

# alternatively...
make launcher -j # Riivolution only
make rawksd -j # RawkSD only
make riifs # RiiFS server
```

See the [automated build scripts](./.github/workflows/build.yml) for reference and further examples.

### Docker

The [devkitpro/devkitppc](https://hub.docker.com/r/devkitpro/devkitppc) Docker image can be used:

```shell
docker run --interactive --tty --rm --mount type=bind,source=$PWD,destination=/mnt devkitpro/devkitppc

dpkg --add-architecture i386
apt-get update && apt-get install -y --no-install-recommends g++ libgcc1:i386 zlib1g:i386 python3 python3-yaml
# dkp-pacman -Syyu

make -C /mnt -j
```

#### Hate Docker?

Everyone loves systemd:

```shell
docker create --name devkitppc devkitpro/devkitppc &&
  docker export devkitppc | machinectl import-tar - devkitppc
docker container rm devkitppc
sudo systemd-nspawn -a -M devkitppc /bin/sh -lc 'dpkg --add-architecture i386 && apt-get update'
sudo systemd-nspawn -a -M devkitppc /usr/bin/apt-get install -y --no-install-recommends g++ libgcc1:i386 zlib1g:i386 python3 python3-yaml

sudo systemd-nspawn -a -M devkitppc --bind $PWD:/mnt --chdir /mnt --bind-ro /etc/passwd -u $(whoami) -E PATH=/usr/bin -E MAKEFLAGS=-j /bin/bash -li
:; make
```

### Debian / Ubuntu

Following [the example from the devkitpro docker image](https://github.com/devkitPro/docker/blob/master/toolchain-base/Dockerfile)...

```shell
dpkg --add-architecture i386
apt-get update
apt-get install --no-install-recommends curl gdebi-core zip unzip libgcc1:i386 zlib1g:i386 python3 python3-yaml g++
curl -O https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.amd64.deb
gdebi -n devkitpro-pacman.amd64.deb
dkp-pacman -Sy wii-dev ppc-portlibs wii-portlibs devkitARM

make -j
```

### Windows

devkitPro needs an MSYS2 environment, which uses pacman for package management:

```shell
pacman -S zip unzip python python-yaml

make -j
```

## Debugging

Set [DEBUG_NET](launcher/include/init.h) to receive debug output remotely:

```shell
socat TCP4-LISTEN:51016,fork,reuseaddr STDOUT
```
