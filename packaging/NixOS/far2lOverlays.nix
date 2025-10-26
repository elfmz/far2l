/*
  custom buid far2l package with python and arclite enabled, where you could personally select revisions
  Also added some postinstall fixes, for example for linking 7z custom build 7z.so
*/
{
  config,
  inputs,
  pkgs,
  stdenv,
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
        version = "25.01";

        src = fetchFromGitHub {
          owner = "ip7z";
          repo = "7zip";
          rev = "5e96a8279489832924056b1fa82f29d5837c9469";
          sha256 = "sha256-uGair9iRO4eOBWPqLmEAvUTUCeZ3PDX2s01/waYLTwY=";
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
        version = "2.7.0";

        src = fetchFromGitHub {
          owner = "elfmz";
          repo = "far2l";
          rev = "b4f641c8c99c62e37e5505302ddc8364b132bdd8";
          sha256 = "sha256-LdZp8NyUGtny3IzqRWFMVsIWKuzN8RRnaGDZgSbK7Kw=";
        };

        nativeBuildInputs = [
          prev.cmake
          prev.ninja
          prev.pkg-config
          prev.perl
          prev.makeWrapper
          prev.python3
        ];

        buildInputs = [
          prev.xorg.libX11
          prev.wxGTK32
          prev.libuchardet
          prev.spdlog
          prev.libxml2
          prev.libarchive
          prev.pcre
          prev.openssl
          prev.libssh
          prev.libnfs
          prev.neon
          final._7z-far
        ]
        ++ lib.optional (!prev.stdenv.hostPlatform.isDarwin) prev.samba
        ++ (with prev.python3Packages; [
          python
          cffi
        ]);

        postPatch = ''
          chmod +x python/src/*.sh
          chmod +x far2l/bootstrap/*.sh
          patchShebangs python/src/prebuild.sh
          patchShebangs python/src/build.sh
          patchShebangs far2l/bootstrap/view.sh
          mkdir -p build/python
          cp -r python/configs build/python/
        '';

        cmakeFlags = [
          "-DTTYX=ON"
          "-DUSEWX=ON"
          "-DUSEUCD=ON"
          "-DCOLORER=ON"
          "-DMULTIARC=ON"
          "-DNETROCKS=ON"
          "-DAWS_S3=OFF"
          "-DPYTHON=ON"
          "-DARCLITE=ON"
        ];

        preBuild = ''
          mkdir -p build/install/Plugins/python/plug
        '';

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
          description = "Linux port of FAR Manager v2 with ArchLite support";
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

