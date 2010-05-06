/*
 * Abstact:
 *   These are the class definitions for the CPI expression parser
 *
 * Author: John F. Miller
 *
 * Date: 2/5/02
 *
 * Current Limitations:
 *    
 *     1) Trig functions do not support arguments of type variable or array element.
 *     3) This has not been tested for leaks.
 *
 */

#ifndef CPI_EXPRESSION_PARSER_H
#define CPI_EXPRESSION_PARSER_H

#include<string>
#include<list>
#include<vector>
#include <math.h>

namespace CPI {


// This is the interface class that the user must implement if they are
// using expressions that contain variables that need to be defined.
class  DefineExpVarInterface {

public:
  virtual double defineVariable( const char* var )=0;
  virtual double defineArrayElement( const char* var, int index )=0;
  virtual ~DefineExpVarInterface(){};
};



// This class is returned to the user after an expression has been parsed.  This
// instance can be reused to avoid regenerating the parser tree.  It is the callers
// responsibility to delete this instance after it is done with it.
class Parser;
class Operation;
class Tokenizer;
class ParsedExpression {

        friend class Parser;

 private:
  DefineExpVarInterface* cb_;
  Operation* first_;
  Operation* last_;
  Tokenizer *tokenizer_;

 protected:
  ParsedExpression(DefineExpVarInterface* cb )
    :cb_(cb),first_(NULL),last_(NULL){};

  // Add an operation to the link list
  void addOperation( Operation* op );

public:

  // This is where the evaluation is done, this method causes the users
  // defineVariable method to be called for all variables in the expression.
  double evaluate();

  // For debug purposes only
  void printOrigList();
  void printWorkingList();

  // Destructor
  ~ParsedExpression();

};


// This is the main expression parser class
class Parser {

 private:
   DefineExpVarInterface* cb_;

 public:

   // Constructor
   Parser( DefineExpVarInterface* cb )
     :cb_(cb){}

  
   // Evaluate Expression, this routine creates the parse tree, but 
   // does not attempt to resolve any variables.
   ParsedExpression* parseExpression( const char* expression );

   // destructor
   ~Parser();

 };


};

void run_exp_tests();

#endif
