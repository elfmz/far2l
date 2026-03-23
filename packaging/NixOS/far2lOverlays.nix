/*
  custom buid far2l package with python and arclite enabled, where you could personally select revisions
  Also added some postinstall fixes, for example for linking 7z custom build 7z.so
*/
{
  pkgs,
  lib,
  ...
}:
let
  fetchFromGitHub = pkgs.fetchFromGitHub;
in
{
  nixpkgs.overlays = [
    (final: prev: {
      # Modified 7zip package with shared library support
      _7z-far = prev.stdenv.mkDerivation rec {
        pname = "_7z-far";
        version = "26.00";

        src = fetchFromGitHub {
          owner = "ip7z";
          repo = "7zip";
          rev = "839151eaaad24771892afaae6bac690e31e58384";
          sha256 = "sha256-B+piugjEI7+8ILTuDitADY8XseltvV0lYIVecXGib7s=";
        };

        nativeBuildInputs = [ prev.gcc ];

        buildPhase = ''
          make -C CPP/7zip/Bundles/Format7zF -f ../../cmpl_gcc.mak
        '';

        installPhase = ''
          mkdir -p $out/lib
          cp -r CPP/7zip/Bundles/Format7zF/b/g/* $out/lib/
        '';

        meta = with lib; {
          description = "7z format plugin from ip7z/7zip";
          homepage = "https://github.com/ip7z/7zip";
          license = licenses.lgpl21Plus;
        };
      };
    })

    (final: prev: {
      # Custom build of far2l
      far2l = prev.stdenv.mkDerivation rec {
        pname = "far2l";
        version = "2.8.0-483dea0";

        #separateDebugInfo = true;

        src = fetchFromGitHub {
          owner = "elfmz";
          repo = "far2l";

          rev = "483dea0818c68a95c054f313e099f8b99b722a3d";
          sha256 = "sha256-LP+agJrYxjH6vLAg6cJTU4/9jYGF9iaZzxA7hozDKNY=";

        };

        patches = [
          # WIP SDL Backend #3261
          (prev.fetchpatch {
            url = "https://github.com/elfmz/far2l/commit/6f2bd38390519cb86e028eae6c1397fb8fdec406.patch";
            sha256 = "sha256-IGiqy33UwsLLD3oYAGiGgNFHoctUMny9J6T4GfJ3auA=";
          })

          # Git Gutter plugin #3248
          (prev.fetchpatch {
            url = "https://github.com/elfmz/far2l/commit/ce003f2693336d04d8c7237002ecca85a617e0d2.patch";
            sha256 = "sha256-oqGbecLeEj1QbM+LCVAY0Y9xNwrcvY4tM9G7sN6C0cM=";
          })

          # Avoid redrawing background shadow on progress message updates #3299
          (prev.fetchpatch {
            url = "https://github.com/elfmz/far2l/commit/ff304407d6e585c8469067926fec8999f3850861.patch";
            sha256 = "sha256-GfD8FK/5GOpJK7xti3vGu7BCYwkXorLU8QAApprJqnM=";
          })

          # Shift+Ctrl+P to save text screen dump with all attrs to ~/f2l_screen.dump in wx backend #3304
          (prev.fetchpatch {
            url = "https://github.com/elfmz/far2l/commit/652112f31f9e7b1f83d6d53a5230d89afc1df66f.patch";
            sha256 = "sha256-PEWHBQh1tDPmVvnUerVXOYjk+Qv9bc/V2ot8sMbw4Us=";
          })

        ];

        postPatch = ''
          chmod +x far2l/bootstrap/*.sh
          patchShebangs far2l/bootstrap/view.sh
        '';

        nativeBuildInputs = [
          prev.cmake
          prev.ninja
          prev.pkg-config
          prev.perl
          prev.makeWrapper
        ];

        buildInputs = [
          # SDL backend deps
          /*
            prev.SDL2
            prev.harfbuzz
            prev.fontconfig
            prev.libxft
          */
          # ----
          prev.libx11
          prev.wxwidgets_3_2
          prev.libuchardet
          prev.spdlog
          prev.libxml2
          prev.libarchive
          prev.pcre
          prev.openssl
          prev.libssh
          prev.libnfs
          prev.neon
          prev.aws-sdk-cpp
          prev.imagemagick
          prev.ffmpeg
          final._7z-far
        ]
        ++ lib.optional (!prev.stdenv.hostPlatform.isDarwin) prev.samba;

        cmakeFlags = [
          "-DTTYX=ON"
          "-DUSEWX=ON"
          "-DUSEUCD=ON"
          "-DCOLORER=ON"
          "-DMULTIARC=ON"
          "-DNETROCKS=ON"
          "-DAWS_S3=ON"
          "-DPYTHON=OFF"
          "-DARCLITE=ON"
          #"-DFAR2L_GUI_BACKEND=SDL"
        ];

        postInstall =
          let
            archiveTools = with prev; [
              unrar
              unzip
              zip
              xz
              gzip
              bzip2
              gnutar
              final._7z-far
            ];
          in
          ''
            # Wrap archivers paths to program bin
            wrapProgram $out/bin/far2l \
              --prefix PATH : ${lib.makeBinPath archiveTools}

            # Link p7z lib to far plugin arclite home
            echo "Linking 7zzz libraries..."
            mkdir -p $out/lib/far2l/Plugins/arclite/plug/
            for file in ${final._7z-far}/lib/*; do
              ln -sf "$file" "$out/lib/far2l/Plugins/arclite/plug/"
            done
          '';

        meta = with lib; {
          description = "Linux port of FAR Manager v2";
          homepage = "https://github.com/elfmz/far2l";
          license = licenses.gpl2Only;
          maintainers = with maintainers; [ tempergate ];
        };
      };
    })
  ];

  environment.systemPackages = with pkgs; [
    far2l
  ];
}
