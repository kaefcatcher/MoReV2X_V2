import sys
import time
import pandas as pd
import numpy as np
import os
from collections import defaultdict


'''
Example of usage :
python3 results/metrics.py  base_path(results/Simulations/Periodic_Dynamic0_avgRRI0_VariableSize0_ReEval0_200_PDB0_867) simulation=1005/1009 metrics=cbr/aoi/pdr Matrix_RC515 
or 
python3 results/metrics.py base_path(results/Simulations/Periodic_Dynamic0_avgRRI0_VariableSize0_ReEval0_200_PDB0_867) simulation=1005/1009 metrics=all Matrix_RC515 
'''

start_time = time.time()
distance_limit = 920
# Specify the fields you want to extract
fields = ['rxTime', 'packetID', 'TxDistance', 'txID', 'rxID', 'decoded', 'lossType']

regions_list = list(range(18))  # Array with indexes of regions
print(regions_list)
headers = ''


def filling_permanent_data(simulation_num, regions_list, pkeep):
    global headers
    headers = 'simulation_num, region_value, pkeep, region_weights[region_value]'
    matrix = []
    for region_value in regions_list:
        matrix.append({'simulation_num': simulation_num, 
                       'region_value': (region_value * 50) + 25, 
                       'pkeep': p_keep, 
                       'region_weight': region_weights[region_value]})

    output_path = 'results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv'
    df = pd.DataFrame(matrix)
    df.to_csv(output_path, index=False, float_format='%.6f')


def calculate_pdr(simulation_num, packet_total, packet_decodedorlost, distance_region_index, regions_list,
                              pkeep):
    pdr_matrix = []
    global headers

    # Calculate PDR_matrix
    for key in packet_total:
        rxID, txID = key
        if key not in PDR_matrix:
            PDR_matrix[key] = 0

        if key in packet_decodedorlost:
            PDR_matrix[key] = packet_decodedorlost[key] / packet_total[key]

    # Calculate average PDR for each distance region
    for region_value in regions_list:
        region_sum = 0
        region_counter = 0

        for key, value in PDR_matrix.items():
            rxID, txID = key
            distance_region = distance_region_index[key]

            if rxID != txID and distance_region == region_value:
                region_sum += value
                region_counter += 1

        if region_counter > 0:
            value_pdr = region_sum / region_counter
        else:
            value_pdr = -1

        pdr_matrix.append([simulation_num, (region_value * 50) + 25, pkeep, region_weights[region_value],  value_pdr])

    output_path = 'results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv'
    if len(headers.split(', ')) < 9:
        headers = headers + ', value_pdr'
    existing_data = np.genfromtxt(output_path, delimiter=',', skip_header=1)
    if len(existing_data) > 0:
        combined_data = np.concatenate((existing_data, np.array(pdr_matrix)[:, -1][:, np.newaxis]), axis=1)
    else:
        combined_data = pdr_matrix
    np.savetxt(output_path, combined_data, delimiter=',', fmt='%.6f', header=headers)


def average_pdr():
    output_path = 'results/KPIs_diff_simulations/' + name_of_simulation + '.csv'
    data = np.genfromtxt(output_path, delimiter=',', names=headers.split(', '))
    filtered_data = data[(data['value_pdr'] > 0) & (data['region_value'] < distance_limit)]
    pkeep_values = np.unique(filtered_data['pkeep'])
    average_pdr_per_distance = []

    for pkeep in pkeep_values:
        subset_data = filtered_data[filtered_data['pkeep'] == pkeep]
        average_pdr = np.mean(subset_data['value_pdr'])
        average_pdr_per_distance.append((pkeep, average_pdr))

    output_path_average = 'results/Average_metrics/Average_pdr' + name_of_simulation + '.csv'
    with open(output_path_average, 'w') as file:
        np.savetxt(file, average_pdr_per_distance, delimiter=',', fmt='%.6f', header="pkeep, average_pdr")


