#include <vector>
#include <string>

class PageTable
{
public:
  // map objects will contain the vpn and pfn of the mapping, whether or not they are dirty
  // if they are a valid mapping and the last time that mapping was referenced for calculating age
  struct Map
  {
    bool valid;
    bool dirty;
    int vpn;
    int pfn;
    int startTime;

    Map(int vpn, int pfn, int startTime, int dirty) : vpn(vpn), pfn(pfn), valid(true), dirty(dirty), startTime(startTime) {}
  };

  // clock objects will point to a mapping and the next clock object in the WSclock
  // and will have their location in the clock for getting the pfn
  struct Clock
  {
    Map *mapping;
    Clock *next;
    int loc;

    Clock(int loc) : mapping(nullptr), loc(loc), next(nullptr) {}
  };

  int numFrames;

  // level objects will have a pointer to the page table to access its data, the depth that they are at and a vector of nextLevel pointers
  // and map pointers, only one will be initialized and mapPtr will only be used on the last level
  struct Level
  {
    PageTable *pageTablePtr;
    int depth;
    std::vector<Level *> nextLevelPtr;
    std::vector<Map *> mapPtr;

    Level(PageTable *pageTable, int depth)
        : pageTablePtr(pageTable), depth(depth)
    {
      for (int i = 0; i < pageTable->entryCount[depth]; i++)
      {
        mapPtr.push_back(nullptr);
      }
      for (int i = 0; i < pageTable->entryCount[depth]; i++)
      {
        nextLevelPtr.push_back(nullptr);
      }
    }
  };

  // fields for page table
  int pageHits;
  int pageReplaces;
  int framesAllocated;

  int levelCount;
  Level *rootNodePtr;
  std::vector<uint32_t> bitmaskAry;
  std::vector<int> shiftAry;
  std::vector<int> entryCount;

  int clockSize;
  int time;
  int clockLoc;
  Clock *currTick;
  Clock *head;
  int storageCount;

  // page table will be initialized with values from constructor and given a root node
  PageTable(int levelCount, std::vector<uint32_t> bitmaskAry,
            std::vector<int> shiftAry,
            std::vector<int> entryCount, int numFrames) : levelCount(levelCount), bitmaskAry(bitmaskAry), shiftAry(shiftAry), entryCount(entryCount), numFrames(numFrames)
  {
    rootNodePtr = new Level(this, 0);
    time = 0;
    clockSize = 0;

    pageHits = 0;
    pageReplaces = 0;
    framesAllocated = 0;

    storageCount = 0;
  }

  void insertVpn2PfnMapping(unsigned int address, int readWriteVal, int age, std::string logType);

private:
  std::vector<unsigned int> getVPNFromVirtualAddress(unsigned int address);
  Map *findVpn2PfnMapping(unsigned int addressPart, Level *lastLevel);
};