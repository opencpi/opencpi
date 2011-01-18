
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
 *   These are the class implementations for the OCPI expression parser
 *
 * Author: John F. Miller
 *
 * Date: 2/5/02
 *
 */


#include <iostream>
#include <stack>
#include "OcpiExpParser.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace std;

//#define NDEBUG
//#undef NDEBUG

/***********************************************************************
 *  This section contains private classes that the application does not
 *  have to use.
 ***********************************************************************/

namespace OCPI {

  // Operation bas class
  class Operation {

  public:

    // Operation element types
    enum OpType {
      NoOp,
      Const,
      Variable,
      ArrayElement,
      Node,
      EndNode,
      BasicOps,
      FuncOps,
    };

    // Invert the value
    bool invert_;

    // Intermediate value of tree
    bool complete_;

    // our value
    double v_;

    // Our Link list
    Operation *next_,*prev_;
    Operation *p_next_,*p_prev_;

    // This allows us to reuse the parsed node and perform multiple operations
    virtual void reset()
    { 
      complete_=false;
      p_prev_ = prev_;
      p_next_ = next_;
      if ( next_ ) {
        next_->reset();
      }
    }

    // Invert value
    virtual void Invert(){v_=-v_;}

    // Link on a new operation, returns true if this link is "full", this allows nested links
    virtual bool link( Operation* op ) {
      this->next_ = op;
      this->p_next_ = op;
      op->prev_ = this;
      op->p_prev_ = this;
      return true;
    }

    // Relink after calculation
    void relink() {
      Operation* tmp;
         
      // relink them
      if ( p_prev_ ) {
        if (  p_prev_->p_prev_ ) {
          tmp = p_prev_->p_prev_;
          p_prev_->p_prev_->p_next_ = this;
          p_prev_ = tmp;
        }
        else {
          p_prev_->p_prev_ = NULL;
        }
      }


      if ( p_next_ ) {
        if (  p_next_->p_next_ ) {
          tmp = p_next_->p_next_;
          p_next_->p_next_->p_prev_ = this;
          p_next_ = tmp;
        }
        else {
          p_next_->p_next_ = NULL;
        }
      }
    }

    // For debug
    virtual void printSelf()=0;

    // Runtime type check
    virtual OpType getType()=0;

    Operation()
      :invert_(false),complete_(false),
       next_(NULL),prev_(NULL),
       p_next_(NULL),p_prev_(NULL){};

    // Gets the value of a node
    virtual double getValue(bool upstream=true)=0;

    // Forces a node to conditionally calculate based upon the priority level
    virtual bool calculate(bool hight_priority)=0;

    virtual ~Operation(){};

  };


  // NoOp is used to buffer end conditions to eliminate runtime checking of 
  // special conditions
  class NoOp : public Operation {
  public:
    NoOp(){}

    virtual double getValue(bool upstream=true) {
      if ( p_next_ ) {
        return p_next_->getValue(upstream);
      }
      return 0.0;
    }

    virtual bool calculate(bool high_priority) {
      if ( p_next_ ) {
        return p_next_->calculate(high_priority);
      }
      return true;
    }

    // For debug
    virtual void printSelf() {
      cout << "NoOp:" << endl;
    }

    // Type
    virtual OpType getType(){return Operation::NoOp;}


  };






  class EndNode : public NoOp {
    virtual OpType getType(){return Operation::EndNode;}
    // For debug
    virtual void printSelf() {
      cout << "EndNode:" << endl;
    }
  };


  class BasicOp : public Operation {
   
  public:
    char op_;
    BasicOp(const char* s ) 
    {
      op_=s[0];
      v_=-1.0;
    }


    // For debug
    virtual void printSelf() {
      cout << "BasicOp:" << op_ << " value = " << v_  << endl;
    }

