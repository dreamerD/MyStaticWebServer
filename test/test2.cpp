#include <iostream>
using namespace std;
int main() {
  std::string s("\n\0");
  cout << s.size();
}