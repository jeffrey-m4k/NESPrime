#ifndef LOADER_H
#define LOADER_H

#include <fstream>
#include <ios>
#include <string>

class loader {
public:
    loader() {};
    ~loader() {};
    void load();
    bool open_file(const std::string& filename);
    std::ifstream& get_file() { return file; }
private:
    bool read_next(const int& bytes = 1);
    bool read_next(char* into, const int& bytes = 1);
    bool load_header();
    void print_metadata();
private:
    static const int BUFFER_SIZE = 512;
    std::ifstream file;
    unsigned char buffer[BUFFER_SIZE];
    long pos = 0;
    unsigned int prg_size, chr_size;
    unsigned char mapper;
    bool flags[2][4];
};

#endif //LOADER_H
