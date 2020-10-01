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
    glm::mat4 depth_mat = getPose(getFilename(dataset, poseIndex, "mat"), DEPTH_CAMERA);
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
    }

    //merge all frames together
    oc::File3d(dataset + "/output.ply", true).WriteModel(merged);

    return 0;
}
