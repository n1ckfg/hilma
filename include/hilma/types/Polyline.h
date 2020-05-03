#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "Mesh.h"

namespace hilma {

enum class CapType {
    BUTT = 0,   // No points added to end of line
    SQUARE = 2, // Two points added to make a square extension
    ROUND = 6   // Six points added in a fan to make a round cap
};

enum class JoinType {
    MITER = 0,  // No points added at line join
    BEVEL = 1,  // One point added to flatten the corner of a join
    ROUND = 5   // Five points added in a fan to make a round outer join
};

class Polyline {
public:

    Polyline();
    Polyline(const Polyline& _poly);
    Polyline(const glm::vec2* _points, int _n);
    Polyline(const float* _array2D, int _m, int _n);
    virtual ~Polyline();


    void                    append(const Polyline& _poly);

    void                    addPoint(const glm::vec2& _point);
    void                    addPoint(const float* _array1D, int _n);

	void                    addPoints(const glm::vec2* _points, int _n);
    void                    addPoints(const float* _array2D, int _m, int _n);

    glm::vec2 &             operator [](const int& _index);
    const glm::vec2 &       operator [](const int& _index) const;

    const std::vector<glm::vec2> & getPoints() const;
    glm::vec2               getPointAt(const float& _dist) const;

    size_t                  size() const { return m_points.size(); }
    bool                    empty() const { return size() == 0; }

    std::vector<Polyline>   splitAt(float _dist);

    void                    clear();
    
protected:
    std::vector<glm::vec2>  m_points;

    friend Mesh surface(const Polyline&);
    friend Mesh extrude(const Polyline&,float, float);
    friend Mesh spline(const Polyline&, float, JoinType, CapType, float);
};

}