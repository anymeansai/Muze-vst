#!/usr/bin/env python3
"""Generates Source/Presets/PresetData.cpp: 59 main-vocal + 37 ad-lib factory presets.

Each preset = genre template (chain + base values) + named variation (overrides).
Run:  python3 tools/gen_presets.py
"""
import os

OUT = os.path.join(os.path.dirname(__file__), "..", "Source", "Presets", "PresetData.cpp")

# --------------------------------------------------------------------------- templates
# values use "<module>_<param>" ids exactly as declared in Source/Core/ModuleRegistry.cpp
COMMON = {
    "gate_thresh": -48, "gate_release": 150, "gate_range": -80,
    "tune_amount": 100, "tune_speed": 25, "tune_humanize": 0,
    "lowcut_freq": 95, "lowcut_q": 0.71,
    "deess_freq": 6500, "deess_thresh": -30, "deess_amount": 9,
    "eq_lowgain": -1.5, "eq_lowfreq": 160, "eq_mid1gain": -2, "eq_mid1freq": 350, "eq_mid1q": 1.0,
    "eq_mid2gain": 2, "eq_mid2freq": 3500, "eq_mid2q": 0.9, "eq_highgain": 1.5, "eq_highfreq": 9000,
    "pop_sens": 50, "pop_strength": 70, "pop_release": 120,
    "comp_thresh": -20, "comp_ratio": 4, "comp_attack": 5, "comp_release": 80, "comp_makeup": 3, "comp_mix": 100,
    "glue_thresh": -22, "glue_ratio": 2, "glue_attack": 15, "glue_release": 250, "glue_makeup": 2,
    "sat_drive": 25, "sat_tone": 0, "sat_mix": 100,
    "air_freq": 12000, "air_amount": 3,
    "limiter_gain": 0, "limiter_ceiling": -0.5, "limiter_release": 60,
    "delay_sync": 3, "delay_time": 375, "delay_feedback": 35, "delay_spread": 100, "delay_tone": 6000,
    "delay_lowcut": 250, "delay_duck": 45, "delay_mix": 18,
    "reverb_size": 55, "reverb_decay": 45, "reverb_predelay": 25, "reverb_tone": 8000, "reverb_width": 100,
    "reverb_duck": 30, "reverb_mix": 20,
    "doubler_detune": 12, "doubler_delay": 20, "doubler_width": 100, "doubler_mix": 35,
    "widener_width": 125, "widener_lowmono": 120,
    "slap_time": 110, "slap_feedback": 10, "slap_tone": 5000, "slap_mix": 15,
    "harmony_interval": -12, "harmony_mix": 22, "harmony_pan": 0,
    "dist_drive": 40, "dist_tone": 8000, "dist_mix": 40,
    "exciter_freq": 5000, "exciter_amount": 30,
    "chorus_rate": 0.6, "chorus_depth": 35, "chorus_mix": 20,
    "lofi_bits": 10, "lofi_rate": 16000, "lofi_mix": 30,
    "phone_low": 500, "phone_high": 3400, "phone_drive": 20, "phone_mix": 100,
    "autopan_rate": 0.5, "autopan_depth": 60,
    "phaser_rate": 0.4, "phaser_depth": 60, "phaser_center": 1000, "phaser_feedback": 20, "phaser_mix": 35,
    "tremolo_rate": 5, "tremolo_depth": 40,
    "flanger_rate": 0.3, "flanger_depth": 50, "flanger_feedback": 40, "flanger_mix": 25,
    "transient_attack": 0, "transient_sustain": 0,
    "highcut_freq": 16000, "highcut_q": 0.71,
}

MAIN_CHAIN = ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "delay", "reverb", "limiter"]

