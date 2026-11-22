#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/glm.hpp>

struct BezierCurve
{
    glm::vec3 p0;
    glm::vec3 c0;
    glm::vec3 c1;
    glm::vec3 p1;
};

inline const std::array<BezierCurve, 5> BEZIER_CURVES =
{{
    {
        glm::vec3(-0.509, 2.403, 6.924),
        glm::vec3(2.412, 12.011, 20.396),
        glm::vec3(-23.731, 5.855, 25.199),
        glm::vec3(-25.732, 15.439, 13.561),
    },
    {
        glm::vec3(-25.732, 15.439, 13.561),
        glm::vec3(-27.069, 21.845, 5.781),
        glm::vec3(-4.807, 26.461, -10.716),
        glm::vec3(-7.574, 25.967, -12.861),
    },
    {
        glm::vec3(-7.574, 25.967, -12.861),
        glm::vec3(-21.299, 23.516, -23.495),
        glm::vec3(-18.956, 13.960, -36.813),
        glm::vec3(-3.324, 11.588, -33.743),
    },
    {
        glm::vec3(-3.324, 11.588, -33.743),
        glm::vec3(4.812, 10.354, -32.145),
        glm::vec3(8.408, 22.100, -7.555),
        glm::vec3(23.264, 6.067, -5.370),
    },
    {
        glm::vec3(23.264, 6.067, -5.370),
        glm::vec3(29.367, -0.518, -4.473),
        glm::vec3(2.412, 12.011, 20.396),
        glm::vec3(-0.088, 12.011, 17.896),
    },
}};


// Évaluation analytique : le trajet ne dépend jamais des sommets d'affichage.
inline glm::vec3 evaluateBezier(const BezierCurve& curve, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    const float u = 1.0f - t;
    return u * u * u * curve.p0 + 3.0f * u * u * t * curve.c0
         + 3.0f * u * t * t * curve.c1 + t * t * t * curve.p1;
}

inline glm::vec3 evaluateSpline(float parameter)
{
    if (parameter <= 0.0f)
        return BEZIER_CURVES.front().p0;
    if (parameter >= static_cast<float>(BEZIER_CURVES.size()))
        return BEZIER_CURVES.back().p1;
    const auto index = static_cast<std::size_t>(parameter);
    return evaluateBezier(BEZIER_CURVES[index], parameter - static_cast<float>(index));
}

class BezierPath
{
public:
    BezierPath()
    {
        glm::vec3 previous = evaluateSpline(0.0f);
        for (std::size_t i = 1; i < distances_.size(); ++i)
        {
            const glm::vec3 point = evaluateSpline(parameterAt(i));
            distances_[i] = distances_[i - 1] + glm::length(point - previous);
            previous = point;
        }
    }

    glm::vec3 position(float progress) const
    {
        if (progress <= 0.0f)
            return BEZIER_CURVES.front().p0;
        if (progress >= 1.0f)
            return BEZIER_CURVES.back().p1;

        // Inversion de la longueur d'arc pour conserver une vitesse régulière,
        // avec une table fixe indépendante du nombre de points dessinés.
        const float distance = progress * distances_.back();
        const auto upper = std::lower_bound(distances_.begin(), distances_.end(), distance);
        const std::size_t index = static_cast<std::size_t>(upper - distances_.begin());
        const float fraction = (distance - distances_[index - 1])
                             / (distances_[index] - distances_[index - 1]);
        return evaluateSpline(glm::mix(parameterAt(index - 1), parameterAt(index), fraction));
    }

private:
    static constexpr std::size_t SAMPLES_PER_CURVE = 1024;
    static constexpr std::size_t SAMPLE_COUNT = BEZIER_CURVES.size() * SAMPLES_PER_CURVE;

    static float parameterAt(std::size_t index)
    {
        return static_cast<float>(index) / static_cast<float>(SAMPLES_PER_CURVE);
    }

    std::array<float, SAMPLE_COUNT + 1> distances_{};
};
