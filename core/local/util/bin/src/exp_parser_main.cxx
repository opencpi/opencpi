

#include <CpiExpParser.h>

using namespace CPI;


/*
class De : public DefineExpVarInterface {
  virtual double defineVariable( const char* var )
  {
    printf("In   virtual double defineVariable( const char* var ) \n");
  }
  virtual double defineArrayElement( const char* var, int index )
  {
    printf("virtual double defineArrayElement( const char* var, int index )\n");
  }
  virtual ~DefineExpVarInterface(){};
};
*/







int main() 
{

  run_exp_tests();

  /*
  ParsedExpression* pe;
  Parser p( new De );
  pe = p.parseExpression( "10*7" );
  printf("Result = %f\n", pe->evaluate() );
  */

}
