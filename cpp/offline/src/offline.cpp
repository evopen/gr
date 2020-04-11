#include "library.h"
#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

const int kTotalThreads = 64;

int main(int argc, char** argv)
{
    try
    {
        std::array<gsl_integration_workspace*, kTotalThreads> workspaces;
        for (int i = 0; i < kTotalThreads; ++i)
        {
            workspaces[i] = gsl_integration_workspace_alloc(1000);
        }
        Skybox skybox;
        LoadSkybox("resource/starfield", skybox);
        Camera cam;
        cam.position = glm::dvec3(1, 2, 15);
        cam.front    = glm::dvec3(0, 0, -1);
        Blackhole bh;
        bh.disk_inner = 2;
        bh.disk_outer = 10;
        bh.position   = glm::dvec3(0, 0, 0);
        GenerateDiskTexture(bh);

        const int kWidth  = 4 * 256;
        const int kHeight = 4 * 256;

         MovieWriter movie("movie", kWidth, kHeight);

        uint8_t* img = new uint8_t[kHeight * kWidth * 3];

        for (int frame = 0; frame < 500; frame++)
        {
#pragma omp parallel for num_threads(kTotalThreads)
            for (int row = 0; row < kHeight; row++)
            {

                auto idx = omp_get_thread_num();

                for (int col = 0; col < kWidth; col++)
                {
                    glm::dvec3 tex_coord = GetTexCoord(row, col, kWidth, kHeight);

                    glm::dvec3 color = Trace(tex_coord, bh, cam, skybox, workspaces[idx]) * 255.0;

                    img[row * kWidth * 3 + col * 3 + 0] = color[0];
                    img[row * kWidth * 3 + col * 3 + 1] = color[1];
                    img[row * kWidth * 3 + col * 3 + 2] = color[2];
                }
            }
            // stbi_write_png("raytraced.png", kWidth, kHeight, STBI_rgb, img, kWidth * 3);
            movie.addFrame(img);
            cam.position += glm::dvec3(-0.01, -0.01, -0.01);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}