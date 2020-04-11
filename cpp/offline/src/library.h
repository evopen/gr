#include "pch.h"

#include <glm/gtx/rotate_vector.hpp>

using namespace boost::math::constants;

struct Skybox
{
    cv::Mat front;
    cv::Mat back;
    cv::Mat top;
    cv::Mat bottom;
    cv::Mat left;
    cv::Mat right;
};

struct Blackhole
{
    glm::dvec3 position;
    double disk_inner;
    double disk_outer;
    std::vector<glm::dvec3> disk_texture;
};

struct Camera
{
    glm::dvec3 position;
    glm::dvec3 front;
};

inline void GenerateDiskTexture(Blackhole& bh)
{
    int disk_resolution = 20;
    bh.disk_texture.resize(disk_resolution);
    for (int i = 0; i < bh.disk_texture.size(); ++i)
    {
        bh.disk_texture[i] = {float(i) / disk_resolution, 1 - float(i) / disk_resolution, 0};
    }
}

inline double CalculateImpactParameter(double theta, double r, double rs)
{
    return r * std::sin(theta) / std::sqrt(1 - rs / r);
}

inline double CalculateImpactParameter(double theta, double r)
{
    return r * std::sin(theta) / std::sqrt(1 - 2 / r);
}

inline double Geodesic(double r, double b)
{
    return 1 / (r * r * sqrt(1 / (b * b) - 1 / (r * r) + 2 / (r * r * r)));
}

inline double Geodesic(double r, void* params)
{
    double b = *(double*) params;
    return Geodesic(r, b);
}

inline double FindClosestApproach1(double r0, double b)
{
    constexpr int kMaxDigits = std::numeric_limits<double>::digits;
    constexpr int kDigits    = kMaxDigits * 3 / 4;

    boost::math::tools::eps_tolerance<double> tol(kDigits);

    std::pair<double, double> result =
        boost::math::tools::bisect([b](double r) { return r / std::sqrt(1 - 2 / r) - b; }, 3.0, r0, tol);

    return (result.first + result.second) / 2 + 1e-7;
}

inline double FindClosestApproach2(double r0, double b)
{
    constexpr int kMaxDigits = std::numeric_limits<double>::digits;
    constexpr int kDigits    = kMaxDigits * 3 / 4;

    boost::math::tools::eps_tolerance<double> tol(kDigits);
    boost::uintmax_t it = 1000000;

    std::pair<double, double> result = boost::math::tools::bracket_and_solve_root(
        [b](double r) { return r / std::sqrt(1 - 2 / r) - b; }, r0, 2.0, true, tol, it);

    return (result.first + result.second) / 2 + 1e-7;
}

inline double Integrate(double r0, double r1, double b)
{
    double dphi;
    if (r0 < r1)
    {
        dphi = boost::math::quadrature::trapezoidal(
            [b](double r) { return 1 / (r * r * sqrt(1 / (b * b) - 1 / (r * r) + 2 / (r * r * r))); }, r0, r1);
    }
    else
    {
        dphi = -boost::math::quadrature::trapezoidal(
            [b](double r) { return 1 / (r * r * sqrt(1 / (b * b) - 1 / (r * r) + 2 / (r * r * r))); }, r1, r0);
    }
    return dphi;
}

inline double Integrate(double r0, double r1, double b, gsl_integration_workspace* w)
{
    gsl_function func;
    func.function = &Geodesic;
    func.params   = &b;

    double dphi, error;

    gsl_integration_qags(&func, r0, r1, 0, 1e-4, 1000, w, &dphi, &error);

    return dphi;
}

inline void LoadSkybox(std::filesystem::path dir, Skybox& skybox)
{
    cv::Mat image;

    image = cv::imread((dir / "front.jpg").string());
    image.convertTo(skybox.front, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.front, skybox.front, cv::COLOR_BGR2RGB);

    image = cv::imread((dir / "back.jpg").string());
    image.convertTo(skybox.back, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.back, skybox.back, cv::COLOR_BGR2RGB);

    image = cv::imread((dir / "top.jpg").string());
    image.convertTo(skybox.top, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.top, skybox.top, cv::COLOR_BGR2RGB);

    image = cv::imread((dir / "bottom.jpg").string());
    image.convertTo(skybox.bottom, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.bottom, skybox.bottom, cv::COLOR_BGR2RGB);

    image = cv::imread((dir / "left.jpg").string());
    image.convertTo(skybox.left, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.left, skybox.left, cv::COLOR_BGR2RGB);

    image = cv::imread((dir / "right.jpg").string());
    image.convertTo(skybox.right, CV_32F, 1.0 / 255);
    cv::cvtColor(skybox.right, skybox.right, cv::COLOR_BGR2RGB);
}

