#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace torus_path
{
    //! \brief Frame structure containing tangent, normal, and binormal vectors
    struct Frame {
        glm::vec3 T;  // Tangent (forward direction)
        glm::vec3 N;  // Normal (upward direction)
        glm::vec3 B;  // Binormal (side direction)
        
        Frame() : T(0.0f, 0.0f, 1.0f), N(0.0f, 1.0f, 0.0f), B(1.0f, 0.0f, 0.0f) {}
        Frame(glm::vec3 tangent, glm::vec3 normal, glm::vec3 binormal) 
            : T(tangent), N(normal), B(binormal) {}
    };

    //! \brief Arc length table for uniform parameterization
    class ArcLengthTable {
    public:
        ArcLengthTable();
        ~ArcLengthTable();
        
        //! \brief Build the arc length table from a spline
        void build(const std::vector<glm::vec3>& controlPoints, int samples = 1000);
        
        //! \brief Get parameter u from arc length s
        float u_from_arc_length(float s) const;
        
        //! \brief Get total arc length
        float get_total_length() const { return total_length; }
        
        //! \brief Check if table is valid
        bool is_valid() const { return !arc_lengths.empty(); }
        
    private:
        std::vector<float> arc_lengths;  // Cumulative arc lengths
        std::vector<float> u_samples;    // Corresponding parameter u values
        float total_length;
        
        int binary_search(float target) const;
    };

    //! \brief Catmull-Rom spline evaluator
    class CatmullRomSpline {
    public:
        CatmullRomSpline();
        ~CatmullRomSpline();
        
        //! \brief Set control points (minimum 4 points required)
        void set_control_points(const std::vector<glm::vec3>& points);
        
        //! \brief Evaluate position at parameter u [0, 1]
        glm::vec3 evaluate(float u) const;
        
        //! \brief Evaluate tangent at parameter u [0, 1]
        glm::vec3 tangent(float u) const;
        
        //! \brief Compute frame at parameter u (using Frenet frame for now)
        Frame compute_frame(float u) const;
        
        //! \brief Check if spline is valid
        bool is_valid() const { return control_points.size() >= 4; }
        
        //! \brief Get number of control points
        size_t get_control_point_count() const { return control_points.size(); }
        
    public:
        std::vector<glm::vec3> control_points;
        
        // Helper functions for Catmull-Rom evaluation
        glm::vec3 catmull_rom_evaluate(const glm::vec3& p0, const glm::vec3& p1, 
                                      const glm::vec3& p2, const glm::vec3& p3, float t) const;
        glm::vec3 catmull_rom_tangent(const glm::vec3& p0, const glm::vec3& p1, 
                                     const glm::vec3& p2, const glm::vec3& p3, float t) const;
        
        // Get control point indices for a given u
        void get_control_indices(float u, int& i0, int& i1, int& i2, int& i3, float& t) const;
    };

    //! \brief Path generator that combines spline and arc length parameterization
    class PathGenerator {
    public:
        PathGenerator();
        ~PathGenerator();
        
        //! \brief Generate a path from control points
        void generate_path(const std::vector<glm::vec3>& controlPoints, int samples = 1000);
        
        //! \brief Get position at arc length s
        glm::vec3 get_position(float s) const;
        
        //! \brief Get tangent at arc length s
        glm::vec3 get_tangent(float s) const;
        
        //! \brief Get frame at arc length s
        Frame get_frame(float s) const;
        
        //! \brief Get total path length
        float get_total_length() const { return arc_length_table.get_total_length(); }
        
        //! \brief Check if path is valid
        bool is_valid() const { return spline.is_valid() && arc_length_table.is_valid(); }
        
        //! \brief Get control points
        const std::vector<glm::vec3>& get_control_points() const { return spline.control_points; }
        
    private:
        CatmullRomSpline spline;
        ArcLengthTable arc_length_table;
    };
}
