import numpy as np
import sys
import configparser
import yaml
from librosa.core import load
from collections import namedtuple
from pathlib import Path
import click

w = 85
SampleConfig = namedtuple('SampleConfig', ('sets', 'sample_rate', 'firmware_size', 'flash_size'))

class BleepSample:
    def __init__(self, name, config, output_path):
        self.name = name
        self.config = config

        self.input_path = Path(config.sets[self.name]['input_path'])
        self.output_path = Path(output_path)
        self.output_path.mkdir(parents=True, exist_ok=True)            # create output_path directory if it doesn't exist
        self.root = self.output_path.parent.parent

        self.samples = config.sets[self.name]['samples']

        click.echo("-" * w)
        click.echo(f"{self.name:^{w}}")
        click.echo("-" * w)

        try:
            self.make_header()
        except Exception as e:
            click.secho(str(e), fg="red")
        self.edit_platformio()
        click.echo("")

    @property
    def available(self):
        """
        Flash memory available for samples
        """
        return self.config.flash_size - self.config.firmware_size


    @classmethod
    def resample(self, source, input_path):
        """
        Opens a WAV file, resamples it, trim zeros and ending, to unsigned integer
        Inspired by: http://bleeplabs.com/bleep-drum-user-guide/
        """
        sample, sr = load(input_path / Path(source.file), sr=source.sr, mono=True)     # load the wav file & resample it
        if source.normalize:
            sample = sample / np.max(np.abs(sample))                # normalize
        sample = (127 * sample).astype(int)                         # convert to int
        sample = np.trim_zeros(sample)                              # trim any leading and trailing zeros
        if source.trim > 0:                                         # 
            sample = sample[:-source.trim]                          # remove the last 'trim' samples to make some space...
        sample = sample + 127                                       # apply offset to use unsigned int
        return sample


    def make_header(self):
        """
        Makes a header file from a set of WAV files
        """
        samples_size = 0

        output_file = Path(self.output_path) / Path(f"samples_{self.name}").with_suffix(".h")
        click.echo(f" -> {output_file.relative_to(self.root).as_posix()}")

        code = list()
        header = ["#include <Arduino.h>", "/*"]
        for i, source in enumerate(self.samples):
            sample_8bit = self.resample(source, self.input_path)

            code.append(
                (
                f"const byte table{i}[] PROGMEM = {{" 
                f"{','.join([str(s) for s in sample_8bit])} }};\n"
                f"uint16_t length{i} = {len(sample_8bit)};\n"
                )
            )

            headline = f" * {f'table{i}':<10}{source.file:<66}{len(sample_8bit):>6}"
            header.append(headline)
            samples_size += len(sample_8bit)
        header.append(f" * {'------':>82}")
        header.append(f" * Total{samples_size:>77}")
        header.append(" */")

        click.echo("\n".join(header[2:-1]))

        with open(output_file, 'w') as _file:
            _file.write("\n".join(header + code))

        if samples_size > self.available:
            raise Exception(f"Not enough memory for {self.name} - set trims or change sample rate to reduce by {samples_size - self.available} bytes")

    def edit_platformio(self):
        """
        Adds the new sample to platformio.ini
        """

        pio_ini = self.root / 'platformio.ini'
        pio = configparser.ConfigParser()
        pio.read(pio_ini)

        pio[f'env:{self.name.lower()}'] = {
            'build_flags': f"\n${{env.build_flags}}\n-D {self.name.upper()}\n-D CUSTOM_SAMPLES"
        }
        
        with open(pio_ini, 'w') as _file:
            pio.write(_file)


def read_config(config_file, default_input_path):
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

    SampleSet = namedtuple('SampleSet', ('file', 'trim', 'sr', 'normalize'), defaults=(None, 0, sr, True))

    samples = dict()
    for key, sample_set in config["samples"].items():
        try:
            input_path = sample_set["input_path"]
        except (KeyError, TypeError):
            input_path = default_input_path

        try:
            sample_list = sample_set["samples"]
        except KeyError:
            raise Exception("couldn't find 'samples'")
        except TypeError:
            sample_list = sample_set

        try:
            samples[key] = {
                'input_path': input_path,
                'samples': [SampleSet(sample) if isinstance(sample, str) else SampleSet(**sample) for sample in sample_list]
            }
        except Exception as e:
            click.echo(f"Error in {key}: ")
            click.secho(str(e), fg='red')

    return SampleConfig(samples, sr, firmware_size, flash_size)



def make_samples_h(env_names, output_path):
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


@click.command()
@click.argument('config', type=click.Path())
@click.option('-i', '--input', type=click.Path())
@click.option('-o', '--output', type=click.Path(), default=(Path(__file__).absolute().parent.parent / Path ('BLEEP_DRUM_15/samples')))
def cli(config, input, output):

    config = Path(config)
    if input is None:
        input = config.parent
    output = Path(output)

    click.echo("*" * w)
    click.echo("\nBleep Drum Sample Converter\n")
    click.echo("*" * w)
    click.echo(f"Config: {config.as_posix()}")
    click.echo(f"Input:  {input.absolute().as_posix()}")
    click.echo(f"Output: {output.absolute().as_posix()}\n")
    click.echo("*" * w)

    config = read_config(config, input)

    for name in config.sets.keys():
        BleepSample(name, config, output)
    
    make_samples_h(config.sets.keys(), output)


if __name__ == "__main__":
    sys.exit(cli())
