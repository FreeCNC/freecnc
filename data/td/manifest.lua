vfs_add {"./"}
vfs_add {"conquer.mix", "general.mix", required="all"}
vfs_add {"aud.mix", "speech.mix", "sounds.mix", required="all"}
vfs_add {"transit.mix", required="all"}
vfs_add {"desert.mix",   "temperat.mix", "winter.mix", required="all"}
vfs_add {"movies.mix", "scores.mix", required="none"}
-- local.mix in DOS version, cclocal.mix later versions
vfs_add {"local.mix",   "cclocal.mix", required="any"}

-- FreeCNC convention for working with GDI and NOD CDs at the same time
vfs_add {"general-gdi.mix", "general-nod.mix", required="none"}
vfs_add {"movies-gdi.mix", "movies-nod.mix", required="none"}

-- Gold Edition specific files
vfs_add {"update.mix",   "updatec.mix", required="none"}
vfs_add {"deseicnh.mix", "tempicnh.mix", "winticnh.mix", required="none"}

-- Covert Operations files
vfs_add {"sc-000.mix",  "sc-001.mix", required="none"}
