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
            buildInputs = with pkgs; [
                hello
            ];
            shellHook = ''
                echo "Welcome to the devShell!"
            '';
        };
    };
}