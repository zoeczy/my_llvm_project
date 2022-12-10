 // clang -c -S -emit-llvm box.cpp  -o box.ll -Ibox.h 
#include <iostream>
using namespace std;

class Box
{
   public:
      double length;   // 长度
      double breadth;  // 宽度
      double height;   // 高度
      // 成员函数声明
      double get(void);
      void set( double len, double bre, double hei );
};