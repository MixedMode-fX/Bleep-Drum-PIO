from pathlib import Path
from setuptools import setup, find_packages

def parse_requirements(filename):
    """ load requirements from a pip requirements file """
    lineiter = (line.strip() for line in open(Path(filename)))
    return [line for line in lineiter if line and not line.startswith("#")]

setup(
    name='bleep',
    version='0.1',
    packages=find_packages(),
    include_package_data=True,
    install_requires=parse_requirements('./tools/requirements.txt'),
    entry_points='''
        [console_scripts]
        bleep=tools.bleep_sample:cli
    ''',
)