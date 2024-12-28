import os
import sys

def combine_sim_files(sim_directory, output_file, lines_per_file=300):
    try:
        # Ensure the sim directory exists
        if not os.path.exists(sim_directory):
            print(f"Error: Directory '{sim_directory}' does not exist.")
            return

        # Get all files in the sim directory sorted by name
        sim_files = sorted(
            [f for f in os.listdir(sim_directory) if os.path.isfile(os.path.join(sim_directory, f))]
        )

        # Open the output file for writing
        with open(output_file, "w") as outfile:
            for sim_file in sim_files:
                file_path = os.path.join(sim_directory, sim_file)
                print(f"Processing file: {sim_file}")

                # Read the first 300 lines from the current file
                with open(file_path, "r") as infile:
                    for i, line in enumerate(infile):
                        if i >= lines_per_file:
                            break
                        outfile.write(line)

        print(f"Combined files written to {output_file}")
    
    except Exception as e:
        print(f"An error occurred: {e}")

# Main entry point
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python combine_sim_files.py <sim_directory> <output_file>")
        sys.exit(1)

    sim_directory = sys.argv[1]
    output_file = sys.argv[2]

    combine_sim_files(sim_directory, output_file)
