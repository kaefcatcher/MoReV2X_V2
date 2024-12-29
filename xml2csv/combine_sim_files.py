from __future__ import print_function
import os
import sys


def combine_sim_files(sim_directory, output_file, lines_per_file=300):
    try:
        if not os.path.exists(sim_directory):
            print("Error: Directory '{}' does not exist.".format(sim_directory))
            return

        sim_files = sorted(
            [
                f
                for f in os.listdir(sim_directory)
                if os.path.isfile(os.path.join(sim_directory, f))
            ]
        )

        with open(output_file, "wb") as outfile:
            for sim_file in sim_files:
                file_path = os.path.join(sim_directory, sim_file)
                print("Processing file: {}".format(sim_file))

                with open(file_path, "rb") as infile:
                    for i, line in enumerate(infile):
                        if i >= lines_per_file:
                            break
                        outfile.write(line)

        print("Combined files written to {}".format(output_file))

    except Exception as e:
        print("An error occurred: {}".format(e))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(
            "Usage: python combine_sim_files.py <sim_directory> <output_file> <number_of_vehicles_in_simulation>"
        )
        sys.exit(1)

    sim_directory = sys.argv[1]
    output_file = sys.argv[2]
    num_vehicles = sys.argv[3]

    combine_sim_files(sim_directory, output_file, num_vehicles)