GENRES = {
    # genre: (chain, overrides)
    "Pop": (MAIN_CHAIN, {"tune_speed": 30, "comp_ratio": 4, "eq_mid2gain": 2.5, "eq_highgain": 2}),
    "R&B": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "doubler", "delay", "reverb", "limiter"],
            {"tune_speed": 60, "tune_humanize": 20, "lowcut_freq": 100, "eq_lowgain": 1, "eq_mid2gain": 1.5, "eq_mid2freq": 3000,
             "eq_highgain": 1, "eq_highfreq": 10000, "comp_ratio": 3, "comp_attack": 8, "comp_release": 120,
             "sat_tone": -20, "doubler_mix": 22, "doubler_detune": 8, "delay_sync": 1, "delay_mix": 14, "delay_feedback": 30,
             "delay_duck": 60, "reverb_size": 70, "reverb_decay": 60, "reverb_mix": 26}),
    "Trap": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "exciter", "glue", "air", "delay", "reverb", "limiter"],
             {"tune_speed": 10, "lowcut_freq": 110, "eq_lowgain": -3, "eq_lowfreq": 180, "eq_mid2gain": 3, "eq_highgain": 3,
              "eq_highfreq": 8000, "comp_ratio": 6, "comp_attack": 3, "comp_release": 60, "comp_makeup": 4, "sat_drive": 35,
              "exciter_amount": 25, "delay_sync": 2, "delay_mix": 20, "delay_feedback": 40, "delay_duck": 50,
              "reverb_size": 50, "reverb_decay": 40, "reverb_mix": 16, "limiter_gain": 2}),
    "Rage": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "dist", "sat", "exciter", "glue", "air", "harmony", "doubler", "delay", "reverb", "limiter"],
             {"tune_speed": 3, "lowcut_freq": 120, "eq_mid2gain": 3.5, "eq_highgain": 3, "comp_ratio": 8, "comp_attack": 2,
              "comp_makeup": 5, "dist_drive": 45, "dist_mix": 40, "sat_drive": 55, "exciter_amount": 35,
              "harmony_interval": -12, "harmony_mix": 18, "doubler_mix": 45, "doubler_detune": 15,
              "delay_sync": 2, "delay_mix": 25, "delay_feedback": 45, "delay_duck": 40,
              "reverb_size": 75, "reverb_decay": 70, "reverb_mix": 30, "limiter_gain": 3}),
    "Hip-Hop": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "slap", "reverb", "limiter"],
                {"tune_amount": 60, "tune_speed": 80, "lowcut_freq": 90, "eq_mid2gain": 2.5, "eq_mid2freq": 4000,
                 "comp_ratio": 4, "comp_attack": 3, "sat_drive": 28, "slap_mix": 14, "reverb_size": 35, "reverb_decay": 30,
                 "reverb_mix": 12}),
    "Soul": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "slap", "reverb", "limiter"],
             {"tune_amount": 70, "tune_speed": 120, "tune_humanize": 40, "eq_lowgain": 1.5, "eq_mid1gain": -1, "eq_highgain": 1,
              "comp_ratio": 3, "comp_attack": 10, "glue_ratio": 3, "glue_thresh": -26, "sat_tone": -30, "sat_drive": 30,
              "slap_mix": 12, "reverb_size": 80, "reverb_decay": 70, "reverb_mix": 30, "reverb_predelay": 40}),
    "Afrobeats": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "chorus", "widener", "delay", "reverb", "limiter"],
                  {"tune_speed": 20, "lowcut_freq": 100, "eq_mid2gain": 2.5, "eq_highgain": 2.5, "chorus_mix": 12,
                   "widener_width": 120, "delay_sync": 6, "delay_mix": 16, "delay_feedback": 30, "reverb_mix": 22}),
    "Indie": (["gate", "pop", "tune", "lowcut", "eq", "comp", "sat", "lofi", "glue", "slap", "reverb", "limiter"],
              {"tune_amount": 50, "tune_speed": 150, "lowcut_freq": 80, "eq_mid1gain": -2.5, "eq_mid1freq": 450,
               "eq_mid2gain": 1, "comp_ratio": 3, "comp_attack": 10, "sat_drive": 35, "sat_tone": -10,
               "lofi_bits": 12, "lofi_rate": 20000, "lofi_mix": 20, "slap_mix": 25, "slap_time": 130,
               "reverb_size": 70, "reverb_decay": 60, "reverb_mix": 30, "reverb_tone": 5000}),
    "Hyperpop": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "dist", "exciter", "glue", "air", "harmony", "doubler", "phaser", "delay", "reverb", "limiter"],
                 {"tune_speed": 0, "lowcut_freq": 130, "deess_amount": 12, "eq_highgain": 5, "eq_highfreq": 10000, "eq_mid2gain": 3,
                  "comp_ratio": 10, "comp_attack": 1, "comp_makeup": 5, "dist_drive": 50, "dist_mix": 50, "exciter_amount": 50,
                  "harmony_interval": 12, "harmony_mix": 25, "doubler_detune": 20, "doubler_mix": 50, "phaser_mix": 20,
                  "delay_sync": 4, "delay_mix": 22, "delay_feedback": 40, "reverb_size": 60, "reverb_decay": 55,
                  "reverb_mix": 25, "limiter_gain": 4, "air_amount": 5}),
    "Plugg": (["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "lofi", "glue", "air", "doubler", "delay", "reverb", "limiter"],
              {"tune_speed": 5, "lowcut_freq": 100, "eq_mid1gain": -3, "eq_mid1freq": 500, "eq_mid2gain": 2, "comp_ratio": 5,
               "sat_drive": 40, "sat_tone": 10, "lofi_bits": 10, "lofi_rate": 14000, "lofi_mix": 30, "doubler_mix": 35,
               "delay_sync": 1, "delay_mix": 18, "delay_feedback": 35, "delay_tone": 4000, "reverb_size": 65, "reverb_decay": 60,
               "reverb_mix": 28}),
}

