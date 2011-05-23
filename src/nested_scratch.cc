#include <iostream>
#include <list>
#include <vector>

using namespace std;

vector<int> outer;
vector<int> inner;


vector<int>::iterator lhsIt = outer.begin();
vector<int>::iterator rhsIt = inner.begin();
vector<int> blocks;
vector<int>::iterator blockIt = blocks.begin();

const int pagesize = 5;
const int npages = 3;

int nrecs = 0;

// bool GetNext() {
//   for(;lhsIt != outer.end(); ) {
//     for(; rhsIt != inner.end(); ) {
//       nrecs++;
//       cout << "[" << *lhsIt << ", " << *rhsIt << "]" << endl;
//       rhsIt++;
//       return true;
//     }
//     lhsIt++;
//     rhsIt = inner.begin();
//   }

//   return false;
// }

bool GetNext() {
  while(blockIt != blocks.end()) {
    while(rhsIt != inner.end()) {
      while((nrecs++ < pagesize)) {
        // nrecs++;
        cout << "[" << *lhsIt << ", " << *rhsIt << "]" << endl;
        lhsIt++;
        return true;
      }
      nrecs = 0;
      rhsIt++;
      // advance to right block start
      lhsIt = outer.begin();
      for (int i = 0; i < *blockIt; i++)
        lhsIt++;

    }
    blockIt++;
    cout << "block " << *blockIt << endl;

    // advance to right block start
    lhsIt = outer.begin();
    for (int i = 0; i < *blockIt; i++)
      lhsIt++;

    rhsIt = inner.begin();
  }
  // eof
  return false;
} 


int main(int argc, char** argv) {
  

  for(int i = 1; i <= 10; i++)
    outer.push_back(i);

  for(unsigned int j = 0; j < outer.size()/pagesize; j++) {
    blocks.push_back(j * pagesize);
    cerr << j << " block - start tuple" <<  blocks[j] << endl;
  }

  for(int i = 1; i <= 20; i++)
    inner.push_back(i);

  blockIt = blocks.begin();
  lhsIt = outer.begin();
  rhsIt = inner.begin();

  int count = 0;
  while(GetNext()) {
    count++;
  }
  cerr << count << " calls to GetNext." << endl;

  // int nlrecs = 0;
  // int nrrecs = 0;
  // while(1) {
  //   int right = *rhsIt;
  //   rhsIt++;
  //   nrrecs++;
  //   int left = *lhsIt;


  //   if((nrrecs-1) % pagesize == 0 && lhsIt != outer.end()) {
  //     cerr << "----" << endl;
  //     if((nlrecs-1) % pagesize != 0)
  //       rhsIt = inner.begin();
  //     lhsIt++;
  //     nlrecs++;
  //   }


  //   if(rhsIt == inner.end()) {
  //     rhsIt = inner.begin();
  //     lhsIt++;
  //     nlrecs++;
  //   }

  //   cout << "[" << left << ", " << right << "]" 
  //        << "nl " << nlrecs << " nr " << nrrecs
  //        << endl;
    
  //   if(nlrecs+nrrecs > outer.size()*inner.size())
  //     break;

  //   if(lhsIt == outer.end()) {
  //     if(rhsIt == inner.end())
  //       break;
  //     else
  //       lhsIt = outer.begin();
  //   }

  // }

  return 0;
}
