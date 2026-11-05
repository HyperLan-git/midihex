{
    description = "Midihex development environment";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    };

    outputs = {self, nixpkgs, ...}:
    let
        system = "x86_64-linux";
        pkgs = import nixpkgs { inherit system; };

        imgui-src = pkgs.fetchFromGitHub {
            owner = "ocornut";
            repo = "imgui";
            tag = "v1.93.0-docking";
            hash = "sha256-NCD0li4ECPSFhhU0egSWMiYNBKSJDoB80rSOUiRAQ6c=";
        };
    in
    {
        devShells.${system}.default = pkgs.mkShell {
            name = "midihex dev";
            buildInputs = with pkgs; [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw3
            ];

            IMGUI_DIR = "${imgui-src}";

            shellHook = ''
                echo "Welcome to the devShell!"
            '';
        };
    };
}