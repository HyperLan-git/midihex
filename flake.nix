{
    description = "Midihex development environment";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    };

    outputs = {self, nixpkgs, ...}:
    let
        system = "x86_64-linux";
        pkgs = import nixpkgs { inherit system; };
    in
    {
        devShells.${system}.default = pkgs.mkShell {
            name = "midihex dev";
            buildInputs = with pkgs; [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw3
                pkgs.imgui
            ];
        };
    };
}