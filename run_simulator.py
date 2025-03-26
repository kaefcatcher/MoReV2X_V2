import yaml
import subprocess
import optparse
from multiprocessing import Pool, cpu_count
import os

def run_command(command):
    """Function to execute a single command in a subprocess"""
    print "Running command: %s" % command
    try:
        # Using Popen instead of call to prevent waiting for completion
        process = subprocess.Popen(command, shell=True)
        return process
    except Exception as e:
        print "Error running command %s: %s" % (command, str(e))
        return None

def parse_and_run(config_file, max_parallel=None):
    with open(config_file, "r") as file:
        config = yaml.safe_load(file)

    base_command = "./waf --run 'scratch/HIGHWAY"

    # Handle Vehicles separately
    vehicles_config = config.pop("Vehicles", None)

    # Process all other parameters
    param_keys = config.keys()
    param_values = [
        config[key] if isinstance(config[key], list) else [config[key]]
        for key in param_keys
    ]

    # Generate combinations for non-Vehicles parameters
    combinations = [{}]
    for key, values in zip(param_keys, param_values):
        new_combinations = []
        for combination in combinations:
            for value in values:
                new_combination = combination.copy()
                new_combination[key] = value
                new_combinations.append(new_combination)
        combinations = new_combinations

    # Now handle Vehicles combinations
    if vehicles_config:
        new_combinations = []
        if not combinations:
            combinations = [{}]

        for vehicle in vehicles_config:
            for combination in combinations:
                new_combination = combination.copy()
                # Handle both single string and list of tracefiles
                tracefiles = vehicle["tracefile"]
                if isinstance(tracefiles, str):
                    tracefiles = [tracefiles]
                new_combination["Vehicles"] = vehicle["_val"]
                new_combination["tracefile"] = tracefiles
                new_combinations.append(new_combination)
        combinations = new_combinations

    # Prepare all commands
    commands = []
    for combination in combinations:
        params = []
        for key, value in combination.items():
            if key == "tracefile":
                # Handle multiple tracefiles by joining them with commas
                if isinstance(value, list):
                    params.append("--%s=%s" % (key, ",".join(value)))
                else:
                    params.append("--%s=%s" % (key, value))
            else:
                params.append("--%s=%s" % (key, value))

        full_command = "%s %s'" % (base_command, " ".join(params))
        commands.append(full_command)

    # Determine number of parallel processes
    if max_parallel is None:
        # Default to number of CPU cores
        try:
            max_parallel = cpu_count()
        except:
            max_parallel = 1

    print "Executing %d commands with %d parallel processes" % (len(commands), max_parallel)

    # Create a process pool and execute commands
    pool = Pool(processes=max_parallel)
    processes = pool.map(run_command, commands)
    
    # Wait for all processes to complete
    for process in processes:
        if process:
            process.wait()
    
    pool.close()
    pool.join()

if __name__ == "__main__":
    parser = optparse.OptionParser(usage="usage: %prog [options]")
    parser.add_option(
        "-c", "--config", dest="config_file", help="Path to the YAML configuration file"
    )
    parser.add_option(
        "-j", "--jobs", dest="max_parallel", type="int", default=None,
        help="Maximum number of parallel jobs (default: number of CPU cores)"
    )
    (options, args) = parser.parse_args()

    if not options.config_file:
        print "Error: You must specify a YAML configuration file using -c or --config"
        exit(1)

    parse_and_run(options.config_file, options.max_parallel)