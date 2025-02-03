import xml.etree.ElementTree as ET
import csv


def find_common_vehicle_ids(file_path, start_timestep, end_timestep):
    tree = ET.parse(file_path)
    root = tree.getroot()

    common_vehicle_ids = None

    for timestep in root.findall("timestep"):
        time = float(timestep.get("time"))

        if start_timestep <= time <= end_timestep:
            current_ids = {
                vehicle.get("id")
                for edge in timestep.findall("edge")
                for lane in edge.findall("lane")
                for vehicle in lane.findall("vehicle")
            }
            if common_vehicle_ids is None:
                common_vehicle_ids = current_ids
            else:
                common_vehicle_ids.intersection_update(current_ids)

    return common_vehicle_ids if common_vehicle_ids else set()


def get_vehicle_positions(file_path, vehicle_ids, start_timestep, end_timestep):
    tree = ET.parse(file_path)
    root = tree.getroot()

    result = {}

    for timestep in root.findall("timestep"):
        time = float(timestep.get("time"))

        if start_timestep <= time <= end_timestep:
            timestep_data = []

            for vehicle_id in vehicle_ids:
                found = False
                for edge in timestep.findall("edge"):
                    for lane in edge.findall("lane"):
                        for vehicle in lane.findall("vehicle"):
                            if vehicle.get("id") == str(vehicle_id):
                                pos = float(vehicle.get("pos", 0.0))
                                speed = float(vehicle.get("speed", 0.0))
                                lane_id = lane.get("id")
                                timestep_data.append((vehicle_id, lane_id, pos, speed))
                                found = True
                                break
                        if found:
                            break
                    if found:
                        break

                if not found:
                    timestep_data.append((vehicle_id, None, None, None))

            result[time] = timestep_data

    return result


def calculate_coordinates_from_lane(shape, pos, lane_length):
    points = [tuple(map(float, point.split(","))) for point in shape.split()]

    x1, y1 = points[0]
    x2, y2 = points[-1]

    x = x1 + (pos / lane_length) * (x2 - x1)
    y = y1 + (pos / lane_length) * (y2 - y1)
    z = 0

    return x, y, z


def convert_to_coordinates(vehicle_data, net_file):
    tree = ET.parse(net_file)
    root = tree.getroot()

    lane_data = {}
    for edge in root.findall("edge"):
        for lane in edge.findall("lane"):
            lane_id = lane.get("id")
            shape = lane.get("shape")
            length = float(lane.get("length"))
            lane_data[lane_id] = {"shape": shape, "length": length}

    result = {}
    for timestep, vehicles in vehicle_data.items():
        timestep_coords = []
        for vehicle_id, lane_id, pos, speed in vehicles:
            if lane_id in lane_data and pos is not None and speed is not None:
                shape = lane_data[lane_id]["shape"]
                length = lane_data[lane_id]["length"]
                x, y, z = calculate_coordinates_from_lane(shape, pos, length)
                timestep_coords.append((vehicle_id, (x, y, z), speed))
            else:
                timestep_coords.append((vehicle_id, None, None))
        result[timestep] = timestep_coords

    return result


def create_coordinates_csv(output_file, coordinates):
    with open(output_file, mode="w") as csvfile:
        writer = csv.writer(csvfile)

        # writer.writerow(["Timestep", "Vehicle ID", "X", "Y", "Z", "Speed"])
        for timestep in sorted(coordinates.keys()):
            timestep_data = sorted(coordinates[timestep], key=lambda x: x[0])
            for _, coord, speed in timestep_data:
                if coord is not None:
                    x, y, z = coord
                    writer.writerow([x, y, z, speed])
                # else:
                #     writer.writerow([timestep, vehicle_id, None, None, None, None])


# Main script
raw_file = "strogino/raw.xml"
net_file = "strogino/strogino.net.xml"
start_timestep = 400
end_timestep = 900

vehicle_quantity = int(input("Enter the number of vehicles to track: "))

vehicle_ids = sorted(
    list(map(int, find_common_vehicle_ids(raw_file, start_timestep, end_timestep)))
)

while vehicle_quantity > len(vehicle_ids):
    vehicle_quantity = int(input("Enter the number of vehicles to track: "))

vehicle_positions = get_vehicle_positions(
    raw_file, vehicle_ids[:vehicle_quantity], start_timestep, end_timestep
)

coordinates = convert_to_coordinates(vehicle_positions, net_file)

output_file = "data.csv"
create_coordinates_csv(output_file, coordinates)
