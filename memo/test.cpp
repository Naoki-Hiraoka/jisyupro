#include<iostream>
#include<bitset>

using namespace std;


int main(void){
  bitset<4> test1{"1010"};
  bitset<4> test2{"1100"};
  cout<<(test1|test2)<<endl;
  return 0;
}
