{
    description = "Midihex development environment";

    # Nixpkgs / NixOS version to use.
    inputs.nixpkgs.url = "https://channels.nixos.org/nixos-26.05/nixexprs.tar.zst";

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