    virtual double getValue(bool upstream=true) {

      if ( complete_ ) {
        return v_;
      }


      if ( !p_prev_ || !p_next_ ) {
        throw std::string( "BasicOp operator needs a left hand and right hand values" );
      }
      else if ( upstream ) {

        switch( op_ ) {
        case '+':
          if ( p_prev_->getType() == Operation::NoOp ) {
            v_ = p_next_->getValue(false);
          }
          else {
            v_ = p_prev_->getValue(false) + p_next_->getValue(false);
          }
             
          break;

        case '-':
          if ( p_prev_->getType() == Operation::NoOp ) {
            v_ = -p_next_->getValue(false);
          }
          else {



            if ( p_prev_->p_prev_->getType() == Operation::BasicOps ) {
              BasicOp* bo = dynamic_cast<BasicOp*>(p_prev_->p_prev_);
              if ( bo->op_ == '-' && 
                   (p_prev_->getType() == Operation::Const ||
                    p_prev_->getType() == Operation::Variable ||
                    p_prev_->getType() == Operation::ArrayElement ||
                    p_prev_->getType() == Operation::Node) ) {

                //double v = p_prev_->getValue(false);

                p_prev_->Invert();
                bo->op_ = '+';
              }
            }
                                

            v_ = p_prev_->getValue(false) - p_next_->getValue(false);

          }
             
          break;

        case '*':
          v_ = p_prev_->getValue(false) * p_next_->getValue(false);
          break;

        case '/':
          v_ = p_prev_->getValue(false) / p_next_->getValue(false);
          break;

        case '^':
          v_ = pow( p_prev_->getValue(false), p_next_->getValue(false) );
          break;

        default:
          cerr << "Unsupported operation: " << op_ << endl;
          break;

        }
        complete_ = true;
        relink();
      }
      return v_;
    }


    virtual bool calculate(bool high_priority) {

      // Force all the downstream to go
      p_next_->calculate(high_priority);

      // mults and divs have priority
      if ( high_priority ) {
        if ( (op_ == '*') || (op_ == '/') || (op_ == '^')) {
          v_ = this->getValue();
        }
      }
      else if ( !complete_ ) {
        v_ = this->getValue();
      }

      // Make sure everyone downstream is done
      return false;

    }

    // Type
    virtual OpType getType(){return Operation::BasicOps;}


  };



  // The Node class is used to contain Operations within parentheses
  static int nest_level=0;
  class Node : public Operation {

    // We have our own list of operation to perform
    Operation* op_;

    bool v_link_;

  public:
    Node()
      :v_link_(true)
    {
      v_=0;
      op_ = new OCPI::NoOp();
      last_ = op_;
      inc_link_ = true;
    }


    virtual double getValue(bool upstream=true) {

      if ( !upstream || !p_next_ || (p_next_->getType() == Operation::NoOp) 
           || (p_next_->getType() == Operation::EndNode) ) {
        if ( ! complete_ ) {
          this->calculate(true);
        }

        if ( invert_ ) {
          return -v_;
        }
        return v_;
      }
      else {
        return p_next_->getValue();
      }
    }

    virtual bool calculate(bool high_priority) {

      if ( p_prev_->getType() == Operation::BasicOps ) {
        BasicOp* bo = dynamic_cast<BasicOp*>(p_prev_);
        if ( bo->op_ == '-' ) {
          invert_ = true;
          bo->op_ = '+';
        }
      }


      // Do our tree first
      if ( ! complete_ ) {
        op_->calculate(true);
        op_->calculate(false);

        // 
        /*
          #ifndef NDEBUG
          #if 0
          Operation* o=op_;

          cout << endl << "Node next list" << endl;
          while(o) {
          o->printSelf();
          o = o->next_;
          }

          cout << endl << "Node p_next list" << endl;
          o=op_;
          while(o) {
          o->printSelf();
          o = o->p_next_;
          }
          #endif
          #endif
        */

        v_ = op_->getValue();
        complete_ = true;
      }
          
      if ( p_next_ ) {
        return p_next_->calculate(high_priority);
      }
      else {
        return false;
      }
    }


