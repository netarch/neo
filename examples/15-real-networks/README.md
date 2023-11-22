To parse dataset:
python3 scripts/dataparse_script.py "dataset directory" "prefix of parsed data" "data type( rf | snap )"

To run neo:
./run.sh "topology file name" "percentage of emulated nodes" "maximum number of leafs to be tested"

0.test-network.txt : 
Basic handwritten example with 8 nodes

1.as-103-nodes.txt : 
Stanford large network dataset AS-level topology. https://snap.stanford.edu/data/as-733.html

see data/rf for refined rocketfuel dataset, the filename should be self-explanatory. 