#pragma once


// Some utility templates for emulating properties --
// preferring a library solution to a new language feature
// Each property has three sets of redundant accessors:
// 1. function call syntax
// 2. get() and set() functions
// 3. overloaded operator =
// a read-write property with data store and
// automatically generated get/set functions.
// this is what C++/CLI calls a trivial scalar property
template <class T>
class Property
{
  T data;
public:
  // access with function call syntax
  Property() : data() { }
  T operator()() const
  {
    return data;
  }
  T operator()(T Value)
  {
    data = Value;
    return data;
  }
  // access with get()/set() syntax
  T get() const
  {
    return data;
  }
  T set(T Value)
  {
    data = Value;
    return data;
  }
  // access with '=' sign
  // in an industrial-strength library,
  // specializations for appropriate types might choose to
  // add combined operators like +=, etc.
  operator T() const
  {
    return data;
  }
  void operator = (T Value)
  {
    data = Value;
  }
  typedef T value_type; // might be useful for template deductions
};

// a read-only property calling a user-defined getter
template <
  class T,
  class Object,
  T (Object::*real_getter)()
  >
class ROProperty
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in a
  // constructor, to initialize the ROProperty so it knows where its
  // real implementation code can be found. obj is usually the containing
  // class, but need not be; it could be a special implementation object.
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  T operator()() const
  {
    return (my_object->*real_getter)();
  }
  // get/set syntax
  T get() const
  {
    return (my_object->*real_getter)();
  }
  void set(T Value);   // reserved but not implemented, per C++/CLI
  // use on rhs of '='
  operator T() const
  {
    return (my_object->*real_getter)();
  }
  typedef T value_type; // might be useful for template deductions
};

// a write-only property calling a user-defined setter
template <
  class T,
  class Object,
  void (Object::*real_setter)(T)
  >
class WOProperty
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in
  // a constructor, to initialize the WOProperty so it knows where its
  // real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  T operator()(T Value)
  {
    return (my_object->*real_setter)(Value);
  }
  // get/set syntax
  T get() const; // name reserved but not implemented per C++/CLI
  void set(T Value)
  {
    void (my_object->*real_setter)(Value);
  }
  // access with '=' sign
  void operator = (T Value)
  {
    (my_object->*real_setter)(Value);
  }
  typedef T value_type; // might be useful for template deductions
};

// a read-write property which invokes user-defined functions
template <
  class T,
  class Object,
  T (Object::*real_getter)(),
  void (Object::*real_setter)(T)
  >
class RWProperty
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in a
  // constructor, to initialize the ROProperty so it knows where its
  // real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  T operator()() const
  {
    return (my_object->*real_getter)();
  }
  void operator()(T Value)
  {
    (my_object->*real_setter)(Value);
  }
  // get/set syntax
  T get() const
  {
    return (my_object->*real_getter)();
  }
  void set(T Value)
  {
    return (my_object->*real_setter)(Value);
  }
  // access with '=' sign
  operator T() const
  {
    return (my_object->*real_getter)();
  }
  void operator = (T Value)
  {
    (my_object->*real_setter)(Value);
  }
  typedef T value_type; // might be useful for template deductions
};

// a read/write property providing indexed access.
// this class simply encapsulates a std::map and changes its interface
// to functions consistent with the other property<> classes.
// note that the interface combines certain limitations of std::map with
// some others from indexed properties as I understand them.
// an example of the first is that operator[] on a map will insert a
// key/Value pair if it isn't already there. A consequence of this is that
// it can't be a const member function (and therefore you cannot access
// a const map using operator [].)
// an example of the second is that indexed properties do not appear
// to have any facility for erasing key/value pairs from the container.
// C++/CLI properties can have multi-dimensional indexes: prop[2,3]. This is
// not allowed by the current rules of standard C++

template <
  class Key,
  class T,
  class Object,
  T (Object::*real_getter)(Key),
  void (Object::*real_setter)(Key, T)
  >
class IndexedProperty
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in a
  // constructor, to initialize the ROProperty so it knows where its
  // real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  T operator()(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void operator()(Key AKey, T AValue)
  {
     (my_object->*real_setter)(AKey, AValue);
  }
  // get/set syntax
  T get_Item(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void set_Item(Key AKey, T AValue)
  {
    (my_object->*real_setter)(AKey, AValue);
  }
  // operator [] syntax
  T operator[](Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
};

template <
  class Key,
  class T,
  class Object,
  T & (Object::*real_getter)(Key),
  void (Object::*real_setter)(Key, T)
  >
class IndexedProperty2
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in a
  // constructor, to initialize the ROProperty so it knows where its
  // real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  T & operator()(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void operator()(Key AKey, T AValue)
  {
     (my_object->*real_setter)(AKey, AValue);
  }
  // get/set syntax
  T & get_Item(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void set_Item(Key AKey, T AValue)
  {
    (my_object->*real_setter)(AKey, AValue);
  }
  // operator [] syntax
  T & operator[](Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
};

template <
  class Key,
  class Object,
  void *& (Object::*real_getter)(Key),
  void (Object::*real_setter)(Key, void *)
  >
class IndexedPropertyVoid
{
  Object * my_object;
public:
  // this function must be called by the containing class, normally in a
  // constructor, to initialize the ROProperty so it knows where its
  // real implementation code can be found
  void operator()(Object * obj)
  {
    my_object = obj;
  }
  // function call syntax
  void *& operator()(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void operator()(Key AKey, void * AValue)
  {
     (my_object->*real_setter)(AKey, AValue);
  }
  // get/set syntax
  void *& get_Item(Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
  void set_Item(Key AKey, void * AValue)
  {
    (my_object->*real_setter)(AKey, AValue);
  }
  // operator [] syntax
  void *& operator[](Key AKey)
  {
    return (my_object->*real_getter)(AKey);
  }
};



