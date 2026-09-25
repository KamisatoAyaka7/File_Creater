// Sample C++ file for smoke testing
#include <iostream>
#include <vector>
#include <string>

class Cat {
public:
    Cat(std::string name) : m_name(std::move(name)) {}
    void meow() const { std::cout << m_name << " says meow\n"; }
private:
    std::string m_name;class ${Name} {
public:
    ${Name}();
    ~${Name}();

private:
    
};
};

/* A multi-line comment
   spanning several lines */
int main() {
    std::vector<Cat> cats;
    for (int i = 0; i < 3; ++i) {
        cats.emplace_back("cat" + std::to_string(i));
    }
    for (const auto &cat : cats) {
        cat.meow();
    }
    return 0;
}
