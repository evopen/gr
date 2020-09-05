#include "Camera.h"
#include "library.h"
#include "pch.h"


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

const int kTotalThreads = 16;

const int kWidth  = 4 * 64;
const int kHeight = 4 * 64;

uint8_t* img;
uint8_t* bloom_buffer;

Skybox skybox;
// dhh::camera::Camera camera(glm::vec3(18, 1, 16));
dhh::camera::Camera camera(glm::vec3(0, 1, 12));
Blackhole bh;
std::mt19937 rng;
std::uniform_real_distribution<double> uni(-0.001, 0.001);

std::vector<glm::vec3> positions;
std::vector<glm::vec3> fronts;
std::vector<glm::vec3> ups;

const int kSamples = 16;

const bool kVideo = false;

int frames = 20 * 25;

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
            bool hit = false;

            for (int sample = 0; sample < kSamples; sample++)
            {
                glm::dvec3 sample_coord;
                if (kSamples != 1)
                    sample_coord = tex_coord + glm::dvec3(uni(rng), uni(rng), uni(rng));
                else
                    sample_coord = tex_coord;
                color += Trace(sample_coord, bh, camera.position, skybox, workspace, &hit) * 255.0 / double(kSamples);
            }
            if (hit)
            {
                bloom_buffer[row * kWidth * 3 + col * 3 + 0] = color[0];
                bloom_buffer[row * kWidth * 3 + col * 3 + 1] = color[1];
                bloom_buffer[row * kWidth * 3 + col * 3 + 2] = color[2];
            }

            img[row * kWidth * 3 + col * 3 + 0] = color[0];
            img[row * kWidth * 3 + col * 3 + 1] = color[1];
            img[row * kWidth * 3 + col * 3 + 2] = color[2];
        }
    }
}

void bloom(uint8_t* img, uint8_t* bloom_buffer)
{
    for (int row = 0; row < kHeight; row++)
    {
        for (int col = 0; col < kWidth; ++col)  // every pixel in bloom_buffer
        {
            int num_sample = 0;
            glm::dvec3 color(0, 0, 0);
            for (int i = -1; i <= 1; ++i)
            {
                for (int j = -1; j <= 1; ++j)
                {
                    if (row - i < 0 || row + i > kHeight - 1 || col - j < 0 || col + j > kWidth - 1)
                        continue;

                    color[0] += bloom_buffer[(row + i) * kWidth * 3 + (col + j) * 3 + 0];
                    color[1] += bloom_buffer[(row + i) * kWidth * 3 + (col + j) * 3 + 1];
                    color[2] += bloom_buffer[(row + i) * kWidth * 3 + (col + j) * 3 + 2];


                    num_sample++;
                }
            }
            color = color / double(num_sample);
            if (glm::length(color) > 1e-5)
            {
                img[row * kWidth * 3 + col * 3 + 0] = color[0];
                img[row * kWidth * 3 + col * 3 + 1] = color[1];
                img[row * kWidth * 3 + col * 3 + 2] = color[2];
            }
            else
            {
                continue;
            }
        }
    }
}

void GenerateMovement()
{
    glm::vec3 pos    = camera.position;
    glm::vec3 up     = camera.up;
    glm::vec3 front  = camera.front;
    glm::vec3 bh_dir = glm::vec3(bh.position) - camera.position;

    glm::vec3 mid_point_1(16, 1, 10);
    float duration     = 8 * 25;
    glm::vec3 velocity = (mid_point_1 - pos) / duration;

    for (int i = 0; i < duration; ++i)
    {
        pos += velocity;
        bh_dir = glm::vec3(0.3, 0.4, 0.5) - pos;

        positions.emplace_back(pos);
        fronts.emplace_back(bh_dir);
        ups.emplace_back(up);
    }

    glm::vec3 mid_point_2(20, 15, -30);
    duration = 4 * 25;
    velocity = (mid_point_2 - pos) / duration;

    for (int i = 0; i < duration; ++i)
    {
        pos += velocity;
        bh_dir = glm::vec3(0.3, -0.4, 0.5) - pos;

        positions.emplace_back(pos);
        fronts.emplace_back(bh_dir);
        ups.emplace_back(up);
    }

    glm::vec3 mid_point_3(30, -30, 60);
    duration = 8 * 25;
    velocity = (mid_point_3 - pos) / duration;

    for (int i = 0; i < duration; ++i)
    {
        pos += velocity;
        bh_dir = glm::vec3(0.3, -0.4, 0.5) - pos;

        positions.emplace_back(pos);
        fronts.emplace_back(bh_dir);
        ups.emplace_back(up);
    }
}

int main(int argc, char** argv)
{
    if (!kVideo)
    {
        frames = 1;
    }

    GenerateMovement();

    try
    {
        camera.front = glm::vec3(0.1, 0.2, 0.3) - camera.position;
        camera.right = glm::normalize(glm::cross(camera.up, camera.front));
        LoadSkybox("resource/starfield", skybox);
        bh.disk_inner = 8;
        bh.disk_outer = 18;
        bh.position   = glm::dvec3(0, 0, 0);
        GenerateDiskTexture(bh);

        MovieWriter movie("movie", kWidth, kHeight);

        img          = new uint8_t[kHeight * kWidth * 3]();
        bloom_buffer = new uint8_t[kHeight * kWidth * 3]();

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

            // bloom(img, bloom_buffer);


            if (!kVideo)
            {
                stbi_write_png("raytraced.png", kWidth, kHeight, STBI_rgb, img, kWidth * 3);
            }

            movie.addFrame(img);

            camera.position = positions[frame];
            camera.front    = fronts[frame];
            camera.up       = ups[frame];
            camera.right    = glm::normalize(glm::cross(camera.up, camera.front));
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