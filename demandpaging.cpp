#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <limits>

#include <sys/resource.h>

#include "vaddr_tracereader.h"
#include "pagetable.h"

int main(int argc, char *argv[])
{

  int f = 999999;
  int n = std::numeric_limits<int>::max();
  int a = 10;
  std::string l = "summary";

  std::vector<int> argVec;
  std::vector<int> levelBits;

  // reads in optional arguments
  // also pushes mandatory arguments to vector to be read later
  if (argc < 4)
  {
    std::cout << "not enough args" << std::endl;
    exit(1);
  }
  else
  {
    for (int i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-f") == 0)
      {
        i += 1;
        f = std::stoi(argv[i]);
      }
      else if (strcmp(argv[i], "-n") == 0)
      {
        i += 1;
        n = std::stoi(argv[i]);
      }
      else if (strcmp(argv[i], "-a") == 0)
      {
        i += 1;
        a = std::stoi(argv[i]);
      }
      else if (strcmp(argv[i], "-l") == 0)
      {
        i += 1;
        l = argv[i];
      }
      else
      {
        argVec.push_back(i);
      }
    }
  }

  // checks conditions for optional arguments
  if (n < 1)
  {
    std::cout << "Number of memory accesses must be a number, greater than 0" << std::endl;
    exit(1);
  }
  if (f < 1)
  {
    std::cout << "Number of available frames must be a number, greater than 0" << std::endl;
    exit(1);
  }
  if (a < 1)
  {
    std::cout << "Age of last access considered recent must be a number, greater than 0" << std::endl;
    exit(1);
  }

  int bitsSum = 0;
  int numBits;

  // checks conditions for the number of bits at each level
  for (int i = 2; i < argVec.size(); i++)
  {
    numBits = std::stoi(argv[argVec[i]]);
    // makes sure each level has atleast 1 bit
    if (numBits < 1)
    {
      std::cout << "Level " << i << " page table must be at least 1 bit" << std::endl;
      exit(1);
    }
    levelBits.push_back(numBits);
    bitsSum += numBits;
  }
  // makes sure the total number of bits for every level is not over 28
  if (bitsSum > 28)
  {
    std::cout << "Too many bits used in page tables" << std::endl;
    exit(1);
  }

  // reads in mandatory arguments
  int trIndex = 0;
  int txtIndex = 1;

  FILE *tracef_h;

  // Open the file for reading
  tracef_h = fopen(argv[argVec[trIndex]], "r");
  if (tracef_h == NULL)
  {
    std::cout << "Unable to open <<" << argv[argVec[trIndex]] << ">>" << std::endl;
  }

  std::ifstream readWrite(argv[argVec[txtIndex]]);

  // creating objects for page table initialization
  int numLevels = levelBits.size();
  std::vector<int> shiftAry;

  // calculates the shift array
  int sumLevels;
  for (int i = 0; i < numLevels; i++)
  {
    sumLevels = 0;
    for (int j = 0; j <= i; j++)
    {
      sumLevels += levelBits[j];
    }
    // magic number
    shiftAry.push_back(32 - sumLevels);
  }

  // initializes bitMask to 32 bits size
  std::vector<uint32_t> bitMask;
  // sets mask to 32 0's
  uint32_t mask = 0x00000000;
  uint32_t tempMask;
  int numOnes;

  // Initialize bit masks for each level
  for (int i = 0; i < numLevels; i++)
  {
    // calculates number that is levelbits[i] 1's in a row
    numOnes = pow(2, levelBits[i]) - 1;
    // Use bitwise OR to set specific bits to 1 based on the shift values
    tempMask = mask | (numOnes << shiftAry[i]);
    bitMask.push_back(tempMask);
  }
  if (l == "bitmasks")
  {
    for (int i = 0; i < bitMask.size(); i++)
    {
      printf("level %d mask %08X\n", i, bitMask[i]);
    }
    return 0;
  }

  // Calculates the number of of bits at each level by shifting 1 the appropriate amount
  for (int i = 0; i < levelBits.size(); i++)
  {
    levelBits[i] = (1 << levelBits[i]);
  }

  // initializes page table
  PageTable myTable = PageTable(numLevels, bitMask, shiftAry, levelBits, f);

  // reads in addresses to vector, put in pagetable.cpp later possibly
  char digit;
  int binaryVal;

  int addProcessed = 0;

  p2AddrTr mtrace;
  uint32_t vAddr;

  // inserts vpns into page table
  for (int i = 0; i < n; i++)
  {
    // gets the next address if there are more addresse
    if (NextAddress(tracef_h, &mtrace))
    {
      addProcessed += 1;
      // gets the next read or write binary value
      if (readWrite.get(digit))
      {
        binaryVal = digit - '0';
      }

      vAddr = mtrace.addr;
      myTable.insertVpn2PfnMapping(vAddr, binaryVal, a, l);
    }
    else
    {
      break;
    }
  }

  if (l == "summary")
  {
    struct rusage usage;

    int pageSize = pow(2, 32 - bitsSum);
    printf("Page size: %d bytes\n", pageSize);
    int numMisses = addProcessed - myTable.pageHits;
    double hit_percent = (double)(myTable.pageHits) / (double)addProcessed * 100.0;
    printf("Addresses processed: %d\n", addProcessed);
    printf("Page hits: %d, Misses: %d, Page Replacements: %d\n", myTable.pageHits, numMisses, myTable.pageReplaces);
    printf("Page hit percentage: %.2f%%, miss percentage: %.2f%%\n", hit_percent, 100 - hit_percent);
    printf("Frames allocated: %d\n", myTable.framesAllocated);
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
      long memoryUsage = usage.ru_maxrss;
      printf("Bytes used:  %ld\n", memoryUsage);
    }

    fflush(stdout);
  }
  return 0;
}
