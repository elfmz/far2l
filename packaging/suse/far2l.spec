# spec file for package far2linux

%define	base_Name far2l
%define autoreqprov no
%define	use_python_plugin	1
%define	has_copyright	0
%define use_netcfg	0

Name:           %{base_Name}
Version:        2.5.0
Release:        227
Group:			Productivity/File utilities
Summary:        Far manager for Linux
License:        GPL-2.0-only
URL:            https://github.com/elfmz/far2l.git
Source0:        %{name}-%{version}.tar.gz
Source1:        changelog
BuildArchitectures: x86_64 i586 aarch64
Distribution: 	openSUSE Tumbleweed

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  make
%if %{defined suse_version}
BuildRequires: 	wxGTK3-3_2-devel 
BuildRequires: 	libxml2-devel libneon-devel libopenssl-devel libuchardet-devel
BuildRequires:	update-desktop-files
%endif
%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires:  wxGTK-devel >= 3.1
BuildRequires: 	libxml2-devel neon-devel openssl-devel uchardet-devel
%endif
BuildRequires:  fmt-devel libarchive-devel
BuildRequires:  libnfs-devel libsmbclient-devel libssh-devel pcre-devel
BuildRequires:  python-rpm-macros
BuildRequires:  python3-devel python3-cffi python3-pip

# BuildRequires:	wxWidgets-devel >= 3.1

AutoReqProv: 	no
Requires:		/bin/sh

%description
Far 2 Linux is enhanced port of the well-known dual-panel console file manager from the Windows world. Despite to the original, 
the far 2 Linux has started from the nearly original code from Eugene Roshal and it is based upon FAR 2 codebase but not FAR 3. 

In addition, Linux version supports both X11 and console modes, with over-ssh access to the clipboard and fully-working keyboard 
rather than ancient termcap limitations. It introduces background operations for copying, live console scrolling and many other 
features have implemented in Linux version. Couple of well-known plug-ins are available out of the box, including 
Colorer, archiving, and network operations.

# per-package separation

%package core
Summary: 	Far for Linux main code, works with terminals
Suggests:	far2l-ttyxi far2l-wxgtk far2l-plugins-netrocks-ftp far2l-plugins-netrocks-sftp far2l-plugins-netrocks-smb 
Suggests:	far2l-plugins-netrocks-webdav far2l-plugins-netrocks-nfs
Suggests:	tar gzip bzip2 exiftool 7zip zstd elfutils gpg2 util-linux
%if %{defined suse_version}
AutoReq: 	no
Requires:	/bin/bash 
Requires:	libarchive13
Requires:	libfmt8 libuchardet0 libxml2 libpcre1
%endif
Conflicts:	far2l-full
Provides:	far2l

%description core
Far for Linux main files with core plug-ins, needs terminal to work

Far 2 Linux is enhanced port of the well-known dual-panel console file manager from the Windows world. Despite to the original, 
the far 2 Linux has started from the nearly original code from Eugene Roshal and it is based upon FAR 2 codebase but not FAR 3. 

In addition, Linux version supports both X11 and console modes, with over-ssh access to the clipboard and fully-working keyboard 
rather than ancient termcap limitations. It introduces background operations for copying, live console scrolling and many other 
features have implemented in Linux version. Couple of well-known plug-ins are available out of the box, including 
Colorer, archiving, and network operations.

%package ttyxi
Summary: Far for Linux enhanced keyboard support for terminals
%if %{defined suse_version}
AutoReq: 	no
Requires:	libXi6 far2l-core
%endif
Conflicts:	far2l-full

%description ttyxi
Far for Linux extension to support enhanced keyboard (for case it have started inside X session)

%package wxgtk
Summary: Far for Linux UI for X11
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libwx_baseu-suse5_0_0 libwx_gtk3u_core-suse5_0_0 
%endif
# wxWidgets > 3.0 ?
Conflicts:	far2l-full

%description wxgtk
Far for Linux UI for X11 on top of wxWidgets and GTK

%package plugins-netrocks-ftp
Summary: Far for Linux plug-in to communicate via FTP
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libopenssl1_1 
%endif
Conflicts:	far2l-full

%description plugins-netrocks-ftp
Far for Linux plug-in to communicate via FTP, as part of NetRocks

%package plugins-netrocks-sftp
Summary: Far for Linux plug-in to communicate via SFTP
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libssh2-1 libopenssl1_1
%endif
Conflicts:	far2l-full

%description plugins-netrocks-sftp
Far for Linux plug-in to communicate via sFTP, as part of NetRocks

