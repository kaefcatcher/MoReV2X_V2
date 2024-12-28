import yaml
import os
import subprocess
import argparse

def parse_and_run(config_file):
    # Load YAML config
    with open(config_file, 'r') as file:
        config = yaml.safe_load(file)
    
    # Base command
    base_command = "./waf --run 'scratch/HIGHWAY"
    
    # Append parameters
    params = []
    for key, value in config.items():
        if isinstance(value, list):
            value = ','.join(map(str, value))  # Convert list to comma-separated string
        params.append("--{}={}".format(key, value))
    
    # Construct the full command
    full_command = "{} {}'".format(base_command, ' '.join(params))
    
    # Print and execute
    print("Running command: {}".format(full_command))
    subprocess.call(full_command, shell=True)

if __name__ == "__main__":
    import optparse
    parser = optparse.OptionParser(usage="usage: %prog [options]")
    parser.add_option("-c", "--config", dest="config_file", 
                      help="Path to the YAML configuration file")
    (options, args) = parser.parse_args()

    if not options.config_file:
        print("Error: You must specify a YAML configuration file using -c or --config")
        exit(1)

    # Parse and run using the provided config file path
    parse_and_run(options.config_file)
