-------------------------------------------------------------------------------
1. Overview
-------------------------------------------------------------------------------

1. Overview
2. Introduction
3. License
4. Compilation
  4.1. Requirements
  4.2. Compiling with GCC G++
  4.3. Compiling with Visual C++
5. Data files
  5.1. Tiberian Dawn (DOS)
  5.2. Tiberian Dawn Gold (Windows 95)
  5.3. Tiberian Dawn Covert Ops (expansion)
  5.4. Tiberian Dawn Demo
  5.5. Red Alert
  5.6. Red Alert Demo
6. Running the game
  6.1. New features
  6.2. Selecting a map
7. Credits
  7.1. Team
  7.2. Previous team members
  7.3. Thanks

-------------------------------------------------------------------------------
2. Introduction
-------------------------------------------------------------------------------

This document describes how to compile freecnc, how to get the data files,
and how to run the game.

-------------------------------------------------------------------------------
3. License
-------------------------------------------------------------------------------

FreeCNC is licensed under the terms of the GPL version 2, see
license.txt for details.

FreeCNC uses Lua 5.1, licensed under the MIT license, see
http://www.lua.org/license.html for more information.

FreeCNC also uses SDL 1.2, licensed under the LGPL, see
http://www.libsdl.org/license.php for more information.

-------------------------------------------------------------------------------
4. Compilation
-------------------------------------------------------------------------------

This section will outline how to compile FreeCNC.

4.1. Requirements
==============================

  Compilers:
  - GCC G++ version 4.1 or newer.
  - Visual C++ 2005 (express will work).

  Libraries:
  - SDL 1.2.11.
  - SDL_mixer 1.2.6.
  - Boost 1.34.

4.2. Compiling with Cmake and GNU-C++
=====================================
  
  You will need cmake and make. In the archive directory run

  $ cmake src
  
  and then
  
  $ make
  
4.3. Compiling with Visual C++
==============================

  First of all make sure you have added boost, SDL and SDL mixer include & lib
  files to the Visual C++ search paths.

  After that is done, simply opening the solution file and building should do
  the trick.

-------------------------------------------------------------------------------
5. Data Files
-------------------------------------------------------------------------------

At this point you should have an executable, you will now need some data
files. (Note that Tiberian Dawn is the first version of Command & Conquer)

FreeCNC supports the following (english only) versions of C&C:
- Tiberian Dawn DOS
- Tiberian Dawn Gold (this version is part of The First Decade)
- Tiberian Dawn Demo

You can use the convert ops expansion pack datafiles in conjunction with any
of the above.

The Red Alert data files can also be used, but are not fully supported at
this time.

To install the data files mentioned in the following sections, all you have
to do is copy the files to the data/mix subdirectory of where the exe is
located.

5.1. Tiberian Dawn (DOS)
========================

  The following files are required:
    * conquer.mix
    * general.mix (contains the mission data for one campaign)
    * desert.mix
    * temperat.mix
    * winter.mix
    * local.mix (located in the install subdirectory of the CD)
    * speech.mix (located in the aud1 subdirectory of the CD)

  The following files are recommended, but not required:
    * scores.mix
    * sounds.mix
    * movies.mix
    * transit.mix

  The files movies.mix and general.mix are the only mix files that differ
  between the two CDs.  If you copy one set of the files to data/mix as
  movies-gdi.mix (or movies-nod.mix) and general-gdi.mix (or general-nod.mix),
  FreeCNC will be able to use both files.

5.2. Tiberian Dawn Gold (Windows 95 and The First Decade versions)
==================================================================

  For this version, you need the following additional files.  They are stored in
  InstallShield archives on the CD, which you'll have to extract (either by
  installing or using an extractor).

   * cclocal.mix
   * updatec.mix
   * deseicnh.mix
   * tempicnh.mix
   * winticnh.mix
   * speech.mix

5.3. Tiberian Dawn Covert Ops (expansion)
=========================================

  Additional covert ops files:
    * sc-000.mix
    * sc-001.mix

5.4. Tiberian Dawn Demo
=======================

  The MIX files in this archive is sufficient to preview FreeCNC:
  ftp://ftp.westwood.com/pub/cc1/previews/demo/cc1demo1.zip

  If you want video and music, get this file as well:
  ftp://ftp.westwood.com/pub/cc1/previews/demo/cc1demo2.zip

  A python script is provided to download and extract the files automatically.
  Simply run getdemo.py in the utils dir.

5.5. Red Alert
==============

  First of all, Red Alert is not currently 'officially' supported.

  Only two files are needed for red alert:
    * redalert.mix
    * main.mix

  Both files can be found on either Red Alert CD.  Both CDs have the maps for
  both campaigns but only the videos for one.

5.6. Red Alert Demo
==================

  The demo version can be downloaded from here:
  ftp://ftp.westwood.com/pub/redalert/previews/demo/ra95demo.zip

  You only need the mix files from the archieve.

  A python script is provided to download and extract the files automatically.
  Simply run getrademo.py in the utils dir.

-------------------------------------------------------------------------------
6. Running the game
-------------------------------------------------------------------------------

6.1. New features
=================

  FreeCNC has a few features not present in the original C&C:
    * Dragging the mouse with the right mouse button down will pan the map
    * The sidebar can be toggled with the new style sidebar as well
    * Mouse wheel will scroll both columns of the sidebar
    * Unit and structure production is can be queued
    * Support for arbitrary resolutions

6.2. Selecting a map
====================

  To select a specific map, launch with freecnc -map <mapname>.

  The naming scheme used by Westwood for their single player maps is roughly:
  SC[GB][0-9][0-9][WE][ABC]

  Examples:
    * SCB01EA (first NOD mission)
    * SCG15EB (one of the final GDI missions),
    * SCB13EC (one of the final NOD missions)

  Higher map numbers are used for the Covert Operations missions.

  Maps included in Tiberian Dawn demo (no NOD missions):
    * SCG01EA
    * SCG03EA
    * SCG05EA
    * SCG06EA
    * SCG07EA
    * SCG10EA

  Maps included in the Red Alert demo:
    * SCG02EA
    * SCG05EA
    * SCU01EA

  If you're interested to see what the multiplayer maps look like, the third
  letter is 'M'.

  Finally, the funpark missions are SCJ01EA up to SCJ05EA (yes we know the
  dinosaurs' movements are "interesting").

-------------------------------------------------------------------------------
7. Credits
-------------------------------------------------------------------------------

7.1. Team
=========

  Euan MacGregor (zx64) - euan@freecnc.org
  developer

  Stijn Gijsen (gonewacko) - stijn@freecnc.org
  website developer

  Thomas Johansson (prencher) - thomas@freecnc.org
  developer, website admin

7.2. Previous team members
==========================

  Bernd Ritter (comrad) - email unknown
  old website

  Jonas Jermann (g0th) - email uknown
  developer

  Kareem Dana (kareemy) - email unknown
  project starter, developer

  Sander van Geloven (Pander) - email uknown
  patch contributor

  Tim Johansson (Tim^) - email unknown
  project starter, developer

  Jos Bol (Gandalf) - email unknown
  project starter, developer

7.3. Thanks
===========

  Thanks to everybody who have contributed patches and insights over the years,
  especially the people who reverse engineered the file formats.

  Thanks to James Crasta for providing website hosting, and Lasse Hjorth for
  hosting our email.

  Thanks to SourceForge for hosting our SVN and project pages.

  And finally thanks to anybody else we may have forgotten - do contact us if
  you belong here :)