# Ad-lib templates: more space, more width, often a lo-fi/phone/harmony twist
ADLIB_EXTRA = {"delay_mix": 38, "delay_feedback": 50, "reverb_mix": 40, "reverb_size": 75, "reverb_decay": 65,
               "widener_width": 150, "limiter_ceiling": -2.0, "comp_ratio": 6, "gate_thresh": -42}
ADLIB_CHAINS = {
    "Trap": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "widener", "delay", "reverb", "limiter"],
    "R&B": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "doubler", "widener", "delay", "reverb", "limiter"],
    "Rage": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "dist", "sat", "glue", "harmony", "doubler", "widener", "delay", "reverb", "limiter"],
    "Pop": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "air", "widener", "delay", "reverb", "limiter"],
    "Hip-Hop": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "widener", "slap", "reverb", "limiter"],
    "Hyperpop": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "dist", "exciter", "glue", "harmony", "doubler", "widener", "delay", "reverb", "limiter"],
    "Plugg": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "lofi", "glue", "doubler", "widener", "delay", "reverb", "limiter"],
    "Afrobeats": ["gate", "pop", "tune", "lowcut", "deess", "eq", "comp", "sat", "glue", "chorus", "widener", "delay", "reverb", "limiter"],
}

# --------------------------------------------------------------------------- variations
# (name, description, overrides, extra_modules_to_insert_before_delay)
MAIN = {
"Pop": [
 ("Radio Ready",   "Polished, bright and forward. The go-to pop lead.", {}, []),
 ("Crystal Lead",  "Extra air and a tight slap for sparkle.", {"air_amount": 5, "eq_highgain": 3}, ["slap"]),
 ("Stadium",       "Big hall and long throws for anthemic hooks.", {"reverb_size": 85, "reverb_decay": 75, "reverb_mix": 32, "delay_mix": 24}, []),
 ("Dry Intimate",  "Close, warm and almost no space.", {"reverb_mix": 8, "delay_mix": 6, "sat_tone": -15, "comp_ratio": 3}, []),
 ("Dance Floor",   "Pumping compression with wide doubles.", {"comp_ratio": 6, "comp_attack": 2, "doubler_mix": 40}, ["doubler"]),
 ("Soft Ballad",   "Gentle tuning and a plush plate.", {"tune_speed": 90, "tune_humanize": 30, "reverb_size": 70, "reverb_mix": 28, "comp_ratio": 3}, []),
 ("Punchy Hook",   "Transient snap and a loud ceiling.", {"transient_attack": 35, "limiter_gain": 3, "comp_makeup": 4}, ["transient"]),
 ("Synthwave",     "Chorus shimmer and a dotted eighth tail.", {"chorus_mix": 22, "delay_mix": 26, "reverb_tone": 6000}, ["chorus"]),
 ("Clean Acoustic","Minimal processing, natural dynamics.", {"tune_amount": 40, "tune_speed": 180, "comp_ratio": 2.5, "sat_drive": 10, "reverb_mix": 15, "delay_mix": 8}, []),
],
"R&B": [
 ("Velvet",        "Smooth and warm with slow, natural tuning.", {}, []),
 ("Late Night",    "Dark tone, long quarter-note echoes.", {"eq_highgain": -1, "sat_tone": -35, "delay_mix": 20, "delay_feedback": 40}, []),
 ("Silk Falsetto", "Airy top end for high, breathy passages.", {"air_amount": 5, "eq_highgain": 2.5, "deess_amount": 12, "comp_ratio": 2.5}, []),
 ("Neo Soul",      "Lo-fi grit and a vintage slap.", {"lofi_bits": 12, "lofi_mix": 18, "slap_mix": 18, "sat_drive": 32}, ["lofi", "slap"]),
 ("Bedroom",       "Dry and close, like whispering into the mic.", {"reverb_mix": 10, "delay_mix": 8, "doubler_mix": 12, "comp_ratio": 3.5}, []),
 ("Harmony Stack", "Wide octave-up harmony for choruses.", {"harmony_interval": 12, "harmony_mix": 18, "doubler_mix": 35}, ["harmony"]),
 ("Slow Jam",      "Lush long reverb with gentle ducking.", {"reverb_size": 85, "reverb_decay": 75, "reverb_mix": 34, "reverb_duck": 50}, []),
 ("Trap Soul",     "Tighter tuning and modern presence.", {"tune_speed": 20, "eq_mid2gain": 3, "eq_highgain": 2.5, "comp_ratio": 4.5}, []),
 ("Warm Tape",     "Saturated, soft and slightly dark.", {"sat_drive": 45, "sat_tone": -40, "eq_highgain": 0}, []),
 ("Chorus Glow",   "Shimmering chorus for dreamy passages.", {"chorus_mix": 25, "chorus_depth": 40, "doubler_mix": 15}, ["chorus"]),
 ("Phone Intro",   "Lo-fi telephone intro vibe.", {"phone_mix": 85, "phone_drive": 30, "reverb_mix": 15}, ["phone"]),
 ("Wide Stack",    "Stereo width for layered backing vocals.", {"widener_width": 160, "doubler_mix": 40, "doubler_detune": 14}, ["widener"]),
 ("Classic 90s",   "Bright, compressed and a short plate.", {"eq_highgain": 3, "comp_ratio": 4, "reverb_size": 45, "reverb_decay": 40, "reverb_mix": 20, "delay_mix": 10}, []),
 ("Breathy",       "Minimal gain reduction and lots of air.", {"comp_ratio": 2, "air_amount": 6, "deess_amount": 14}, []),
 ("Smoky Lounge",  "Dark, roomy and vintage.", {"sat_tone": -45, "eq_highgain": -2, "reverb_tone": 4500, "reverb_mix": 28, "lofi_bits": 11, "lofi_mix": 15}, ["lofi"]),
 ("Modern Hook",   "Bigger, brighter, louder for the hook section.", {"eq_highgain": 3, "comp_makeup": 5, "limiter_gain": 2, "doubler_mix": 35, "reverb_mix": 30}, []),
],
"Trap": [
 ("Hard Lock",     "Zero-speed tuning and aggressive presence.", {"tune_speed": 0}, []),
 ("Melodic",       "Slightly slower tuning, lush tail.", {"tune_speed": 30, "reverb_mix": 26, "reverb_size": 65, "delay_mix": 24}, []),
 ("Dark Mode",     "Rolled-off highs and a deep octave layer.", {"eq_highgain": 0, "harmony_mix": 20, "sat_tone": -20}, ["harmony"]),
 ("Ice Cold",      "Super bright with heavy exciter.", {"eq_highgain": 4.5, "exciter_amount": 45, "air_amount": 5}, []),
 ("Mumble",        "Soft compression and thick doubles for laid-back flows.", {"comp_ratio": 4, "comp_attack": 6, "doubler_mix": 40}, ["doubler"]),
 ("Drill Lead",    "Punchy, dry and in your face.", {"reverb_mix": 10, "delay_mix": 12, "comp_ratio": 7, "transient_attack": 30, "limiter_gain": 3}, ["transient"]),
 ("Autotune Hero", "Classic hard-tuned trap melody.", {"tune_speed": 0, "tune_amount": 100, "doubler_mix": 30, "delay_mix": 26}, ["doubler"]),
 ("Spacey",        "Long ping-pong echoes and a hall.", {"delay_mix": 30, "delay_feedback": 55, "reverb_size": 80, "reverb_decay": 70, "reverb_mix": 28}, []),
 ("Gritty",        "Distortion and lo-fi edge.", {"dist_drive": 30, "dist_mix": 30, "lofi_bits": 11, "lofi_mix": 20}, ["dist", "lofi"]),
 ("Phone Flow",    "Telephone band-pass for verses.", {"phone_mix": 90, "phone_drive": 35}, ["phone"]),
 ("Club Banger",   "Compressed hard with a hot ceiling.", {"comp_ratio": 8, "comp_makeup": 6, "limiter_gain": 4, "glue_ratio": 3}, []),
 ("Wavy",          "Chorus and slow auto-pan movement.", {"chorus_mix": 20, "autopan_rate": 0.25, "autopan_depth": 40}, ["chorus", "autopan"]),
 ("Clean Trap",    "Transparent, modern and radio-safe.", {"sat_drive": 15, "exciter_amount": 15, "comp_ratio": 5, "delay_mix": 16}, []),
],
"Rage": [
 ("Shredder",      "Distorted, doubled, enormous.", {}, []),
 ("Blown Out",     "Maximum distortion and drive.", {"dist_drive": 75, "dist_mix": 60, "sat_drive": 80, "limiter_gain": 5}, []),
 ("Octave Monster","Heavy octave-down layer.", {"harmony_mix": 35, "harmony_interval": -12, "doubler_mix": 50}, []),
 ("Screamer",      "Bright and piercing with feedback flanger.", {"eq_highgain": 5, "flanger_mix": 25, "flanger_feedback": 60}, ["flanger"]),
 ("Cavern",        "Huge reverb and feedback delay.", {"reverb_size": 95, "reverb_decay": 85, "reverb_mix": 40, "delay_feedback": 60, "delay_mix": 32}, []),
 ("Glitch",        "Bit-crushed and crunchy.", {"lofi_bits": 7, "lofi_rate": 9000, "lofi_mix": 45, "dist_mix": 50}, ["lofi"]),
 ("Dark Rage",     "Low-passed menace with a deep octave.", {"highcut_freq": 7000, "harmony_mix": 30, "eq_highgain": 0, "dist_tone": 5000}, ["highcut"]),
 ("Clean Rage",    "The rage chain without the distortion.", {"dist_mix": 0, "sat_drive": 35, "doubler_mix": 40}, []),
],
"Hip-Hop": [
 ("Boom Bap",      "Dry, punchy and slightly saturated.", {}, []),
 ("Lyrical",       "Clear, present and intelligible.", {"eq_mid2gain": 3.5, "deess_amount": 12, "comp_ratio": 4, "slap_mix": 10}, []),
 ("Vintage Mic",   "Lo-fi band-limited character.", {"lofi_bits": 11, "lofi_mix": 25, "highcut_freq": 10000, "sat_drive": 40}, ["lofi", "highcut"]),
],
"Soul": [
 ("Church",        "Big natural room with gentle tuning.", {}, []),
 ("Motown",        "Tape warmth and a tight slap.", {"sat_drive": 45, "sat_tone": -35, "slap_mix": 20, "reverb_size": 55, "reverb_mix": 22}, []),
],
"Afrobeats": [
 ("Lagos Nights",  "Bright, wide and rhythmic triplet echoes.", {}, []),
 ("Sunset",        "Softer tuning and a longer plate.", {"tune_speed": 50, "reverb_size": 70, "reverb_decay": 60, "reverb_mix": 28}, []),
],
"Indie": [
 ("Garage",        "Lo-fi, saturated and roomy.", {}, []),
 ("Dream Pop",     "Washed in reverb and chorus.", {"chorus_mix": 30, "reverb_size": 90, "reverb_decay": 80, "reverb_mix": 40, "lofi_mix": 10}, ["chorus"]),
],
"Hyperpop": [
 ("Candy",         "Bright, pitched-up and glittering.", {}, []),
 ("Overdrive",     "Harder distortion and octave stacks.", {"dist_drive": 70, "dist_mix": 60, "harmony_mix": 35, "doubler_mix": 55}, []),
],
"Plugg": [
 ("Cloudy",        "Warm, lo-fi and dreamy.", {}, []),
 ("Nostalgic",     "Extra crunch and a long quarter-note echo.", {"lofi_bits": 8, "lofi_mix": 40, "delay_mix": 24, "delay_feedback": 45}, []),
],
}

