import pandas as pd
import numpy as np
import time
import os


def process_log_file(df):
    """

    :param df:
    :return:
    """
    df = df.drop(df[df['distance'] == 'distance'].index)
    
    df['packetID'] = df['packetID'].astype(int)
    df['txID'] = df['txID'].astype(int)
    df['rxID'] = df['rxID'].astype(int)
    df['decoded'] = df['decoded'].astype(int)
    df['lossType'] = df['lossType'].astype(int)
    df['rxTime'] = df['rxTime'].astype(float)
    df['distance'] = df['distance'].astype(float)

    return df


def get_pair_region_id(distance, list_id_region, list_bounds_region):
    for i in list_id_region:
        if (distance <= list_bounds_region[i+1]) & (distance > list_bounds_region[i]):
            
            return i


def process_vehicle_pairs(df):
    
    df_pairs = df.groupby(['txID','rxID'])['distance'].apply(list).reset_index()
    df_pairs['distance'] = df_pairs['distance'].apply(set)
    df_pairs['distance'] = df_pairs['distance'].apply(list)
    df_pairs['region_distance_id'] = df_pairs['distance'].apply(lambda x: get_pair_region_id(x[0], distance_region_id, distance_region_bounds))
    df['region_distance_id'] = df['distance'].apply(lambda x: get_pair_region_id(x, distance_region_id, distance_region_bounds))

    return df_pairs


def form_df_regions(list_id_region, df_regions):
    
    for i in list_id_region:
        pairs_number = int(len(df_pairs[df_pairs['region_distance_id'] == i])/2)
        decoded_count = len(df[(df['decoded'] == 1) & (df['region_distance_id'] == i)])
        collision_count = len(df[(df['lossType'] == 3) & (df['region_distance_id'] == i)])
        propagation_count = len(df[((df['lossType'] == 1) | (df['lossType'] == 0)) & (df['region_distance_id'] == i)])

        df_regions.loc[i, 'pairs_number'] += pairs_number
        df_regions.loc[i, 'decoded'] += decoded_count
        df_regions.loc[i, 'collision'] += collision_count
        df_regions.loc[i, 'propagation'] += propagation_count

        df_regions = df_regions.astype(int) 

    return df_regions


def get_pir(input_df):
    test_df = pd.DataFrame(columns=['rxTime', 'pir', 'pir_sqr', 'mean_pir', 'mean_pir_sqr'])

    test_df['rxTime'] = input_df.groupby(['txID', 'rxID'])['rxTime'].apply(list)
    test_df['pir'] = test_df['rxTime'].apply(np.diff)
    test_df['pir_sqr'] = test_df['pir'] ** 2
    test_df['mean_pir'] = test_df['pir'].apply(np.mean)
    test_df['mean_pir_sqr'] = test_df['pir_sqr'].apply(np.mean)
    test_df.reset_index()
    return test_df


def get_metrics(list_region_id, df_regions, df_pairs, df_pir, p_keep):

    df_metrics = pd.DataFrame()

    for i in list_region_id:

        df_metrics.loc[i, 'region_id'] = i
        df_metrics.loc[i, 'region_max_distance'] = distance_region_bounds[i]
        df_metrics.loc[i, 'weight'] = df_regions['pairs_number'][i] / (len(df_pairs)/2)
        df_metrics.loc[i, 'p_keep'] = p_keep
        df_metrics.loc[i, 'PDR'] = df_regions['decoded'][i] / (df_regions['decoded'][i] + df_regions['collision'][i] + df_regions['propagation'][i])
        df_metrics.loc[i, 'CLR'] = df_regions['collision'][i] / (df_regions['decoded'][i] + df_regions['collision'][i] + df_regions['propagation'][i])
        df_metrics.loc[i, 'PLR'] = df_regions['propagation'][i] / (df_regions['decoded'][i] + df_regions['collision'][i] + df_regions['propagation'][i])
        df_metrics['region_id'] = df_metrics['region_id'].astype(int)
    
    mean_pir_by_pair = df_pir.groupby(['txID','rxID'])[['mean_pir', 'mean_pir_sqr']].mean()
    df_pairs = df_pairs.merge(mean_pir_by_pair, left_on=['txID','rxID'], right_index=True, how='left')
    df_pairs['aoi'] = df_pairs['mean_pir_sqr'] / (2 * df_pairs['mean_pir'])
        
    df_metrics['mean_paoi'] = df_pairs.groupby('region_distance_id')['mean_pir'].mean()
    df_metrics['mean_aoi'] = df_pairs.groupby('region_distance_id')['aoi'].mean()

    return df_metrics