    // Link on a new operation
#define USE_NESTED_NODES
#ifndef USE_NESTED_NODES
    Operation* last_;
    int node_count_;
    stack<Operation*> nodes_;
    virtual bool link( Operation* op ) {


      // We will link vertically until we hit our end bracket
      if ( op->getType() == Operation::EndNode ) {

        // This is our end to vertical linkage
        last_->link(op);
        v_link_ = false;
        return false;
      }

     
      // We will link vertically until we hit our end bracket
      if ( v_link_ ) {
        last_->link(op);
        last_ = op;
        return false;
      }
      else {
        Operation::link( op );
      }

      return true;
    }
#else

    Operation* last_;
    bool inc_link_;
    stack<Operation*> nodes_;
    virtual bool link( Operation* op ) {

          


      // We will link vertically until we hit our end bracket
      if ( inc_link_ && op->getType() == Operation::EndNode ) {

        // This is our end to vertical linkage
        last_->link(op);
        v_link_ = false;
        return false;
      }

     
      // We will link vertically until we hit our end bracket
      if ( v_link_ ) {
        inc_link_ = last_->link(op);
        if ( inc_link_ ) {
          last_ = op;
        }
        return false;
      }
      else {
        return Operation::link( op );
      }

      return true;

    }
#endif



    void reset()
    { 
      // We need to reset everyone in our vertical list
      if ( op_ ) {
        op_->reset();
      }
      Operation::reset();
      inc_link_ = true;
    }

    // For debug
    virtual void printSelf() {
      nest_level++;
      cout << "Node: " << v_ << endl;
      Operation *o=op_;
      while ( o ) {
        int inc = nest_level*3;
        for( int n=0;n<inc;n++) cout << " ";
        o->printSelf();
        o = o->p_next_;
      }
      nest_level--;
    }

    // Type
    virtual OpType getType(){return Operation::Node;}

    virtual ~Node() {

      Operation* o = op_->next_;
      while ( op_ ) {
        delete op_;
        if ( o ) {
          op_ = o;
          o = o->next_;
        }
        else {
          break;
        }
      }


    }

  };

 

  class Const : public Operation {
  

  public:
  
    Const( const char* s) 
    {
      v_ = (double)atof(s);
    }
   
    virtual double getValue(bool upstream=true) {

      if ( !upstream || !p_next_ || (p_next_->getType() == Operation::NoOp) 
           || (p_next_->getType() == Operation::EndNode) ) {
        if ( invert_ ) {
          return -v_;
        }
        return v_;
      }
      else {
        return p_next_->getValue();
      }
    }

    // For debug
    virtual void printSelf() {
      cout << "Const: value = " << v_  << endl;
    }

    virtual bool calculate(bool high_priority) {

        
      if ( p_prev_->getType() == Operation::BasicOps ) {
        BasicOp* bo = dynamic_cast<BasicOp*>(p_prev_);
        if ( bo->op_ == '-' ) {
          invert_ = true;
          bo->op_ = '+';
        }
      }
           

      // we dont calculate, but there is a special case
      return p_next_->calculate(high_priority); 

      complete_ = true;
      return false;
    };

    // Type
    virtual OpType getType(){return Operation::Const;}


   
  };


  class Variable : public Operation {
   
    string name_;
    OCPI::DefineExpVarInterface* cb_;
    bool auto_invert_;

  public:
    Variable(  const char* s,  DefineExpVarInterface* cb) 
      :cb_(cb),auto_invert_(false) {
      if ( s[0] == '-' ) {
        auto_invert_ = true;
        name_ = &s[1];
      }
      else {
        name_ = s;
      }
          
    }
   
    virtual double getValue(bool upstream=true) {

      // We need to ask the caller to define our value
      if ( ! complete_ ) {

        if ( cb_ ) {
          v_ = cb_->defineVariable(name_.c_str());
          if ( auto_invert_ || invert_) {
            v_ = -v_;
          }
        }
        complete_ = true;
      }

      if ( !upstream || !p_next_ || (p_next_->getType() == Operation::NoOp) 
           || (p_next_->getType() == Operation::EndNode) ) {
        return v_;
      }
      else {
        return p_next_->getValue();
      }
    }


