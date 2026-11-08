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
        packages.${system}.default = pkgs.stdenv.mkDerivation {
            pname = "midihex";
            version = "0.1.0";
            src = ./.;

            IMGUI_DIR = "${imgui-src}";

            buildInputs = with pkgs; [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw3
            ];
            nativeBuildInputs = with pkgs; [ pkg-config ];

            makeFlags = [ "PREFIX=$(out)" ];
        };
        devShells.${system}.default = pkgs.mkShell {
            name = "midihex dev";
            buildInputs = with pkgs; [
                pkgs.gnumake
                pkgs.gcc

                pkgs.glfw-wayland
            ];

            IMGUI_DIR = "${imgui-src}";

            shellHook = ''
                export LD_LIBRARY_PATH=${pkgs.lib.makeLibraryPath [
                    pkgs.libEGL
                    pkgs.wayland
                ]}:$LD_LIBRARY_PATH
                echo "Welcome to the devShell!"
                echo "Use make to compile the project and make DEBUG=1 to add asan"
            '';
        };
    };
}