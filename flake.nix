{
  description = "openh264";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    zig = {
      url = "github:mitchellh/zig-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    zls = {
      url = "github:zigtools/zls";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { self, nixpkgs, ... }@inputs:
    inputs.flake-utils.lib.eachSystem (builtins.attrNames inputs.zig.packages) (
      system:
      let
        overlays = [
          (final: prev: { zigpkgs = inputs.zig.packages.${prev.system}; })
          (final: prev: { zlspkgs = inputs.zls.packages.${prev.system}; })
        ];
        pkgs = import nixpkgs { inherit overlays system; };
      in
      {
        devShells.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
          packages = with pkgs; [
            zigpkgs.default
            zls
            nasm
          ];
        };
      }
    );
}
