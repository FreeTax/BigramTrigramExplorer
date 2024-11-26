#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <omp.h>
#include <numeric> // For std::accumulate
#include <string>


// Funzione di hash per calcolare l'indice di un bigramma
size_t bigram_hash(const std::string& word1, const std::string& word2, size_t table_size) {
    std::hash<std::string> hasher;
    return (hasher(word1) ^ (hasher(word2) << 1)) % table_size;
}

void compute_bigrams(const std::vector<std::string>& words, std::unordered_map<std::string, int>& bigram_count, bool parallel, int num_threads) {
    const size_t table_size = 1 << 20; // Dimensione della tabella (es: 2^20 celle)
    std::vector<int> bigram_table(table_size, 0); // Struttura preallocata

#pragma omp parallel if(parallel) num_threads(num_threads)
    {
        // Ogni thread accede direttamente al vettore globale
#pragma omp for
        for (size_t i = 0; i < words.size() - 1; i++) {
            size_t index = bigram_hash(words[i], words[i + 1], table_size);

            // Incremento atomico sulla singola cella
#pragma omp atomic
            bigram_table[index]++;
        }
    }

}



// Funzione di hash per calcolare l'indice di un trigramma
size_t trigram_hash(const std::string& word1, const std::string& word2, const std::string& word3, size_t table_size) {
    std::hash<std::string> hasher;
    return (hasher(word1) ^ (hasher(word2) << 1) ^ (hasher(word3) << 2)) % table_size;
}

void compute_trigrams(const std::vector<std::string>& words, std::unordered_map<std::string, int>& trigram_count, bool parallel, int num_threads) {
    const size_t table_size = 1 << 20; // Dimensione della tabella (es: 2^20 celle)
    std::vector<std::atomic<int>> trigram_table(table_size); // Tabella globale atomica

    // Inizializza la tabella
    for (size_t i = 0; i < table_size; ++i) {
        trigram_table[i] = 0;
    }

#pragma omp parallel if(parallel) num_threads(num_threads)
    {
        // Ogni thread processa un sottoinsieme delle parole
#pragma omp for
        for (size_t i = 0; i < words.size() - 2; i++) {
            // Calcola l'hash del trigramma
            size_t index = trigram_hash(words[i], words[i + 1], words[i + 2], table_size);

            // Incremento atomico sulla cella
            trigram_table[index].fetch_add(1, std::memory_order_relaxed);
        }
    }


}

// Function to run a single trial and measure time
double run_trial(const std::vector<std::string>& words, bool parallel, bool is_bigram, int num_threads) {
    std::unordered_map<std::string, int> ngram_count;
    double start_time = omp_get_wtime();

    if (is_bigram) {
        compute_bigrams(words, ngram_count, parallel, num_threads);
    } else {
        compute_trigrams(words, ngram_count, parallel, num_threads);
    }

    double end_time = omp_get_wtime();
    return end_time - start_time;
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

    // Fractions of the text to test
    std::vector<double> fractions = {0.1, 0.25, 0.5, 1.0};
    std::vector<int> thread_counts = {2, 4, 6, 8, 11, 12, 14, 16, 20, 24, 32};

    // Output results
    std::ofstream result_file("timing_results.txt");
    if (!result_file) {
        std::cerr << "Error opening the output file!\n";
        return 1;
    }

    // Run tests
    for (double fraction : fractions) {
        std::cout << "Testing with " << static_cast<int>(fraction * 100) << "% of the text...\n";
        size_t current_size = static_cast<size_t>(words.size() * fraction);
        std::vector<std::string> subset(words.begin(), words.begin() + current_size);

        for (bool is_bigram : {true, false}) {
            std::cout << "Running tests for " << (is_bigram ? "bigrams" : "trigrams") << "...\n";

            // Sequential test: Perform only 3 trials
            double baseline_time = 0.0;
            std::vector<double> seq_times;

            for (int i = 0; i < 3; ++i) {
                double time = run_trial(subset, false, is_bigram, 1);
                seq_times.push_back(time);
            }

            baseline_time = std::accumulate(seq_times.begin(), seq_times.end(), 0.0) / seq_times.size();

            result_file << "N-gram: " << (is_bigram ? "Bigrams" : "Trigrams") << "\n";
            result_file << "Parallelization: No\n";
            result_file << "Threads: 1\n";
            result_file << "Text size: " << current_size << " words (" << fraction * 100 << "% of full text)\n";
            result_file << "Execution times (3 trials): ";
            for (const auto& t : seq_times) {
                result_file << t << " ";
            }
            result_file << "\nAverage time: " << baseline_time << " seconds\n";
            result_file << std::string(80, '-') << "\n";

            // Parallelized tests
            for (int num_threads : thread_counts) {
                std::cout << "Parallel test with " << num_threads << " threads...\n";
                std::vector<double> times;

                for (int i = 0; i < 3; ++i) {
                    double time = run_trial(subset, true, is_bigram, num_threads);
                    times.push_back(time);
                }

                double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
                double speedup = baseline_time / avg_time;
                double efficiency = speedup / num_threads;

                result_file << "N-gram: " << (is_bigram ? "Bigrams" : "Trigrams") << "\n";
                result_file << "Parallelization: Yes\n";
                result_file << "Threads: " << num_threads << "\n";
                result_file << "Text size: " << current_size << " words (" << fraction * 100 << "% of full text)\n";
                result_file << "Execution times (3 trials): ";
                for (const auto& t : times) {
                    result_file << t << " ";
                }
                result_file << "\nAverage time: " << avg_time << " seconds\n";
                result_file << "Speedup: " << speedup << "\n";
                result_file << "Efficiency: " << efficiency << "\n";
                result_file << std::string(80, '-') << "\n";
            }
        }
    }

    result_file.close();
    std::cout << "Results saved in 'timing_results.txt'.\n";
    return 0;
}

