/*
  custom buid far2l package with python and arclite enabled, where you could personally select revisions
  Also added some postinstall fixes, for example for linking p7zip 7z.so
*/
{
  config,
  inputs,
  pkgs,
  stdenv,
  ...
}:
{
  environment.systemPackages = [
    pkgs.far2l
  ];

  nixpkgs.overlays = [
    (final: prev: {
      #Modified p7zip package with shared library support
      p7zip =
        let
          baseP7zip = prev.p7zip.override { enableUnfree = true; };
        in
        baseP7zip.overrideAttrs (oldAttrs: {
          buildPhase = ''
            runHook preBuild
            make $buildFlags

            # Corrected: Build 7z.so using top-level make target
            make bin/7z.so
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            make install $makeFlags

            # Corrected: Install from top-level bin directory
            install -Dm755 bin/7z.so $out/lib/p7zip/7z.so
            runHook postInstall
          '';
        });

      # Custom build of far2l
      far2l = prev.stdenv.mkDerivation rec {
        pname = "far2l";
        version = "2.6.5-3b604892";  # Change that version if you need to build a package with other version name

        src = prev.fetchFromGitHub {
          owner = "elfmz";
          repo = "far2l";
          rev = "3b60489";    # Version revision you could change
          sha256 = "sha256-nlCgrPXhsBV5PIHDI/+TdinEslY6Xh6CwtKpPQFOTCA="; # Hash sha256 depending on your revision
        };

        nativeBuildInputs = [
          prev.cmake
          prev.ninja
          prev.pkg-config
          prev.m4
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
          final.p7zip # Use our custom-built p7zip with shared library
        ]
        ++ prev.lib.optional (!prev.stdenv.hostPlatform.isDarwin) prev.samba
        ++ (with prev.python3Packages; [
          python
          cffi
          debugpy
          pcpp
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
          "-DNETROCKS=ON" # Keep NetRocks without S3
          "-DAWS_S3=OFF" # Explicitly disable S3
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
              final.p7zip
            ];
          in
          ''
            # Warp archivers pathes to program bin
            wrapProgram $out/bin/far2l \
              --prefix PATH : ${prev.lib.makeBinPath archiveTools}

            # link p7z lib to far pluggin arclite home
            ln -s ${pkgs.p7zip.lib}/lib/p7zip/7z $out/lib/far2l/Plugins/arclite/plug/7z
            ln -s ${pkgs.p7zip.lib}/lib/p7zip/7za $out/lib/far2l/Plugins/arclite/plug/7za
            ln -s ${pkgs.p7zip.lib}/lib/p7zip/7zCon.sfx $out/lib/far2l/Plugins/arclite/plug/7zCon.sfx
            ln -s ${pkgs.p7zip.lib}/lib/p7zip/7z.so $out/lib/far2l/Plugins/arclite/plug/7z.so
            ln -s ${pkgs.p7zip.lib}/lib/p7zip/Codecs $out/lib/far2l/Plugins/arclite/plug/Codecs


          '';

        meta = with prev.lib; {
          description = "Linux port of FAR Manager v2 with ArchLite support";
          homepage = "https://github.com/elfmz/far2l";
          license = licenses.gpl2Only;
          maintainers = with maintainers; [ tempergate ];
        };
      };
    })
  ];

}
