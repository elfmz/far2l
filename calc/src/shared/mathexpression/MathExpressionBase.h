/*
 * MathExpressionBase.h
 * 0.6.6 Alpha
 *
 *  Created on: Apr 24, 2009
 *      Author: amyznikov
 *
 *  Last Modified:
 *      Wednesday, May 31 2009
 *
 *  Be aware that this source still under alpha-development stage,
 *  and therefore can be redesigned frequently. Lookup for updates
 *  at http://sourceforge.net/projects/mathexpression
 */

#ifndef __MATHEXPRESSIONBASE_H__
#define __MATHEXPRESSIONBASE_H__

#include <stdio.h>
#include <wctype.h>
#include <cctype>
#include <cassert>
#include <cstdarg>
#include <vector>
#include <string>
#include <algorithm>

// Try to workaround gedanken MSVC.
#ifdef _MSC_VER
  #define TEMPLATE
#else
  #define TEMPLATE template
#endif

// XXX:
static const int VARARG = -1;

// XXX: sorry... needed for dynamic ops
extern void MathOpNameCallback(std::wstring name);


enum PARSER_ERROR
{
	PARSER_ERROR_OK = 0,
	PARSER_ERROR_UNEXPECTED,			// "unexpected text after end of expression"
	PARSER_ERROR_MISSING_O_BRACE,		// "missing '('"
	PARSER_ERROR_MISSING_C_BRACE,		// "missing ')'"
	PARSER_ERROR_NO_ARGUMENTS,			// "at least 1 argument is required"
	PARSER_ERROR_ARGUMENTS_MISMATCH,	// "arguments number mismatch"
	PARSER_ERROR_UNKNOWN_ID,			// "unknown identifier"
};