def calculate_plr(simulation_num, packet_total, packet_collided, packet_prop_loss, distance_region_index,
                          regions_list, pkeep):
    plr_matrix = []
    global headers
    for key in packet_total:
        rxID, txID = key

        if key not in PLR_matrix:
            PLR_matrix[key] = 0

        if key in packet_prop_loss:
            PLR_matrix[key] = packet_prop_loss[key] / packet_total[key]

    # Calculate average PLR for each distance region
    for region_value in regions_list:
        region_sum = 0
        region_counter = 0

        for key, value in PLR_matrix.items():
            rxID, txID = key
            distance_region = distance_region_index[key]

            if rxID != txID and distance_region == region_value:
                region_sum += value
                region_counter += 1

        if region_counter > 0:
            value_plr = region_sum / region_counter
        else:
            value_plr = -1

        plr_matrix.append(
            [simulation_num, (region_value * 50) + 25, pkeep, region_weights[region_value], value_plr])

    output_path = 'results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv'
    if len(headers.split(', ')) < 9:
        headers = headers + ', value_plr'
    existing_data = np.genfromtxt(output_path, delimiter=',', skip_header=1)
    if len(existing_data) > 0:
        combined_data = np.concatenate((existing_data, np.array(plr_matrix)[:, -1][:, np.newaxis]), axis=1)
    else:
        combined_data = plr_matrix
    np.savetxt(output_path, combined_data, delimiter=',', fmt='%.6f', header=headers)


def average_plr():
    output_path = 'results/KPIs_diff_simulations/' + name_of_simulation + '.csv'
    data = np.genfromtxt(output_path, delimiter=',', names=headers.split(', '))
    filtered_data = data[(data['value_plr'] > 0) & (data['region_value'] < distance_limit)]
    pkeep_values = np.unique(filtered_data['pkeep'])
    average_plr_per_distance = []

    for pkeep in pkeep_values:
        subset_data = filtered_data[filtered_data['pkeep'] == pkeep]
        average_plr = np.mean(subset_data['value_plr'])
        average_plr_per_distance.append((pkeep, average_plr))

    output_path_average = 'results/Average_metrics/Average_plr' + name_of_simulation + '.csv'
    with open(output_path_average, 'w') as file:
        np.savetxt(file, average_plr_per_distance, delimiter=',', fmt='%.6f', header="pkeep, average_plr")


def calculate_clr(simulation_num, packet_total, packet_collided, packet_prop_loss, distance_region_index,
                          regions_list, pkeep):
    clr_matrix = []
    global headers
    # Calculate CLR_matrix
    for key in packet_total:
        rxID, txID = key
        if key not in CLR_matrix:
            CLR_matrix[key] = 0

        if key in packet_collided:
            CLR_matrix[key] = packet_collided[key] / packet_total[key]


    # Calculate average CLR for each distance region
    for region_value in regions_list:
        region_sum = 0
        region_counter = 0

        for key, value in CLR_matrix.items():
            rxID, txID = key
            distance_region = distance_region_index[key]

            if rxID != txID and distance_region == region_value:
                region_sum += value
                region_counter += 1

        if region_counter > 0:
            value_clr = region_sum / region_counter
        else:
            value_clr = -1

        clr_matrix.append([simulation_num, (region_value * 50) + 25, pkeep, region_weights[region_value], value_clr])

    output_path = 'results/KPIs_diff_simulation_calls/' +  name_of_simulation + '_' + str(simulation_num) + '_.csv'
    if len(headers.split(', ')) < 9:
        headers = headers + ', value_clr'
    existing_data = np.genfromtxt(output_path, delimiter=',', skip_header=1)
    if len(existing_data) > 0:
        combined_data = np.concatenate((existing_data, np.array(clr_matrix)[:, -1][:, np.newaxis]), axis=1)
    else:
        combined_data = clr_matrix
    np.savetxt(output_path, combined_data, delimiter=',', fmt='%.6f', header=headers)


