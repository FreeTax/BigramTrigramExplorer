#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <omp.h>

// Function to compute bigrams (parallelized)
void compute_bigrams(const std::vector<std::string>& words, std::unordered_map<std::string, int>& bigram_count) {
#pragma omp parallel
    {
        std::unordered_map<std::string, int> local_bigrams;

        // Each thread processes a part of the words
#pragma omp for
        for (size_t i = 0; i < words.size() - 1; i++) {
            std::string bigram = words[i] + " " + words[i + 1];
            local_bigrams[bigram]++;
        }

        // Merge local results into the global map
#pragma omp critical
        {
            for (const auto& pair : local_bigrams) {
                bigram_count[pair.first] += pair.second;
            }
        }
    }
}

// Function to compute trigrams (parallelized)
void compute_trigrams(const std::vector<std::string>& words, std::unordered_map<std::string, int>& trigram_count) {
#pragma omp parallel
    {
        std::unordered_map<std::string, int> local_trigrams;

        // Each thread processes a part of the words
#pragma omp for
        for (size_t i = 0; i < words.size() - 2; i++) {
            std::string trigram = words[i] + " " + words[i + 1] + " " + words[i + 2];
            local_trigrams[trigram]++;
        }

        // Merge local results into the global map
#pragma omp critical
        {
            for (const auto& pair : local_trigrams) {
                trigram_count[pair.first] += pair.second;
            }
        }
    }
}

int main() {
    // Open the input text file
    std::ifstream file("testo.txt");
    if (!file) {
        std::cerr << "Error opening the file!\n";
        return 1;
    }

    // Read the file content into a string
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Split the text into words
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        words.push_back(word);
    }

    // Compute bigrams
    std::unordered_map<std::string, int> bigram_count;
    compute_bigrams(words, bigram_count);

    // Compute trigrams
    std::unordered_map<std::string, int> trigram_count;
    compute_trigrams(words, trigram_count);

    // Print bigram and trigram counts
    std::cout << "Bigram counts:\n";
    for (const auto& pair : bigram_count) {
        std::cout << pair.first << ": " << pair.second << "\n";
    }

    std::cout << "\nTrigram counts:\n";
    for (const auto& pair : trigram_count) {
        std::cout << pair.first << ": " << pair.second << "\n";
    }

    return 0;
}