%package plugins-netrocks-smb
Summary: Far for Linux plug-in to communicate via SMB
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libsmbclient0
%endif
Conflicts:	far2l-full

%description plugins-netrocks-smb
Far for Linux plug-in to communicate via SMB, as part of NetRocks

%package plugins-netrocks-webdav
Summary: Far for Linux plug-in to communicate via WebDAV
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libneon27
%endif
Conflicts:	far2l-full

%description plugins-netrocks-webdav
Far for Linux plug-in to communicate via WebDAV, as part of NetRocks

%package plugins-netrocks-nfs
Summary: Far for Linux plug-in to communicate via NFS
%if %{defined suse_version}
AutoReq: 	no
Requires: 		far2l-core libnfs13
%endif
Conflicts:	far2l-full

%description plugins-netrocks-nfs
Far for Linux plug-in to communicate via NFS, as part of NetRocks

# and full

%package full
Summary: Far for Linux (complete installation)
%if %{defined suse_version}
AutoReq: 	no
Requires: 		/bin/sh libarchive13 libopenssl1_1 libfmt8 libnfs13 libssh2-1 libuchardet0 libxml2 libneon27
Requires: 		libsmbclient0 libpcre2-32-0 libwx_baseu-suse5_0_0 libwx_gtk3u_core-suse5_0_0 
Requires:		python3
%endif
Conflicts:	far2l-core far2l-ttyxi far2l-wxgtk far2l-plugins-netrocks-ftp far2l-plugins-netrocks-sftp far2l-plugins-netrocks-smb far2l-plugins-netrocks-webdav far2l-plugins-netrocks-nfs
Provides:	far2l
Suggests:	tar gzip bzip2 exiftool 7zip zstd elfutils gpg2 util-linux
Suggests:	chafa jp2a pandoc-cli poppler-tools colordiff catdoc catppt ctags gptfdisk rpm dpkg

%description full
Far 2 Linux is enhanced port of the well-known dual-panel console file manager from the Windows world. Despite to the original, 
the far 2 Linux has started from the nearly original code from Eugene Roshal and it is based upon FAR 2 codebase but not FAR 3. 

In addition, Linux version supports both X11 and console modes, with over-ssh access to the clipboard and fully-working keyboard 
rather than ancient termcap limitations. It introduces background operations for copying, live console scrolling and many other 
features have implemented in Linux version. Couple of well-known plug-ins are available out of the box, including 
Colorer, archiving, and network operations.

# netcfg

%if %{use_netcfg}
%package plugins-netcfg
Summary: Far for Linux plug-in to manage network interfaces
%if %{defined suse_version}
AutoReq: 	no
%endif
Requires: 		far2l

%description plugins-netcfg
Far for Linux plug-in to manage network interfaces, separate plug-in with MIT license from https://github.com/VPROFi/netcfgplugin
%endif

%if %{use_python_plugin}
%package plugins-python
%if %{defined suse_version}
AutoReq: 	no
%endif
Requires:		far2l python3

Summary: Far for Linux plug-in to support Python scripting

%description plugins-python
Far for Linux plug-in to support Python scripting, with extra samples inside
%endif

# build

%prep
%autosetup -p1

%build
mkdir build
pushd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=%{buildroot}%{_prefix} \
%if %{use_netcfg}
	-DNETCFG=yes \
%else
	-DNETCFG=no \
%endif
	-DUSEWX=yes \
%if %{use_python_plugin}
	-DPYTHON=yes -DVIRTUAL_PYTHON=python3 \
%else
	-DPYTHON=no \
%endif
	-DCMAKE_BUILD_TYPE=Release ..
make
popd

%install
pushd build
make install
popd
%if %{defined suse_version}
%suse_update_desktop_file %{base_Name}
%endif
%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version}

%endif
%if %{defined debian}

%endif

%post
%if %{defined suse_version}
%suse_update_desktop_file -r %{base_Name} System Utility
%endif
if [ -x /usr/bin/update-desktop-database ]; then
	/usr/bin/update-desktop-database > /dev/null || :
fi

%postun
if [ -x /usr/bin/update-desktop-database ]; then
	/usr/bin/update-desktop-database > /dev/null || :
fi

# per-package files

%files core
%license LICENSE.txt

%_bindir/%{base_Name}
%dir %{_prefix}/lib/%{base_Name}
%{_prefix}/lib/%name/%{base_Name}_askpass
%{_prefix}/lib/%name/%{base_Name}_sudoapp

