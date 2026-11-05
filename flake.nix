{
    description = "Midihex development environment";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    };

    outputs = {self, nixpkgs, ...}: {
        devShells.default = pkgs.mkShell {
            buildInputs = [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw3
                pkgs.imgui
            ];
        };
    };
}