template<class T>
class MathExpressionBase
  {


protected:

  /** Node Types
      Node
      |---> ConstantValueNode
      |---> ArgNode
      |---> BoundParameterNode
      |---> FunctionalNode
                   |---> FunctionNode
                   |---> FunctionPointerNode
                   |---> FunctorNode
                   |---> MathExpressionNode
   */

  class Node;
  class ConstValueNode;
  class ArgNode;
  template<class P> class BoundParameterNode;
  class FunctionalNode;
  template<class F> class FunctionPointerNode;
  template<class F, F fn> class FunctionNode;
  template<class F, int numargs> class FunctorNode;
  template<class R> class MathExpressionNode;

  typedef std::wstring string;

  template<class R>
  static inline R & dereference ( R * f )
    { return *f;
    }

  template<class R>
  static inline R & dereference ( R & f )
    { return  f;
    }


  class Node
    {
  public:
    virtual ~Node() {}
    virtual T eval(const T _args[] = 0 ) = 0;
    };


  class NodeList
    : public std::vector<Node* >
    {
    typedef std::vector<Node* > mybase;

  public:
    explicit NodeList( size_t n = 0 )
      : mybase(n)
      {}
    explicit NodeList( Node * arg )
      { mybase::push_back(arg);
      }
    explicit NodeList( Node * arg1, Node * arg2 )
      { mybase::push_back(arg1);
        mybase::push_back(arg2);
      }
    };


  class ConstValueNode
    : public Node
    {
    const T myvalue;
  public:
    ConstValueNode( const T & value )
      : myvalue(value)
      {}
    T eval(const T _args[] )
      { return myvalue;
      }
    };


  class ArgNode
    : public Node
    {
    const int argindex;
  public:
    ArgNode( const int _argindex )
      : argindex(_argindex)
      {}
    T eval(const T _args[] )
      { return _args[argindex];
      }
    };


  template <class P>
  class BoundParameterNode
    : public Node
    {
    const P * pointer_to_the_value;
  public:
    BoundParameterNode( const P * pointer_to_value )
      : pointer_to_the_value(pointer_to_value)
      {}
    T eval(const T _args[] )
      { return (T)(*pointer_to_the_value);
      }
    };


  class FunctionalNode
    : public Node
    {
  protected:
    NodeList args;

    FunctionalNode( const NodeList & arglist )
      : args(arglist)
      {}

    ~FunctionalNode()
      { for( typename NodeList::iterator ii=args.begin(); ii!=args.end(); ++ii)
          delete (*ii);
      }
    };


  template<class R, R (*fn)()>
  class FunctionNode<R (*)(),fn>
    : public FunctionalNode
    {
  public:
    enum  { numargs = 0 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn();
      }
    };

  template<class R>
  class FunctionPointerNode<R (*)()>
    : public FunctionalNode
    {
    typedef R (*F)();
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 0 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f),fn_name(name) {}
    T eval( const T _args[] )
      {
		MathOpNameCallback(fn_name);
		return fn();
      }
    };

  template<class F>
  class FunctorNode<F,0>
    : public FunctionalNode
    {
    F fn;
  public:
    enum  { numargs = 0 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { // The fn can be either or pointer to functor object, or an object instance,
        // therefore we need dereference it
      return dereference(fn)();
      }
    };



  template<class R, class P1, R (*fn)(P1)>
  class FunctionNode<R (*)(P1),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 1 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args));
      }
    };

  template<class R, class P1>
  class FunctionPointerNode<R(*)(P1)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(P1);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 1 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f),fn_name(name) {}
    T eval(const T _args[] )
      {
		// XXX: sorry... needed for dynamic ops
		P1 a1 = args[0]->eval(_args);
		MathOpNameCallback(fn_name);
		return fn(a1);
      }
    };

  template<class F>
  class FunctorNode<F,1>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 1 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return (dereference(fn)(args[0]->eval(_args)));
      }
    };



  template<class R, class P1, class P2, R (*fn)(P1,P2)>
  class FunctionNode<R (*)(P1,P2),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 2 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args));
      }
    };

  template<class R, class P1, class P2>
  class FunctionPointerNode<R (*)(P1,P2)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(P1,P2);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 2 };
	FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f),fn_name(name) {}
    T eval( const T _args[] )
      {
		// XXX: sorry... needed for dynamic ops
//		if (!&_args) return 0;
		P1 a1 = args[0]->eval(_args);
		P1 a2 = args[1]->eval(_args);
		MathOpNameCallback(fn_name);
		return fn(a1,a2);
      }
    };

  template<class F>
  class FunctorNode<F,2>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 2 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args));
      }
    };



  template<class R, class P1, class P2, class P3, R (*fn)(P1,P2,P3)>
  class FunctionNode<R (*)(P1,P2,P3),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 3 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args));
      }
    };

  template<class R, class P1, class P2, class P3>
  class FunctionPointerNode<R (*)(P1,P2,P3)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef  R (*F)(P1,P2,P3);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 3 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f), fn_name(name) {}
    T eval( const T _args[] )
      {

		// XXX: sorry... needed for dynamic ops
		P1 a1 = args[0]->eval(_args);
		P1 a2 = args[1]->eval(_args);
		P1 a3 = args[2]->eval(_args);
		MathOpNameCallback(fn_name);
		return fn(a1,a2,a3);
      }
    };

  template<class F>
  class FunctorNode<F,3>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 3 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args));
      }
    };



  template<class R, class P1, class P2, class P3, class P4, R (*fn)(P1,P2,P3,P4)>
  class FunctionNode<R (*)(P1,P2,P3,P4),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 4 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args));
      }
    };

  template<class R, class P1, class P2, class P3, class P4>
  class FunctionPointerNode<R (*)(P1,P2,P3,P4)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef  R (*F)(P1,P2,P3,P4);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 4 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f),fn_name(name) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args));
      }
    };

  template<class F>
  class FunctorNode<F,4>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 4 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args));
      }
    };



  template<class R, class P1, class P2, class P3, class P4, class P5, R (*fn)(P1,P2,P3,P4,P5)>
  class FunctionNode<R (*)(P1,P2,P3,P4,P5),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 5 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args));
      }
    };

  template<class R, class P1, class P2, class P3, class P4, class P5>
  class FunctionPointerNode<R (*)(P1,P2,P3,P4,P5)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(P1,P2,P3,P4,P5);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 5 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args),fn(f),fn_name(name) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args));
      }
    };

  template<class F>
  class FunctorNode<F,5>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args));
      }
    };


  template<class R, class P1, class P2, class P3, class P4, class P5, class P6, R (*fn)(P1,P2,P3,P4,P5,P6)>
  class FunctionNode<R (*)(P1,P2,P3,P4,P5,P6),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 6 };
    FunctionNode( const NodeList & args )
      : FunctionalNode(args)
      {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args), args[4]->eval(_args),
          args[5]->eval(_args));
      }
    };

  template<class R, class P1, class P2, class P3, class P4, class P5, class P6>
  class FunctionPointerNode<R (*)(P1,P2,P3,P4,P5,P6)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(P1,P2,P3,P4,P5,P6);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 6 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args), fn(f),fn_name(name) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args),
          args[5]->eval(_args));
      }
    };

  template<class F>
  class FunctorNode<F,6>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 6 };
    FunctorNode( const F & f, const NodeList & args )
      : FunctionalNode(args),fn(f)
      {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args), args[4]->eval(_args),
          args[5]->eval(_args));
      }
    };


  template<class R, class P1, class P2, class P3, class P4, class P5, class P6, class P7, R (*fn)(P1,P2,P3,P4,P5,P6,P7)>
  class FunctionNode<R (*)(P1,P2,P3,P4,P5,P6,P7),fn>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
  public:
    enum  { numargs = 7 };
    FunctionNode( const NodeList & args ) : FunctionalNode(args) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args),
          args[5]->eval(_args),args[6]->eval(_args));
      }
    };

  template<class R, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
  class FunctionPointerNode<R (*)(P1,P2,P3,P4,P5,P6,P7)>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(P1,P2,P3,P4,P5,P6,P7);
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = 7 };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args), fn(f),fn_name(name) {}
    T eval( const T _args[] )
      { return fn(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args),
          args[5]->eval(_args),args[6]->eval(_args));
      }
    };

  template<class F>
  class FunctorNode<F,7>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = 7 };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(args[0]->eval(_args),args[1]->eval(_args),args[2]->eval(_args),args[3]->eval(_args),args[4]->eval(_args),
          args[5]->eval(_args),args[6]->eval(_args));
      }
    };


  template<class R>
  class FunctionPointerNode<R (*)(const T * , const NodeList & )>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    typedef R (*F)(const T *, const NodeList & );
    F fn;
	// XXX: sorry... needed for dynamic ops
	string fn_name;
  public:
    enum  { numargs = VARARG };
    FunctionPointerNode( F f, const NodeList & args, const string & name ) : FunctionalNode(args), fn(f),fn_name(name) {}
    T eval( const T _args[] )
      { return fn(_args,args);
      }
    };

  template<class F>
  class FunctorNode<F,VARARG>
    : public FunctionalNode
    {
    using FunctionalNode :: args;
    F fn;
  public:
    enum  { numargs = VARARG };
    FunctorNode( const F & f, const NodeList & args ) : FunctionalNode(args),fn(f) {}
    T eval(const T _args[] )
      { return dereference(fn)(_args,args);
      }
    };

  template<class R>
  class MathExpressionNode
    : public FunctionalNode
    {
    const MathExpressionBase<R> * fn;
    std::vector<T> __args;
  public:
    MathExpressionNode( const MathExpressionBase<R> * f, const NodeList & args )
      : FunctionalNode(args),fn(f)
      { __args.resize(args.size());
      }
    T eval( const T _args[] )
      {
		for(size_t i=0; i<FunctionalNode::args.size(); ++i )
		{ __args[i] = FunctionalNode::args[i]->eval(_args);
		}
        return __args.empty() ? fn->eval() : fn->eval(&*__args.begin());
      }
    };

