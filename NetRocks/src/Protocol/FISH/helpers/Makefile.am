fishdir = $(libexecdir)/@PACKAGE@/fish

# Files to install and distribute other than fish scripts
FISH_MISC  = README.fish

# Install and distribute FISH helper scripts w/o shebang & executable bit as data
fish_DATA = $(FISH_MISC) ls mkdir fexists unlink chown chmod rmdir ln mv hardlink get send append info utime
fishconfdir = $(sysconfdir)/@PACKAGE@

EXTRA_DIST = $(fish_DATA)