    // For debug
    virtual void printSelf() {
      cout << "Variable: value = " << v_  << endl;
    }

 
    virtual bool calculate(bool high_priority) {

      if ( p_prev_->getType() == Operation::BasicOps ) {
        BasicOp* bo = dynamic_cast<BasicOp*>(p_prev_);
        if ( bo->op_ == '-' ) {
          invert_ = true;
          bo->op_ = '+';
        }
      }

      // we dont calculate, but there is a special case
      if ( !p_prev_ && !complete_ ) {
        getValue();
      }
      else {
        return p_next_->calculate(high_priority); 
      }
      return false;
    };

    // Type
    virtual OpType getType(){return Operation::Variable;}

  };



  class ArrayElement : public Operation {

    string name_;
    int index;
    DefineExpVarInterface* cb_;
    bool auto_invert_;

  public:
    ArrayElement( const char* s,  DefineExpVarInterface* cb) 
      :cb_(cb),auto_invert_(false) {
                
      try {
        string t;

        if ( s[0] == '-' ) {
          t = &s[1];
          auto_invert_ = true;
        }
        else {
          t = s;
        }

        string idx;
        bool name_ended=false;
        for( unsigned int n=0; n<t.length(); n++ ) {
          char c = t[n];
          if ( c == '[' ) {
            name_ended = true;
            continue;
          }
          else if ( c == ']' ) {
            break;
          }
          if ( ! name_ended ) {
            name_ += c;
          }
          else {
            idx += c;
          }
        }
        index = atoi(idx.c_str());
      }
      catch( ... ) {
        throw std::string(s);
      }
    }

    virtual double getValue(bool upstream=true) {

      // We need to ask the caller to define our value
      if ( ! complete_ ) {
        if ( cb_ ) {
          v_ = cb_->defineArrayElement(name_.c_str(), index);
          if ( auto_invert_ || invert_) {
            v_ = -v_;
          }
        }
        complete_ = true;
      }
     
      if ( !upstream || !p_next_ || (p_next_->getType() == Operation::NoOp) 
           || (p_next_->getType() == Operation::EndNode) ) {
        return v_;
      }
      else {
        return p_next_->getValue();
      }
    }


    // For debug
    virtual void printSelf() {
      cout << "ArrayElement: value = " << v_  << endl;
    }

    virtual bool calculate(bool high_priority) {

      if ( p_prev_->getType() == Operation::BasicOps ) {
        BasicOp* bo = dynamic_cast<BasicOp*>(p_prev_);
        if ( bo->op_ == '-' ) {
          invert_ = true;
          bo->op_ = '+';
        }
      }


      // we dont calculate, but there is a special case
      if ( !p_prev_ && !complete_ ) {
        getValue();
      }
      else {
        return p_next_->calculate(high_priority); 
      }
      return false;
    };

    // Type
    virtual OpType getType(){return Operation::ArrayElement;}


   
  };






  class BasicFunc: public Operation {
   
    string sop_;
 

    // Functions that we support
    enum FuncOp {
      SIN,
      COS,
      TAN,
      ATAN,
    };
    FuncOp op_;

  public:
    BasicFunc(const char* s ) 
    {
      string t(s);
      v_=-1.0;

      bool func_found=false;
      string v;
      for ( unsigned int n=0; n<t.length();n++ ) {

        char c = t[n];
        if ( c == '(' ) {
          func_found = true;
          continue;
        }
        else if ( c == ')' ) {
          break;
        }

        if ( func_found ) {
          v += c;
        }
        else {
          sop_ += (c | 0x20);
        }
      }
      v_ = atof( v.c_str() );
          
      if( sop_ == "sin" ) {
        op_ = SIN;
      }
      else if ( sop_ == "cos" ) {
        op_ = COS;
      }
      else if ( sop_ == "tan" ) {
        op_ = TAN;
      }
      else if ( sop_ == "atan" ) {
        op_ = ATAN;
      }




    }


