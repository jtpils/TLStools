# TLStrees

A command line tool for automatic detection and segmentation of tree stems from Terrestrial Laser Scanning (TLS) point clouds.

## Getting Started

To run the executable just run `TLStrees -h` from shell and the following help page will be loaded:

```
# /*** TLStrees ***/
# /*** Command line arguments ***/

# -i or --input         : input file path
# -o or --otxtdir       : output directory for the report files (defaults to input file directory)
# -O or --olazdir       : output directory for the point cloud files (defaults to input file directory)
# -l or --lower         : slice's lower height (default = 1.0 m)
# -u or --upper         : slice's upper height (default = 3.0 m)
# -z or --zheight       : height interval to search for stem segments (default = 0.5 m)
# -p or --pixel         : pixel size, in meters (default = 0.025 m)
# -r or --radius        : maximum radius to test (default = 0.25 m)
# -d or --density       : minimum density to consider on the Hough transform, from 0 to 1 (default = 0.1)
# -v or --votes         : minimum votes count at the output (default = 3 votes per pixel)
# -? or -h or --help    : print this help

``` 

## Adopted methods

The algorithm we used for detecting stem points is an adapted Hough transform. It filters cells from rasterized layers of the point cloud searching for circular patterns. The final radii for tree segments is estimated through a least squares approach combined with the RANSAC algorithm. More in depth descriptions of the methods are given by [Olofsson et al. (2014)](http://www.mdpi.com/2072-4292/6/5/4323) and [Conto et al. (2017)](https://doi.org/10.1016/j.compag.2017.10.019).

### Further details and recommendations on the command line usage

The command arguments follow the Unix style, specified by short `(-char)` and/or long `(--string)` parameters, followed by their respective values, when required.

* `-i (--input)`

the input path must lead to a **normalized** TLS point cloud file in `las`, `laz` or `txt` format. The lack of normalization is likely to ommit trees in the final output, due to the way the algorithm operates, assigning the ground level to be at the lowest point's height.

* `-o (--otxtdir)`

the report file is saved in this output directory, named *input_file_name*`_reslt.txt`. This file summarizes the results, assigning all stem segments found to their respective trees, their x and y coordinates, radii, height interval, number of points they encompass and number of votes they received on the hough transform circle search. Another file named *input_file_name*`_segmt.txt` is also saved, which contains information of the circles found on the horizontal point cloud layers rasterized for the hough transform.

* `-O (--olazdir)`

a point cloud containing only stem points is saved in this output directory, named *input_file_name*`_trees.laz`. Another file, named *input_file_name*`_segmt.laz` is saved, showing the regions at which potential tree positions were detected on each horizontal (raster) layer.

* `-l (--lower)`

the absolute height to start the rasterization and tree position search.

* `-u (--upper)`

the absolute height that the algorithm can reach when searching for tree positions.

* `-z (--zheight)`

the height interval to take point cloud slices for rasterization. Must be less than the distace between `-l` and `-u`. The number of raster layers used to search for tree positions is equivalent to `n = (upper - lower) / zheight`. A latter filter is applied, accepting as trees only regions with at least `n * 3/4` of circles vertically aligned.

* `-p (--pixel)`

the pixel size gives the resolution of the final output. All radii, at this moment, are measured in pixel units, thus the pixel size dictates the resolution, accuracy and precision of the algorithm. Processing speed is proportional to the pixel size (the larger, the faster).

* `-r (--radius)`

this parameter specifies the maximum radius tolerated as a tree's dimension.

* `-d (--density)`

filtering parameter for the Hough transform. Before it starts the circle search, the algorithm filters out low density pixels, assuming they do not belong to a stem. The density here is defined as the proportion of points in a pixel (`np`), relative to the pixel with most points (`Np`) in that raster, thus `d = np / Np`.

* `-v (--votes)`

circle selection criterion for the Hough transform. It indicates how many pixels (at least) must be aligned on a curve to be considered part of a circle.

### Further comments and acknowledgments

* Main developer: [Tiago de Conto](https://github.com/tiagodc/)
* Thanks to Martin Isenburg for the initial insights and for the [LASlib](https://github.com/LAStools/LAStools/tree/master/LASlib) API.
* The system has been developed under the auspices of the [proLiDAR project](http://www.luizestraviz.com/pesquisa/), a cooperative research program led by [IPEF](http://www.ipef.br) and coordinated by [Luiz C. E. Rodriguez](http://www.researcherid.com/rid/D-7043-2012), forest management professor at the University of Sao Paulo, Brazil. Initial contributions by [Caio Hamamura](http://www.bv.fapesp.br/pt/pesquisador/103895/caio-hamamura/) are also acknowledged.
* We provided a `R script` (**viewResults.R**) with a short 3D vizualisation routine.
* A sample TLS forest plot is found at the `sample_data` folder, just run `TLStrees -i square.las` for a quick demo. 
