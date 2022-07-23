{ pkgs ? import <nixpkgs> {} }:

with pkgs;

mkShell {
  buildInputs = [
# apt-get install build-essential libpulse-dev libasound2-dev libx11-dev libxext-dev libxinerama-dev libusb-1.0-0-dev libudev-dev
libglvnd xorg.libXinerama xorg.libXext xorg.libX11 xorg.libXtst xorg.libXi alsa-lib libpulseaudio libusb1
pkg-config
  ];
}
