#include <iostream>
#include <queue>

// Aux class that implements the priority-queue's comparing function
class myComparison
{
public:
  myComparison() {};
  bool operator() (const int& lhs, const int& rhs) const
  {
    if (lhs > rhs) // deadline comparison
      return true;
    else
      return false;
  }
};


int main()
{
  int numbers[4] = {2, 4, 5, 1};
  std::priority_queue< int,
                       std::vector<int>,
                       myComparison > PQ;

  for (int i = 0; i < 4; i++)
  {
    PQ.push(numbers[i]);
  }

  int out;
  for (int i = 0; i < 4; i++)
  {
    out = PQ.top();
    PQ.pop();
    std::cout << out << " ";
  }
  std::cout << "\n";
  
  return 0;
}
