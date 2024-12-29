from __future__ import division, print_function
import os
import csv
import sys


def process_and_split_csv(input_csv, output_dir, length):

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    timestamp_data = {}

    with open(input_csv, mode="r") as csvfile:
        csv_reader = csv.DictReader(csvfile)
        for row in csv_reader:
            timestamp = float(row["time"])
            vehicle_id = row["vehicle_id"]
            edge_id = row["edge_id"]
            lane_id = row["lane_id"]
            pos = float(row["position"])
            speed = float(row["speed"])

            if "forward" in vehicle_id:
                x = pos
            else:  # reverse direction
                x = length - pos

            y = 10
            z = {
                "E0_0": 0,
                "E0_1": 5,
                "E0_2": 10,
                "E1_0": 15,
                "E1_1": 20,
                "E1_2": 25,
            }.get(lane_id, 0)

            if timestamp not in timestamp_data:
                timestamp_data[timestamp] = []
            timestamp_data[timestamp].append((vehicle_id, x, y, z, speed))

    for timestamp, data in timestamp_data.iteritems():
        sorted_data = sorted(data, key=lambda x: x[0])

        if timestamp < 1000:
            output_file = os.path.join(output_dir, "pos_%d.csv" % int(timestamp))
        elif timestamp < 10000:
            output_file = os.path.join(output_dir, "pos_p%d.csv" % int(timestamp))
        else:
            output_file = os.path.join(output_dir, "pos_pp%d.csv" % int(timestamp))

        with open(output_file, mode="wb") as csvfile:
            csv_writer = csv.writer(csvfile)
            # csv_writer.writerow(["x", "y", "z", "speed"])  # header
            for _, x, y, z, speed in sorted_data:
                csv_writer.writerow([x, y, z, speed])

    print("Processed CSV files saved in '{}' directory.".format(output_dir))


if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Usage: python split_csv.py {input_csv} {output_dir} {highway_length}")
        sys.exit(1)

    input_csv = sys.argv[1]
    output_dir = sys.argv[2]
    length = int(sys.argv[3])
    process_and_split_csv(input_csv, output_dir, length)
