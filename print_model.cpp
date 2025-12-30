#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

int main() {
    const char* model_path = "models/light_Tencent_AILab_ChineseEmbedding.bin";
    std::ifstream file(model_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open model file." << std::endl;
        return 1;
    }

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "Error: Could not read header." << std::endl;
        return 1;
    }

    std::stringstream ss(line);
    int vocab_size = 0, dim = 0;
    ss >> vocab_size >> dim;

    if (vocab_size == 0) {
        std::cout << "Raw header line: [" << line << "]" << std::endl;
        return 1;
    }

    std::cout << "Header: Vocab Size = " << vocab_size << ", Dimension = " << dim << "\n" << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::string word;
        char c;
        while (file.get(c) && c != ' ' && c != '\n' && c != '\t') {
            word += c;
        }
        
        if (word.empty() && i > 0) break;

        std::vector<float> vec(dim);
        file.read(reinterpret_cast<char*>(vec.data()), dim * sizeof(float));

        if (!file) {
            std::cerr << "Error reading vector for word " << i+1 << std::endl;
            break;
        }

        std::cout << "Word " << i + 1 << ": " << word << std::endl;
        std::cout << "Vector (first 5): ";
        for (int j = 0; j < 5 && j < dim; j++) {
            std::cout << std::fixed << std::setprecision(6) << vec[j] << " ";
        }
        std::cout << "..." << std::endl << std::endl;

        if (file.peek() == '\n') file.get();
    }

    file.close();
    return 0;
}
