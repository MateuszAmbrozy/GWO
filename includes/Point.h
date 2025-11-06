#pragma once
#include <iostream>
#include <cmath>

class Point {
private:
    int x;
    int y;

public:
    Point()
    :   x(0), y(0)
    {}
    Point(int x, int y)
        :x(x), y(y) 
    {}
    float dist(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    //get
    inline int get_x() const {
        return this->x;
    }
    int get_y() const {
        return this->y;
    }
    //set
    void set_x(int x) {
        this->x = x;
    }
    void set_y(int y) {
        this->y = y;
    }
};