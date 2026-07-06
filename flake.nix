{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      esp-dev,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;

          # esptool dependency
          config.permittedInsecurePackages = [
            "python3.13-ecdsa-0.19.1"
          ];
        };

        esp-pkgs = esp-dev.packages.${system};
      in
      {
        formatter = pkgs.nixfmt-tree;

        devShells.default = pkgs.mkShell {
          name = "snackyou-shell";
          packages =
            with pkgs;
            with esp-pkgs;
            [
              # API
              go
              air

              # Firmware (ESP-IDF 5.5.4)
              (esp-idf-full.override {
                rev = "v5.5.4";
                sha256 = "sha256-rItbBrwItkfJf8tKImAQsiXDR95sr0LqaM51gDZG/nI=";
              })

              # App / local testing
              python3

              # Deployment and repo tooling
              bun
              commitlint
              docker
              gcc
              gh
              git
              git-lfs
              prek
              prettier
              uv
            ];

          shellHook = ''
            # Keep normal Go/API work on the host toolchain. The ESP-IDF shell
            # adds Espressif cross-compilers to PATH, which breaks `go run`
            # when cgo is enabled.
            export CGO_ENABLED=0
          '';
        };
      }
    );

  nixConfig = {
    extra-substituters = [
      "https://spikonado.cachix.org"
    ];
    extra-trusted-public-keys = [
      "spikonado.cachix.org-1:MwA4hqRN0+DdP7/UnTn0yvJgVu65S1S0QVnAnsguev4="
    ];
  };
}