def average_clr():
    output_path = 'results/KPIs_diff_simulations/' + name_of_simulation + '.csv'
    data = np.genfromtxt(output_path, delimiter=',', names=headers.split(', '))
    filtered_data = data[(data['value_clr'] > 0) & (data['region_value'] < distance_limit)]
    pkeep_values = np.unique(filtered_data['pkeep'])
    average_clr_per_distance = []

    for pkeep in pkeep_values:
        subset_data = filtered_data[filtered_data['pkeep'] == pkeep]
        average_clr = np.mean(subset_data['value_clr'])
        average_clr_per_distance.append((pkeep, average_clr))

    output_path_average = 'results/Average_metrics/Average_clr' + name_of_simulation + '.csv'
    with open(output_path_average, 'w') as file:
        np.savetxt(file, average_clr_per_distance, delimiter=',', fmt='%.6f', header="pkeep, average_clr")


# Function to calculate Packet Inter-Reception (PIR) for each transmitter-receiver pair
def calculate_pir(rxTime, txID_dec, rxID_dec, pir_dict):
    pair_key = (txID_dec, rxID_dec)

    rx_times = pir_dict[pair_key]['rx_times']
    pir_values = pir_dict[pair_key]['pir_values']
    pir_squared_values = pir_dict[pair_key]['pir_squared_values']

    rx_times.append(rxTime)

    # Calculate PIR for each neighboring pair on-the-fly
    if len(rx_times) > 1:
        pir = rx_times[-1] - rx_times[-2]
        pir_values.append(pir)
        pir_squared_values.append(pir * pir)


def calculate_paoi(simulation_num, pir_dict, regions_list, pkeep):
    global headers
    average_paoi_matrix = []
    pair_paoi_matrix = {}
    for pair_key, data in pir_dict.items():
        txID_dec, rxID_dec = pair_key
        pir_squared_values = data['pir_squared_values']
        pir_values = data['pir_values']

        if pir_squared_values and len(pir_values) >= 10 and rxID_dec != txID_dec:
            mean_pir_squared = np.mean(pir_squared_values)
            mean_pir = np.mean(pir_values)
            pair_paoi_matrix[pair_key] = mean_pir

    for region_value in regions_list:
        value_paoi = 0
        counter = 0
        paoi_sum = 0

        for pair_key, data in pair_paoi_matrix.items():
            txID_dec, rxID_dec = pair_key
            distance_region = AoI_distance_region_index.get(pair_key, None)

            if distance_region == region_value and rxID_dec != txID_dec:
                paoi_sum += pair_paoi_matrix[pair_key]
                counter += 1

        if counter > 0:
            value_paoi = paoi_sum / counter

        if paoi_sum == 0:
            value_paoi = -1

        average_paoi_matrix.append(
            [simulation_num, (region_value * 50) + 25, pkeep, region_weights[region_value], value_paoi])

    output_path = 'results/KPIs_diff_simulation_calls/' +  name_of_simulation + '_' + str(simulation_num) + '_.csv'
    if len(headers.split(', ')) < 9:
        headers = headers + ', value_paoi'
    existing_data = np.genfromtxt(output_path, delimiter=',', skip_header=1)
    if len(existing_data) > 0:
        combined_data = np.concatenate((existing_data, np.array(average_paoi_matrix)[:, -1][:, np.newaxis]), axis=1)
    else:
        combined_data = average_paoi_matrix
    np.savetxt(output_path, combined_data, delimiter=',', fmt='%.6f', header=headers)


def average_paoi():
    output_path = 'results/KPIs_diff_simulations/' + name_of_simulation + '.csv'
    data = np.genfromtxt(output_path, delimiter=',', names=headers.split(', '))
    filtered_data = data[(data['value_paoi'] > 0) & (data['region_value'] < distance_limit)]
    pkeep_values = np.unique(filtered_data['pkeep'])
    average_paoi_per_distance = []

    for pkeep in pkeep_values:
        subset_data = filtered_data[filtered_data['pkeep'] == pkeep]
        average_paoi = np.mean(subset_data['value_paoi'])
        average_paoi_per_distance.append((pkeep, average_paoi))

    output_path_average = 'results/Average_metrics/Average_paoi' + name_of_simulation + '.csv'
    with open(output_path_average, 'w') as file:
        np.savetxt(file, average_paoi_per_distance, delimiter=',', fmt='%.6f', header="pkeep, average_paoi")


