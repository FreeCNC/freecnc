Windows
--------------------------------------------------------------------------------

To use this utility, you'll need `zip.exe`, available from
http://unxutils.sourceforge.net/.

Copy it into the directory, or PATH, and then just execute 'win32_snapshot.bat'.

Linux
--------------------------------------------------------------------------------

The script posix_snapshot.sh in this directory builds the current tree and
produces a tarball of the resulting main binary, the docs and the data directory.
Take note of the version of g++ displayed at the end as you might run into
binary compatability issues with some platforms.

MacOS X
--------------------------------------------------------------------------------

The posix_snapshot.sh script should also work, but I'd also like to make a DMG
with the SDL libraries needed for ease of use on that platform.
