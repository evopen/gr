#pragma once

#include "movie.h"

#include <boost/math/quadrature/trapezoidal.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/numeric/odeint.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <gsl/gsl_integration.h>
#include <omp.h>
#include <opencv2/opencv.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>