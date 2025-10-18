with import <nixpkgs> { };
mkShell {
  packages = [
    platformio
    platformio-core
    avrdude
    openocd
    python3
  ];
}
