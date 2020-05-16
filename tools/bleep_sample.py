import numpy as np
import sys
import configparser
from librosa.core import load
from collections import namedtuple
from pathlib import Path

firmware_size = 11586
flash_size = 32256
available = flash_size - firmware_size

input_path = "./tools/samples/"
output_path = "./BLEEP_DRUM_15/samples"

sr = 9813 * 2 

Sample = namedtuple('Sample', ['file', 'trim', 'sr'])

def resample(source, trim=0, sr=22050, varname="table", lengthname="length"):
    """
    Opens a WAV file, resamples it to unsigned integer and returns the C header code to use in Bleep Drum
    """
    sample, sr = load(source, sr=sr, mono=True)                 # load the wav file & resample it to 10kHz
    sample = sample / np.max(np.abs(sample))                    # normalize
    sample = (127 * sample).astype(int)                         # convert to int
    sample = np.trim_zeros(sample)                              # trim any leading and trailing zeros
    if trim > 0:                                                # remove the last 'trim' samples to make some space...
        sample = sample[:-trim]
    sample = sample + 127                                       # apply offset to use unsigned int

    output = (
        f"const byte {varname}[] PROGMEM = {{" 
        f"{','.join([str(s) for s in sample])} }};\n"
        f"uint16_t {lengthname} = {len(sample)};\n"
        )
    return output, len(sample)


def make_header(input_path, samples, name):
    """
    Makes a header file from a set of WAV files
    """
    samples_size = 0

    code = list()
    header = ["#include <Arduino.h>", "/*"]
    for i, sample in enumerate(samples):
        sample_output, size = resample(Path(input_path) / Path(sample.file), sample.trim, sample.sr, f"table{i}", f"length{i}")
        code.append(sample_output)
        header.append(f" * {f'table{i}':<10}{sample.file:<20}{size:>6}")
        samples_size += size
    header.append(f" * {'------':>36}")
    header.append(f" * Total{samples_size:>31}")
    header.append(" */")

    output_file = Path(output_path) / Path(f"samples_{name}").with_suffix(".h")
    with open(output_file, 'w') as _file:
        _file.write("\n".join(header + code))

    if samples_size > available:
        raise Exception(f"Not enough memory for {name} - set trims or change sample rate to reduce by {samples_size - available} bytes")

def make_headers(samples):
    for name, sample_set in samples.items():
        try:
            make_header(input_path, sample_set, name)
        except Exception as e:
            print(str(e))
    


def edit_platformio(samples):
    """
    Adds the new sample to platformio.ini
    """

    config = configparser.ConfigParser()
    config.read('platformio.ini')

    for env in samples.keys():
        config[f'env:{env.lower()}'] = {
            'build_flags': f"\n${{env.build_flags}}\n-D {env.upper()}\n-D CUSTOM_SAMPLES"
        }
    
    with open('platformio.ini', 'w') as configfile:
        config.write(configfile)


def make_samples_h(samples):
    """
    generates samples.h to include all custom environments
    """
    output = list()
    for i, env in enumerate(samples.keys()):
        if i == 0:
            output.append(f"#ifdef {env.upper()}")
        else:
            output.append(f"#elif {env.upper()}")
        output.append(f'#include "samples_{env.lower()}.h"')
    output.append("#endif")

    output_file = Path(output_path) / Path(f"samples").with_suffix(".h")
    with open(output_file, 'w') as _file:
        _file.write("\n".join(output))

if __name__ == "__main__":

    samples = {
        "tr808": [
            Sample("808-hat.wav", 0, sr),
            Sample("808-tom.wav", 0, sr),
            Sample("808-snare.wav", 0, sr),
            Sample("808-kick.wav", 0, sr),
        ],
        "tr909": [
            Sample("909-clap.wav", 0, sr),
            Sample("909-tom.wav", 134, sr),
            Sample("909-snare.wav", 0, sr),
            Sample("909-kick.wav", 0, sr),
        ],
        "traks": [
            Sample("traks-cowbell.wav", 0, sr),
            Sample("traks-tom.wav", 0, sr),
            Sample("traks-snare.wav", 0, sr),
            Sample("traks-kick.wav", 0, sr),
        ],
        "dx": [
            Sample("dx-shaker.wav", 0, sr),
            Sample("dx-tom.wav", 6300, sr),
            Sample("dx-snare.wav", 0, sr),
            Sample("dx-kick.wav", 0, sr),
        ],
    }


    make_headers(samples)
    edit_platformio(samples)
    make_samples_h(samples)