import yaml
import subprocess
import optparse


def parse_and_run(config_file):
    with open(config_file, "r") as file:
        config = yaml.safe_load(file)

    base_command = "./waf --run 'scratch/HIGHWAY"

    param_keys = config.keys()
    param_values = [
        config[key] if isinstance(config[key], list) else [config[key]]
        for key in param_keys
    ]

    combinations = [{}]
    for key, values in zip(param_keys, param_values):
        new_combinations = []
        for combination in combinations:
            for value in values:
                new_combination = combination.copy()
                new_combination[key] = value
                new_combinations.append(new_combination)
        combinations = new_combinations

    for combination in combinations:
        params = []
        for key, value in combination.items():
            params.append("--{}={}".format(key, value))
        full_command = "{} {}'".format(base_command, " ".join(params))
        print("Running command: {}".format(full_command))
        subprocess.call(full_command, shell=True)


if __name__ == "__main__":
    parser = optparse.OptionParser(usage="usage: %prog [options]")
    parser.add_option(
        "-c", "--config", dest="config_file", help="Path to the YAML configuration file"
    )
    (options, args) = parser.parse_args()

    if not options.config_file:
        print("Error: You must specify a YAML configuration file using -c or --config")
        exit(1)

    parse_and_run(options.config_file)
