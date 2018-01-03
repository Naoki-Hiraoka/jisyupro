#include<iostream>
#include<thread>
#include<chrono>
#include<ratio>
#include<future>
#include<vector>

using namespace std;

int task(int x,int* stop){
  for(int i=x;i>0;i--){
    if(*stop)return -1;
    this_thread::sleep_for(chrono::seconds{1});
    cout<<i<<endl;
  }
  return x;
}

int main(void){
  int ans;
  int temp=0;
  int stop[10];
  while(true){
    stop[temp]=0;
    auto fu=async(launch::async,task,5-temp,&stop[temp]);
    auto fs=fu.wait_for(chrono::seconds{2});
    if(fs==future_status::timeout){
      cout<<"timeout"<<endl;
      stop[temp]=1;
      temp++;
      continue;
    }
    ans=fu.get();
    break;
  }
  cout<<ans<<endl;
  return 0;
}
