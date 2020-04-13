#include "library.h"
#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

const int kTotalThreads = 16;

const int kWidth  = 4 * 16;
const int kHeight = 4 * 16;

uint8_t* img;

Skybox skybox;
Camera cam;
Blackhole bh;


void Worker(int idx)
{
    gsl_integration_workspace* workspace = gsl_integration_workspace_alloc(1000);
    for (int row = idx; row < kHeight; row += kTotalThreads)
    {
        for (int col = 0; col < kWidth; ++col)
        {
            glm::dvec3 tex_coord = GetTexCoord(row, col, kWidth, kHeight);
            glm::dvec3 color     = Trace(tex_coord, bh, cam, skybox, workspace) * 255.0;

            img[row * kWidth * 3 + col * 3 + 0] = color[0];
            img[row * kWidth * 3 + col * 3 + 1] = color[1];
            img[row * kWidth * 3 + col * 3 + 2] = color[2];
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        LoadSkybox("resource/starfield", skybox);
        cam.position = glm::dvec3(0, 2, 15);
        cam.front    = glm::dvec3(0, 0, -1);
        bh.disk_inner = 2;
        bh.disk_outer = 10;
        bh.position   = glm::dvec3(0, 0, 0);
        GenerateDiskTexture(bh);

        MovieWriter movie("movie", kWidth, kHeight);

        img = new uint8_t[kHeight * kWidth * 3];

        auto start = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < 200; frame++)
        {

            std::vector<std::thread> threads;
            for (int i = 0; i < kTotalThreads; ++i)
            {
                threads.emplace_back(std::thread(Worker, i));
                //std::cout << "Thread " << i << " has started.\n";
            }

            for (int i = 0; i < kTotalThreads; ++i)
            {
                threads[i].join();
                //std::cout << "Thread " << i << " has finished.\n";
            }

            // stbi_write_png("raytraced.png", kWidth, kHeight, STBI_rgb, img, kWidth * 3);
            movie.addFrame(img);
            cam.position += glm::dvec3(-0.01, -0.01, -0.01);
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