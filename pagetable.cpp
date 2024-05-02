#include <bitset>
#include <cstdint>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string>
#include <cmath>

#include "pagetable.h"

// from a given address returns a vector of the bits for each level of the vpn
std::vector<unsigned int> PageTable::getVPNFromVirtualAddress(unsigned int address)
{
  std::vector<unsigned int> addressLevels;
  unsigned int bitmask;
  unsigned int extractedBits;

  // for each level apply the bit mask shift accordingly and push to vector
  for (int i = 0; i < bitmaskAry.size(); i++)
  {
    bitmask = bitmaskAry[i];
    extractedBits = address & bitmask;
    extractedBits = extractedBits >> shiftAry[i];
    addressLevels.push_back(extractedBits);
  }
  return addressLevels;
}

// inserts page into page table if not already present and runs page replacement if necessary
void PageTable::insertVpn2PfnMapping(unsigned int address, int readWriteVal, int age, std::string logType)
{
  std::vector<unsigned int> leveledAddresses = getVPNFromVirtualAddress(address);
  int offsetBits = shiftAry[shiftAry.size() - 1];
  std::vector<uint32_t> bitMask;
  // sets mask to 32 0's
  uint32_t mask = 0x00000000;
  uint32_t numOnes = pow(2, offsetBits) - 1;
  uint32_t offset = address & numOnes;
  uint32_t vpn = address >> offsetBits;

  // adds Level object to nextLevelPtr for locations of masked numbers
  Level *currLevel = rootNodePtr;

  unsigned int addressPart;
  unsigned int pfn;

  unsigned int vpnreplaced = -1;
  bool pthit = false;
  unsigned int currVpn;
  unsigned int currPfn;

  // for offset
  if (logType == "offset")
  {
    fprintf(stdout, "%08X\n", (offset));
  }

  // navigates to final level adding levels on the way
  for (int i = 0; i < currLevel->pageTablePtr->levelCount - 1; i++)
  {

    addressPart = leveledAddresses[i];

    // if their is no next level where it needs to be, it creates a new level there
    if (!currLevel->nextLevelPtr[addressPart])
    {
      currLevel->nextLevelPtr[addressPart] = new Level(currLevel->pageTablePtr, currLevel->depth + 1);
      storageCount += sizeof(currLevel->nextLevelPtr[addressPart]);
    }
    // navigates to the next level
    currLevel = currLevel->nextLevelPtr[addressPart];
  }
  addressPart = leveledAddresses[leveledAddresses.size() - 1];

  // finds if their is a valid mapping for vpn
  Map *found = findVpn2PfnMapping(addressPart, currLevel);

  if (found == nullptr)
  {

    // runs page replacement if clock is full
    if (clockSize == numFrames)
    {
      while (true)
      {
        //  checks for time thresh hold
        if (time - currTick->mapping->startTime > age)
        {
          // if mapping dirty sets to false

          if (currTick->mapping->dirty)
          {
            currTick->mapping->dirty = false;
          }
          // if mapping clean sets current mapping at tick to invalid and points currTick mapping to new mapping
          else
          {
            // stores the vpn getting replaced and incriments page replacements
            vpnreplaced = currTick->mapping->vpn;
            pageReplaces += 1;

            // invalidates old mapping and creates new mapping
            currTick->mapping->valid = false;
            currLevel->mapPtr[addressPart] = new Map(vpn, currTick->loc, readWriteVal, time);
            currTick->mapping = currLevel->mapPtr[addressPart];

            // logging
            if (logType == "va2pa")
            {
              fprintf(stdout, "%08X -> %08X\n", address, (currTick->mapping->pfn << offsetBits) + offset);
              fflush(stdout);
            }

            if (logType == "vpns_pfn")
            {
              for (int i = 0; i < leveledAddresses.size(); i++)
              {
                fprintf(stdout, "%X ", leveledAddresses[i]);
              }
              fprintf(stdout, "-> %d\n", currTick->mapping->pfn);
              fflush(stdout);
            }
            time += 1;
            currTick = currTick->next;
            break;
          }
        }
        currTick = currTick->next;
        time += 1;
      }
    }
    // if clock is not full, inserts new mapping to page table and creates new clock tick
    else
    {
      framesAllocated += 1;
      // if clock size is 0 creates tick with location 0 and sets it as the head
      if (clockSize == 0)
      {
        currTick = new Clock(0);
        storageCount += sizeof(currTick);

        head = currTick;
      }
      // if clock already has ticks in it, creates a new tick at the next pointer of the currTick and goes to that tick
      else
      {
        currTick->next = new Clock(currTick->loc + 1);
        storageCount += sizeof(currTick);
        currTick = currTick->next;
      }

      currLevel->mapPtr[addressPart] = new Map(vpn, currTick->loc, readWriteVal, time);
      storageCount += sizeof(currLevel->mapPtr[addressPart]);

      currTick->mapping = currLevel->mapPtr[addressPart];

      time += 1;
      clockSize += 1;
      // if clock is full, points the last tick of the clock to the head ad goes to the head
      if (clockSize == numFrames)
      {
        // for va2pa
        if (logType == "va2pa")
        {
          fprintf(stdout, "%08X -> %08X\n", address, currTick->mapping->pfn * offset);
          fflush(stdout);
        }

        currTick->next = head;
        currTick = currTick->next;
      }
    }
  }
  // if valid vpn mapping is already in table
  else
  {
    pthit = true;
    // updates start time of current ticks mapping and sets dirty if it is a write operation
    if (readWriteVal == 1)
    {
      found->dirty = true;
    }
    pageHits += 1;
  }
  // for va2pa
  if (logType == "va2pa")
  {
    if (vpnreplaced == -1)
    {
      fprintf(stdout, "%08X -> %08X\n", address, (currLevel->mapPtr[addressPart]->pfn << offsetBits) + offset);
    }
  }
  if (logType == "vpns_pfn")
  {
    if (vpnreplaced == -1)
    {
      for (int i = 0; i < leveledAddresses.size(); i++)
      {
        fprintf(stdout, "%X ", leveledAddresses[i]);
      }
      if (found)
      {
        fprintf(stdout, "-> %X\n", found->pfn);
      }
      else
      {
        fprintf(stdout, "-> %X\n", currTick->mapping->pfn);
      }

      fflush(stdout);
    }
  }

  // vpn2pfn_pr
  if (logType == "vpn2pfn_pr")
  {
    fprintf(stdout, "%08X -> %08X, ", address >> offsetBits, currLevel->mapPtr[addressPart]->pfn);

    fprintf(stdout, "pagetable %s", pthit ? "hit" : "miss");

    if (vpnreplaced != -1) // vpn was replaced due to page replacement
      fprintf(stdout, ", %08X page was replaced\n", vpnreplaced);
    else
    {
      fprintf(stdout, "\n");
    }

    fflush(stdout);
  }
}

// finds if current mapping for vpn is valid given the last level of pagetable for given address otherwise returns a nullptr
PageTable::Map *PageTable::findVpn2PfnMapping(unsigned int addressPart, Level *lastLevel)
{
  if (lastLevel->mapPtr[addressPart])
  {
    if (lastLevel->mapPtr[addressPart]->valid)
    {

      return lastLevel->mapPtr[addressPart];
    }
  }

  return nullptr;
}
