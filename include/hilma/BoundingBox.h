#pragma once

#include "glm/glm.hpp"

namespace hilma {

class BoundingBox {
public:

    glm::vec3 min;
    glm::vec3 max;
    
    BoundingBox(): min(99999999.9f), max(-99999999.9f) {}

    float  getWidth() const { return fabs(max.x - min.x); }
    float  getHeight() const { return fabs(max.y - min.y); }
    float  getDepth() const { return fabs(max.z - min.z); }
    
    glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    
    bool    containsX(float _x) const { return _x >= min.x && _x <= max.x; }
    bool    containsY(float _y) const { return _y >= min.y && _y <= max.y; }
    bool    containsZ(float _z) const { return _z >= min.z && _z <= max.z; }

    bool    contains(float _x, float _y) const { return containsX(_x) && containsY(_y); }
    bool    contains(const glm::vec2& _v) const { return containsX(_v.x) && containsY(_v.y); }

    bool    contains(float _x, float _y, float _z) const { return containsX(_x) && containsY(_y) && containsZ(_z); }
    bool    contains(const glm::vec3& _v) const { return containsX(_v.x) && containsY(_v.y) && containsZ(_v.z); }

    void    expand(const glm::vec2& _v) { expand(_v.x, _v.y); }
    void    expand(const glm::vec3& _v) { expand(_v.x, _v.y, _v.z); }

    void    expand(float _x, float _y) {
        min.x = std::min(min.x, _x);
        max.x = std::max(max.x, _x);
        min.y = std::min(min.y, _y);
        max.y = std::max(max.y, _y);
    }

    void    expand(float _x, float _y, float _z) {
        expand(_x, _y);
        min.z = std::min(min.z, _z);
        max.z = std::max(max.z, _z);
    }

};

}