#include <cxxopts.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <array>
#include <exception>
#include <filesystem>
#include <vector>

struct Arguments
{
    std::filesystem::path skybox_folder_path;
    std::filesystem::path disk_texture_path;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    glm::dvec3 camera_pos;
    glm::dvec3 camera_lookat;
    std::filesystem::path camera_pos_path;
    bool animate;
};

void Parse(int argc, char* argv[], Arguments& args)
{
    try
    {
        std::filesystem::path filename = std::filesystem::path(argv[0]).filename();
        cxxopts::Options options(filename.string(), " - A Schwarzschild Blackhole Generator");

        std::vector<double> camera_pos_vec;
        std::vector<double> camera_lookat_vec;

        // clang-format off
        options.add_options()
            ("h, help", "Print help");

        options.add_options("Required")
            ("skybox", "Path to skybox folder", cxxopts::value<std::filesystem::path>(args.skybox_folder_path), "PATH")
            ("disk", "Path to disk texture file", cxxopts::value<std::filesystem::path>(args.disk_texture_path), "FILE")
            ("width", "width for output file", cxxopts::value<uint32_t>(args.width), "NUM")
            ("height", "height for output file", cxxopts::value<uint32_t>(args.height), "NUM")
            ("sample", "samples for pixel", cxxopts::value<uint32_t>(args.samples), "NUM");

        options.add_options("Image")
            ("pos", "Camera position", cxxopts::value<std::vector<double>>(camera_pos_vec), "VECOTR")
            ("lookat", "Camera lookat", cxxopts::value<std::vector<double>>(camera_lookat_vec), "VECTOR");

        options.add_options("Video")
            ("pos-file", "Path to camera position file", cxxopts::value<std::filesystem::path>(args.camera_pos_path), "FILE");
        // clang-format on

        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            std::cout << options.help({"Required", "Image", "Video", ""}) << std::endl;
            exit(0);
        }

        if (result.count("skybox") != 1 || result.count("disk") != 1 || result.count("sample") != 1
            || result.count("width") != 1 || result.count("height") != 1)
        {
            std::cout << "missing required arguments\n" << options.help({"Required", "Image", "Video", ""}) << "\n";
            exit(0);
        }

        if (result.count("pos") == 1 && result.count("lookat") == 1 && result.count("pos-file") == 0)  // image mode
        {
            args.animate = false;
            if (camera_lookat_vec.size() != 3)
            {
                std::cout << "camera lookat vector must be 3-d"
                          << "\n";
                exit(0);
            }
            if (camera_pos_vec.size() != 3)
            {
                std::cout << "camera position vector must be 3-d"
                          << "\n";
                exit(0);
            }
            for (int i = 0; i < 3; ++i)
            {
                args.camera_pos[i]    = camera_pos_vec[i];
                args.camera_lookat[i] = camera_lookat_vec[i];
            }
        }
        else if (result.count("pos") == 0 && result.count("lookat") == 0 && result.count("pos-file") == 1)
        {  // video mode
            args.animate = true;
            if (!std::filesystem::exists(args.camera_pos_path)
                || std::filesystem::is_regular_file(args.camera_pos_path))
            {
                std::cout << "camera position file not exist\n";
                exit(0);
            }
        }
        else
        {
            std::cout << "can either work in image mode or video mode\n";
            exit(0);
        }


        if (!std::filesystem::exists(args.skybox_folder_path))
        {
            std::cout << "skybox texture folder does not exist"
                      << "\n";
            exit(0);
        }

        if (!std::filesystem::is_directory(args.skybox_folder_path))
        {
            std::cout << "skybox option must be a directory"
                      << "\n";
            exit(0);
        }

        if (!std::filesystem::exists(args.disk_texture_path))
        {
            std::cout << "accretion texture does not exist"
                      << "\n";
            exit(0);
        }

        if (!std::filesystem::is_regular_file(args.disk_texture_path))
        {
            std::cout << "disk texture must be a single file"
                      << "\n";
            exit(0);
        }
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        Arguments args;
        Parse(argc, argv, args);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}