protected:

  /** Node Factory
   */

  class ConstValueNodeFactory {
  public:
    string name, description;
    ConstValueNodeFactory( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~ConstValueNodeFactory() {}
    virtual Node * create_node() const = 0;
    };


  class ArgNodeFactory {
  public:
    string name, description;
    ArgNodeFactory( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~ArgNodeFactory() {}
    virtual Node * create_node() const = 0;
    };


  class BoundParameterNodeFactory {
  public:
    string name, description;
    BoundParameterNodeFactory( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~BoundParameterNodeFactory() {}
    virtual Node * create_node() const = 0;
    };


  class FunctionalNodeFactory {
  public:
    string name, description;
    FunctionalNodeFactory ( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~FunctionalNodeFactory() {}
    virtual Node * create_node( const NodeList & args, const string & fn_name   ) const = 0;
    virtual int getnumargs() const = 0;
    };


  class UnaryOperationNodeFactory {
  public:
    string name, description;
    UnaryOperationNodeFactory ( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~UnaryOperationNodeFactory() {}
    Node * create_node( Node * arg ) const
      { return create_node(NodeList(arg), name);
      }
  private:
    virtual Node * create_node( const NodeList & args, const string & fn_name  ) const = 0;
    };


  class BinaryOperationNodeFactory {
  public:
    string name, description;
    BinaryOperationNodeFactory ( const string & _name, const string & _description=L"" ) : name(_name),description(_description) {}
    virtual ~BinaryOperationNodeFactory() {};
    Node * create_node( Node * left, Node * right  ) const
      { return create_node(NodeList(left,right), name);
      }
  private:
    virtual Node * create_node( const NodeList & args, const string & fn_name ) const = 0;
    };


protected:

  /** Node Factory Lists
   */

  template<class Factory>
  class NodeFactoryList
    {
  public:
    typedef NodeFactoryList this_class;
    typedef std::vector<Factory*> _Container_type;
    typedef typename _Container_type :: iterator iterator;
    typedef typename _Container_type :: const_iterator const_iterator;
    static  const size_t npos = static_cast<size_t>(-1);

    size_t size() const
      { return c.size();
      }

    const Factory * at( size_t index ) const
      { return c[index];
      }

    size_t index_of( const string & name ) const
      { const_iterator ii=find(name);
        return ii==c.end() ? npos : ii-c.begin();
      }

    const Factory * find_by_name( const string & name ) const
      { const_iterator ii = find(name);
        return ii==c.end() ? 0 : *ii;
      }

    const Factory * lookup( const wchar_t * expression ) const
      {
      for( const_iterator ii=c.begin(); ii!=c.end(); ++ii )
      if( (*ii)->name.compare(0,(*ii)->name.size(),expression,(*ii)->name.size()) == 0 )
        return *ii;
      return 0;
      }

    void remove( const string & name )
      { iterator ii = find(name);
        if( ii != c.end() ) { delete *ii; c.erase(ii); }
      }

    void remove_at( size_t index )
      { delete c[index];
        c.erase(c.begin()+index);
      }

    void remove_all()
      { for( iterator ii = c.begin(); ii!=c.end(); ++ii ) delete *ii;
        c.clear();
      }

	void clear_all()
	{
	c.clear();
      }


  protected:

    _Container_type c;

    NodeFactoryList()
      {
      }

    ~NodeFactoryList()
      { remove_all();
      }

    static bool compare_by_name_length( const Factory * prev, const Factory * next )
      { return prev->name.size() > next->name.size();
      }

    iterator find( const string & name )
      { iterator ii = c.begin();
      for( ; ii!=c.end(); ++ii )
        if( (*ii)->name == name )
          break;
      return ii;
      }

    const_iterator find( const string & name ) const
      { const_iterator ii = c.begin();
      for( ; ii!=c.end(); ++ii )
        if( (*ii)->name == name )
          break;
      return ii;
      }

    void push_back( Factory * obj )
      { c.push_back(obj);
      }

    template<class _Comparator_type>
    void sort( const _Comparator_type & cmp )
      { std::sort(c.begin(),c.end(),cmp);
      }

    void sort_by_name_length()
      { this->sort( &this_class :: compare_by_name_length);
      }

    };


  class NamedConstantList
    : public NodeFactoryList<ConstValueNodeFactory>
    {

    typedef NodeFactoryList<ConstValueNodeFactory> mybase;

    class ConstValueNodeFactoryImpl
      : public ConstValueNodeFactory
      {
    public:

      ConstValueNodeFactoryImpl( const string & name, const T & cvalue, const string & description=L"" )
        : ConstValueNodeFactory(name,description), value(cvalue)
        {}

      Node * create_node() const
        { return new ConstValueNode(value);
        }

      T value;
      };

  public:

    template<class P>
    void add( const string & name, const P & value, const string & description=L"" )
      { this->push_back( new ConstValueNodeFactoryImpl(name,(T)value,description));
      }

    };


  class ArgList
    : public NodeFactoryList<ArgNodeFactory>
    {

    typedef NodeFactoryList<ArgNodeFactory> mybase;

    class ArgNodeFactoryImpl
      : public ArgNodeFactory
      {
    public:

      ArgNodeFactoryImpl( const string & name, int _index, const string & description=L"" )
        : ArgNodeFactory(name,description), index(_index)
        {}

      Node * create_node() const
        { return new ArgNode(index);
        }

      int index;
      };

  public:

    void add( const string & name, int arg_index, const string & description=L"" )
      { this->push_back( new ArgNodeFactoryImpl(name,arg_index,description));
      }

    };


  class BoundParameterList
    : public NodeFactoryList<BoundParameterNodeFactory >
    {
    typedef NodeFactoryList<BoundParameterNodeFactory > mybase;

    template<class P>
    class BoundParameterNodeFactoryImpl
      : public BoundParameterNodeFactory
      {
    public:

      BoundParameterNodeFactoryImpl( const string & name, const P * value_ptr, const string & description=L""  )
        : BoundParameterNodeFactory (name,description), pointer_to_value(value_ptr)
        {}

      Node * create_node() const
        { return new BoundParameterNode<P>(pointer_to_value);
        }

      const P * pointer_to_value;
      };

  public:

    template<class P>
    void add( const string & name, const P * pointer_to_value, const string & description=L"" )
      { this->push_back( new BoundParameterNodeFactoryImpl<P>(name,pointer_to_value, description));
      }

    };


  template<class NodeFactory>
  class FunctionListBase
    : public NodeFactoryList<NodeFactory >
    {
    typedef NodeFactoryList<NodeFactory > mybase;

  public:
    template<class F, F fn>
    class FunctionNodeFactory
      : public NodeFactory {
    public:
      FunctionNodeFactory( const string & name, const string & description=L"" )
        : NodeFactory (name,description)
        {}
      Node * create_node(const NodeList & args ) const
        { return new FunctionNode<F,fn>(args);
        }
      int getnumargs() const
        { return FunctionNode<F,fn> :: numargs;
        }
      };

    template<class F>
    class FunctionPointerNodeFactory
      : public NodeFactory {
      F fn;
    public:
      FunctionPointerNodeFactory( const string & name, F f, const string & description=L"" )
        : NodeFactory (name,description), fn(f)
        {}
      Node * create_node(const NodeList & args, const string & fn_name ) const
        {
		  // XXX:
		  FunctionPointerNode<F> *fpn = new FunctionPointerNode<F>(fn,args,fn_name);
		  return fpn;
        }
      int getnumargs() const
        { return FunctionPointerNode<F>::numargs;
        }
      };

    template<class F, int numargs>
    class FunctorNodeFactory
      : public NodeFactory {
      F fn;
    public:
      FunctorNodeFactory( const string & name, const F & f, const string & description=L"" )
        : NodeFactory (name,description), fn(f)
        {}
      Node * create_node(const NodeList & args ) const
        { return new FunctorNode<F,numargs>(fn,args);
        }
      int getnumargs() const
        { return FunctorNode<F,numargs> :: numargs;
        }
      };

    template<class R>
    class MathExpressionNodeFactory
      : public NodeFactory {
      const MathExpressionBase<R> * fn;
    public:
      MathExpressionNodeFactory( const string & name, const MathExpressionBase<R> * f, const string & description=L"" )
        : NodeFactory (name,description), fn(f)
        {}
      Node * create_node(const NodeList & args, const string & ) const
        {
		  return new MathExpressionNode<R>(fn,args);
        }
      int getnumargs() const
        { return (int)fn->Arguments.size();
        }
      };

    void push_back( NodeFactory * f )
      { mybase :: push_back(f);
        mybase :: sort_by_name_length();
      }
    };


  class FunctionList
    : public FunctionListBase<FunctionalNodeFactory>
    {
    typedef FunctionListBase<FunctionalNodeFactory> mybase;

  public:
    template<class F, F fn >
    void add( const string & name, const string & description=L"" )
      { this->push_back( new typename mybase::template FunctionNodeFactory<F,fn>(name,description) );
      }
    template<int numargs,class F>
    void add( const string & name, const F & f, const string & description=L"" )
      { this->push_back( new typename mybase::template FunctorNodeFactory<F,numargs>(name,f,description) );
      }
    template<class F>
    void add( const string & name, F * f, const string & description=L"" )
      { this->push_back( new typename mybase::template FunctionPointerNodeFactory<F*>(name,f,description) );
      }
    template<class R>
    void add( const string & name, const MathExpressionBase<R> * f, const string & description=L"" )
      { this->push_back( new typename mybase::template MathExpressionNodeFactory<R>(name,f,description) );
      }
    };


  class UnaryOperationTable
    : public FunctionListBase<UnaryOperationNodeFactory >
    {
    typedef FunctionListBase<UnaryOperationNodeFactory > mybase;

  public:

    template<class R>
    void add( const string & operator_string, R (*pointer_to_operator_function)(T), const string & description=L"" )
      { this->push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(T)>(operator_string,pointer_to_operator_function,description) );
      }
    template<class R>
    void add( const string & operator_string, R (*pointer_to_operator_function)(const T&), const string & description=L"" )
      { this->push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(const T&)>(operator_string,pointer_to_operator_function,description) );
      }
    template<class R, class P >
    void add( const string & operator_string, R (*pointer_to_operator_function)(P), const string & description=L"" )
      { this->push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(P)>(operator_string,pointer_to_operator_function,description) );
      }

    template<class F, F fn >
    void add( const string & name, const string & description=L""  )
      { this->push_back( new typename mybase::template FunctionNodeFactory<F,fn>(name,description) );
      }
    template<class F>
    void add( const string & name, const F & f, const string & description=L""  )
      { this->push_back( new typename mybase::template FunctorNodeFactory<F,1>(name,f,description) );
      }

#ifndef _MSC_VER
    template<T (*fn)(T) >
    void add( const string & name, const string & description=L""  )
      { this->push_back( new typename mybase::template FunctionNodeFactory<T(*)(T),fn>(name,description) );
      }

    template<T (*fn)(const T &) >
    void add( const string & name, const string & description=L""  )
      { this->push_back( new typename mybase::template FunctionNodeFactory<T(*)(const T&),fn>(name,description) );
      }

    template<const T (*fn)(const T &) >
    void add( const string & name, const string & description=L""  )
      { this->push_back( new typename mybase::template FunctionNodeFactory<const T(*)(const T&),fn>(name,description) );
      }
#endif

    };


  class BinaryOperationTable
    {
  public:

    class PriorityLevel
      : public FunctionListBase<BinaryOperationNodeFactory >
      {
      typedef FunctionListBase<BinaryOperationNodeFactory > mybase;

    public:

      template<class R>
      void add( const string & operator_string, R (*pointer_to_operator_function)(T,T), const string & description=L"" )
        { push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(T,T)>(operator_string,pointer_to_operator_function,description) );
        }
      template<class R>
      void add( const string & operator_string, R (*pointer_to_operator_function)(const T&,const T&), const string & description=L"" )
        { this->push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(const T&,const T&)>(operator_string,pointer_to_operator_function,description) );
        }
      template<class R, class P1, class P2 >
      void add( const string & operator_string, R (*pointer_to_operator_function)(P1,P2), const string & description=L"" )
        { this->push_back( new typename mybase::template FunctionPointerNodeFactory<R(*)(P1,P2)>(operator_string,pointer_to_operator_function,description) );
        }

      template<class F, F fn >
      void add( const string & name, const string & description=L"" )
        { this->push_back( new typename mybase::template FunctionNodeFactory<F,fn>(name,description) );
        }
      template<class F>
      void add( const string & name, const F & f, const string & description=L""  )
        { this->push_back( new typename mybase::template FunctorNodeFactory<F,2>(name,f,description) );
        }

#ifndef _MSC_VER
      template<T (*fn)(T,T) >
      void add( const string & name, const string & description=L"" )
        { this->push_back( new typename mybase::template FunctionNodeFactory<T(*)(T,T),fn>(name,description) );
        }

      template<T (*fn)(const T &, const T &) >
      void add( const string & name, const string & description=L""  )
        { this->push_back( new typename mybase::template FunctionNodeFactory<T(*)(const T&,const T&),fn>(name,description) );
        }

      template<const T (*fn)(const T &, const T &) >
      void add( const string & name, const string & description=L""  )
        { this->push_back( new typename mybase::template FunctionNodeFactory<const T(*)(const T&,const T &),fn>(name,description) );
        }

      template<bool (*fn)(T,T) >
      void add( const string & name, const string & description=L"" )
        { this->push_back( new typename mybase::template FunctionNodeFactory<bool(*)(T,T),fn>(name,description) );
        }

      template<bool (*fn)(const T &, const T &) >
      void add( const string & name, const string & description=L""  )
        { this->push_back( new typename mybase::template FunctionNodeFactory<bool(*)(const T&,const T&),fn>(name,description) );
        }
#endif
      };

    typedef std::vector<PriorityLevel*> _Container_type;
    typedef typename _Container_type :: iterator iterator;
    typedef typename _Container_type :: const_iterator const_iterator;

    BinaryOperationTable()
      {
      }

    ~BinaryOperationTable()
      { remove_all();
      }

    size_t get_num_levels() const
      { return c.size();
      }

    void set_num_levels( size_t newsize )
      {
      if( newsize > c.size() )
        { insert( c.size(), newsize-c.size() );
        }
      else if( newsize < c.size() )
        { for (size_t i=newsize; i<c.size(); ++i )
            delete c[i];
          c.resize(newsize);
        }
      }

    PriorityLevel * operator[] (size_t index )
      { return c[index];
      }

    const PriorityLevel * operator[] (size_t index ) const
      { return c[index];
      }

    void remove_at( size_t index )
      { delete c[index];
        erase(c.begin()+index);
      }

    void remove_all()
      {
      for( iterator ii=c.begin(); ii!=c.end(); ++ii )
        delete *ii;
      c.clear();
      }

	void clear_all()
	{
		c.clear();
    }

    void insert( size_t index, size_t count=1 )
      {
      c.insert(c.begin()+index, count, NULL);
      for( size_t i=0; i<count; ++i )
        c[index+i] = new PriorityLevel();
      }


  protected:
    _Container_type c;

    };