ADLIB = {
"Trap": [
 ("Echo Chamber",  "Classic wide ad-lib with long eighth-note echoes.", {}, []),
 ("Whisper",       "Low, hushed layer with heavy reverb.", {"reverb_mix": 50, "comp_ratio": 8, "eq_highgain": 1}, []),
 ("Phone Yell",    "Telephone band-pass and distortion.", {"phone_mix": 100, "phone_drive": 45}, ["phone"]),
 ("Pitched Down",  "Octave-down ad-lib.", {"harmony_interval": -12, "harmony_mix": 60}, ["harmony"]),
 ("Pitched Up",    "Octave-up chipmunk layer.", {"harmony_interval": 12, "harmony_mix": 55}, ["harmony"]),
 ("Panner",        "Auto-panned across the stereo field.", {"autopan_depth": 85, "autopan_rate": 0.7}, ["autopan"]),
 ("Reverse Feel",  "Long pre-delay swell for a reversed vibe.", {"reverb_predelay": 120, "reverb_size": 90, "reverb_decay": 80, "reverb_mix": 55}, []),
 ("Stutter",       "Fast tremolo chop.", {"tremolo_rate": 10, "tremolo_depth": 80}, ["tremolo"]),
 ("Dry Shout",     "Dry and loud for emphasis words.", {"reverb_mix": 12, "delay_mix": 14, "limiter_gain": 4}, []),
],
"R&B": [
 ("Oohs",          "Smooth, wide and lush.", {}, []),
 ("Backing Stack", "Doubled and widened for harmonies.", {"doubler_mix": 50, "doubler_detune": 15, "widener_width": 170}, []),
 ("Distant",       "Pushed far back in the mix.", {"reverb_mix": 60, "highcut_freq": 8000, "comp_ratio": 6}, ["highcut"]),
 ("Chorus Pad",    "Chorused for a pad-like texture.", {"chorus_mix": 40, "chorus_depth": 50, "reverb_mix": 45}, ["chorus"]),
 ("Breath Layer",  "Airy with lots of de-essing.", {"air_amount": 6, "deess_amount": 16}, ["air"]),
 ("Low Harmony",   "Octave-down support layer.", {"harmony_interval": -12, "harmony_mix": 50}, ["harmony"]),
 ("Tape Echo",     "Dark feedback echoes.", {"delay_tone": 3500, "delay_feedback": 60, "delay_sync": 1}, []),
 ("Quiet Talk",    "Spoken-word ad-lib, dry and close.", {"reverb_mix": 15, "delay_mix": 10, "tune_amount": 0}, []),
],
"Rage": [
 ("Screams",       "Distorted, doubled and huge.", {}, []),
 ("Demon",         "Octave-down with heavy distortion.", {"harmony_mix": 60, "dist_drive": 70, "dist_mix": 60}, []),
 ("Chipmunk",      "Octave-up rage ad-lib.", {"harmony_interval": 12, "harmony_mix": 55}, []),
 ("Crusher",       "Bit-crushed chaos.", {"lofi_bits": 6, "lofi_rate": 8000, "lofi_mix": 60}, ["lofi"]),
 ("Flange Scream", "Sweeping flanger on top.", {"flanger_mix": 35, "flanger_feedback": 70}, ["flanger"]),
 ("Far Away",      "Drowned in cavernous reverb.", {"reverb_size": 100, "reverb_decay": 90, "reverb_mix": 65}, []),
 ("Yeah Shout",    "Dry and punchy call-outs.", {"reverb_mix": 15, "delay_mix": 18, "limiter_gain": 5}, []),
],
"Pop": [
 ("Shimmer",       "Bright and airy with wide echoes.", {}, []),
 ("Harmony Up",    "Octave-up sparkle layer.", {"harmony_interval": 12, "harmony_mix": 45}, ["harmony"]),
 ("Soft Pad",      "Chorused and washed out.", {"chorus_mix": 35, "reverb_mix": 50}, ["chorus"]),
 ("Telephone",     "Phone-filtered pop ad-lib.", {"phone_mix": 100, "phone_drive": 25}, ["phone"]),
 ("Double Me",     "Thick stereo doubles.", {"doubler_mix": 55, "doubler_detune": 18}, ["doubler"]),
],
"Hip-Hop": [
 ("Hype",          "Dry, loud and compressed hype ad-lib.", {}, []),
 ("Crowd",         "Wide, roomy crowd chant.", {"reverb_size": 65, "reverb_mix": 40, "doubler_mix": 50}, ["doubler"]),
 ("Megaphone",     "Megaphone grit and band-pass.", {"phone_mix": 100, "phone_drive": 60, "phone_low": 700, "phone_high": 2800}, ["phone"]),
],
"Hyperpop": [
 ("Glitter",       "Pitched-up, distorted sparkle.", {}, []),
 ("Robot",         "Hard tuned, bit-crushed and phased.", {"lofi_bits": 8, "lofi_mix": 50, "phaser_mix": 40}, ["lofi", "phaser"]),
],
"Plugg": [
 ("Cloud Echo",    "Lo-fi echoes and soft tails.", {}, []),
 ("Underwater",    "Dark low-passed wash.", {"highcut_freq": 5000, "reverb_mix": 55, "lofi_mix": 40}, ["highcut"]),
],
"Afrobeats": [
 ("Chant",         "Wide triplet echoes for group chants.", {"doubler_mix": 45}, ["doubler"]),
],
}

