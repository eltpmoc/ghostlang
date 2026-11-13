import re

with open('include/GhostNumberG.hpp', 'r') as f:
    content = f.read()

# Add methods inside the class
methods = """
    GhostNumberG relu() const;
    GhostNumberG sigmoid() const;
    GhostNumberG sin() const;
    GhostNumberG cos() const;
    GhostNumberG tan() const;
"""

# Insert before "bool isValid() const;"
content = content.replace('bool isValid() const;', methods + '\n    bool isValid() const;')

with open('include/GhostNumberG.hpp', 'w') as f:
    f.write(content)

with open('src/GhostNumberG.cpp', 'a') as f:
    f.write("""
#include <cmath>

GhostNumberG GhostNumberG::relu() const {
    double new_x = (x > 0.0) ? x : 0.0;
    double new_p = (x > 0.0) ? p : 0.0;
    return GhostNumberG(new_x, y, new_p, theta2);
}

GhostNumberG GhostNumberG::sigmoid() const {
    double sig = 1.0 / (1.0 + std::exp(-x));
    double new_p = p * (sig * (1.0 - sig));
    return GhostNumberG(sig, y, new_p, theta2);
}

GhostNumberG GhostNumberG::sin() const {
    return GhostNumberG(std::sin(x), y, p * std::cos(x), theta2);
}

GhostNumberG GhostNumberG::cos() const {
    return GhostNumberG(std::cos(x), y, p * -std::sin(x), theta2);
}

GhostNumberG GhostNumberG::tan() const {
    double sec_x = 1.0 / std::cos(x);
    return GhostNumberG(std::tan(x), y, p * (sec_x * sec_x), theta2);
}
""")
