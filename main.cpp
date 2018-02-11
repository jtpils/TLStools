#include <iostream>
#include "lasreader.hpp"
#include "laswriter.hpp"
#include "lasfilter.hpp"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <unordered_map>
#include <set>
#include <vector>
#include <typeinfo>
#include <fstream>
#include <algorithm>
#include <string.h>
#include <sstream>
#include "classes.hpp"
#include "methods.hpp"

using namespace std;

/*************************************************************************************************************/
CommandLine globalArgs;

static const char *optString = "i:o:l:u:p:r:d:v:O:z:h?";

static const struct option longOpts[] = {
    { "input", required_argument, NULL, 'i' },
    { "output", required_argument, NULL, 'o' },
    { "lower", required_argument, NULL, 'l' },
    { "upper", required_argument, NULL, 'u' },
    { "pixel", required_argument, NULL, 'p' },
    { "radius", required_argument, NULL, 'r' },
    { "density", required_argument, NULL, 'd' },
    { "votes", required_argument, NULL, 'v' },
    { "output-cloud", required_argument, NULL, 'O' },
    { "help", no_argument, NULL, 'h' },
    { "height-int", required_argument, NULL, 'z' },
    { "stack-stats", required_argument, NULL, 0},
    { "stack-cloud", required_argument, NULL, 0},
    {NULL, no_argument, NULL, 0}
};

/*************************************************************************************************************/

/*** general functions ***/

void printHelp(){

    cout <<
        "\n# /*** TLStools - las2rings ***/\n# /*** Command line arguments ***/\n\n"
        "# -i --input         : input file path\n"
        "# -o --output        : output results file path (.txt)\n"
        "# -O --output-cloud  : output point cloud with the tree stems (.laz/.las/.txt)\n"
        "# --stack-stats      : output file path for the tree center statistics (.txt)\n"
        "# --stack-cloud      : output file path for the trees center layer stack (.laz/.las/.txt)\n"
        "# -l --lower         : slice's lower height\n"
        "# -u --upper         : slice's upper height\n"
        "# -p --pixel         : pixel size, in meters\n"
        "# -r --radius        : maximum radius to test\n"
        "# -d --density       : minimum density to consider on the Hough transform\n"
        "# -v --votes         : minimum votes count at the output\n"
        "# -z --height-int    : height interval to measure stem segments\n"
        "# -? -h --help       : print help\n"
    << endl;

        exit(1);

}

/*** batch processing ***/
//statistics per tree
vector<PlotCloudTreeSlice> getTlsMetrics(vector<HoughCenters>& centers){

    vector<PlotCloudTreeSlice> tlsMetrics;
    PlotCloudTreeSlice metric;

    for(unsigned int i = 0; i < centers.size() ; ++i){

        metric.n_tree = i+1;
        metric.n_slice = 0;
        metric.x_average = centers[i].avg_x;
        metric.y_average = centers[i].avg_y;
        metric.x_main = centers[i].circles[ centers[i].main_circle ].x_center;
        metric.y_main = centers[i].circles[ centers[i].main_circle ].y_center;
        metric.radius = centers[i].circles[ centers[i].main_circle ].radius;
        metric.votes = centers[i].circles[ centers[i].main_circle ].n_votes;
        metric.z_min = centers[i].low_z;
        metric.z_max = centers[i].up_z;

        tlsMetrics.push_back(metric);
    }

    return tlsMetrics;

}
/**/
//plot-wise
void plotProcess(CommandLine global){

    vector<HoughCenters> treeMap;
    Raster ras;

    cout << "# getting cloud statistics" << endl;
    CloudStats cstats = getStats(global.file_path);

    std::stringstream ss, ss2;
    ss << global.lower_slice;
    ss2 << global.upper_slice;
    float hLow;
    float hUp;
    ss >> hLow;
    ss2 >> hUp;

    cout << "# mapping tree positions" << endl;
    for(float h = hLow; h <= (hUp - global.height_interval); h += global.height_interval){

        cout << "## layer " << 1 + ( h - hLow ) / global.height_interval << endl;

        std::stringstream ss, ss2;
        ss << h;
        ss2 << (h+global.height_interval);
        string ht;
        string ht2;
        ss >> ht;
        ss2 >> ht2;

        //cout << "# reading point cloud" << endl;
        Slice slc = getSlice(global.file_path, ht, ht2, cstats.z_min);

        //cout << "# rasterizing cloud's slice" << endl;
        ras = getCounts(&slc, global.pixel_size);

        //cout << "# extracting center candidates" << endl;
        vector<HoughCenters> hough = getCenters(&ras, global.max_radius, global.min_density, global.min_votes);

        //cout << "# extracting center estimates" << endl;
        getPreciseCenters(hough);

        //treeMap.resize(treeMap.size() + hough.size());
        treeMap.insert(treeMap.end(), hough.begin(), hough.end());

    }

    cout << "# writing cloud of center candidates: " << global.output_stack << endl;
    saveCloud(&treeMap, global.output_stack);

    cout << "# writing layer stack results: " << global.output_stack_coordinates << endl;
    saveReport(treeMap, global.output_stack_coordinates);


    cout << "# extracting main coordinate per tree" << endl;
    int nLayers = 0.75 * (hUp - hLow) / global.height_interval;
    vector<vector<HoughCircle*>> singleTreeMap = isolateSingleTrees(treeMap, global.max_radius * 2, nLayers);


    cout << "# getting trunk statistics" << endl;
    vector<vector<StemSegment>> trees;
    for(int i = 0; i < singleTreeMap.size(); ++i){
        cout << "\ntree " << i+1 << " of " << singleTreeMap.size() << endl;

        vector<HoughCircle*> temp = singleTreeMap[i];

        float xBase = temp[0]->x_center;
        float yBase = temp[0]->y_center;
        cout << "center: " << xBase << " , " << yBase << endl;

        StemSegment base = baselineStats(cstats, global, true, xBase, yBase, 1.2);
        /*cout << "props: " <<
        base.model_circle.x_center << " , " <<
        base.model_circle.y_center << " , " <<
        base.model_circle.radius << " , " <<
        base.model_circle.n_votes << endl;*/

        vector<Slice> pieces = sliceList(global.file_path, cstats, global.height_interval, true, xBase, yBase, global.max_radius*3);
        cout << "segments: " << pieces.size() << endl;

        //cout << "## points" << endl;
        vector<StemSegment> bole = stemPoints(base, pieces, global);
        trees.push_back(bole);
    }

    cout << endl;

    cout << "# writing tree-wise statistics to " << global.output_path << " and stem point cloud to " << global.output_las << endl;
    saveStemsOnly(trees, global.file_path, global.output_path, global.output_las);

    cout << "# done" << endl;

}