def calculate_aoi(simulation_num, pir_dict, regions_list, pkeep):
    global headers
    average_aoi_matrix = []
    pair_aoi_matrix = {}

    for pair_key, data in pir_dict.items():
        txID_dec, rxID_dec = pair_key
        pir_squared_values = data['pir_squared_values']
        pir_values = data['pir_values']

        if pir_squared_values and len(pir_values) >= 10 and rxID_dec != txID_dec:
            mean_pir_squared = np.mean(pir_squared_values)
            mean_pir = np.mean(pir_values)
            pair_aoi_matrix[pair_key] = mean_pir_squared / (2 * mean_pir)

    for region_value in regions_list:
        value_aoi = 0
        aoi_sum = 0
        counter = 0

        for pair_key, data in pair_aoi_matrix.items():
            txID_dec, rxID_dec = pair_key
            distance_region = AoI_distance_region_index.get(pair_key, None)

            if distance_region == region_value and rxID_dec != txID_dec:
                aoi_sum += pair_aoi_matrix[pair_key]
                counter += 1

        if counter > 0:
            value_aoi = aoi_sum / counter

        if aoi_sum == 0:
            value_aoi = -1

        average_aoi_matrix.append(
            [simulation_num, (region_value * 50) + 25, pkeep, region_weights[region_value], value_aoi])

    output_path = 'results/KPIs_diff_simulation_calls/' +  name_of_simulation + '_' + str(simulation_num) + '_.csv'
    if len(headers.split(', ')) < 9:
        headers = headers + ', value_aoi'
    existing_data = np.genfromtxt(output_path, delimiter=',', skip_header=1)
    if len(existing_data) > 0:
        combined_data = np.concatenate((existing_data, np.array(average_aoi_matrix)[:, -1][:, np.newaxis]), axis=1)
    else:
        combined_data = average_aoi_matrix
    np.savetxt(output_path, combined_data, delimiter=',', fmt='%.6f', header=headers)


def average_aoi():
    output_path = 'results/KPIs_diff_simulations/' + name_of_simulation + '.csv'
    data = np.genfromtxt(output_path, delimiter=',', names=headers.split(', '))
    filtered_data = data[(data['value_aoi'] > 0) & (data['region_value'] < distance_limit)]
    pkeep_values = np.unique(filtered_data['pkeep'])
    average_aoi_per_distance = []

    for pkeep in pkeep_values:
        subset_data = filtered_data[filtered_data['pkeep'] == pkeep]
        average_aoi = np.mean(subset_data['value_aoi'])
        average_aoi_per_distance.append((pkeep, average_aoi))

    output_path_average = 'results/Average_metrics/Average_aoi' + name_of_simulation + '.csv'
    with open(output_path_average, 'w') as file:
        np.savetxt(file, average_aoi_per_distance, delimiter=',', fmt='%.6f', header="pkeep, average_aoi")


def process_log_file(file_path, packet_total, packet_decoded, packet_collision_loss, packet_prop_loss,
                     distance_region_index):
    with open(file_path, 'r') as file:
        for line in file:
            values = line.strip().split(',')
            txID = int(values[3])
            rxID = int(values[4])
            distance_pair = float(values[2])

            for i, border in enumerate(border_values):
                if distance_pair <= border:
                    region_value = i
                    current_distance_region_index = i
                    break

            distance_region_index[(rxID, txID)] = current_distance_region_index

            if (rxID, txID) not in packet_total:
                packet_total[(rxID, txID)] = 0
            packet_total[(rxID, txID)] += 1

            if int(values[5]) == 1:
                if (rxID, txID) not in packet_decoded:
                    packet_decoded[(rxID, txID)] = 0

                packet_decoded[(rxID, txID)] += 1

                rxTime = float(values[0])
                txID_dec = int(values[3])
                rxID_dec = int(values[4])
                distance[(rxID_dec, txID_dec)] = float(values[2])
                calculate_pir(rxTime, txID_dec, rxID_dec, pir_dict)
            elif int(values[6]) == 2 or int(values[6]) == 0:
                if (rxID, txID) not in packet_prop_loss:
                    packet_prop_loss[(rxID, txID)] = 0

                packet_prop_loss[(rxID, txID)] += 1
            elif int(values[6]) == 3:
                if (rxID, txID) not in packet_collision_loss:
                    packet_collision_loss[(rxID, txID)] = 0

                packet_collision_loss[(rxID, txID)] += 1