protected:

  virtual bool parse_expression( size_t priority_level, const wchar_t * & expression, Node ** ppNode );
  virtual bool parse_terminal_token( const wchar_t * & curpos, Node ** ppNode );
  virtual bool can_be_part_of_identifier( wchar_t ch );
  virtual const wchar_t * skip_white_spaces( const wchar_t * & curpos );

#ifdef USE_DEFAULT_ERROR
  virtual void set_error_msg( const wchar_t * format, ... );
#else
  virtual void set_error_id(PARSER_ERROR);
#endif
  virtual bool parse_number( T * value, const wchar_t * curpos, wchar_t ** endptr ) = 0;


  Node * Root;
  const wchar_t * pointer_to_syntax_error;
#ifdef USE_DEFAULT_ERROR
  string error_msg;
#else
  PARSER_ERROR error_id;
#endif

public:
  ArgList Arguments;
  FunctionList Functions;
  NamedConstantList NamedConstants;
  BoundParameterList BoundParameters;
  UnaryOperationTable UnaryOpTable;
  BinaryOperationTable BinaryOpTable;

  wchar_t OBRACE,CBRACE,DELIM;

public:

  MathExpressionBase();
  virtual ~MathExpressionBase();

  virtual bool parse( const wchar_t * expression );
  virtual T eval(const T _args[] = 0 ) const;
  virtual void destroy_tree();

  const wchar_t * get_error_pos() const;