inline glm::dvec3 SkyboxSampler(const glm::dvec3& tex_coord, const Skybox& skybox)
{
    int tex_size = 4096;

    glm::dvec3 coord_abs = glm::abs(tex_coord);
    int side_index       = 0;

    if (coord_abs.x - coord_abs.y < DBL_EPSILON && coord_abs.x - coord_abs.z < DBL_EPSILON)
    {
        side_index = 2;
    }
    else
    {
        if (coord_abs.x > coord_abs.y && coord_abs.x > coord_abs.z)
        {
            side_index = 0;
        }
        else if (coord_abs.y > coord_abs.x && coord_abs.y > coord_abs.z)
        {
            side_index = 1;
        }
        else
        {
            side_index = 2;
        }
    }

    double scale = 0;
    if (tex_coord[side_index] < 0)
    {
        scale = -1 / tex_coord[side_index];
    }
    else
    {
        scale = 1 / tex_coord[side_index];
    }
    tex_coord = tex_coord * scale;

    const cv::Mat* image;
    glm::dvec2 coord_2d;

    if (side_index == 2)
    {
        if (tex_coord[side_index] > 0)
        {
            image = &skybox.back;
        }
        else if (tex_coord[side_index] < 0)
        {
            image = &skybox.front;
        }
        coord_2d.x = (tex_coord.x + 1) / 2;
        coord_2d.y = (tex_coord.y + 1) / 2;
    }
    else if (side_index == 1)
    {
        if (tex_coord[side_index] > 0)
        {
            image = &skybox.top;
        }
        else if (tex_coord[side_index] < 0)
        {
            image = &skybox.bottom;
        }
        coord_2d.x = (tex_coord.x + 1) / 2;
        coord_2d.y = (tex_coord.z + 1) / 2;
    }
    else if (side_index == 0)
    {
        if (tex_coord[side_index] > 0)
        {
            image = &skybox.right;
        }
        else if (tex_coord[side_index] < 0)
        {
            image = &skybox.left;
        }
        coord_2d.x = (tex_coord.y + 1) / 2;
        coord_2d.y = (tex_coord.z + 1) / 2;
    }

    cv::Vec3f color;

    int row = std::lround((1 - coord_2d[1]) * (tex_size - 1));
    int col = std::lround(coord_2d[0] * (tex_size - 1));
    color   = image->at<cv::Vec3f>(row, col);
    return glm::dvec3(color[0], color[1], color[2]);
}

inline glm::dvec3 DiskSampler(glm::dvec3 start_pos, double b, double r0, double r1, glm::dvec3 rotation_axis,
    const Blackhole& bh, gsl_integration_workspace* w)
{
    int direction = 0;
    if (r0 < r1)
    {
        direction = 1;
    }
    else
    {
        direction = -1;
    }
    start_pos                = start_pos / length(start_pos) * r0;
    double step_size         = 0.05;
    glm::dvec3 pos_near_disk = start_pos;
    glm::dvec3 pos           = start_pos;
    double integrate_span    = std::abs(r0 - r1);

    int steps = int(integrate_span / step_size);

    for (int step = 1; step <= steps; ++step)
    {
        glm::dvec3 last_pos = pos;
        if (abs(pos[1]) < abs(pos_near_disk[1]))
        {
            pos_near_disk = pos;
        }
        double dphi = Integrate(r0, r0 + step * step_size * direction, b, w);
        pos         = glm::rotate(start_pos, abs(dphi), rotation_axis);
        pos         = pos / glm::length(pos) * (r0 + step * step_size * direction);
        if (pos[1] * last_pos[1] < 0)
            break;
    }
    double dr = glm::length(pos_near_disk) - bh.disk_inner;

    int sample_index = dr / (bh.disk_outer - bh.disk_inner) * (bh.disk_texture.size() - 1);

    return bh.disk_texture[sample_index];
}