# MAIN

if len(sys.argv) < 4:
    print("Usage: python3 results/script.py <base_name of the file> simulation=start_simulation/end_simulation"
          " metrics=metrics name_of_simulation")
    sys.exit(1)

header_flag = True
base_path = sys.argv[1]
parameters = sys.argv[3].split('=')
value_metrics = parameters[0]
other_parameters = parameters[1].split('/')
parameters_sim = sys.argv[2].split('=')
value_sim = parameters_sim[0]
other_parameters_sim = parameters_sim[1].split('/')

parent_dir = os.path.dirname(os.path.dirname(__file__))
file_path = os.path.join(parent_dir, "src", "MoReV2X", "model", "nr-v2x-ue-mac.cc")
with open(file_path, "r") as f:
    for line in f:
        if "m_keepProbability" in line:
            p_keep = float(line.split("m_keepProbability")[1][2:5].strip())
            break

name_of_simulation = sys.argv[4]

if value_sim == 'simulation':
    if len(other_parameters_sim) == 2:
        start_simulation = int(other_parameters_sim[0]) # number of simulation give infomation about pkeep. If simulation is 1000, pkeep is 0; 1001, pkeep is 0.1 etc. if 1099 pkeep = 0.99
        end_simulation = int(other_parameters_sim[1])
    else:
        print("Invalid simulations")

pir_dict = {}  # Dictionary to store PIR data

