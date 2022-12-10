#include <iostream>
using namespace std;
int sum = 0;
int main(int argc, char **argv)
{
	// int sum = 0;
	int i = 0;
	for(i = 0; i < 101; i++)
	{
		sum += i;
	}
	cout << "Sum = " << sum << endl;
	return 0;
}
