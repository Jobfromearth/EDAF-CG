#include "torus_path.hpp"
#include "core/Log.h"

#include <algorithm>
#include <cmath>

namespace torus_path
{
    // ArcLengthTable implementation
    ArcLengthTable::ArcLengthTable() : total_length(0.0f) {}

    ArcLengthTable::~ArcLengthTable() {}

    void ArcLengthTable::build(const std::vector<glm::vec3>& controlPoints, int samples) {
        if (controlPoints.size() < 4) {
            LogError("ArcLengthTable::build(): Need at least 4 control points");
            return;
        }
        
        arc_lengths.clear();
        u_samples.clear();
        arc_lengths.reserve(samples + 1);
        u_samples.reserve(samples + 1);
        
        total_length = 0.0f;
        arc_lengths.push_back(0.0f);
        u_samples.push_back(0.0f);
        
        // Create temporary spline for evaluation
        CatmullRomSpline tempSpline;
        tempSpline.set_control_points(controlPoints);
        
        glm::vec3 prevPoint = tempSpline.evaluate(0.0f);
        
        for (int i = 1; i <= samples; ++i) {
            float u = static_cast<float>(i) / static_cast<float>(samples);
            glm::vec3 currentPoint = tempSpline.evaluate(u);
            
            float segment_length = glm::length(currentPoint - prevPoint);
            total_length += segment_length;
            
            arc_lengths.push_back(total_length);
            u_samples.push_back(u);
            
            prevPoint = currentPoint;
        }
        
        LogInfo("ArcLengthTable built: %d samples, total length: %.2f", samples + 1, total_length);
    }

    float ArcLengthTable::u_from_arc_length(float s) const {
        if (arc_lengths.empty()) return 0.0f;
        
        // Clamp s to valid range
        s = std::max(0.0f, std::min(s, total_length));
        
        // Binary search for the correct interval
        int index = binary_search(s);
        
        if (index == 0) return u_samples[0];
        if (index >= static_cast<int>(arc_lengths.size())) return u_samples.back();
        
        // Linear interpolation within the interval
        float s0 = arc_lengths[index - 1];
        float s1 = arc_lengths[index];
        float u0 = u_samples[index - 1];
        float u1 = u_samples[index];
        
        float t = (s - s0) / (s1 - s0);
        return u0 + t * (u1 - u0);
    }

    int ArcLengthTable::binary_search(float target) const {
        int left = 0;
        int right = static_cast<int>(arc_lengths.size()) - 1;
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            
            if (arc_lengths[mid] == target) {
                return mid;
            } else if (arc_lengths[mid] < target) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        
        return left; // Return the index where target should be inserted
    }

    // CatmullRomSpline implementation
    CatmullRomSpline::CatmullRomSpline() {}

    CatmullRomSpline::~CatmullRomSpline() {}

    void CatmullRomSpline::set_control_points(const std::vector<glm::vec3>& points) {
        if (points.size() < 4) {
            LogError("CatmullRomSpline::set_control_points(): Need at least 4 control points");
            return;
        }
        
        control_points = points;
        LogInfo("CatmullRomSpline initialized with %zu control points", points.size());
    }

    glm::vec3 CatmullRomSpline::evaluate(float u) const {
        if (!is_valid()) return glm::vec3(0.0f);
        
        // Clamp u to [0, 1]
        u = std::max(0.0f, std::min(1.0f, u));
        
        int i0, i1, i2, i3;
        float t;
        get_control_indices(u, i0, i1, i2, i3, t);
        
        return catmull_rom_evaluate(control_points[i0], control_points[i1], 
                                   control_points[i2], control_points[i3], t);
    }

    glm::vec3 CatmullRomSpline::tangent(float u) const {
        if (!is_valid()) return glm::vec3(0.0f, 0.0f, 1.0f);
        
        // Clamp u to [0, 1]
        u = std::max(0.0f, std::min(1.0f, u));
        
        int i0, i1, i2, i3;
        float t;
        get_control_indices(u, i0, i1, i2, i3, t);
        
        return catmull_rom_tangent(control_points[i0], control_points[i1], 
                                  control_points[i2], control_points[i3], t);
    }

    Frame CatmullRomSpline::compute_frame(float u) const {
        if (!is_valid()) return Frame();
        
        glm::vec3 T = glm::normalize(tangent(u));
        
        // Simple Frenet frame construction
        glm::vec3 N;
        if (std::abs(T.y) < 0.9f) {
            N = glm::normalize(glm::cross(T, glm::vec3(0.0f, 1.0f, 0.0f)));
        } else {
            N = glm::normalize(glm::cross(T, glm::vec3(1.0f, 0.0f, 0.0f)));
        }
        
        glm::vec3 B = glm::normalize(glm::cross(T, N));
        
        return Frame(T, N, B);
    }

    glm::vec3 CatmullRomSpline::catmull_rom_evaluate(const glm::vec3& p0, const glm::vec3& p1, 
                                                     const glm::vec3& p2, const glm::vec3& p3, float t) const {
        float t2 = t * t;
        float t3 = t2 * t;
        
        glm::vec3 result = 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * t +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
        
        return result;
    }

    glm::vec3 CatmullRomSpline::catmull_rom_tangent(const glm::vec3& p0, const glm::vec3& p1, 
                                                    const glm::vec3& p2, const glm::vec3& p3, float t) const {
        float t2 = t * t;
        
        glm::vec3 result = 0.5f * (
            (-p0 + p2) +
            2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t +
            3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t2
        );
        
        return result;
    }

    void CatmullRomSpline::get_control_indices(float u, int& i0, int& i1, int& i2, int& i3, float& t) const {
        int num_segments = static_cast<int>(control_points.size()) - 3;
        float segment_u = u * static_cast<float>(num_segments);
        int segment = static_cast<int>(segment_u);
        
        // Clamp segment to valid range
        segment = std::max(0, std::min(segment, num_segments - 1));
        
        i0 = segment;
        i1 = segment + 1;
        i2 = segment + 2;
        i3 = segment + 3;
        
        t = segment_u - static_cast<float>(segment);
    }

    // PathGenerator implementation
    PathGenerator::PathGenerator() {}

    PathGenerator::~PathGenerator() {}

    void PathGenerator::generate_path(const std::vector<glm::vec3>& controlPoints, int samples) {
        spline.set_control_points(controlPoints);
        arc_length_table.build(controlPoints, samples);
        
        LogInfo("PathGenerator: Path generated with %zu control points, %d samples", 
                controlPoints.size(), samples + 1);
    }

    glm::vec3 PathGenerator::get_position(float s) const {
        if (!is_valid()) return glm::vec3(0.0f);
        
        float u = arc_length_table.u_from_arc_length(s);
        return spline.evaluate(u);
    }

    glm::vec3 PathGenerator::get_tangent(float s) const {
        if (!is_valid()) return glm::vec3(0.0f, 0.0f, 1.0f);
        
        float u = arc_length_table.u_from_arc_length(s);
        return glm::normalize(spline.tangent(u));
    }

    Frame PathGenerator::get_frame(float s) const {
        if (!is_valid()) return Frame();
        
        float u = arc_length_table.u_from_arc_length(s);
        return spline.compute_frame(u);
    }
}