#ifdef USE_DEFAULT_ERROR
  const wchar_t * get_error_msg() const;
#else
  PARSER_ERROR get_error_id() const;
#endif


public:
  void add_named_constant(const string & name, const T & value, const string & description=L"" )
    { NamedConstants.add(name,value,description);
    }
  void remove_named_constant(const string & name)
    { NamedConstants.remove(name);
    }
  void remove_all_named_constants()
    { NamedConstants.remove_all();
    }

public:
  void add_argument(const string & name, int arg_index, const string & description=L"" )
    { Arguments.add(name,arg_index,description);
    }
  void remove_all_arguments()
    { Arguments.remove_all();
    }

public:
  template<class P>
  void bind(const string & name, const P * pointer_to_value, const string & description=L"" )
    { BoundParameters.add(name,pointer_to_value,description);
    }
  void unbind(const string & name)
    { BoundParameters.remove(name);
    }
  void unbind_all()
    { BoundParameters.remove_all();
    }

public:

  template<class F, F fn>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<F,fn>(fname,description);
    }
  template<int numargs,class F>
  void add_function( const string & fname, const F & f, const string & description=L"" )
    { Functions.TEMPLATE add<numargs,F>(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, const MathExpressionBase<R> * f, const string & description=L"" )
    { Functions.add(fname,f,description);
    }

  void add_function( const string & fname, T (*f)(), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T,T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T,T,T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T,T,T,T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  void add_function( const string & fname, T (*f)(T,T,T,T,T,T,T), const string & description=L"" )
    { Functions.add(fname,f,description);
    }


  template<class R>
  void add_function( const string & fname, R (*f)(), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&,const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&,const T&,const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&,const T&,const T&,const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R>
  void add_function( const string & fname, R (*f)(const T&,const T&,const T&,const T&,const T&,const T&,const T&), const string & description=L"" )
    { Functions.add(fname,f,description);
    }


  template<class R, class P1>
  void add_function( const string & fname, R (*f)(P1), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2>
  void add_function( const string & fname, R (*f)(P1,P2), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2,class P3>
  void add_function( const string & fname, R (*f)(P1,P2,P3), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2,class P3,class P4>
  void add_function( const string & fname, R (*f)(P1,P2,P3,P4), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2,class P3,class P4,class P5>
  void add_function( const string & fname, R (*f)(P1,P2,P3,P4,P5), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2,class P3,class P4,class P5,class P6>
  void add_function( const string & fname, R (*f)(P1,P2,P3,P4,P5,P6), const string & description=L"" )
    { Functions.add(fname,f,description);
    }
  template<class R, class P1,class P2,class P3,class P4,class P5,class P6,class P7>
  void add_function( const string & fname, R (*f)(P1,P2,P3,P4,P5,P6,P7), const string & description=L"" )
    { Functions.add(fname,f,description);
    }


#ifndef _MSC_VER
  template<T(*fn)()>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(),fn>(fname,description);
    }
  template<T(*fn)(T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T),fn>(fname,description);
    }
  template<T(*fn)(T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T),fn>(fname,description);
    }
  template<T(*fn)(T,T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T,T),fn>(fname,description);
    }
  template<T(*fn)(T,T,T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T,T,T),fn>(fname,description);
    }
  template<T(*fn)(T,T,T,T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T,T,T,T),fn>(fname,description);
    }
  template<T(*fn)(T,T,T,T,T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T,T,T,T,T),fn>(fname,description);
    }
  template<T(*fn)(T,T,T,T,T,T,T)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(T,T,T,T,T,T,T),fn>(fname,description);
    }


  template<T(*fn)(const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&,const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<T(*fn)(const T&,const T&,const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<T(*)(const T&,const T&,const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }


  template<const T(*fn)()>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&,const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }
  template<const T(*fn)(const T&,const T&,const T&,const T&,const T&,const T&,const T&)>
  void add_function( const string & fname, const string & description=L"" )
    { Functions.TEMPLATE add<const T(*)(const T&,const T&,const T&,const T&,const T&,const T&,const T&),fn>(fname,description);
    }
#endif


  void remove_function(const string & name)
    { Functions.remove(name);
    }
  void remove_all_functions()
    { Functions.remove_all();
    }

public: // Most used operators

  static inline T operator_unary_minus ( T x );
  static inline T operator_unary_plus  ( T x );
  static inline T operator_multiply    ( T x, T y );
  static inline T operator_divide      ( T x, T y );
  static inline T operator_modulus     ( T x, T y );
  static inline T operator_plus        ( T x, T y );
  static inline T operator_minus       ( T x, T y );

  static inline T operator_logical_not ( T x );
  static inline T operator_logical_or  ( T x, T y );
  static inline T operator_logical_and ( T x, T y );

  static inline T operator_eq          ( T x, T y );
  static inline T operator_not_eq      ( T x, T y );
  static inline T operator_lt          ( T x, T y );
  static inline T operator_le          ( T x, T y );
  static inline T operator_gt          ( T x, T y );
  static inline T operator_ge          ( T x, T y );

public: // conditional function
  static T inline conditional_func( T cond, T x, T y );

public: // most used aggregate functions
  static inline T min( const T _args[], const NodeList & args );
  static inline T max( const T _args[], const NodeList & args );
  static inline T sum( const T _args[], const NodeList & args );
  static inline T product( const T _args[], const NodeList & args );


  };



