#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void Usage(std::string name)
{
    std::cout << "Usage: " << name << " [-stride N] [-LEDs N] [-bits N]" << std::endl;
    std::cout << "       -stride: How many fields to shift between LEDs (default 1)" << std::endl;
    std::cout << "       -LEDs: How many LEDs are we encoding (default 48)" << std::endl;
    std::cout << "       -bits: How many bitss to use for encoding (default 6)" << std::endl;
    exit(-1);
}


// Compute parity of unsigned n. It returns 1
// if n has odd parity, and returns 0 if n has even
// parity.
bool hasOddParity(unsigned int n)
{
    bool parity = 0;
    while (n) {
        parity = !parity;
        n = n & (n - 1);
    }
    return parity;
}

// Fill a vector of fields with the encoding of an LED's pattern.  The
// list consists of high-power (1) and low-power (0) fields encoded
// as described in the "OSVR: HDK LED Patterns" document.
// If there are not enough bits to encode the pattern, returns an
// empty vector.
std::vector<int> encodePattern(unsigned int ID, size_t bits)
{
    std::vector<int> ret;

    // If we don't have enough bits to encode, return empty.
    if (ID >= (static_cast<unsigned int>(1) << bits)) { return ret; }

    // Encode the start-of-frame marker
    ret.push_back(1);
    ret.push_back(1);

    // Encode the parity bit.
    ret.push_back(0);
    if (hasOddParity(ID)) {
        ret.push_back(1);
    }
    else {
        ret.push_back(0);
    }

    // Encode the data bits, MSB first.
    for (unsigned int myBit = (1 << (bits - 1) ); myBit > 0; myBit >>= 1) {
        ret.push_back(0);
        if ((ID & myBit) != 0) {
            ret.push_back(1);
        }
        else {
            ret.push_back(0);
        }
    }

    // Encode the stop bit
    ret.push_back(0);
    ret.push_back(0);

    return ret;
}

// Print out an encoding table in human-readible format.
void printTable(std::vector< std::vector<int> > table)
{
    for (size_t row = 0; row < table.size(); row++) {
        std::cout.width(3);
        std::cout.fill(' ');
        std::cout << row << ": ";
        std::cout.width(0);
        for (size_t col = 0; col < table[row].size(); col++) {
            if (table[row][col] != 0) {
                std::cout << "*";
            }
            else {
                std::cout << ".";
            }
        }
        std::cout << std::endl;
    }
}

// Compute a vector that is a histogram of column sums
// for a table.
std::vector<int> columnSums(std::vector< std::vector<int> > table)
{
    std::vector<int> ret;
    size_t rowLength = table[0].size();
    for (size_t col = 0; col < rowLength; col++) {
        unsigned brightCount = 0;
        for (size_t row = 0; row < table.size(); row++) {
            if (table[row][col] != 0) {
                brightCount++;
            }
        }
        ret.push_back(brightCount);
    }
    return ret;
}

// Print a vector of column sums
void printColumnSums(std::vector<int> sums)
{
    for (size_t i = 0; i < sums.size(); i++) {
        std::cout.fill(' ');
        std::cout.width(3);
        std::cout << sums[i];
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    // Parse the command line to replace default parameters.
    unsigned int stride = 1;
    unsigned int LEDs = 48;
    unsigned int bits = 6;
    unsigned int realParams = 0;
    for (size_t i = 1; i < argc; i++) {
        if (std::string("-LEDs") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            LEDs = atoi(argv[i]);
        }
        else if (std::string("-stride") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            stride = atoi(argv[i]);
        }
        else if (std::string("-bits") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            bits = atoi(argv[i]);
        }
        else if (argv[i][0] == '-') {
            Usage(argv[0]);
        }
        else switch (++realParams) {
        case 1:
        default:
            Usage(argv[0]);
        }
    }
    if (realParams != 0) {
        Usage(argv[0]);
    }

    // Check that things will work.
    if (LEDs >= (static_cast<unsigned>(1) << bits)) {
        std::cerr << "Not enough bits to encode all of the LEDs" << std::endl;
    }

    // Fill a table with the encodings, not shifted.
    std::vector< std::vector<int> > encodingTable;
    for (unsigned i = 0; i < LEDs; i++) {
        encodingTable.push_back(encodePattern(i, bits));
    }

    // Print the unshifted table.
    std::cout << "Unshifted table: " << std::endl;
    printTable(encodingTable);

    // Compute and print the counts of high LEDs in each column.
    std::vector<int> sums = columnSums(encodingTable);
    std::cout << "Histogram of high LEDs per time step:" << std::endl;
    printColumnSums(sums);

    // Compute and print the maximum instantaneous brightness.
    std::cout << std::endl << "Maximum brightness: "
        << *std::max_element(sums.begin(), sums.end()) << std::endl;

    // Shift the encodings based on the requested stride between elements.
    size_t rowLength = encodingTable[0].size();
    for (size_t i = 0; i < LEDs; i++) {
        size_t newFirst = (i * stride) % rowLength;
        newFirst = rowLength - newFirst;
        std::rotate(encodingTable[i].begin()
            , encodingTable[i].begin() + newFirst
            , encodingTable[i].end());
    }

    // Print the shifted table.
    std::cout << "Shifted table: " << std::endl;
    printTable(encodingTable);

    // Compute and print the counts of high LEDs in each column.
    sums = columnSums(encodingTable);
    std::cout << "Histogram of high LEDs per time step:" << std::endl;
    printColumnSums(sums);

    // Compute and print the maximum instantaneous brightness.
    std::cout << std::endl << "Maximum brightness: "
        << *std::max_element(sums.begin(), sums.end())
        << std::endl << std::endl;
}