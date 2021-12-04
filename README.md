## Building

```shell
docker run --interactive --tty --rm --mount type=bind,source=$PWD,destination=/rawk devkitpro/devkitppc

dpkg --add-architecture i386 # for dollz3
apt-get update && apt-get install -y --no-install-recommends g++ libgcc1:i386 zlib1g:i386 python2 python-yaml
# optional: dkp-pacman -Syyu

make -C /rawk -j
```