template<class T>
MathExpressionBase<T> :: MathExpressionBase()
  : Root(0),pointer_to_syntax_error(L""),OBRACE('('),CBRACE(')'),DELIM(',')
  {
  }

template<class T>
MathExpressionBase<T> :: ~MathExpressionBase()
  { delete Root;
  }

template<class T>
const wchar_t * MathExpressionBase<T> :: get_error_pos() const
  { return pointer_to_syntax_error;
  }


#ifdef USE_DEFAULT_ERROR
template<class T>
const wchar_t * MathExpressionBase<T> :: get_error_msg() const
  { return error_msg.c_str();
  }

template<class T>
void MathExpressionBase<T> :: set_error_msg( const wchar_t * format, ... )
  {
  if( error_msg.empty() )
    {
    wchar_t cbuff[1024];
    va_list arglist;

    va_start(arglist,format);
    _vsnwprintf(cbuff,1000,format,arglist);
    va_end(arglist);

    error_msg = cbuff;
    }
  }

#else

template<class T>
PARSER_ERROR MathExpressionBase<T> :: get_error_id() const
{
	return error_id;
}

template<class T>
void MathExpressionBase<T> :: set_error_id(PARSER_ERROR pe)
{
	if (error_id == 0)
    {
		error_id = pe;
    }
  }
