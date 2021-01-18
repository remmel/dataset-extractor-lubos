# dataset-extractor-lubos

Install differents tools & libs needed:  
`sudo apt-get install git cmake libpng-dev libturbojpeg-dev libglm-dev`

Get code and build:  

```
git clone git@github.com:remmel/dataset-extractor-lubos.git
cd dataset-extractor-lubos
cmake .
make
./dataset_extractor ~/dataset
```


If you use CodeBlocks, to crate the Project.cbp file  
`cmake . -G"CodeBlocks - Unix Makefiles"`

Code provided by Lubos on Slack : https://lvonasek.slack.com/archives/CS68GSRGU/p1601419794008900



Bulk resize:
`sudo apt install imagemagick`
`mogrify -resize 180x320 -path resized180 *.jpg`