def get_avg_metrics_p_keep(df_metrics):   
    df_avg_metrics= pd.DataFrame(columns=['p_keep', 'mean_PDR', 'mean_CLR', 'mean_PLR', 'mean_PAoI', 'mean_AoI'])

    new_row = {
        'p_keep': df_metrics['p_keep'][0],
        'mean_PDR': (df_metrics['PDR'][df_metrics["region_max_distance"] <= 400] * df_metrics['weight']).sum() / df_metrics['weight'].sum(),
        'mean_CLR': (df_metrics['CLR'][df_metrics["region_max_distance"] <= 400] * df_metrics['weight']).sum() / df_metrics['weight'].sum(),
        'mean_PLR': (df_metrics['PLR'][df_metrics["region_max_distance"] <= 400] * df_metrics['weight']).sum() / df_metrics['weight'].sum(),
        'mean_PAoI': (df_metrics['mean_paoi'][df_metrics["region_max_distance"] <= 400] * df_metrics['weight']).sum() / df_metrics['weight'].sum(),
        'mean_AoI': (df_metrics['mean_aoi'][df_metrics["region_max_distance"] <= 400] * df_metrics['weight']).sum() / df_metrics['weight'].sum()
    }
    new_row_df = pd.DataFrame([new_row])
    df_avg_metrics_p_keep = pd.concat([df_avg_metrics, new_row_df], ignore_index=True)
    
    return df_avg_metrics_p_keep



#######################################################################
start_time = time.time()

distance_region_id = list(range(18))
distance_region_bounds = [x * 50 for x in range(19)]


base_path = 'Simulations'
file_paths = []
for file in os.listdir(base_path):
    file_path = os.path.join(base_path, file)
    file_paths.append(file_path)

for file_path_folder in file_paths:
    file_path = file_path_folder + "/ReceivedLog.txt"
    #file_path = file_path_folder + "/test.txt"
    print (file_path)
    p_keep = 0.1
    if os.path.exists(file_path):
        columns_to_keep = ['rxTime', 'packetID', 'distance', 'txID', 'rxID', 'decoded', 'lossType']
        chunk = 10000
        ff = pd.read_csv(file_path, sep=",", header=None, names=columns_to_keep, chunksize=chunk)
        i = 0
        df_regions = pd.DataFrame()
        for k in distance_region_id:
            df_regions.loc[k, 'pairs_number'] = 0
            df_regions.loc[k, 'decoded'] = 0
            df_regions.loc[k, 'collision'] = 0
            df_regions.loc[k, 'propagation'] = 0

        for idx in ff:
            print((i + 1) * chunk)
            print('...')
            try:

                df = process_log_file(idx)
                end_time = time.time()
                print("Done! process_log_file time:", (end_time - start_time), "seconds")

            except Exception:

                continue

            df_pairs = process_vehicle_pairs(df)
            end_time = time.time()
            print("Done! process_vehicle_pairs:", (end_time - start_time), "seconds")
            
            df_regions = form_df_regions(distance_region_id, df_regions)
            end_time = time.time()
            print("Done! form_df_regions time:", (end_time - start_time), "seconds")

            if i == 0:
                pir_df = df[df['decoded'] == 1][['txID', 'rxID', "rxTime"]]
                df_pir = get_pir(pir_df)
            else:
                pir_df = pd.concat([pir_df, df[df['decoded'] == 1][['txID', 'rxID', "rxTime"]]])
                print(pir_df.info())
                print(len(df))
                if len(df) != chunk:
                    print("Start pir calculating")
                    df_pir = get_pir(pir_df)
            end_time = time.time()
            print("Done! get_pir time:", (end_time - start_time), "seconds")
            i += 1

        df_metrics = get_metrics(distance_region_id, df_regions, df_pairs, df_pir, p_keep)
        end_time = time.time()
        print("Done! get_metrics time:", (end_time - start_time), "seconds")
        df_metrics_avg = get_avg_metrics_p_keep(df_metrics)
        end_time = time.time()
        print("Done! get_metrics time:", (end_time - start_time), "seconds")
        df_metrics.to_csv('KPIs_diff_simulations/metrics_distance.csv', mode='a', header=False, index=False)
        df_metrics_avg.to_csv('Average_metrics/Average_metrics_p_keep.csv', mode='a', header=False, index=False)
    else:
        print(file_path + " does not exist")
    
with open('Name_files_for_graphics.txt', 'w') as f:
    f.write('results/KPIs_diff_simulations/metrics_distance.csv\n')
    f.write('results/Average_metrics/Average_metrics_p_keep.csv\n')


end_time = time.time()
print("Done! Script running time:", (end_time - start_time), "seconds")