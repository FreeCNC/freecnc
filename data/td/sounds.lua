sound_music_add {
    "aoi.aud", "ccthang.aud", "fwp.aud", "ind.aud", "ind2.aud", "justdoit.aud",
    "linefire.aud", "march.aud", "target.aud"
}

if (vfs_archive_loaded("foo.mix")) then 
    sound_music_add {
        "nomercy.aud", "otp.aud", "prp.aud", "stopthem.aud", "trouble.aud",
        "warfare.aud", "heavyg.aud", "j1.aud", "jdi_v2.aud",
        "radio.aud", "rain.aud"
    }
end

sound_effects_add { ... }

sound_talkback_add { ... }