    // For debug
    virtual void printSelf() {
      cout << "BasicOp:" << op_ << " value = " << v_  << endl;
    }

    virtual double getValue(bool upstream=true) {

      if ( complete_ ) {
        return v_;
      }


      if ( !p_prev_ || !p_next_ ) {
        throw std::string( "BasicOp operator needs a left hand and right hand values" );
      }
      else if ( upstream ) {

        switch( op_ ) {
        case SIN:
          v_ = sin( v_ );
          break;

        case COS:
          v_ = cos( v_ );
          break;

        case TAN:
          v_ = tan( v_ );
          break;

        case ATAN:
          v_ = atan( v_ );
          break;
        }
        complete_ = true;
        relink();
      }
      return v_;
    }


    virtual bool calculate(bool high_priority) {
        
      // mults and divs have priority
      if ( high_priority ) {
        if ( (op_ == '*') || (op_ == '/') || (op_ == '^')) {
          v_ = this->getValue();
        }
      }
      else if ( !complete_ ) {
        v_ = this->getValue();
      }

      // Make sure everyone downstream is done
      return p_next_->calculate(high_priority);

    }

    // Type
    virtual OpType getType(){return Operation::BasicOps;}


  };

 

  class OpsFactory {
  public:
    static Operation* create( const char* token, Operation::OpType op, int index, DefineExpVarInterface* cb ) {
      Operation* oper = NULL;

      switch( op ) {
      case  Operation::BasicOps:
        oper = new BasicOp(token);
        break;

      case Operation::Const:
        oper = new Const(token);
        break;

      case Operation::Variable:
        oper = new Variable(token,cb);
        break;

      case Operation::ArrayElement:
        oper = new ArrayElement(token,cb);
        break;

      case Operation::Node:
        oper = new Node();
        break;

      case Operation::EndNode:
        oper = new EndNode();
        break;

      case Operation::FuncOps:
        oper = OpsFactory::createFuncOps(index,token);
        break;
      default:
        break;
      };
      return oper;
    }



    static Operation* createFuncOps( int index, const char* func )
    {
      ( void ) index;
      Operation* ops=NULL;
      ops = new BasicFunc(func);
      return ops;
    }
  };



  class Tokenizer {

    // Do we have more tokens
    bool empty_;

    // Original string
    string orig_;


    // Current index into the expression string
    int c_index_;

    // Current token
    string c_token_;

    // Callback
    DefineExpVarInterface* cb_;

    // index into the string where the token was found
    int t_index_;

    // Count the brackets
    int curlies_start_;
    int curlies_end_;
    int squares_start_;
    int squares_end_;

    // Basic token type
    bool isa_digit_;
    bool isa_var_;
    bool isa_node_;
    bool isa_array_;
    bool isa_basic_ops_;
    bool isa_end_node_;
    bool last_token_was_node_;

  public:
    Tokenizer( const char* str, DefineExpVarInterface* cb );
    ~Tokenizer(){}
    bool empty();
    void operator ++(int);
    Operation* token();

    // Early syntax check
    inline bool matchedBrackets() {
      if ( ( curlies_start_ != curlies_end_ ) ||
           ( squares_start_ != squares_end_ ) ) {
        return false;
      }
      return true;
    }

  };


};






// Evaluate Expression, this routine creates the parse tree, but 
// does not attempt to resolve any variables.
OCPI::ParsedExpression* OCPI::Parser::parseExpression( const char* exp )
{
  OCPI::ParsedExpression* pexp = new OCPI::ParsedExpression( cb_ );
  pexp->tokenizer_ = new Tokenizer(exp,cb_);
  pexp->addOperation( new NoOp );
  while( !pexp->tokenizer_->empty() ) {
    (*pexp->tokenizer_)++;
    Operation* op = pexp->tokenizer_->token();
    if ( !op ) {
      cerr << "**** Got back a null token" << endl;
    }
    pexp->addOperation( op );

  }
  pexp->addOperation( new NoOp );
  if ( !pexp->tokenizer_->matchedBrackets() ) {
    std::string s("Brackets mismatch in expression ");
    s += exp;
    throw s;
  }
  return pexp;
}


