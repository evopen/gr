#include "Camera.h"
#include "library.h"
#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

const int kTotalThreads = 16;

const int kWidth  = 4 * 512;
const int kHeight = 4 * 512;

uint8_t* img;

Skybox skybox;
Camera cam;
dhh::camera::Camera camera(glm::vec3(3, 2, 20));
Blackhole bh;
std::mt19937 rng;
std::uniform_real_distribution<double> uni(-0.001, 0.001);


const int kSamples = 32;

const bool kVideo = false;

int frames = 400;

void Worker(int idx)
{
    gsl_integration_workspace* workspace = gsl_integration_workspace_alloc(1000);
    for (int row = idx; row < kHeight; row += kTotalThreads)
    {
        if (idx == 0)
            std::cout << double(row) / kHeight << std::endl;
        for (int col = 0; col < kWidth; ++col)
        {
            glm::dvec3 tex_coord = dhh::camera::GetTexCoord(row, col, kWidth, kHeight, camera);
            glm::dvec3 color(0, 0, 0);
            for (int sample = 0; sample < kSamples; sample++)
            {
                glm::dvec3 sample_coord = tex_coord + glm::dvec3(uni(rng), uni(rng), uni(rng));
                color += Trace(sample_coord, bh, cam, skybox, workspace) * 255.0 / double(kSamples);
            }

            img[row * kWidth * 3 + col * 3 + 0] = color[0];
            img[row * kWidth * 3 + col * 3 + 1] = color[1];
            img[row * kWidth * 3 + col * 3 + 2] = color[2];
        }
    }
}

int main(int argc, char** argv)
{
    if (!kVideo)
    {
        frames = 1;
    }

    try
    {
        LoadSkybox("resource/starfield", skybox);
        bh.disk_inner = 2;
        bh.disk_outer = 10;
        bh.position   = glm::dvec3(0, 0, 0);
        GenerateDiskTexture(bh);

        MovieWriter movie("movie", kWidth, kHeight);

        img = new uint8_t[kHeight * kWidth * 3];

        auto start = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < frames; frame++)
        {

            std::vector<std::thread> threads;
            for (int i = 0; i < kTotalThreads; ++i)
            {
                threads.emplace_back(std::thread(Worker, i));
                // std::cout << "Thread " << i << " has started.\n";
            }

            for (int i = 0; i < kTotalThreads; ++i)
            {
                threads[i].join();
                // std::cout << "Thread " << i << " has finished.\n";
            }

            if (!kVideo)
            {
                stbi_write_png("raytraced.png", kWidth, kHeight, STBI_rgb, img, kWidth * 3);
            }
            movie.addFrame(img);
            /* camera.ProcessMove(dhh::camera::CameraMovement::kForward, 0.05);
             camera.ProcessMove(dhh::camera::CameraMovement::kDown, 0.05);
             camera.ProcessMove(dhh::camera::CameraMovement::kLeft, 0.05);*/
            camera.ProcessMouseMovement(2, 0);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}