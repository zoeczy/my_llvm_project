#include <iostream>
using namespace std;
int sum = 0;
int invariant=6;
int main(int argc, char **argv)
{
	// int sum = 0;
	int i = 0;
    int look=0;
	for(i = 0; i < 101; i++)
	{
		sum += i;
        
        for(int j=0;j<10;j++){
			invariant=5;
            look = invariant +j;
        }
	}
	cout << "Sum = " << sum << endl;
	return 0;
}