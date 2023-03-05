#include <iostream>
using namespace std;
int main() {
  string ans = "good";
  string& ref = ans;
  string ans_ref(move(ref));
  cout << ans << endl;
  cout << ans_ref << endl;
}