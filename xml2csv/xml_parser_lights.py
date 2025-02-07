import xml.etree.ElementTree as ET
import csv

def xml_parser_lights(raw_file, net_file, output_file):
    def reassign_vehicle_ids(file_path):
        tree = ET.parse(file_path)
        root = tree.getroot()

        vehicle_mapping = {}
        new_id = 1

        for timestep in root.findall("timestep"):
            for edge in timestep.findall("edge"):
                for lane in edge.findall("lane"):
                    for vehicle in lane.findall("vehicle"):
                        old_id = vehicle.get("id")
                        if old_id not in vehicle_mapping:
                            """
                            if new_id > 306:
                                raise ValueError("Number of vehicles exceeds 306")
                            """
                            vehicle_mapping[old_id] = str(new_id)
                            new_id += 1
                        vehicle.set("id", vehicle_mapping[old_id])

        return vehicle_mapping


    def get_vehicle_positions(file_path, vehicle_ids):
        tree = ET.parse(file_path)
        root = tree.getroot()

        result = {}

        for timestep in root.findall("timestep"):
            time = float(timestep.get("time"))
            timestep_data = []

            for vehicle_id, raw_number_id in vehicle_ids.items():
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
                    timestep_data.append((vehicle_id, None, -10**6-5*10**3*(int(raw_number_id)-1), 0))

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
                if lane_id in lane_data and lane_id is not None:
                    shape = lane_data[lane_id]["shape"]
                    length = lane_data[lane_id]["length"]
                    x, y, z = calculate_coordinates_from_lane(shape, pos, length)
                    timestep_coords.append((vehicle_id, (x, y, z), speed))
                else:
                    timestep_coords.append((vehicle_id, pos, speed))
            result[timestep] = timestep_coords

        return result

    def create_coordinates_csv(output_file, coordinates):
        with open(output_file, mode="w") as csvfile:
            writer = csv.writer(csvfile)
            for timestep in sorted(coordinates.keys()):
                timestep_data = sorted(coordinates[timestep], key=lambda x: x[0])
                for _, coord, speed in timestep_data:
                    if type(coord) != int:
                        x, y, z = coord
                        writer.writerow([x, y, z, speed])
                    else:
                        writer.writerow([coord, 0, 0, speed])

    vehicle_mapping = reassign_vehicle_ids(raw_file)
    vehicle_mapping.keys()

    vehicle_positions = get_vehicle_positions(raw_file, vehicle_mapping)

    coordinates = convert_to_coordinates(vehicle_positions, net_file)

    create_coordinates_csv(output_file, coordinates)

xml_parser_lights("traffic_light_big/raw.xml", "traffic_light_big/highway.net.xml", "data_traffic_light_big.csv")
print(1)
xml_parser_lights("traffic_light_large/raw.xml", "traffic_light_large/highway.net.xml", "data_traffic_light_large.csv")
print(2)
xml_parser_lights("traffic_light_medium/raw.xml", "traffic_light_medium/highway.net.xml", "data_traffic_light_medium.csv")
print(3)
xml_parser_lights("traffic_light_small/raw.xml", "traffic_light_small/highway.net.xml", "data_traffic_light_small.csv")
print(4)
xml_parser_lights("2_traffic_lights_large/raw.xml", "2_traffic_lights_large/highway.net.xml", "data_2_traffic_lights_large.csv")
print(5)
xml_parser_lights("baseline_large/raw.xml", "baseline_large/highway.net.xml", "data_baseline_large.csv")
print(6)