%dir %_datadir/%{base_Name}
%dir %_datadir/%{base_Name}/Plugins
%dir %_datadir/%{base_Name}/Plugins/align
%dir %_datadir/%{base_Name}/Plugins/autowrap
%dir %_datadir/%{base_Name}/Plugins/calc
%dir %_datadir/%{base_Name}/Plugins/colorer
%dir %_datadir/%{base_Name}/Plugins/compare
%dir %_datadir/%{base_Name}/Plugins/drawline
%dir %_datadir/%{base_Name}/Plugins/editcase
%dir %_datadir/%{base_Name}/Plugins/editorcomp
%dir %_datadir/%{base_Name}/Plugins/filecase
%dir %_datadir/%{base_Name}/Plugins/incsrch
%dir %_datadir/%{base_Name}/Plugins/inside
%dir %_datadir/%{base_Name}/Plugins/multiarc
%dir %_datadir/%{base_Name}/Plugins/SimpleIndent
%dir %_datadir/%{base_Name}/Plugins/NetRocks
%dir %_datadir/%{base_Name}/Plugins/tmppanel
%_datadir/%{base_Name}/*.*

%_datadir/%{base_Name}/Plugins/align/*
%_datadir/%{base_Name}/Plugins/autowrap/*
%_datadir/%{base_Name}/Plugins/calc/*
%_datadir/%{base_Name}/Plugins/colorer/*
%_datadir/%{base_Name}/Plugins/compare/*
%_datadir/%{base_Name}/Plugins/drawline/*
%_datadir/%{base_Name}/Plugins/editcase/*
%_datadir/%{base_Name}/Plugins/editorcomp/*
%_datadir/%{base_Name}/Plugins/filecase/*
%_datadir/%{base_Name}/Plugins/incsrch/*
%_datadir/%{base_Name}/Plugins/inside/*
%_datadir/%{base_Name}/Plugins/multiarc/*
%_datadir/%{base_Name}/Plugins/SimpleIndent/*
%_datadir/%{base_Name}/Plugins/NetRocks/*
%_datadir/%{base_Name}/Plugins/tmppanel/*

%_datadir/applications/%{base_Name}.desktop

%_datadir/icons/%{base_Name}.svg
%_datadir/icons/hicolor/1024x1024/apps/%{base_Name}.svg
%_datadir/icons/hicolor/128x128/apps/%{base_Name}.svg
%_datadir/icons/hicolor/16x16/apps/%{base_Name}.svg
%_datadir/icons/hicolor/192x192/apps/%{base_Name}.svg
%_datadir/icons/hicolor/24x24/apps/%{base_Name}.svg
%_datadir/icons/hicolor/256x256/apps/%{base_Name}.svg
%_datadir/icons/hicolor/32x32/apps/%{base_Name}.svg
%_datadir/icons/hicolor/48x48/apps/%{base_Name}.svg
%_datadir/icons/hicolor/512x512/apps/%{base_Name}.svg
%_datadir/icons/hicolor/64x64/apps/%{base_Name}.svg
%_datadir/icons/hicolor/72x72/apps/%{base_Name}.svg
%_datadir/icons/hicolor/96x96/apps/%{base_Name}.svg

#%{_prefix}/share/doc/far2l/copyright
%{_prefix}/share/man/man1/*
%{_prefix}/share/man/ru/man1/*

%dir %{_prefix}/lib/%{base_Name}/Plugins
%dir %{_prefix}/lib/%{base_Name}/Plugins/align
%dir %{_prefix}/lib/%{base_Name}/Plugins/autowrap
%dir %{_prefix}/lib/%{base_Name}/Plugins/calc
%dir %{_prefix}/lib/%{base_Name}/Plugins/colorer
%dir %{_prefix}/lib/%{base_Name}/Plugins/compare
%dir %{_prefix}/lib/%{base_Name}/Plugins/drawline
%dir %{_prefix}/lib/%{base_Name}/Plugins/editcase
%dir %{_prefix}/lib/%{base_Name}/Plugins/editorcomp
%dir %{_prefix}/lib/%{base_Name}/Plugins/filecase
%dir %{_prefix}/lib/%{base_Name}/Plugins/incsrch
%dir %{_prefix}/lib/%{base_Name}/Plugins/inside
%dir %{_prefix}/lib/%{base_Name}/Plugins/multiarc
%dir %{_prefix}/lib/%{base_Name}/Plugins/NetRocks
%dir %{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug
%dir %{_prefix}/lib/%{base_Name}/Plugins/SimpleIndent
%dir %{_prefix}/lib/%{base_Name}/Plugins/tmppanel

%{_prefix}/lib/%{base_Name}/Plugins/align/*
%{_prefix}/lib/%{base_Name}/Plugins/autowrap/*
%{_prefix}/lib/%{base_Name}/Plugins/calc/*
%{_prefix}/lib/%{base_Name}/Plugins/colorer/*
%{_prefix}/lib/%{base_Name}/Plugins/compare/*
%{_prefix}/lib/%{base_Name}/Plugins/drawline/*
%{_prefix}/lib/%{base_Name}/Plugins/editcase/*
%{_prefix}/lib/%{base_Name}/Plugins/editorcomp/*
%{_prefix}/lib/%{base_Name}/Plugins/filecase/*
%{_prefix}/lib/%{base_Name}/Plugins/incsrch/*
%{_prefix}/lib/%{base_Name}/Plugins/inside/*
%{_prefix}/lib/%{base_Name}/Plugins/multiarc/*
%{_prefix}/lib/%{base_Name}/Plugins/SimpleIndent/*
%{_prefix}/lib/%{base_Name}/Plugins/tmppanel/*
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks.far-plug-wide
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-FILE.broker

%files ttyxi
%{_prefix}/lib/%name/%{base_Name}_ttyx.broker

%files wxgtk
%{_prefix}/lib/%name/%{base_Name}_gui.so

%files plugins-netrocks-ftp
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-FTP.broker

%files plugins-netrocks-sftp
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-SFTP.broker

%files plugins-netrocks-smb
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-SMB.broker

%files plugins-netrocks-webdav
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-WebDAV.broker

%files plugins-netrocks-nfs
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/plug/NetRocks-NFS.broker

# full files

%files full
%license LICENSE.txt

%_bindir/%{base_Name}
%dir %{_prefix}/lib/%{base_Name}
%{_prefix}/lib/%name/%{base_Name}_askpass
%{_prefix}/lib/%name/%{base_Name}_sudoapp
%{_prefix}/lib/%name/%{base_Name}_gui.so
%{_prefix}/lib/%name/%{base_Name}_ttyx.broker

%dir %_datadir/%{base_Name}
%dir %_datadir/%{base_Name}/Plugins
%dir %_datadir/%{base_Name}/Plugins/align
%dir %_datadir/%{base_Name}/Plugins/autowrap
%dir %_datadir/%{base_Name}/Plugins/calc
%dir %_datadir/%{base_Name}/Plugins/colorer
%dir %_datadir/%{base_Name}/Plugins/compare
%dir %_datadir/%{base_Name}/Plugins/drawline
%dir %_datadir/%{base_Name}/Plugins/editcase
%dir %_datadir/%{base_Name}/Plugins/editorcomp
%dir %_datadir/%{base_Name}/Plugins/filecase
%dir %_datadir/%{base_Name}/Plugins/incsrch
%dir %_datadir/%{base_Name}/Plugins/inside
%dir %_datadir/%{base_Name}/Plugins/multiarc
%dir %_datadir/%{base_Name}/Plugins/SimpleIndent
%dir %_datadir/%{base_Name}/Plugins/NetRocks
%dir %_datadir/%{base_Name}/Plugins/tmppanel
%_datadir/%{base_Name}/*.*

%_datadir/%{base_Name}/Plugins/align/*
%_datadir/%{base_Name}/Plugins/autowrap/*
%_datadir/%{base_Name}/Plugins/calc/*
%_datadir/%{base_Name}/Plugins/colorer/*
%_datadir/%{base_Name}/Plugins/compare/*
%_datadir/%{base_Name}/Plugins/drawline/*
%_datadir/%{base_Name}/Plugins/editcase/*
%_datadir/%{base_Name}/Plugins/editorcomp/*
%_datadir/%{base_Name}/Plugins/filecase/*
%_datadir/%{base_Name}/Plugins/incsrch/*
%_datadir/%{base_Name}/Plugins/inside/*
%_datadir/%{base_Name}/Plugins/multiarc/*
%_datadir/%{base_Name}/Plugins/SimpleIndent/*
%_datadir/%{base_Name}/Plugins/NetRocks/*
%_datadir/%{base_Name}/Plugins/tmppanel/*

%dir %{_prefix}/lib/%{base_Name}/Plugins
%dir %{_prefix}/lib/%{base_Name}/Plugins/align
%dir %{_prefix}/lib/%{base_Name}/Plugins/autowrap
%dir %{_prefix}/lib/%{base_Name}/Plugins/calc
%dir %{_prefix}/lib/%{base_Name}/Plugins/colorer
%dir %{_prefix}/lib/%{base_Name}/Plugins/compare
%dir %{_prefix}/lib/%{base_Name}/Plugins/drawline
%dir %{_prefix}/lib/%{base_Name}/Plugins/editcase
%dir %{_prefix}/lib/%{base_Name}/Plugins/editorcomp
%dir %{_prefix}/lib/%{base_Name}/Plugins/filecase
%dir %{_prefix}/lib/%{base_Name}/Plugins/incsrch
%dir %{_prefix}/lib/%{base_Name}/Plugins/inside
%dir %{_prefix}/lib/%{base_Name}/Plugins/multiarc
%dir %{_prefix}/lib/%{base_Name}/Plugins/NetRocks
%dir %{_prefix}/lib/%{base_Name}/Plugins/SimpleIndent
%dir %{_prefix}/lib/%{base_Name}/Plugins/tmppanel

%{_prefix}/lib/%{base_Name}/Plugins/align/*
%{_prefix}/lib/%{base_Name}/Plugins/autowrap/*
%{_prefix}/lib/%{base_Name}/Plugins/calc/*
%{_prefix}/lib/%{base_Name}/Plugins/colorer/*
%{_prefix}/lib/%{base_Name}/Plugins/compare/*
%{_prefix}/lib/%{base_Name}/Plugins/drawline/*
%{_prefix}/lib/%{base_Name}/Plugins/editcase/*
%{_prefix}/lib/%{base_Name}/Plugins/editorcomp/*
%{_prefix}/lib/%{base_Name}/Plugins/filecase/*
%{_prefix}/lib/%{base_Name}/Plugins/incsrch/*
%{_prefix}/lib/%{base_Name}/Plugins/inside/*
%{_prefix}/lib/%{base_Name}/Plugins/multiarc/*
%{_prefix}/lib/%{base_Name}/Plugins/SimpleIndent/*
%{_prefix}/lib/%{base_Name}/Plugins/tmppanel/*
%{_prefix}/lib/%{base_Name}/Plugins/NetRocks/*

%_datadir/applications/%{base_Name}.desktop

%_datadir/icons/%{base_Name}.svg

# Leap 15 has no 1024x1024 icon folders
%if ( 0%{?sle_version} == 150400 || 0%{?sle_version} == 150300 || 0%{?sle_version} == 150500 ) && 0%{?is_opensuse}
%dir %_datadir/icons/hicolor/1024x1024
%dir %_datadir/icons/hicolor/1024x1024/apps
%endif
%_datadir/icons/hicolor/1024x1024/apps/%{base_Name}.svg
%_datadir/icons/hicolor/128x128/apps/%{base_Name}.svg
%_datadir/icons/hicolor/16x16/apps/%{base_Name}.svg
%_datadir/icons/hicolor/192x192/apps/%{base_Name}.svg
%_datadir/icons/hicolor/24x24/apps/%{base_Name}.svg
%_datadir/icons/hicolor/256x256/apps/%{base_Name}.svg
%_datadir/icons/hicolor/32x32/apps/%{base_Name}.svg
%_datadir/icons/hicolor/48x48/apps/%{base_Name}.svg
%_datadir/icons/hicolor/512x512/apps/%{base_Name}.svg
%_datadir/icons/hicolor/64x64/apps/%{base_Name}.svg
%_datadir/icons/hicolor/72x72/apps/%{base_Name}.svg
%_datadir/icons/hicolor/96x96/apps/%{base_Name}.svg

%if %{has_copyright}
%{_prefix}/share/doc/far2l/copyright
%endif

%{_prefix}/share/man/man1/*
%{_prefix}/share/man/ru/man1/*

# 3rd party plugins
%if %{use_netcfg}
%files plugins-netcfg
%dir %_datadir/%{base_Name}/Plugins/netcfg
%dir %{_prefix}/lib/%{base_Name}/Plugins/netcfg
%_datadir/%{base_Name}/Plugins/netcfg/*
%{_prefix}/lib/%{base_Name}/Plugins/netcfg/*
%endif

%if %{use_python_plugin}
%files plugins-python
%dir %{_prefix}/lib/%{base_Name}/Plugins/python
%dir %_datadir/%{base_Name}/Plugins/python

%{_prefix}/lib/%{base_Name}/Plugins/python/*
%_datadir/%{base_Name}/Plugins/python/*
%endif

%if 0%{?sle_version} > 150500 && 0%{?is_opensuse}
%changelog full
%include %{SOURCE1}

%changelog core
%include %{SOURCE1}
%endif