#endif

template<class T>
bool MathExpressionBase<T> :: parse( const wchar_t * expression )
  {
  destroy_tree();
  bool ret= parse_expression( 0, expression, &Root );
  if (ret && *expression != 0 )
    { delete Root, Root = 0;
#ifdef USE_DEFAULT_ERROR
    set_error_msg(L"unexpected text after end of expression");
#else
	set_error_id(PARSER_ERROR_UNEXPECTED);
#endif
    }
  pointer_to_syntax_error = expression;
  return ret && Root != 0;
  }

template<class T>
T MathExpressionBase<T> :: eval( const T _args[] ) const
  { assert( Root != 0 );
    return Root->eval(_args);
  }

template<class T>
void MathExpressionBase<T> :: destroy_tree()
  {
#ifdef USE_DEFAULT_ERROR
  error_msg = L"";
#else
  error_id = PARSER_ERROR_OK;
#endif

  delete Root;
  Root = 0;
  }

template<class T>
bool MathExpressionBase<T> :: parse_expression( size_t priority_level, const wchar_t * & curpos, Node ** ppNode )
  {
  /*
   * expression:
   *  arg1 binary-operation arg2 ...
   *
   * Binary operation priorities must be arranged from lowest priority at the begin of table
   * to the highest priority at the end of table
   */

  *ppNode = 0;

  if( priority_level >= BinaryOpTable.get_num_levels() )
    { return parse_terminal_token(curpos,ppNode);
    }

  const BinaryOperationNodeFactory * binop;
  Node *arg1=0, *arg2=0;

  // XXX: fix?
  const wchar_t *old_curpos = curpos;

  bool ret = parse_expression( priority_level+1, curpos, &arg1 );
  if (!ret)
    {
	  // XXX:
	  if (curpos == old_curpos || !arg1)
		  return false;
    }

  // XXX:
  old_curpos = curpos;

  while( (binop = BinaryOpTable[priority_level]->lookup( skip_white_spaces(curpos) ) ) )
    {
	  ret = true;

    curpos += binop->name.size();

    if( !parse_expression(priority_level+1, curpos, &arg2 ) )
      {
		// XXX:

		//delete arg1;
		//return false;

	    curpos = old_curpos;
		ret = false;
		break;
      }

    bool args_are_constants = (dynamic_cast<ConstValueNode * >(arg1) != 0 && dynamic_cast<ConstValueNode * >(arg2) != 0 );

    arg1 = binop->create_node( arg1, arg2 );

    if( args_are_constants )
      {
      T value = arg1->eval();
      delete arg1;
      arg1 = new ConstValueNode(value);
      }

    }

  *ppNode = arg1;
  return ret;
  }


template<class T>
bool MathExpressionBase<T> :: parse_terminal_token( const wchar_t * & curpos, Node ** ppNode )
  {
  /*
   * terminal_token:
   *  [unary-operation] <(expression)|numerical-constant|argument-name|bound-parameter-name|named-constant|function-name>
   */

  *ppNode = 0;

  /* Check if we have an unary operation */
  const UnaryOperationNodeFactory * unop = UnaryOpTable.lookup( skip_white_spaces(curpos) );
  if( unop != 0 )
    {
    if( parse_terminal_token( curpos += unop->name.size(), ppNode ) )
      {
      bool arg_is_const = dynamic_cast<ConstValueNode * >(*ppNode) != 0;

      *ppNode = unop->create_node(*ppNode);

      if( arg_is_const )
        {
		  T value = (*ppNode)->eval();
          delete (*ppNode);
          *ppNode = new ConstValueNode(value);
        }

      }
    return *ppNode != 0;
    }



  /* Check if we have an subexpression in braces */
  if( *curpos == OBRACE )
    {
    if( !parse_expression( 0, ++curpos, ppNode) )
      { return false;
      }
    if( *skip_white_spaces(curpos) != CBRACE )
      { delete *ppNode, *ppNode = 0;
#ifdef USE_DEFAULT_ERROR
        set_error_msg(L"missing '%c'",CBRACE);
#else
		set_error_id(PARSER_ERROR_MISSING_C_BRACE);
#endif
        return false;
      }
    ++curpos;
    return true;
    }


  /* Check if we have an numerical value */
  T value;
  wchar_t * endptr;
  if( parse_number(&value,curpos,&endptr) )
    { curpos = endptr;
      *ppNode = new ConstValueNode(value);
      return true;
    }



  /* Check if we have an named object */
  string name;
  const ArgNodeFactory * af = 0;
  const BoundParameterNodeFactory * pf = 0;
  const ConstValueNodeFactory * cf = 0;
  const FunctionalNodeFactory * ff = 0;

  const wchar_t * tmp = skip_white_spaces(curpos);
  while( can_be_part_of_identifier(*tmp) )
    { name += *tmp++;
    }

  if( (af = Arguments.find_by_name(name)) )
    { skip_white_spaces(curpos=tmp);
      *ppNode = af->create_node();
    }
  else if( (pf = BoundParameters.find_by_name(name)) )
    { skip_white_spaces(curpos=tmp);
      *ppNode = pf->create_node();
    }
  else if( (ff = Functions.find_by_name(name)) )
    {
      if( *skip_white_spaces(curpos=tmp) != OBRACE )
        {
		  // XXX: try consts first
		  if( (cf = NamedConstants.find_by_name(name)) )
		  {
			  skip_white_spaces(curpos=tmp);
			  *ppNode = cf->create_node();
			  return *ppNode != 0;
		  }

#ifdef USE_DEFAULT_ERROR
		  set_error_msg(L"missing '%c' in function call '%s'",OBRACE,name.c_str());
#else
		  set_error_id(PARSER_ERROR_MISSING_O_BRACE);
#endif
          return false;
        }

      NodeList args;
      Node * arg = 0;
      bool success = true;

      int numargs = ff->getnumargs();
      bool all_args_are_constants = true;

      //
      // parse argument list
      //

      if( numargs == 0 )
        { ++curpos;  // just skip the '(', as we will not enter the loop
        }
      else
        {
        while( ((int)args.size() < numargs || numargs == VARARG) && (success=parse_expression(0, ++curpos, &arg )) )
          {
          args.push_back(arg);

          if( dynamic_cast<ConstValueNode * >(arg) == 0 )
            { all_args_are_constants = false;
            }

          skip_white_spaces(curpos);

          if( ((int)args.size() < numargs || numargs == VARARG) && *curpos != DELIM )
            { break;
            }
          }
        }

      if( *skip_white_spaces(curpos) != CBRACE )
        { success = false;
#ifdef USE_DEFAULT_ERROR
        set_error_msg(L"missing '%c'",CBRACE);
#else
		set_error_id(PARSER_ERROR_MISSING_C_BRACE);
#endif
        }
      else if( ( numargs!=VARARG && (int)args.size() != numargs) )
        { success = false;
#ifdef USE_DEFAULT_ERROR
        set_error_msg(L"%s expects %d arguments, but %d provided",name.c_str(),numargs,(int)args.size());
#else
		set_error_id(PARSER_ERROR_ARGUMENTS_MISMATCH);
#endif
        }
      else if( (numargs==VARARG && args.size() < 1 ) )
        { success = false;
#ifdef USE_DEFAULT_ERROR
        set_error_msg(L"at least 1 argument is required for %s",name.c_str());
#else
		set_error_id(PARSER_ERROR_NO_ARGUMENTS);
#endif
        }

      if( !success )
        { for( size_t i=0; i<args.size(); ++i ) { delete args[i]; }
          return false;
        }

      *ppNode = ff->create_node(args, L"");

      if( all_args_are_constants /* XXX: */ && numargs > 0)
        { T value = (*ppNode)->eval();
          delete (*ppNode);
          *ppNode = new ConstValueNode(value);
        }

      ++curpos;
    }
  else if( (cf = NamedConstants.find_by_name(name)) )
  { skip_white_spaces(curpos=tmp);
  *ppNode = cf->create_node();
  }
  else if( !name.empty() )
    {
#ifdef USE_DEFAULT_ERROR
	  set_error_msg(L"unknown identifier '%s'",name.c_str());
#else
	  set_error_id(PARSER_ERROR_UNKNOWN_ID);
#endif
    }

  return *ppNode != 0;
  }

