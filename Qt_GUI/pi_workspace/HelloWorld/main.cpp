#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Hello World!" << std::endl;

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
            std::cout << argv[i] << ' ';
        std::cout << std::endl;
    }
}
