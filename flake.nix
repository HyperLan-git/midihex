{
    description = "Midihex development environment";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    };

    outputs = {nixpkgs, ...}: let
        system = "x86_64-linux";
        pkgs = import nixpkgs { inherit system; };

    in {
        devShells.${system} = {
            simple = pkgs.mkShell {
                packages = [ pkgs.midihex ];
                shellHook = ''
                    ${pkgs.midihex}/bin/hello
                '';
            };
        };
        default = pkgs.mkShell {
            packages = [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw3
                pkgs.imgui
            ];
        };
    };
}