#include "library.h"

#include <gtest/gtest.h>

const double kPi = std::acos(-1);

TEST(LibraryTest, ImpactParameterT)
{
    EXPECT_NEAR(CalculateImpactParameter(kPi / 4, 20), 14.9071198499986, 1.0e-10);
}

TEST(LibraryTest, GeodesicT)
{
    EXPECT_NEAR(Geodesic(20, 10), 0.02839809171235324, 1.0e-10);
    EXPECT_NEAR(Geodesic(20, 20), 0.15811388300841897, 1.0e-10);
}

TEST(LibraryTest, ClosestApproach1T)
{
    EXPECT_NEAR(FindClosestApproach1(20, 10), 8.788850662499728, 1.0e-10);
    EXPECT_NEAR(FindClosestApproach1(20, std::sqrt(28)), 3.384042943260197, 1.0e-10);
}

// TEST(LibraryTest, ClosestApproach2T)
//{
//    EXPECT_NEAR(FindClosestApproach2(20, 10), 8.788850662499728, 1.0e-10);
//    EXPECT_NEAR(FindClosestApproach2(20, std::sqrt(28)), 3.384042943260197, 1.0e-10);
//}

TEST(LibraryTest, IntegrateT)
{
    EXPECT_NEAR(Integrate(30, 20, 10), -0.18206352097090867, 1.0e-5);
    EXPECT_NEAR(Integrate(10, 60, 8), 0.7641980989670553, 1.0e-8);
    EXPECT_NEAR(Integrate(10, 600, 8), 0.8845859084325743, 1.0e-5);
    EXPECT_NEAR(Integrate(10, 6000, 8), 0.8965863021431181, 1.0e-5);
    EXPECT_NEAR(Integrate(13, 2000, 14), 1.5835582261400813, 1.0e-5);
}

TEST(LibraryTest, Integrate2T)
{
    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1000);
    EXPECT_NEAR(Integrate(30, 20, 10, w), -0.18206352097090867, 1.0e-5);
    EXPECT_NEAR(Integrate(10, 60, 8, w), 0.7641980989670553, 1.0e-8);
    EXPECT_NEAR(Integrate(10, 600, 8, w), 0.8845859084325743, 1.0e-5);
    EXPECT_NEAR(Integrate(10, 6000, 8, w), 0.8965863021431181, 1.0e-5);
    EXPECT_NEAR(Integrate(13, 2000, 14, w), 1.5835582261400813, 1.0e-5);
}

TEST(LibraryTest, SkyboxSamplerT)
{
    Skybox skybox;
    LoadSkybox("resource/starfield", skybox);
    glm::dvec3 color = SkyboxSampler(glm::dvec3(-1, 1, -1), skybox);
    EXPECT_NEAR(color.r, 0.03137255, 1.0e-3);
    EXPECT_NEAR(color.g, 0.0, 1.0e-3);
    EXPECT_NEAR(color.b, 0.05098039, 1.0e-3);

    color = SkyboxSampler(glm::dvec3(0.5, 0.3, -1), skybox);
    EXPECT_NEAR(color.r, 0.05882353, 1.0e-3);
    EXPECT_NEAR(color.g, 0.02352941, 1.0e-3);
    EXPECT_NEAR(color.b, 0.10588235, 1.0e-3);
}


TEST(LibraryTest, GetTexCoordT)
{
    glm::dvec3 tex_coord = GetTexCoord(2, 3, 16, 16);
    EXPECT_NEAR(tex_coord[0], -0.6, 1.0e-5);
    EXPECT_NEAR(tex_coord[1], 0.73333333, 1.0e-5);
    EXPECT_NEAR(tex_coord[2], -1, 1.0e-5);
}

TEST(LibraryTest, DiskSamplerT)
{
    glm::dvec3 color;
    Blackhole bh;
    bh.position = glm::dvec3(0, 0, 0);
    bh.disk_inner = 2;
    bh.disk_outer = 10;
    GenerateDiskTexture(bh);
    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1000);

    color = DiskSampler(glm::dvec3(0, 3, 4), 6, 10, 5, glm::dvec3(3, 4, 5), bh, w);

    try
    {

        EXPECT_NEAR(color[0], 0.6, 1.0e-5);
        EXPECT_NEAR(color[1], 0.4, 1.0e-5);
        EXPECT_NEAR(color[2], 0, 1.0e-5);
    }catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
