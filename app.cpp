#include <png.h>
#include <sstream>
#include <data/file3d.h>

typedef float Tango3DR_Vector4[4];

std::vector<oc::Mesh> merged;

std::string getFilename(std::string dataset, int poseIndex, std::string extension)
{
    //get file name
    std::ostringstream ss;
    ss << poseIndex;
    std::string number = ss.str();
    while(number.size() < 8)
        number = "0" + number;

    return dataset + "/" + number + "." +extension;
}

void writePNG16(std::string filename, int width, int height, unsigned short* data)
{
    // Open file for writing (binary mode)
    FILE* file = fopen(filename.c_str(), "wb");

    // init PNG library
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_init_io(png_ptr, file);
    png_set_IHDR(png_ptr, info_ptr, (png_uint_32) width, (png_uint_32) height,
                 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    png_set_swap(png_ptr);

    // write image data
    unsigned short int row[width];
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            row[x] = data[(y * width + x)];
        }
        png_write_row(png_ptr, (png_const_bytep)row);
    }
    png_write_end(png_ptr, NULL);

    /// close all
    if (file != NULL) fclose(file);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}

glm::mat4 getPose(std::string filename, int pose)
{
    LOGI("Reading %s", filename.c_str());

    glm::mat4 mat;
    FILE* file = fopen(filename.c_str(), "r");
    for (int k = 0; k <= pose; k++)
        for (int j = 0; j < 4; j++)
            fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
    fclose(file);
    return mat;
}

void convertFrame(std::string dataset, int poseIndex)
{
    //get point cloud
    int size = 0;
    std::string filename = getFilename(dataset, poseIndex, "pcl");
    LOGI("Reading %s", filename.c_str());
    FILE* file = fopen(filename.c_str(), "rb");
    fread(&size, sizeof(int), 1, file);
    Tango3DR_Vector4* points = new Tango3DR_Vector4[size];
    fread(points, sizeof(Tango3DR_Vector4), size, file);
    fclose(file);

    //get matrices and captured photo
    glm::mat4 depth_mat = getPose(getFilename(dataset, poseIndex, "mat"), COLOR_CAMERA);
    glm::mat4 viewproj_mat = getPose(getFilename(dataset, poseIndex, "mat"), SCREEN_CAMERA);
    oc::Image* jpg = new oc::Image(getFilename(dataset, poseIndex, "jpg"));

    //convert point cloud into world space and colorize it
    oc::Mesh pcl;
    for (unsigned int i = 0; i < size; i++)
    {
        glm::vec4 v = depth_mat * glm::vec4(points[i][0], points[i][1], points[i][2], 1.0f);
        glm::vec4 t = viewproj_mat * glm::vec4(v.x, v.y, v.z, 1.0f);
        t /= fabs(t.w);
        t = t * 0.5f + 0.5f;
        int x = (int)(t.x * jpg->GetWidth());
        int y = (int)(t.y * jpg->GetHeight());
        pcl.colors.push_back(jpg->GetColor(x, y));
        pcl.vertices.push_back(glm::vec3(v.x, v.y, v.z));
    }
    delete[] points;

    //save PLY
    std::vector<oc::Mesh> output;
    output.push_back(pcl);
    merged.push_back(pcl);
    oc::File3d(getFilename(dataset, poseIndex, "ply"), true).WriteModel(output);
}

void exportDepthmap(std::string dataset, int poseIndex)
{
    //get point cloud
    int size = 0;
    std::string filename = getFilename(dataset, poseIndex, "pcl");
    LOGI("Reading %s", filename.c_str());
    FILE* file = fopen(filename.c_str(), "rb");
    fread(&size, sizeof(int), 1, file);
    Tango3DR_Vector4* points = new Tango3DR_Vector4[size];
    fread(points, sizeof(Tango3DR_Vector4), size, file);
    fclose(file);

    //get matrices
    glm::mat4 depth_mat = getPose(getFilename(dataset, poseIndex, "mat"), COLOR_CAMERA);
    glm::mat4 viewproj_mat = getPose(getFilename(dataset, poseIndex, "mat"), SCREEN_CAMERA);

    //init variables
    int width = 180;
    int height = 320; //180/9*16 (resize 4:3 to 16:9)
    unsigned short data[width * height];
    bool valid[width * height];
    for (unsigned int i = 0; i < width * height; i++)
    {
        data[i] = 0;
        valid[i] = false;
    }

    //convert point cloud into depthmap
    bool fillHoles = true;
    for (unsigned int i = 0; i < size; i++)
    {
        glm::vec4 v = depth_mat * glm::vec4(points[i][0], points[i][1], points[i][2], 1.0f);
        glm::vec4 t = viewproj_mat * glm::vec4(v.x, v.y, v.z, 1.0f);
        t /= fabs(t.w);
        t = t * 0.5f + 0.5f;
        int x = (int)(t.x * width + 0.5f);
        int y = (int)((1.0f - t.y) * height + 0.5f);
        unsigned short depth = fabs(v.y) * 1000;//mm
        if (fillHoles)
        {
            for (int k = x - 1; k <= x + 1; k++)
            {
                for (int l = y - 1; l <= y + 1; l++)
                {
                    if ((k < 0) || (l < 0) || (k >= width) || (l >= height))
                        continue;
                    if (!valid[l * width + k])
                    {
                        data[l * width + k] = depth;
                        if ((x == k) && (y == l))
                            valid[l * width + k] = true;
                    }
                }
            }
        }
        else
        {
            if ((x < 0) || (y < 0) || (x >= width) || (y >= height))
                continue;
            data[y * width + x] = depth;
        }
    }
    delete[] points;

    //save depthmap
    writePNG16(getFilename(dataset, poseIndex, "png"), width, height, data);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        LOGI("Usage: ./dataset_extractor <path to the dataset>");
        return 0;
    }

    //init dataset
    int poseCount = 0;
    std::string dataset = argv[1];
    FILE* file = fopen((dataset + "/state.txt").c_str(), "r");
    if (file)
    {
        fscanf(file, "%d", &poseCount);
        fclose(file);
    }

    //convert each frame
    for (int i = 0; i < poseCount; i++)
    {
        convertFrame(dataset, i);
        exportDepthmap(dataset, i);
    }

    //merge all frames together
    oc::File3d(dataset + "/output.ply", true).WriteModel(merged);

    return 0;
}
