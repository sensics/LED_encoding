#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void Usage(std::string name)
{
    std::cout << "Usage: " << name << " [-simple_encoding] [-parity N] [-stride N] [-stride_optimize N] [-LEDs N] [-bits N] [-csv]" << std::endl;
    std::cout << "       -simple_encoding: Use simple encoding (default not)" << std::endl;
    std::cout << "       -parity: For non-simple encoding, use even (2), odd (1) or no (0) parity (default 2)" << std::endl;
    std::cout << "       -stride: How many fields to shift between LEDs (default is to optimize)" << std::endl;
    std::cout << "       -stride_optimize: How many iterations to try optimizing strides (default is 20)" << std::endl;
    std::cout << "       -LEDs: How many LEDs are we encoding (default 40)" << std::endl;
    std::cout << "       -bits: How many bitss to use for encoding (default 10)" << std::endl;
    std::cout << "       -csv: Also print the table as comma-separated-values" << std::endl;
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

// Returns true if the test vector is not the same as any of the
// vectors already in the table for any rotation of the test vector.
bool isRotationallyInvariant(std::vector<int> testVec, const std::vector< std::vector<int> > &table)
{
    // If any of the rotations match, return false.
    for (size_t r = 0; r < testVec.size(); r++) {
        for (size_t t = 0; t < table.size(); t++) {
            if (testVec == table[t]) { return false; }
        }
        std::rotate(testVec.begin()
            , testVec.begin() + 1
            , testVec.end());
    }
    return true;
}

// Recursively construct all of the rotationally-invariant b-bit patterns with
// "ones" of the bits being 1.  It is called with the number of 1 bits remaining
// to be filled in, the first location where it is legal to put a 1 bit, and
// a vector of the encoding size filled with 0's.
void recursiveRotationallyInvariant(
    std::vector< std::vector<int> > &table  // The table we're filling in with results
    , std::vector<int> testVec              // The vector we're building to test
    , size_t ones                           // How many 1's we have left to add
    , size_t first                          // The first location we can put a 1
    )
{
    // How many bits are we using to encode?
    size_t bits = testVec.size();

    // Have we filled in all of our 1's?  If so, check our guess against the
    // current ones to see if it is rotationally symmetric with any of them.
    // If not, add it to the list.  In any case, return because we're done.
    if (ones == 0) {
        if (isRotationallyInvariant(testVec, table)) {
            table.push_back(testVec);
        }
        return;
    }

    // Try adding a 1 in all of the possible locations (leaving room for the
    // remainder of the bits) and recursing with one fewer bit.  Then remove
    // the 1 from where we put it.
    for (size_t i = first; i + ones <= bits; i++) {
        testVec[i] = 1;
        recursiveRotationallyInvariant(table, testVec, ones - 1, i + 1);
        testVec[i] = 0;
    }
}

// Construct all of the rotationally-invariant b-bit patterns with
// "ones" of the bits being 1.
std::vector< std::vector<int> > constructRotationallyInvariant(size_t ones, size_t bits)
{
    // Construct an empty table to pass to the recursive function.
    std::vector< std::vector<int> > ret;

    // Construct a vector of all 0's to send to the recursive function.
    std::vector<int> zeroVector;
    for (size_t i = 0; i < bits; i++) {
        zeroVector.push_back(0);
    }
    recursiveRotationallyInvariant(ret, zeroVector, ones, 0);
    return ret;
}

// Find an optimal encoding for 'LEDs' count of LEDs in 'bits' bits.
// It starts with the smallest number of "1" bits and includes all
// encodings with that number of bits that are not rotationally
// symmetric with each other, then moves up to a larger number of
// "1" bits until it has found enough values to encode the requested
// number of LEDs.
//   The parity can be specified as 0 (none), 1 (odd), or 2 (even).
// If specified, only patterns with the designated parity will be
// included.
//   Returns an empty vector if it cannot find enough encodings
// matching the specified constraints.
std::vector< std::vector<int> > greedyOptimalEncode(size_t LEDs, size_t bits, unsigned parity)
{
    std::vector< std::vector<int> > ret;
    if (LEDs == 0) { return ret; }

    for (size_t b = 1; b <= bits; b++) {
        // Make sure our parity matches that specified
        if ( ((parity == 1) && (b % 2 != 1)) ||
             ((parity == 2) && (b % 2 != 0)) ) {
            continue;
        }

        // Construct all of the b-bit patterns that are not rotationally
        // symmetric with one another.  Add them to the list.  If we
        // fill up all the ones we need, return.
        std::vector< std::vector<int> > bBitPatterns =
            constructRotationallyInvariant(b, bits);
        for (size_t i = 0; i < bBitPatterns.size(); i++) {
            ret.push_back(bBitPatterns[i]);
            if (ret.size() == LEDs) { return ret; }
        }
    }

    // We cannot succeed because we don't have enough bits, so return an empty
    // result.
    ret.clear();
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

// Print out an encoding table as comma-separated values.
void printTableCSV(std::vector< std::vector<int> > table)
{
    for (size_t row = 0; row < table.size(); row++) {
        for (size_t col = 0; col < table[row].size(); col++) {
            if (table[row][col] != 0) {
                std::cout << "1,";
            }
            else {
                std::cout << "0,";
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
// It also attempts to maximize the minimum maximum count over
// all columns, to try and level the number of 1's per column.
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
        // maximum overlap count and, within that, the largest
        // minimum overlap count.
        std::vector<int> overlaps = columnSums(table);
        int minMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
        int maxMinOverlap = *std::min_element(overlaps.begin(), overlaps.end());
        size_t minRotation = 0;
        for (size_t j = 1; j < rowLength; j++) {
            std::rotate(table[i].begin()
                , table[i].begin() + 1
                , table[i].end());

            overlaps = columnSums(table);
            int thisMaxOverlap = *std::max_element(overlaps.begin(), overlaps.end());
            int thisMinOverlap = *std::min_element(overlaps.begin(), overlaps.end());
            if (thisMaxOverlap < minMaxOverlap) {
                minMaxOverlap = thisMaxOverlap;
                maxMinOverlap = thisMinOverlap;
                minRotation = j;
            }
            if ((thisMaxOverlap == minMaxOverlap) && (thisMinOverlap >= maxMinOverlap)) {
                minMaxOverlap = thisMaxOverlap;
                maxMinOverlap = thisMinOverlap;
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
    unsigned int bits = 10;
    bool simple_encoding = false;
    unsigned parity = 2;    // Even parity by default
    bool print_CSV = false;
    unsigned int realParams = 0;
    for (size_t i = 1; i < argc; i++) {
        if (std::string("-LEDs") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            LEDs = atoi(argv[i]);
        }
        else if (std::string("-simple_encoding") == argv[i]) {
            simple_encoding = true;
        }
        else if (std::string("-csv") == argv[i]) {
            print_CSV = true;
        }
        else if (std::string("-parity") == argv[i]) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            parity = atoi(argv[i]);
            if (parity > 2) { Usage(argv[0]); }
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
    if (simple_encoding) {
        for (unsigned i = 0; i < LEDs; i++) {
            encodingTable.push_back(encodePattern(i, bits));
        }
    }
    else {
        encodingTable = greedyOptimalEncode(LEDs, bits, parity);
    }

    // Make sure our construction worked.
    if (encodingTable.size() == 0) {
        std::cerr << "Could not construct table with " << bits << " bits for " << LEDs << " LEDs." << std::endl;
        return -3;
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

    // Reverse the order of the elements to put the ones with the most
    // bits first.  This will mean that we pack the hardest ones first
    // and have a better chance of "filling in" loose slots later,
    // producing a more compact packing.
    std::reverse(encodingTable.begin(), encodingTable.end());

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

    // Count up all of the 1's and compute how many (at minimum)
    // must be lined up in a single column given the number of bits,
    // irrespective of the rotationally-invariant coding or packing
    // rotation chosen.
    int numOnes = 0;
    for (size_t i = 0; i < sums.size(); i++) {
        numOnes += sums[i];
    }
    int minOnes = numOnes / bits;
    if (numOnes % bits != 0) {
        minOnes++;
    }
    std::cout << "Theoretical minimum for packing this many 1's: " << minOnes << std::endl;

    // Print the CSV table if asked.
    if (print_CSV) {
        std::cout << "Shifted table: " << std::endl;
        printTableCSV(encodingTable);
    }
}