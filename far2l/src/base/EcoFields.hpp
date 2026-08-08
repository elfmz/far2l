#pragma once
#include <assert.h>
#include <stdlib.h>

/**
Lazy-allocated gready-freed data fields helper.

Synopsys:
	struct YourClass
	{
		struct Fields
		{
			int i{0};
			std::string s;

			bool IsDefault() const {return i == 0 && s.emtpy();}
		};

		EcoFields<Fields> fields;

		void Foo()
		{
			EcoFields<Fields>::Use my(fields);
			printf("Before Foo: %d %s\n", my->i, my->s.c_str();
			my->i = 123;
			my->s = "hello";
		}

		void Bar()
		{
			EcoFields<Fields>::Use my(fields);
			printf("Before Bar: %d %s\n", my->i, my->s.c_str();
			my->i = 0;
			my->s = "";
		}
	};

Description:
	Each time Use used in a function which has no Fields yet - instance of Fields created on stack,
	it associated with object instance for potential recursive calls and when last Use goes out of scope:
	if Fields::IsDefault() evaluated to true - then fields copied into on-heap instance and persisted til next use,
	if Fields::IsDefault() evaluated to false - then no memory used as its epeheral stack instance just eliminated.
*/


template <class FieldsT>
	class EcoFields
{
	friend class Use;

	class Container : public FieldsT
	{
		friend class EcoFields<FieldsT>::Use;
		unsigned int _refs{0};
	} mutable *_container{nullptr};

public:
	~EcoFields()
	{
		delete _container;
	}

	class Use
	{
		char _placement[sizeof(Container)];
		const EcoFields &_fields;

	public:
		Use(const EcoFields &fields) : _fields(fields)
		{
			if (!fields._container) {
				fields._container = new (&_placement[0]) Container;
			}
			fields._container->_refs++;
			assert(fields._container->_refs != 0); // if trapped - likely there is infinite recursion
		}

		~Use()
		{
			_fields._container->_refs--;
			if (_fields._container->_refs == 0) {
				if ((char *)_fields._container != &_placement[0]) { // allocated on heap - release memory
					if (_fields._container->IsDefault()) {
						delete _fields._container;
						_fields._container = nullptr;
					}
				} else if (_fields._container->IsDefault()) {
					_fields._container->~Container();
					_fields._container = nullptr;
				} else {
					auto *my_on_heap = new (std::nothrow) Container(std::move(*_fields._container));
					_fields._container->~Container();
					_fields._container = my_on_heap;
					if (UNLIKELY(!my_on_heap)) {
						fprintf(stderr, "EcoFields: no memory - state lost\n");
					}
				}
			}
		}

		FieldsT *operator ->()
		{
			return _fields._container;
		}
	};
};
