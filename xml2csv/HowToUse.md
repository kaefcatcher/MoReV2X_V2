## First step: Download Sumo

sudo add-apt-repository ppa:sumo/stable

sudo apt-get update

sudo apt-get install sumo sumo-tools sumo-doc

## Second Step

sumo-gui highway.sumocfg

## Third step

Run-> Green triangle at left top corner

## How to read config files

In sumocfg (sumo config) you can see 2 values:
- net-file
- route-files

Net files describe the net itself: The Highway, its length, number of lanes. The Highway is an edge between 2 Junction- dead ends. Their coordinates and other info is also described.

Route files describe the flow on the edges. Version 1 specifies 3 traffic types that occur in each direction: slow, medium and fast, each with its own velocity.

As a result we have a flow that is super loaded at the start and not so loaded in the end.
