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
      # Custom build of far2l
      far2l = prev.stdenv.mkDerivation rec {
        pname = "far2l";
        version = "2.9.1-337e578";

        #separateDebugInfo = true;

        src = fetchFromGitHub {
          owner = "elfmz";
          repo = "far2l";

          rev = "337e57830905a1cbd02bd3d9f54ed19b643699af";
          sha256 = "sha256-AzwG4w++mu/5XeURQ86xu6HfXpFlc5VuK9CbMx6kPBU=";
        };

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
          # we need it anyway
          prev.bash
          # SDL GUI (testing)
          prev.SDL2
          prev.harfbuzz
          prev.fontconfig
          prev.libxft
          # WX GUI
          prev.libx11
          prev.wxwidgets_3_2
          # Colorer and formattes
          prev.libuchardet
          prev.spdlog
          prev.libxml2
          prev.pcre
          # Netrocks
          prev.openssl
          prev.libssh
          prev.libnfs
          prev.neon
          prev.gnutls
          prev.libtasn1
          prev.p11-kit
          # ImageViewer
          prev.imagemagick
          prev.ffmpeg
          # ADB
          prev.android-tools
          # MTP
          prev.libmtp
          prev.libusb1
          # GIT
          prev.git
          # archivers
          prev.libarchive
          prev._7zip-zstd-rar
          prev._7zz
        ]
        ++ lib.optional (!prev.stdenv.hostPlatform.isDarwin) prev.samba;

        cmakeFlags = [
          "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
          # Pluggins setup
          "-DADB=ON"
          "-DALIGN=ON"
          "-DARCLITE=ON"
          "-DAUTOWRAP=ON"
          "-DCALC=ON"
          "-DCOLORER=ON"
          "-DCOMPARE=ON"
          "-DDRAWLINE=ON"
          "-DEDITCASE=ON"
          "-DEDITORCOMP=ON"
          "-DEDSORT=ON"
          "-DFARFTP=ON"
          "-DFILECASE=ON"
          "-DGITGUTTER=ON"
          "-DHEXITOR=ON"
          "-DIMAGEVIEWER=ON"
          "-DINCSRCH=ON"
          "-DINSIDE=ON"
          "-DMEMO=ON"
          "-DMTP=ON"
          "-DMULTIARC=ON"
          "-DNETROCKS=ON"
          "-DOPENWITH=ON"
          "-DSIMPLEINDENT=ON"
          "-DTMPPANEL=ON"
          "-DTRUNCATE=ON"
          # Python pluggins support
          "-DPYTHON=OFF"
          # Backend setup
          "-DUSESDL=YES"
          "-DUSEWX=YES"
          "-DTTYX=YES"
          # Libruaries setup
          "-DMTP_SYSTEM_LIBUSB=ON"
          "-DMTP_SYSTEM_LIBMTP=ON"
        ];

        postInstall =
          let
            farTools = with prev; [
              # archivers
              unrar
              unzip
              zip
              xz
              gzip
              bzip2
              gnutar
              _7zip-zstd-rar
              _7zz
              # cli tools
              git
              android-tools
              libmtp
              libusb1
              bash
            ];
          in
          ''
            # Wrap tools paths to program bin
            wrapProgram $out/bin/far2l \
              --prefix PATH : ${lib.makeBinPath farTools}

            # Link p7z lib to far plugin arclite home
            echo "Linking 7zzz libraries..."
            mkdir -p $out/lib/far2l/Plugins/arclite/plug/
            for file in ${prev._7zz.lib}/lib/*; do
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
