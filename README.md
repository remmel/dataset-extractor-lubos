# dataset-extractor-lubos

`sudo apt-get install git cmake libpng-dev libturbojpeg-dev libglm-dev`

git clone git@github.com:remmel/dataset-extractor-lubos.git
cd dataset-extractor-lubos
cmake .
make
./dataset_extractor ~/dataset


If you use CodeBlocks, to crate the Project.cbp file
`cmake . -G"CodeBlocks - Unix Makefiles"`
