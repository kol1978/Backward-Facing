#include <cmath>
#include <iostream>
int main() {
    double a = 0.0/0.0;  // NaN
    double b = 1.0/0.0;  // Inf
    std::cout << "isnan(NaN): " << std::isnan(a) << std::endl;
    std::cout << "isinf(Inf): " << std::isinf(b) << std::endl;
    return 0;
}
