# Makefile

.PHONY: sim_300 sim_600

sim_300:
	@sumo -c highway_v1_3_traffic_types/highway.sumocfg
	@python parser.py 605 10690 highway_v1_3_traffic_types/raw.xml
	@python split_csv.py output.csv sim_300 60000
	@python combine_sim_files.py sim_300/ trace_300.csv 300

sim_600:
	@sumo -c highway_v2_3_traffic_types/highway.sumocfg
	@python parser.py 1000 11101 highway_v2_3_traffic_types/raw.xml
	@python split_csv.py output.csv sim_600 60000
	@python combine_sim_files.py sim_600/ trace_600.csv 600

clean_300:
	rm highway_v1_3_traffic_types/raw.xml
	rm -rf sim_300/
	rm output.csv
	rm trace_300.csv

clean_600:
	rm highway_v2_3_traffic_types/raw.xml
	rm -rf sim_600/
	rm trace_600.csv
	rm output.csv

clean_all:
	clean_300
	touch output.csv
	clean_600