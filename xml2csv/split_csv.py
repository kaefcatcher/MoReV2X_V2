import os
import csv
import sys

def process_and_split_csv(input_csv):
    # Create 'sim' directory if it doesn't exist
    output_dir = "sim"
    os.makedirs(output_dir, exist_ok=True)

    # Dictionary to store data grouped by timestamp
    timestamp_data = {}

    # Read the input CSV
    with open(input_csv, mode='r') as csvfile:
        csv_reader = csv.DictReader(csvfile)
        for row in csv_reader:
            timestamp = float(row["time"])
            vehicle_id = row["vehicle_id"]
            edge_id = row["edge_id"]
            lane_id = row["lane_id"]
            pos = float(row["position"])
            speed = float(row["speed"])

            # Calculate position and coordinates
            if "forward" in vehicle_id:
                x = pos
            else:  # reverse direction
                x = 20000 - pos

            y = 10  # constant
            z = {"E0_0": 0, "E0_1": 5, "E0_2": 10, "E1_0": 15, "E1_1": 20, "E1_2": 25}.get(lane_id, 0)

            # Add processed data to the dictionary
            if timestamp not in timestamp_data:
                timestamp_data[timestamp] = []
            timestamp_data[timestamp].append((vehicle_id, x, y, z, speed))

    # Write each timestamp data to a separate CSV
    for timestamp, data in timestamp_data.items():
        # Sort by vehicle_id
        sorted_data = sorted(data, key=lambda x: x[0])

        # Output filename
        if timestamp<1000:
            output_file = os.path.join(output_dir, f"pos_{int(timestamp)}.csv")
        else:
            output_file = os.path.join(output_dir, f"pos__{int(timestamp)}.csv")

        # Write the processed data to the CSV
        with open(output_file, mode='w', newline='') as csvfile:
            csv_writer = csv.writer(csvfile)
            # csv_writer.writerow(["x", "y", "z", "speed"])  # header
            for _, x, y, z, speed in sorted_data:
                csv_writer.writerow([x, y, z, speed])

    print(f"Processed CSV files saved in '{output_dir}' directory.")

if __name__ == "__main__":
    # Check arguments
    if len(sys.argv) != 2:
        print("Usage: python split_csv.py {input_csv}")
        sys.exit(1)

    # Get the input CSV file
    input_csv = sys.argv[1]

    # Process and split the CSV
    process_and_split_csv(input_csv)