# --------------------------------------------------------------------------- build
def build(genre, variants, adlib):
    out = []
    chain_base, g_over = GENRES[genre]
    if adlib:
        chain_base = ADLIB_CHAINS[genre]
    for name, desc, over, extra in variants:
        values = dict(COMMON)
        values.update(g_over)
        if adlib:
            values.update(ADLIB_EXTRA)
        values.update(over)
        chain = list(chain_base)
        for m in extra:
            if m not in chain:
                # insert before the first space module (delay/slap/reverb) or before limiter
                idx = next((i for i, c in enumerate(chain) if c in ("slap", "delay", "reverb", "limiter")), len(chain))
                chain.insert(idx, m)
        # only keep values for modules that are in the chain (keeps presets compact)
        keep = {k: v for k, v in values.items() if k.split("_")[0] in chain}
        full = f"{genre} - {name}"
        out.append((full, genre, "adlib" if adlib else "main", desc, chain, keep))
    return out

# Init presets first: a blank canvas and a transparent utility chain to build on
INIT = [
    ("Init - Empty", "Init", "main", "Blank canvas. Open the MIX tab, press + ADD and build your own chain.", [], {}),
    ("Init - Clean Chain", "Init", "main",
     "Hygiene only: light gate, pop filter, low cut, gentle de-esser, flat EQ, 2:1 compression, safety limiter.",
     ["gate", "pop", "lowcut", "deess", "eq", "comp", "limiter"],
     {"gate_thresh": -60, "gate_release": 200, "gate_range": -30,
      "pop_sens": 50, "pop_strength": 70, "pop_release": 120,
      "lowcut_freq": 80, "lowcut_q": 0.71,
      "deess_freq": 6500, "deess_thresh": -30, "deess_amount": 6,
      "eq_lowgain": 0, "eq_lowfreq": 150, "eq_mid1gain": 0, "eq_mid1freq": 400, "eq_mid1q": 1.0,
      "eq_mid2gain": 0, "eq_mid2freq": 3000, "eq_mid2q": 1.0, "eq_highgain": 0, "eq_highfreq": 9000,
      "comp_thresh": -24, "comp_ratio": 2, "comp_attack": 10, "comp_release": 100, "comp_makeup": 0, "comp_mix": 100,
      "limiter_gain": 0, "limiter_ceiling": -0.5, "limiter_release": 60}),
    ("Init - Full Chain", "Init", "main",
     "Every standard vocal tool in a sensible order, all set neutral: turn up what you want.",
     ["gate", "pop", "tune", "lowcut", "breath", "deess", "eq", "comp", "sat", "glue", "air", "doubler", "delay", "reverb", "limiter"],
     {"gate_thresh": -60, "gate_release": 200, "gate_range": -30,
      "pop_sens": 50, "pop_strength": 70, "pop_release": 120,
      "tune_amount": 100, "tune_speed": 120, "tune_humanize": 40, "tune_flex": 20,
      "lowcut_freq": 80, "lowcut_q": 0.71,
      "breath_sens": 35, "breath_reduction": 8, "breath_speed": 80,
      "deess_freq": 6500, "deess_thresh": -30, "deess_amount": 6,
      "eq_lowgain": 0, "eq_lowfreq": 150, "eq_mid1gain": 0, "eq_mid1freq": 400, "eq_mid1q": 1.0,
      "eq_mid2gain": 0, "eq_mid2freq": 3000, "eq_mid2q": 1.0, "eq_highgain": 0, "eq_highfreq": 9000,
      "comp_thresh": -24, "comp_ratio": 2, "comp_attack": 10, "comp_release": 100, "comp_makeup": 0, "comp_mix": 100,
      "sat_drive": 0, "sat_tone": 0, "sat_mix": 100,
      "glue_thresh": -24, "glue_ratio": 1.5, "glue_attack": 20, "glue_release": 300, "glue_makeup": 0,
      "air_freq": 12000, "air_amount": 0,
      "doubler_detune": 10, "doubler_delay": 18, "doubler_width": 100, "doubler_mix": 0,
      "delay_sync": 2, "delay_time": 375, "delay_feedback": 35, "delay_spread": 100, "delay_tone": 6000, "delay_lowcut": 250, "delay_duck": 45, "delay_mix": 0,
      "reverb_size": 55, "reverb_decay": 45, "reverb_predelay": 25, "reverb_tone": 8000, "reverb_width": 100, "reverb_duck": 30, "reverb_mix": 0,
      "limiter_gain": 0, "limiter_ceiling": -0.5, "limiter_release": 60}),
]

