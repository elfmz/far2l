#pragma once
#include <assert.h>
#include <stdlib.h>

/**
Lazy-allocated gready-freed data fields helper.

Synopsys:
	struct YourClass : EcoFields<YourClass>
	{
		struct Fields
		{
			int i{0};
			std::string s;

			bool IsDefault() const {return i == 0 && s.emtpy();}
		};

		void Foo()
		{
			MyEcoFields my(this);
			printf("Before Foo: %d %s\n", my->i, my->s.c_str();
			my->i = 123;
			my->s = "hello";
		}

		void Bar()
		{
			MyEcoFields my(this);
			printf("Before Bar: %d %s\n", my->i, my->s.c_str();
			my->i = 0;
			my->s = "";
		}
	};

Description:
	Each time MyEcoFields used in a function which has no Fields yet - instance of Fields created on stack,
	it associated with object instance for potential recursive calls and when last MyEcoFields goes out of scope:
	if Fields::IsDefault() evaluated to true - then fields copied into on-heap instance and persisted til next use,
	if Fields::IsDefault() evaluated to false - then no memory used as its epeheral stack instance just eliminated.
*/


template <class OwnerT>
	class EcoFields
{
	friend class MyEcoFields;

	class FieldsContainer : public OwnerT::Fields
	{
		friend class EcoFields<OwnerT>::MyEcoFields;
		unsigned int _refs{0};
	} mutable *_my_ecofields{nullptr};

public:
	virtual ~EcoFields()
	{
		delete _my_ecofields;
	}

	class MyEcoFields
	{
		char _placement[sizeof(FieldsContainer)];
		const OwnerT *_owner;

	public:
		MyEcoFields(const OwnerT *owner) : _owner(owner)
		{
			if (!owner->_my_ecofields) {
				owner->_my_ecofields = new (&_placement[0]) FieldsContainer;
			}
			owner->_my_ecofields->_refs++;
			assert(_owner->_my_ecofields->_refs != 0); // if trapped - likely there is infinite recursion
		}

		~MyEcoFields()
		{
			_owner->_my_ecofields->_refs--;
			if (_owner->_my_ecofields->_refs == 0) {
				if ((char *)_owner->_my_ecofields != &_placement[0]) { // allocated on heap - release memory
					if (_owner->_my_ecofields->IsDefault()) {
						delete _owner->_my_ecofields;
						_owner->_my_ecofields = nullptr;
					}
				} else if (_owner->_my_ecofields->IsDefault()) {
					_owner->_my_ecofields->~FieldsContainer();
					_owner->_my_ecofields = nullptr;
				} else {
					auto *my_on_heap = new (std::nothrow) FieldsContainer(std::move(*_owner->_my_ecofields));
					_owner->_my_ecofields->~FieldsContainer();
					_owner->_my_ecofields = my_on_heap;
					if (UNLIKELY(!my_on_heap)) {
						fprintf(stderr, "EcoFields: no memory - state lost\n");
					}
				}
			}
		}

		typename OwnerT::Fields *operator ->()
		{
			return _owner->_my_ecofields;
		}
	};
};