/*
//tree-wise
void treeProcess(CommandLine global){

    cout << "# getting cloud statistics" << endl;
    CloudStats stats = getStats(globalArgs.file_path);

    cout << "# getting baseline segment" << endl;
    StemSegment base = baselineStats(stats, global);

    cout << "# slicing cloud" << endl;
    vector<Slice> pieces = sliceList(global.file_path, stats, global.height_interval);

    cout << "# finding stem segments" << endl;

    float xt = base.model_circle.x_center;
    float yt = base.model_circle.y_center;
    float dt = base.model_circle.radius + global.pixel_size*4;

    vector<StemSegment> stem_sections = {};

    for( unsigned i = 0; i < pieces.size(); ++i){
        StemSegment temp = getSegment(pieces[i], global, xt, yt, dt);
        xt = temp.model_circle.x_center;
        yt = temp.model_circle.y_center;
        dt = (temp.model_circle.radius >= (dt + global.pixel_size) ) ? dt : (temp.model_circle.radius + global.pixel_size*3);

        stem_sections.push_back(temp);
    }

    cout << "# writing results: " << global.output_path << endl;
    saveStemReport(stem_sections, global.output_path);

    cout << "# writing cloud: " << global.output_las << endl;
    saveStemCloud(stem_sections, pieces, global.pixel_size, global.output_las);

    cout << "# done" << endl;

}
*/

int main(int argc, char *argv[])
{
    /** define global variables **/
    globalArgs.file_path = " ";
    globalArgs.help = false;
    globalArgs.lower_slice = "1.0";
    globalArgs.max_radius = 0.25;
    globalArgs.min_density = 0.1;
    globalArgs.min_votes = 3;
    globalArgs.output_las = " ";
    globalArgs.output_path = " ";
    globalArgs.pixel_size = 0.025;
    globalArgs.upper_slice = "3.0";
    globalArgs.height_interval = 0.5;
    globalArgs.output_stack = " ";
    globalArgs.output_stack_coordinates = " ";

    int opt = 0;
	int longIndex = 0;

    opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    while( opt != -1 ) {
        switch( opt ) {
            case 'i':
                globalArgs.file_path = std::string(optarg);
                break;

            case 'o':
                globalArgs.output_path = std::string(optarg);
                break;

            case 'l':
                globalArgs.lower_slice = std::string(optarg);
                break;

            case 'u':
                globalArgs.upper_slice = std::string(optarg);
                break;

            case 'p':
                globalArgs.pixel_size = atof(optarg);
                break;

            case 'r':
                globalArgs.max_radius = atof(optarg);
                break;

            case 'd':
                globalArgs.min_density = atof(optarg);
                break;

            case 'v':
                globalArgs.min_votes = atoi(optarg);
                break;

            case 'O':
                globalArgs.output_las = std::string(optarg);
                break;

            case 'z':
                globalArgs.height_interval = atof(optarg);
                break;

            case 'h':
            case '?':
                globalArgs.help = true;
                break;

            case 0:

                if( strcmp( "stack-stats", longOpts[longIndex].name ) == 0 ) {
                    globalArgs.output_stack_coordinates = std::string(optarg);

                }else if(strcmp( "stack-cloud", longOpts[longIndex].name ) == 0){
                    globalArgs.output_stack = std::string(optarg);
                }

                break;

            default:
                break;
        }

        opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    }


    /*** parse command line options ***/

    // print helpvector<HoughCenters>&
    if(globalArgs.help){
        printHelp();
        return 0;
    }
/*
    globalArgs.file_path = "lcer.las";
*/
    if(globalArgs.file_path == " "){
        cout << "\n# input file (-i) missing.\n";
        printHelp();
        return 0;
    }

    // rename output
    if(globalArgs.output_path == " "){
        globalArgs.output_path = outputNameAppend(globalArgs.file_path);
    }

    if(globalArgs.output_las == " "){
        globalArgs.output_las = outputNameAppend(globalArgs.file_path, "_stems.laz");
    }

    if(globalArgs.output_stack == " "){
        globalArgs.output_stack = outputNameAppend(globalArgs.file_path, "_layerStack.laz");
    }

    if(globalArgs.output_stack_coordinates == " "){
        globalArgs.output_stack_coordinates = outputNameAppend(globalArgs.file_path, "_layerStats.txt");
    }

    /*** process ***/

    plotProcess(globalArgs);

    return 0;
}
