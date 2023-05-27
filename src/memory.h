#ifndef MEMORY_H
#define MEMORY_H


class memory {
public:
    memory() {};
    ~memory() {};
    const unsigned char read(const unsigned short& addr);
    void write(const unsigned short& addr, const unsigned char& data);
private:
    unsigned char mem[0xFFFF]{};
};


#endif //MEMORY_H
