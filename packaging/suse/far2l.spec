# spec file for package far2linux
Name:           far2l
Version:        2.3.1
Release:        1
Group:			System / Applications
Summary:        Far manager for Linux
License:        GPLv2
URL:            https://github.com/elfmz/far2l.git
Source0:        %{name}-%{version}.tar.gz
BuildArch: 		x86_64
Distribution: 	openSUSE Tumbleweed

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  gawk m4 make
BuildRequires:  wxWidgets-3_2-devel spdlog-devel fmt-devel libxerces-c-devel libarchive-devel libneon-devel
BuildRequires:  libnfs-devel libsmbclient-devel libopenssl-devel libssh-devel libuchardet-devel pcre2-devel

AutoReqProv: 	no
Requires: 		/bin/sh libarchive13 libopenssl1_1 libfmt8 libnfs13 libssh2 libuchardet0 libxerces-c-3_2 libneon27 
Requires: 		libsmbclient0 libpcre2-32-0 libwx_baseu-suse5_0_0 libwx_gtk2u_core-suse5_0_0 

%description
Far 2 Linux is enhanced port of the well-known dual-panel console file manager from the Windows world. Despite to the original, 
the far 2 Linux has started from the nearly original code from Eugene Roshal and it is based upon FAR 2 codebase but not FAR 3. 

In addition, Linux version supports both X11 and console modes, with over-ssh access to the clipboard and fully-working keyboard 
rather than ancient termcap limitations. It introduces background operations for copying, live console scrolling and many other 
features have implemented in Linux version. Couple of well-known plug-ins are available out of the box, including 
Colorer, archiving, and network operations.

%prep
%autosetup -p1

%build
mkdir build
pushd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=%{buildroot}%{_prefix} \
	-DUSEWX=yes -DCMAKE_BUILD_TYPE=Release ..
make
popd

%install
pushd build
make install
popd

%files
%license LICENSE.txt

%_bindir/%{name}
%dir %{_prefix}/lib/%{name}
%{_prefix}/lib/%name/%{name}_askpass
%{_prefix}/lib/%name/%{name}_sudoapp
%{_prefix}/lib/%name/%{name}_gui.so
%{_prefix}/lib/%name/%{name}_ttyx.broker

%dir %{_prefix}/lib/%{name}/Plugins
%{_prefix}/lib/%{name}/Plugins/*

%dir %_datadir/%{name}
%_datadir/%{name}/*

%_datadir/applications/%{name}.desktop

%_datadir/icons/%{name}.svg
%_datadir/icons/hicolor/1024x1024/apps/%{name}.svg
%_datadir/icons/hicolor/128x128/apps/%{name}.svg
%_datadir/icons/hicolor/16x16/apps/%{name}.svg
%_datadir/icons/hicolor/192x192/apps/%{name}.svg
%_datadir/icons/hicolor/24x24/apps/%{name}.svg
%_datadir/icons/hicolor/256x256/apps/%{name}.svg
%_datadir/icons/hicolor/32x32/apps/%{name}.svg
%_datadir/icons/hicolor/48x48/apps/%{name}.svg
%_datadir/icons/hicolor/512x512/apps/%{name}.svg
%_datadir/icons/hicolor/64x64/apps/%{name}.svg
%_datadir/icons/hicolor/72x72/apps/%{name}.svg
%_datadir/icons/hicolor/96x96/apps/%{name}.svg

%changelog
