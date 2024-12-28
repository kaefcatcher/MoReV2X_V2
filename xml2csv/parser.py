import sys
import xml.etree.ElementTree as ET
import csv

def parse_netstate_xml(xml_file, csv_file, start_time, end_time):
    try:
        # Parse XML file
        tree = ET.parse(xml_file)
        root = tree.getroot()

        # Open CSV file for writing
        with open(csv_file, mode='w', newline='') as csvfile:
            csv_writer = csv.writer(csvfile)
            # Write header
            csv_writer.writerow(["time", "vehicle_id", "edge_id", "lane_id", "position", "speed"])

            # Iterate over timestep elements
            for timestep in root.findall('timestep'):
                time = float(timestep.get('time'))
                if start_time <= time <= end_time:
                    # Iterate over edges
                    for edge in timestep.findall('edge'):
                        edge_id = edge.get('id')
                        for lane in edge.findall('lane'):
                            lane_id = lane.get('id')
                            for vehicle in lane.findall('vehicle'):
                                vehicle_id = vehicle.get('id')
                                position = vehicle.get('pos')
                                speed = vehicle.get('speed')
                                # Write row to CSV
                                csv_writer.writerow([time, vehicle_id, edge_id, lane_id, position, speed])
        
        print(f"CSV file '{csv_file}' generated successfully!")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    # Check arguments
    if len(sys.argv) != 4:
        print("Usage: python parser.py {begin_timestamp} {end_timestamp} {xml_file}")
        sys.exit(1)

    # Get arguments
    begin_timestamp = float(sys.argv[1])
    end_timestamp = float(sys.argv[2])
    xml_file = sys.argv[3]
    csv_file = "output.csv"

    # Run parser
    parse_netstate_xml(xml_file, csv_file, begin_timestamp, end_timestamp)