template<class T>
const wchar_t * MathExpressionBase<T> :: skip_white_spaces( const wchar_t * & curpos )
  {
  while( iswspace(*curpos) )
    ++curpos;
  return curpos;
  }

template<class T>
bool MathExpressionBase<T> :: can_be_part_of_identifier( wchar_t ch )
  { return iswalnum(ch) || ch == '_' || ch == '$';
  }

template<class T>
inline T MathExpressionBase<T> :: operator_unary_minus ( T x )
  { return -x;
  }

template<class T>
inline T MathExpressionBase<T> :: operator_unary_plus  ( T x )
  { return x;
  }

template<class T>
inline T MathExpressionBase<T> :: operator_multiply    ( T x, T y )
  { return (T)(x*y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_divide      ( T x, T y )
  { return (T)(x/y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_modulus     ( T x, T y )
  { return (T)(x%y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_plus        ( T x, T y )
  { return (T)(x+y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_minus       ( T x, T y )
  { return (T)(x-y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_logical_not ( T x )
  { return (T)(!x);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_logical_or  ( T x, T y )
  { return (T)(x || y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_logical_and ( T x, T y )
  { return (T)(x && y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_eq          ( T x, T y )
  { return (T)(x == y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_not_eq      ( T x, T y )
  { return (T)(x != y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_lt          ( T x, T y )
  { return (T)(x < y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_le          ( T x, T y )
  { return (T)(x <= y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_gt          ( T x, T y )
  { return (T)(x > y);
  }

template<class T>
inline T MathExpressionBase<T> :: operator_ge          ( T x, T y )
  { return (T)(x >= y);
  }

template<class T>
inline T MathExpressionBase<T> :: conditional_func( T cond, T x, T y )
  { return cond?x:y;
  }

template<class T>
inline T MathExpressionBase<T> :: sum( const T _args[], const NodeList & args )
  {
  typename MathExpressionBase<T> :: NodeList :: const_iterator ii = args.begin();
  T s = (*ii)->eval(_args);
  while( ++ii != args.end() )
    s += (*ii)->eval(_args);
  return s;
  }

template<class T>
inline T MathExpressionBase<T> :: product( const T _args[], const NodeList & args )
  {
  typename MathExpressionBase<T> :: NodeList :: const_iterator ii = args.begin();
  T s = (*ii)->eval(_args);
  while( ++ii != args.end() )
    s *= (*ii)->eval(_args);
  return s;
  }

template<class T>
inline T MathExpressionBase<T> :: min( const T _args[], const NodeList & args )
  {
  typename MathExpressionBase<T> :: NodeList :: const_iterator ii = args.begin();
  T s = (*ii)->eval(_args);
  while( ++ii != args.end() )
    {
    T tmp = (*ii)->eval(_args);
    if( tmp < s )
      s = tmp;
    }
  return s;
  }

template<class T>
inline T MathExpressionBase<T> :: max( const T _args[], const NodeList & args )
  {
  typename MathExpressionBase<T> :: NodeList :: const_iterator ii = args.begin();
  T s = (*ii)->eval(_args);
  while( ++ii != args.end() )
    {
    T tmp = (*ii)->eval(_args);
    if( tmp > s )
      s = tmp;
    }
  return s;
  }

#undef TEMPLATE
#endif /* __MATHEXPRESSIONBASE_H__ */
