There are example scripts and core spec file for RPM build.

Preface
--------

This is the SPEC file for Far 2 Linux (vanilla) from https://github.com/elfmz/far2l with multiple packages, as follows:

1. far2l-full includes the Far2l complete installation, including wxGTK UI, TTY and TTY extra X11 input capabilities, and all plug-ins except Python.
2. far2l-plugins-python package as an option to the main Far2l package (can be installed optionally).

In addition, it provides an mutually exclusive packaging option with extra plug-ins splitting, as follows:

1. far2l-core includes the TYY-only version of the Far2l (like debian mainstream does), with all plug-ins except python and few netrocks protocols.
2. far2l-wxgtk as wxGTK-based GUI front-end to the far2l-core.
3. far2l-ttyxi as Xinput extensions to support core X11 input for Far2L
4. far2l-plugins-pythonpackage as an option to the main Far2l package (can be installed optionally).
5. Extra packages for NetRocks to manage every protocol individually, as follows:
5.1. far2l-plugins-netrocks-ftp
5.2. far2l-plugins-netrocks-nfs
5.3. far2l-plugins-netrocks-sftp
5.4. far2l-plugins-netrocks-smb
5.5. far2l-plugins-netrocks-webdav

As a bonus, spec file allows to build the 3rd party plug-in https://github.com/VPROFi/netcfgplugin if necessary (disabled by default). 
It provides far2l-plugins-netcfg package (if enabled, see below).

Online builds
--------------

Online build results are available here: https://download.opensuse.org/repositories/home:/viklequick/

The supported platforms are:

 - openSUSE x86_64 15.3, 15.14. 15.5
 - openSUSE Factory x86_64, x86, and AArch64
 - openSUSE Thumbleweed
 - Fedora Rawhide
 - Fedora 38
 - (to be added other RPM-based distros)

Packaging details
------------------

SPEC file provides build for all-at-onnce options without any tricks with many compilation attempts, and then shrinks and packages the same 
CMake installation files to multiple packages, as described above.

To build everything, the folder comtains few scripts to manage:

 - build-far2l.sh as sample script to build
 - git-rpm-changelog.sh to generate changelog from Git
 - filter-changelog.py to filter changelog, remove unnecessary lines, and then merge multiple commits into one cumulative record per-day.

The typical build steps are:

1. Build changelog:

	./git-rpm-changelog.sh --since '2022-01-01' | python3 ./filter-changelog.py > changelog

2. Prepare all necessary content by creating source archive (should be created out-of-the source tree):

	mkdir -p far2l-2.5.0
	cp -aR far2l/* far2l-2.5.0
	cp -aR far2l/.git far2l-2.5.0
	tar cf far2l-2.5.0.tar.gz far2l-2.5.0/
	rm -rf far2l-2.5.0/

3. Then source files should be copied to the RPM-friendly folders:

	cp far2l-2.5.0.tar.gz ~/rpmbuild/SOURCES
	cp far2l/packaging/suse/far2l.spec ~/rpmbuild/SPECS/far2l.spec
	cp far2l/packaging/suse/copyright ~/rpmbuild/SPECS/far2l.spec

4. The next step is building the RPMs:

	rpmbuild -bb ~/rpmbuild/SPECS/far2l.spec

5. Once it had completed, the artifacts needs to be copied somewhere, for further usage:

	cp ~/rpmbuild/RPMS/x86_64/far2l-*$VER-*.x86_64.rpm .

The sample build-far2l.sh script collects all steps at once and should be launched as 

	./build-far2l.sh

Of course it is only an example; all eror checks are removed for demo purposes.


Python plug-in build
---------------------

The original Python plug-in author prefer to use virtualenv to build and launch Python plug-in with sub-plugins. Unfortunately, 
automatic build environments prohibites to use network access at the build time, so for example OpenSuse Build Service cannot be 
used to build virtualenv just because piup install command will fail. We need all python modules to be pre-installed as system-wide
instead.

So, the Python plug-in is being built without virtualenv and with -DPYTHON=yes -DVIRTUAL_PYTHON-python3 options for CMake to let
build system (and then run-time) to use system-wide Python packages without virtual environment completely. Many thanks to the 
Python plug-in author to provide the according possibilities.

Netcfg plug-in build
---------------------

In addition, sp-ec provides an extra ability to build netcfg plug-in as part of the far2l tree (it is temporary workaround unless 
the netcfg plug-in becomes the part of main tree of course).

So, to build netcfg, you need:

1. Fetch the original sources from the https://github.com/VPROFi/netcfgplugin by the command

	git clone https://github.com/VPROFi/netcfgplugin

2. Then you need to copy the one sub-folder from the plug-in sources:

	cd netcfgplugin
	git pull
	[ ! -d /path/to/far2l/netcfg ] && mkdir /path/to/far2l/netcfg
	cp -aR src/* /path/to/far2l/netcfg

3. The next step is really dangerous - you need to update root CMakelists.txt file of the far2l source tree, by adding the following lines:

	if (DEFINED NETCFG AND NETCFG)
    	message(STATUS "NETCFG plugin enabled")
	    add_subdirectory(netcfg)
	endif()

These lines should be placed nearby to the other plug-ins, for example prior to the lines:

...
	if (NOT DEFINED SIMPLEINDENT OR SIMPLEINDENT)
    	message(STATUS "SIMPLEINDENT plugin enabled")
		add_subdirectory (SimpleIndent)
...

4. And last step: you need  to enable the plug-in on the spec file, by replacing line

	%define use_netcfg	0

to the

	%define use_netcfg	1

respectively. And then do not forget to build RPMs as described above.

Good luck!

If you need any assistance please push the Telagram channel https://t.me/far2l_ru or in the issues.
