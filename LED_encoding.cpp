#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void Usage(std::string name)
{
    std::cout << "Usage: " << name << " [-simple_encoding] [-stride N] [-stride_optimize N] [-LEDs N] [-bits N]" << std::endl;
    std::cout << "       -simple_encoding: Use simple encoding (default not)" << std::endl;
    std::cout << "       -stride: How many fields to shift between LEDs (default is to optimize)" << std::endl;
    std::cout << "       -stride_optimize: How many iterations to try optimizing strides (default is 20)" << std::endl;
    std::cout << "       -LEDs: How many LEDs are we encoding (default 40)" << std::endl;
    std::cout << "       -bits: How many bitss to use for encoding (default 8)" << std::endl;
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
// for a table.  Optionally, specify the number of rows.
// If the number of rows is specified as 0, all rows in
// the table are used.
std::vector<int> columnSums(std::vector< std::vector<int> > table, size_t nRows = 0)
{
    std::vector<int> ret;
    size_t rowLength = table[0].size();
    if (nRows == 0) { nRows = table.size(); }
    for (size_t col = 0; col < rowLength; col++) {
        unsigned brightCount = 0;
        for (size_t row = 0; row < nRows; row++) {
            if (table[row][col] != 0) {
                brightCount++;
            }
        }
        ret.push_back(brightCount);
    }
    return ret;
}

void applyFixedStride(int stride, std::vector< std::vector<int> > &table)
{
    if (table.size() == 0) { return; }

    size_t rowLength = table[0].size();
    for (size_t i = 1; i < table.size(); i++) {
        size_t newFirst = (i * stride) % rowLength;
        newFirst = rowLength - newFirst;
        std::rotate(table[i].begin()
            , table[i].begin() + newFirst
            , table[i].end());
    }
}

// Attempt to rotate the second and following rows such that
// the maximum number of overlapping bright LEDs in a single
// column for that row plus all the ones above it is minimized.
// Repeating this function will select different solutions;
// it picks the maximum equivalent rotation each time.
void greedyOptimumStride(std::vector< std::vector<int> > &table)
{
    if (table.size() == 0) { return; }

    // Leave the first row un-rotated.
    // For the following rows, pick the least-rotated choice
    // with the minimal overlap with previous rows.
    size_t rowLength = table[0].size();
    for (size_t i = 1; i < table.size(); i++) {
        // Record the initial overlap and its index, then
        // try all of the other ones to see if any are an improvement.
        // Keep track of the maximum rotation that has the lowest
        // overlap count.
        std::vector<int> overlaps = columnSums(table, i+1);
        int minMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
        size_t minRotation = 0;
        for (size_t j = 1; j < rowLength; j++) {
            std::rotate(table[i].begin()
                , table[i].begin() + 1
                , table[i].end());

            overlaps = columnSums(table, i+1);
            int thisMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
            if (thisMaxOverlap <= minMaxOverlap) {
                minMaxOverlap = thisMaxOverlap;
                minRotation = j;
            }
        }

        // Rotate back to the original position.
        std::rotate(table[i].begin()
            , table[i].begin() + 1
            , table[i].end());

        // Rotate by the best amount to reach minimum.
        if (minRotation != 0) {
            std::rotate(table[i].begin()
                , table[i].begin() + minRotation
                , table[i].end());
        }
    }
}

// Attempt to rotate all rows such that the maximum number of
// overlapping bright LEDs in a single column is minimized.
// Repeating this function will select different solutions;
// it picks the maximum equivalent rotation for each row
// each time it is run.
void greedyReduceOverlaps(std::vector< std::vector<int> > &table)
{
    if (table.size() == 0) { return; }

    size_t rowLength = table[0].size();
    for (size_t i = 0; i < table.size(); i++) {
        // Record the initial overlap and its index, then
        // try all of the other ones to see if any are an improvement.
        // Keep track of the maximum rotation that has the lowest
        // overlap count.
        std::vector<int> overlaps = columnSums(table);
        int minMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
        size_t minRotation = 0;
        for (size_t j = 1; j < rowLength; j++) {
            std::rotate(table[i].begin()
                , table[i].begin() + 1
                , table[i].end());

            overlaps = columnSums(table);
            int thisMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
            if (thisMaxOverlap <= minMaxOverlap) {
                minMaxOverlap = thisMaxOverlap;
                minRotation = j;
            }
        }

        // Rotate back to the original position.
        std::rotate(table[i].begin()
            , table[i].begin() + 1
            , table[i].end());

        // Rotate by the best amount to reach minimum.
        if (minRotation != 0) {
            std::rotate(table[i].begin()
                , table[i].begin() + minRotation
                , table[i].end());
        }
    }
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
    int stride = -1;
    unsigned stride_optimizations = 20;
    unsigned int LEDs = 40;
    unsigned int bits = 8;
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
        else if (std::string("-stride_optimize") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            stride_optimizations = atoi(argv[i]);
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
    // If the stride is negative, we do a greedy optimization, optionally
    // followed by repeated attempts to improve it.
    // If the stride is >=0, we use it consistently across the board.
    if (stride >= 0) {
        applyFixedStride(stride, encodingTable);
    }
    else {
        greedyOptimumStride(encodingTable);
    }

    // Try to find better strides by shifting each row by the maximum
    // stride that doesn't make things worse.
    for (size_t i = 0; i < stride_optimizations; i++) {
        greedyReduceOverlaps(encodingTable);
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