# Bleep Drum for PlatformIO

This is the code used in [arduino in platformio : programming the bleep drum](https://www.youtube.com/watch?v=HCvKrhoXOpg)

Based on the original code from Bleep Labs : https://github.com/BleepLabs/Bleep-Drum

## Build instruction

### Bleep Drum Samples

`pio run -e bleep`

### Dam Drum samples

`pio run -e dam`

### Dam Drum v2 samples

`pio run -e dam2`

### Dam Drum v3 samples

`pio run -e dam3`


## Program custom samples

0. Install the bleep tool with Python 3

```pip3 install .```

If you plan on modifying the script, make this installation editable

```pip3 install -e .```


1. Create YAML file to specify the source of the samples, let's call it `samples.yml`


```yaml
firmware_size: 11658        # put firmware size without the samples here - this is used to warn you if you samples won't fit on flash
# sample_rate: 17000        # sample rate for resampling the WAV files - leave it commented to use the default

samples:
  tr808:                    # this is the name of the environment
    input_path: path/to/my/808
    samples:
      - file: 808-hat.wav   # 
        normalize: no       # don't normalize
        sr: 15000           # resample at 15kHz
        trim: 100           # trim the end by 100 samples
      - 808-tom.wav         # only the file name is required, just enter it as a string if you don't need to tweak other options
      - 808-snare.wav
      - 808-kick.wav

  tr909:                    # without specifying an input path, it will use the location of this file as a root directory
    - 909-clap.wav
    - file: 909-tom.wav
      trim: 134
    - 909-snare.wav
    - 909-kick.wav
```

2. Run the script : `bleep samples.yml`

This will generate all header files from the specified WAV files and update platformio.ini with the new custom environments.

3. Program your samples by specifying the environment name `pio run -t upload -e tr808`


# Bleep Drum Release notes

April 2020:<br>
V15 <br>
Click track – Hold shift for 4 seconds to turn on and off click while the device is playing.<br>
Clock rate stability – The sequence could slow down if too much was happening in the old version.<br>
Better noise mode – Greater range of distorted sounds (hold shift while turning device on)<br>
https://bleeplabs.com/2020/04/19/limited-quantity-of-bleep-drum-available-to-ship-now/<br>
<br><br>

v12<br>
http://bleeplabs.com/store/bleep-drum-midi/<br>
Now compatible with current versions of MIDI, bounce and pgmspace.<br>
It is no longer necessary to edit MIDI.h<br>
<br>
Dam Drum sounds.txt contains the samples used in the second and third Dam Drum. All that was differnt in the Dam vs Bleep drums was the sounds. http://www.stonesthrow.com/news/2013/01/dam-drum<br>
<br>
Here's a guide for getting your own sounds into the Bleep Drum.
http://bleeplabs.com/2013/04/07/putting-your-own-samples-in-the-bleep-drum/<br>
<br>
All work licensed under a Creative Commons Attribution-ShareAlike 3.0