double OCPI::ParsedExpression::evaluate()
{
  double value=0;

#ifndef NDEBUG
  //  cout << "Original list" << endl;
  //  this->printOrigList();
#endif

  // force an new iterative calculation
  first_->reset();
  first_->calculate(true);

#ifndef NDEBUG
  //  cout << endl << "After first pass list" << endl;
  //  this->printWorkingList();
#endif

  first_->calculate(false);

#ifndef NDEBUG
  //   cout << endl << "After second pass list" << endl;
  //  this->printWorkingList();
#endif

  value = first_->getValue();
  return value;
}


OCPI::Tokenizer::Tokenizer( const char* str,DefineExpVarInterface* cb )
  :empty_(false),orig_(str),c_index_(0),cb_(cb)
{
  curlies_start_ = curlies_end_ = squares_start_ = squares_end_ = 0;
  isa_digit_ = false;
  isa_var_ = false;
  isa_node_ = false;
  isa_array_ = false;
  isa_basic_ops_ = false;
  isa_end_node_ =false;
  last_token_was_node_=true;
}

bool OCPI::Tokenizer::empty()
{
  if ( c_index_ >= (int)orig_.length() ) {
    empty_ = true;
  }
  return empty_;
}
   
void OCPI::Tokenizer::operator ++(int)
{

  bool isa_hex = false;

  if ( !empty() ) {

    c_token_.erase();
    bool tnf=true;
    bool wait_for_closing_curly_bracket = false;
    isa_digit_ = false;
    isa_var_ = false;
    isa_node_ = false;
    isa_array_ = false;
    isa_basic_ops_ = false;
    isa_end_node_ =false;
    bool negate=false;
        
    while( tnf && c_index_<(int)orig_.length() ) {
                        
      char c = orig_[c_index_++];
      // It can only be a variable, an array, a bracket, or an operator
      if ( isalpha(c) ) {

        if ( c == 'x' ) {
          if ( c_token_.length() > 0 ) {
            if ( c_token_[0] == '0' ) {
              isa_hex = true;
            }
          }
        }

        if ( c_token_.length() == 0 ) {
          isa_var_ = true;
        }
        c_token_ += c;
      }
      else if ( c == '-' && last_token_was_node_ && (c_token_.length()==0) ) {

        /*
          isa_digit_ = true;
          c_token_ += c;
        */

        negate = true;
        continue;
      }
      else if ( c == '.' ) {
        if ( c_token_.length() == 0 ) {
          isa_digit_ = true;
        }
        c_token_ += c;
      }
      else if ( isdigit(c) ) {

        if ( c_token_.length() == 0 ) {
          isa_digit_ = true;
        }
        c_token_ += c;
      }
      else if ( isspace(c) ) {
        if ( c_token_.length() == 0 ) {
          continue;
        }
        else {
          break;
        }
      }
      else if ( c == '(' ) {
        curlies_start_++;

        if ( c_token_.length() != 0 ) {  // must be a function with arg
          isa_var_ = false;
          wait_for_closing_curly_bracket = true;
          c_token_ += c;
        }
        else {  // must be a priory calculation
          c_token_ = "Node";
          isa_node_ = true;
          break;
        }

      }
      else if ( c == ')' ) {
        curlies_end_++;
        if ( wait_for_closing_curly_bracket ) {
          c_token_ += c;
          break;
        }
        else {
          if ( c_token_.length() == 0 ) {
            c_token_ = "EndNode";
            isa_end_node_ = true;
            break;
          }
          else {
            curlies_end_--;
            c_index_--;
            break;
          }
        }
      }
      else if ( c == '[' ) { 
        squares_start_++;
        // we only support this as a means of indexing arrays, so
        if ( c_token_.length() == 0 ) {
          std::string s("Invalid Syntax: ");
          s += orig_;
          throw s;
        }
        c_token_ += c;
        isa_var_=false;
        isa_array_=true;
      }
      else if ( c == ']' ) {
        c_token_ += c;
        squares_end_++;
      }

      // must be an operator
      else {
        if ( isa_digit_ || isa_var_ || isa_array_ ) {
          c_index_--;
          break;
        }
        isa_basic_ops_=true;
        c_token_ += c;
        break;
      }
    }


    last_token_was_node_ = isa_node_;


    if ( negate ) {
      c_token_.insert(0,"-");
    }

                
#ifndef NDEBUG
    // printf("%s\n", c_token_.c_str() );
#endif

  }


  // if this is a hex value, convert it now
  if ( isa_hex ) {
    char c[128];
    int i;
    sscanf( c_token_.c_str(), "0x%x", &i );
    sprintf( c, "%.3f", (float)i );
    c_token_ = c;
  }
        


}