inline double GetCosAngle(glm::dvec3 v1, glm::dvec3 v2)
{
    return dot(v1, v2) / (length(v1) * length(v2));
}

inline glm::dvec3 Trace(
    glm::dvec3 tex_coord, const Blackhole& bh, const Camera& cam, const Skybox& skybox, gsl_integration_workspace* w)
{
    glm::dvec3 bh_dir        = bh.position - cam.position;
    glm::dvec3 rotation_axis = glm::normalize(glm::cross(tex_coord, bh_dir));
    double cos_theta         = GetCosAngle(tex_coord, bh_dir);
    double theta             = std::acos(cos_theta);
    double r0                = glm::length(cam.position);
    double b                 = CalculateImpactParameter(theta, r0);
    double integrate_end     = 2000;
    if (b < std::sqrt(27))
    {
        double dphi                 = std::fmod(Integrate(r0, bh.disk_outer, b, w), 2 * pi<double>());  /// here
        glm::dvec3 photon_pos_start = glm::rotate(cam.position, -dphi, rotation_axis);
        double dphi_in_disk         = Integrate(bh.disk_outer, bh.disk_inner, b, w);
        if (std::abs(dphi_in_disk) > pi<double>())
        {
            return DiskSampler(photon_pos_start, b, bh.disk_outer, bh.disk_inner, rotation_axis, bh, w);
        }
        dphi                      = std::fmod(dphi + dphi_in_disk, pi<double>() * 2);
        glm::dvec3 photon_pos_end = glm::rotate(cam.position, -dphi, rotation_axis);

        if (photon_pos_start[1] * photon_pos_end[1] < 0)
        {
            return DiskSampler(photon_pos_start, b, bh.disk_outer, bh.disk_inner, rotation_axis, bh, w);
        }
        return glm::dvec3(0, 0, 0);
    }
    else
    {
        double r3 = FindClosestApproach1(r0, b);
        if (r3 > bh.disk_outer)
        {
            double dphi = std::fmod(Integrate(r0, r3, b, w) - Integrate(r3, integrate_end, b, w), pi<double>() * 2);

            glm::dvec3 distort_coord = glm::rotate(cam.position, -dphi, rotation_axis);
            return SkyboxSampler(distort_coord, skybox);
        }
        else
        {
            double dphi                 = std::fmod(Integrate(r0, bh.disk_outer, b, w), 2 * pi<double>());
            glm::dvec3 photon_pos_start = glm::rotate(cam.position, -dphi, rotation_axis);

            if (r3 < bh.disk_inner)  // TODO:later
            {
            }
            else
            {
                double dphi_in_disk       = Integrate(bh.disk_outer, r3, b, w);
                dphi                      = std::fmod(dphi + dphi_in_disk, pi<double>() * 2);
                glm::dvec3 photon_pos_end = glm::rotate(cam.position, -dphi, rotation_axis);
                if (photon_pos_start[1] * photon_pos_end[1] < 0)
                {
                    return DiskSampler(photon_pos_start, b, bh.disk_outer, r3, rotation_axis, bh, w);
                }
                photon_pos_start = photon_pos_end;
                dphi_in_disk     = Integrate(r3, bh.disk_outer, b, w);
                dphi             = std::fmod(dphi - dphi_in_disk, pi<double>() * 2);
                photon_pos_end   = glm::rotate(cam.position, -dphi, rotation_axis);
                if (photon_pos_start[1] * photon_pos_end[1] < 0)
                {
                    return DiskSampler(photon_pos_start, b, r3, bh.disk_outer, rotation_axis, bh, w);
                }

                // not hit
                dphi = std::fmod(dphi - Integrate(bh.disk_outer, integrate_end, b, w), pi<double>() * 2);
                glm::dvec3 distort_coord = glm::rotate(cam.position, -dphi, rotation_axis);

                return SkyboxSampler(distort_coord, skybox);
            }
        }
    }
    return glm::dvec3(0, 0, 0);
}

glm::dvec3 GetTexCoord(int row, int col, int width, int height)
{
    double z = -1;
    double x = double(col) / (width - 1) * (1 - (-1)) - 1;
    double y = double(height - row - 1) / (height - 1) * (1 - (-1)) - 1;
    return glm::dvec3(x, y, z);
}