import numpy as np
import sys
import configparser
import yaml
from librosa.core import load
from collections import namedtuple
from pathlib import Path



config_file = "./tools/samples/samples.yml"
input_path = Path(config_file).parent
output_path = "./BLEEP_DRUM_15/samples"

SampleConfig = namedtuple('SampleConfig', ('sets', 'sample_rate', 'firmware_size', 'flash_size'))

class BleepSample:
    def __init__(self, name, config, input_path = input_path, output_path = output_path):
        self.name = name
        self.config = config

        self.input_path = Path(input_path)
        self.output_path = Path(output_path)
        self.output_path.mkdir(parents=True, exist_ok=True)            # create output_path directory if it doesn't exist

        self.make_header()
        self.edit_platformio()

    @property
    def available(self):
        """
        Flash memory available for samples
        """
        return self.config.flash_size - self.config.firmware_size


    def resample(self, source):
        """
        Opens a WAV file, resamples it, trim zeros and ending,  to unsigned integer
        """
        sample, sr = load(self.input_path / Path(source.file), sr=source.sr, mono=True)                 # load the wav file & resample it to 10kHz
        sample = sample / np.max(np.abs(sample))                    # normalize
        sample = (127 * sample).astype(int)                         # convert to int
        sample = np.trim_zeros(sample)                              # trim any leading and trailing zeros
        if source.trim > 0:                                         # remove the last 'trim' samples to make some space...
            sample = sample[:-source.trim]                          # TODO: apply window or fade out to avoid any click
        sample = sample + 127                                       # apply offset to use unsigned int
        return sample


    def make_header(self):
        """
        Makes a header file from a set of WAV files
        """
        samples_size = 0

        code = list()
        header = ["#include <Arduino.h>", "/*"]
        for i, source in enumerate(self.config.sets[self.name]):
            sample_8bit = self.resample(source)

            code.append(
(               f"const byte table{i}[] PROGMEM = {{" 
                f"{','.join([str(s) for s in sample_8bit])} }};\n"
                f"uint16_t length{i} = {len(sample_8bit)};\n"
                )
            )

            header.append(f" * {f'table{i}':<10}{source.file:<20}{len(sample_8bit):>6}")
            samples_size += len(sample_8bit)
        header.append(f" * {'------':>36}")
        header.append(f" * Total{samples_size:>31}")
        header.append(" */")

        output_file = Path(output_path) / Path(f"samples_{self.name}").with_suffix(".h")
        with open(output_file, 'w') as _file:
            _file.write("\n".join(header + code))

        if samples_size > self.available:
            raise Exception(f"Not enough memory for {self.name} - set trims or change sample rate to reduce by {samples_size - self.available} bytes")

    def edit_platformio(self):
        """
        Adds the new sample to platformio.ini
        """

        pio = configparser.ConfigParser()
        pio.read('platformio.ini')

        pio[f'env:{self.name.lower()}'] = {
            'build_flags': f"\n${{env.build_flags}}\n-D {self.name.upper()}\n-D CUSTOM_SAMPLES"
        }
        
        with open('platformio.ini', 'w') as _file:
            pio.write(_file)


def read_config(config_file):
    """
    Read configuration YAML and return the results
    """
    config = yaml.load(open(config_file, 'r'), Loader=yaml.FullLoader)

    try:
        firmware_size = config["firmware_size"]
    except KeyError:
        firmware_size = 0

    try:
        flash_size = config["flash_size"]
    except KeyError:
        flash_size = 32256

    try:
        sr = config["sr"]
    except KeyError:
        sr = 9813 * 2 # this number is taken from Bleep Drum's measured audio clock

    SampleSet = namedtuple('SampleSet', ('file', 'trim', 'sr'), defaults=(None, 0, sr))

    samples = dict()
    for key, sample_set in config["samples"].items():
        samples[key] = [SampleSet(sample) if isinstance(sample, str) else SampleSet(**sample) for sample in sample_set]
    
    return SampleConfig(samples, sr, firmware_size, flash_size)



def make_samples_h(env_names):
    """
    generates samples.h to include all custom environments
    """
    output = list()
    for i, env in enumerate(env_names):
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

    config = read_config(config_file)

    for name in config.sets.keys():
        BleepSample(name, config, input_path, output_path)
    
    make_samples_h(config.sets.keys())