OCPI::Operation* OCPI::Tokenizer::token()
{
  // map the basic type
  Operation::OpType type;
  if ( this->isa_digit_ ) {
    type = Operation::Const;
  }
  else if ( this->isa_node_ ) {
    type = Operation::Node;
  }
  else if ( this->isa_end_node_ ) {
    type = Operation::EndNode;
  }
  else if ( this->isa_var_ ) {
    type = Operation::Variable;
  }
  else if ( this->isa_array_ ) {
    type = Operation::ArrayElement;
  }
  else if ( this->isa_basic_ops_ ) {
    type = Operation::BasicOps;
  }
  else {
    type = Operation::FuncOps;
  }

  return OpsFactory::create( c_token_.c_str(), type, t_index_,cb_ );
};


OCPI::ParsedExpression::~ParsedExpression()
{
  delete tokenizer_;
  Operation* o = first_->next_;
  while ( first_ ) {
    delete first_;
    if ( o ) {
      first_ = o;
      o = o->next_;
    }
    else {
      break;
    }
  }
}


// For debug
void OCPI::ParsedExpression::printOrigList()
{
  Operation* op = first_;
  while( op ) {
    op->printSelf();
    op = op->next_;
  }
}

void OCPI::ParsedExpression::printWorkingList()
{
  Operation* op = first_;
  while( op ) {
    op->printSelf();
    op = op->p_next_;
  }

}


OCPI::Parser::~Parser()
{


}

void OCPI::ParsedExpression::addOperation( Operation* op )
{
  if ( first_ == NULL ) {
    first_ = op;
    last_ = op;
  }
  else {
    if ( last_->link( op ) ) {
      last_ = op;
    }
  }
}






class MyVarDefiner : public OCPI::DefineExpVarInterface {

public:

  virtual double defineVariable( const char* var )
    throw (std::string) {
    return atof( &var[1] );        
  }

  virtual double defineArrayElement( const char* var, int index )
    throw (std::string) {
    ( void ) var;
    return (double)index;
  }

};




#define CHECK_VALUE( s,v,ans)                                                \
  if ( (int)v != (int)ans ) {                                                \
    cerr << "PARSER ERROR !!, Expected " << ans << " Got " << v << " from expression (" << s << ")" << endl; \
  } else {                                                                \
    cerr << "Parser passes for expression (" << s << ") Got " << v << " Expected " << ans << endl; \
  }



#define CHECK_D_VALUE( s,v,ans)                                                \
  if ( (int)(v*10000.0) != (int)ans ) {                                        \
    cerr << "PARSER ERROR !!, Expected " << ((double)ans/10000.0) << " Got " << v << " from expression (" << s << ")" << endl; \
  } else {                                                                \
    cerr << "Parser passes for expression (" << s << ") Got " << v << " Expected " << ((double)ans/10000.0) << endl; \
  }


