
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstact:
 *   These are the class definitions for the OCPI expression parser
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

#ifndef OCPI_EXPRESSION_PARSER_H
#define OCPI_EXPRESSION_PARSER_H

#include<string>
#include<list>
#include<vector>
#include <math.h>

namespace OCPI {


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
