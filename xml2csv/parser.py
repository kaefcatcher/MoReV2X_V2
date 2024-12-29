from __future__ import division, print_function
import sys
import xml.etree.ElementTree as ET
import csv


def parse_netstate_xml(xml_file, csv_file, start_time, end_time):
    try:
        tree = ET.parse(xml_file)
        root = tree.getroot()

        with open(csv_file, mode="wb") as csvfile:
            csv_writer = csv.writer(csvfile)
            csv_writer.writerow(
                ["time", "vehicle_id", "edge_id", "lane_id", "position", "speed"]
            )

            for timestep in root.findall("timestep"):
                time = float(timestep.get("time"))
                if start_time <= time <= end_time:
                    for edge in timestep.findall("edge"):
                        edge_id = edge.get("id")
                        for lane in edge.findall("lane"):
                            lane_id = lane.get("id")
                            for vehicle in lane.findall("vehicle"):
                                vehicle_id = vehicle.get("id")
                                position = vehicle.get("pos")
                                speed = vehicle.get("speed")
                                csv_writer.writerow(
                                    [
                                        time,
                                        vehicle_id,
                                        edge_id,
                                        lane_id,
                                        position,
                                        speed,
                                    ]
                                )

        print("CSV file '{}' generated successfully!".format(csv_file))

    except Exception as e:
        print("Error: {}".format(e))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python parser.py {begin_timestamp} {end_timestamp} {xml_file}")
        sys.exit(1)

    begin_timestamp = float(sys.argv[1])
    end_timestamp = float(sys.argv[2])
    xml_file = sys.argv[3]
    csv_file = "output.csv"

    parse_netstate_xml(xml_file, csv_file, begin_timestamp, end_timestamp)