void run_exp_tests()
{

  double value;
  try {
    MyVarDefiner *my_var_definer = new MyVarDefiner;
    OCPI::Parser *exp_parser = new OCPI::Parser( my_var_definer );
    OCPI::ParsedExpression* exp;
    const char *ex;

        

    // Some simple expression
    ex = "1";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;

    ex = "-1";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -1);
    delete exp;

    ex = "10+10";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 20);
    delete exp;

    ex = "10 *10";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 100);
    delete exp;

    ex = "20 /10";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 2);
    delete exp;

    ex = "20*10/(9+1)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 20);
    delete exp;

    ex = "(-2-1)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -3);
    delete exp;

    ex = "v1+(10*  43)-v2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 429);
    delete exp;

    ex = "3+10 + 4*2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 21);
    delete exp;

    ex = "2*3 + 4+8+1*   2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 20);
    delete exp;

    ex = "(2*3) + (4+8)+(1*   2)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 20);
    delete exp;

    ex = "1+(2+1)+1+1";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 6);
    delete exp;


    ex = "1+(10*  43)-2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 429);
    delete exp;


    ex = "1+((2+3)+4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 10);
    delete exp;


    ex = "(+1+((2+3)+4))";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 10);
    delete exp;

    ex = "2*2*2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 8);
    delete exp;

    ex = "2*2*2*2*2*2*2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 128);
    delete exp;

    ex = "2*((3+4)+8+1)* 2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 64);
    delete exp;

    ex = "-2-1";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -3);
    delete exp;

    ex = "4^2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 16);
    delete exp;

    ex = "(8*2)+4^2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 32);
    delete exp;

    ex = "-1+(8*2)+4^2";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 31);
    delete exp;

    ex = "0xf";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 15);
    delete exp;


    ex = "0xf-10";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 5);
    delete exp;

    ex = "((0xf-10)*2)+(1*2)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 12);
    delete exp;
        
    ex = "SIN(5)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_D_VALUE( ex,value, -9589);
    delete exp;
        
    ex = "v1";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;

    ex = "v[101]";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 101);
    delete exp;

    ex = "101-1*(8*4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 69);
    delete exp;
         

    ex = "101-1-(8*4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 68);
    delete exp;

    ex = "10-1-(2*2+4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;


    ex = "-10-1-(2*2+4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -19);
    delete exp;


    ex = "10-1-(v[2]*2+4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;

    ex = "v10-v[1]-(v[2]*2+v4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;


    ex = "v[10]-v1-(v[2]*2+v4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 1);
    delete exp;

    ex = "-v[10]-v1-(v[2]*2+v4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -19);
    delete exp;


    ex = "-v10-v[1]-(v[2]*2+v4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -19);
    delete exp;

    ex = "-10-v[1]-(v[2]*2+v4)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -19);
    delete exp;

    ex = "1*(8)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 8);
    delete exp;

    ex = "(8)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 8);
    delete exp;

    ex = "1*(v8)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 8);
    delete exp;

    ex = "1*(v[8])";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, 8);
    delete exp;

    ex = "1-2-3-1-5";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -10);
    delete exp;

    ex = "1-2-3-1-5*(v[8])";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -45);
    delete exp;

    ex = "v1-2-v3-v[1]-v[5]*(v[8])";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -45);
    delete exp;


    ex = "(1-2-3-1-5)*8";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -80);
    delete exp;

    ex = "(v1-v2-v[3]-v[1]-5)*v8";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -80);
    delete exp;

    ex = "(1-(2-3)-1-5)*8";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -32);
    delete exp;

    ex = "(1-((2-3)-1)-5)*8";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -16);
    delete exp;

    ex = "(1-((2-3)-1)-5)*(8)";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -16);
    delete exp;

    ex = "((1-((2-3)-1)-5)*(8))";
    exp = exp_parser->parseExpression( ex );
    value = exp->evaluate();
    CHECK_VALUE( ex,value, -16);
    delete exp;

     

  }
  catch( std::string& ex ) {
    cerr << "Exception !!" << ex.c_str() << endl;
  }
  catch( ... ) {
    cerr << "Caught an unknown exception !!" << endl;
  }
}