presets = list(INIT)
for g in ["Pop", "R&B", "Trap", "Rage", "Hip-Hop", "Soul", "Afrobeats", "Indie", "Hyperpop", "Plugg"]:
    presets += build(g, MAIN[g], False)
for g in ["Trap", "R&B", "Rage", "Pop", "Hip-Hop", "Hyperpop", "Plugg", "Afrobeats"]:
    presets += build(g, ADLIB[g], True)

n_main = sum(1 for p in presets if p[2] == "main" and p[1] != "Init")
n_adlib = sum(1 for p in presets if p[2] == "adlib")
assert n_main == 59, n_main
assert n_adlib == 37, n_adlib
names = [p[0] for p in presets]
assert len(set(names)) == len(names), "duplicate preset names"

def fmt(v):
    if isinstance(v, float):
        s = f"{v:.3f}".rstrip("0").rstrip(".")
        return s if s else "0"
    return str(v)

lines = ["// GENERATED by tools/gen_presets.py - do not edit by hand.",
         "// Format: name|genre|main/adlib|description|chain|id=value;id=value",
         "namespace vocalyst {",
         "extern const char* const kFactoryPresetData[];",
         "extern const int kNumFactoryPresets;",
         "const char* const kFactoryPresetData[] = {"]
for full, genre, cat, desc, chain, keep in presets:
    kv = ";".join(f"{k}={fmt(v)}" for k, v in sorted(keep.items()))
    s = f"{full}|{genre}|{cat}|{desc}|{','.join(chain)}|{kv}"
    s = s.replace("\\", "\\\\").replace('"', '\\"')
    lines.append(f'    "{s}",')
lines.append("};")
lines.append(f"const int kNumFactoryPresets = {len(presets)};")
lines.append("} // namespace vocalyst")

with open(OUT, "w") as f:
    f.write("\n".join(lines) + "\n")
print(f"wrote {OUT}: {len(INIT)} init + {n_main} main + {n_adlib} adlib = {len(presets)} presets")