for simulation_num in range(start_simulation, end_simulation + 1):
    file_path = base_path + "_" + str(simulation_num) + "/ReceivedLog.txt"
    PDR_matrix = {}  # Matrix of PDR for each pair
    CLR_matrix = {}  # Matrix of CLR for each pair
    PLR_matrix = {}  # Matrix of PLR for each pair
    packet_total = {}  # Dictionary to store total packet counts for each (rxID, txID) pair
    packet_decoded = {}  # Dictionary to store decoded packet counts for each (rxID, txID) pair
    packet_collision_loss = {}  # Dictionary to store lost by propagation packet counts for each (rxID, txID) pair
    packet_prop_loss = {}  # Dictionary to store lost by collision packet counts for each (rxID, txID) pair
    distance_region_index = {}  # Matrix to store indexes of regions
    AoI_distance_region_index = {}  # Matrix to store indexes of regions for AoI calculation
    distance = {}  # Distance between pairs
    region_weights = {region_value: 0 for region_value in regions_list} 
    border_values = [50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900]

    pir_dict = defaultdict(lambda: {'rx_times': [], 'pir_values': [], 'pir_squared_values': []})

    if os.path.exists(file_path):
        process_log_file(file_path, packet_total, packet_decoded, packet_collision_loss, packet_prop_loss,
                         distance_region_index)

        # Create a dictionary to store counts of unique pairs in each distance region
        pair_counts_by_region = defaultdict(int)

        # Initialize the counts for each region
        pair_counts_by_region = {i: 0 for i in range(len(border_values))}

        # Iterate over the distance dictionary
        for pair_key, distance_data in distance.items():
            rxID_dec, txID_dec = pair_key
            distance_value = distance_data

            # Check if the distance is within a specific border
            for i, border in enumerate(border_values):
                if distance_value <= border:
                    region_value = i
                    AoI_distance_region_index[(rxID_dec, txID_dec)] = i
                    pair_counts_by_region[region_value] += 1
                    break

        # Calculate the total number of unique pairs
        total_unique_pairs = sum(pair_counts_by_region.values())

        # Print the counts in each region
        for region_value, count in pair_counts_by_region.items():
            print("Region {}: {} pairs".format(region_value, count))

        # Print the overall count
        print("Overall count: {}".format(total_unique_pairs))

        # Create a structure to hold the weights
        region_weights = {region_value: count / total_unique_pairs for region_value, count in
                          pair_counts_by_region.items()}

    if value_metrics == 'metrics':
        filling_permanent_data(simulation_num, regions_list, p_keep)
        if other_parameters[0] == 'all':
            calculate_clr(simulation_num, packet_total, packet_collision_loss, packet_prop_loss,
                          distance_region_index, regions_list, p_keep)
            calculate_plr(simulation_num, packet_total, packet_collision_loss, packet_prop_loss,
                          distance_region_index, regions_list, p_keep)
            calculate_paoi(simulation_num, pir_dict, regions_list, p_keep)
            calculate_aoi(simulation_num, pir_dict, regions_list, p_keep)
            calculate_pdr(simulation_num, packet_total, packet_decoded, distance_region_index, regions_list,
                          p_keep)
            data_to_matrix = np.genfromtxt('results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv', delimiter=',')
            if header_flag:
                with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                    np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
            else:
                with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                    np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
            header_flag = False
        else:
            for param in other_parameters:
                if param == 'clr':
                    calculate_clr(simulation_num, packet_total, packet_collision_loss, packet_prop_loss,
                                  distance_region_index, regions_list, p_keep)
                    data_to_matrix = np.genfromtxt('results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv',
                                                   delimiter=',')
                    if header_flag:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
                    else:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
                elif param == 'plr':
                    calculate_plr(simulation_num, packet_total, packet_collision_loss, packet_prop_loss,
                                  distance_region_index, regions_list, p_keep)
                    data_to_matrix = np.genfromtxt('results/KPIs_diff_simulation_calls/' + name_of_simulation + '.csv',
                                                   delimiter=',')
                    if header_flag:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
                    else:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
                elif param == 'paoi':
                    calculate_paoi(simulation_num, pir_dict, regions_list, p_keep)
                    data_to_matrix = np.genfromtxt('results/results/KPIs_diff_simulation_calls/' + name_of_simulation + '.csv',
                                                   delimiter=',')
                    if header_flag:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
                    else:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
                elif param == 'aoi':
                    calculate_aoi(simulation_num, pir_dict, regions_list, p_keep)
                    data_to_matrix = np.genfromtxt('results/KPIs_diff_simulation_calls/' + name_of_simulation + '.csv',
                                                   delimiter=',')
                    if header_flag:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
                    else:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
                elif param == 'pdr':
                    calculate_pdr(simulation_num, packet_total, packet_decoded, distance_region_index, regions_list,
                                  p_keep)
                    data_to_matrix = np.genfromtxt('results/KPIs_diff_simulation_calls/' + name_of_simulation + '_' + str(simulation_num) + '_.csv',
                                                   delimiter=',')
                    if header_flag:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'w') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f', header=headers)
                    else:
                        with open('results/KPIs_diff_simulations/' + name_of_simulation + '.csv', 'a') as file:
                            np.savetxt(file, data_to_matrix, delimiter=',', fmt='%.6f')
            header_flag = False

    else:
        print("Invalid value metrics")

if value_metrics == 'metrics':
    if other_parameters[0] == 'all':
        average_pdr()
        average_clr()
        average_plr()
        average_aoi()
        average_paoi()
    else:
        for param in other_parameters:
            if param == 'clr':
                average_clr()
            elif param == 'plr':
                average_plr()
            elif param == 'paoi':
                average_paoi()
            elif param == 'aoi':
                average_aoi()
            elif param == 'pdr':
                average_pdr()

with open('Name_files_for_graphics.txt', 'w') as f:
    f.write('results/KPIs_diff_simulations/' + name_of_simulation + '.csv' + '\n')
    f.write('results/Average_metrics/Average_pdr' + name_of_simulation + '.csv' + '\n')
    f.write('results/Average_metrics/Average_plr' + name_of_simulation + '.csv' + '\n')
    f.write('results/Average_metrics/Average_clr' + name_of_simulation + '.csv' + '\n')
    f.write('results/Average_metrics/Average_paoi' + name_of_simulation + '.csv' + '\n')
    f.write('results/Average_metrics/Average_aoi' + name_of_simulation + '.csv' + '\n')
    
end_time = time.time()
print("Done successfully! Script running time: {} seconds.".format(end_time - start_time))