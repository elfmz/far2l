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

		EcoLazy<Fields> fields;

		void Foo()
		{
			EcoLazy<Fields>::Use my(fields);
			printf("Before Foo: %d %s\n", my->i, my->s.c_str();
			my->i = 123;
			my->s = "hello";
		}

		void Bar()
		{
			EcoLazy<Fields>::Use my(fields);
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
	class EcoLazy
{
	struct Container : FieldsT
	{
		const void *root_use;
		Container(const void *root_use_) : root_use(root_use_) {}

		Container(Container&&) noexcept = default;

		Container() = delete;
		Container(const Container&) = delete;
		Container &operator =(const Container&) = delete;

	} mutable *_container{nullptr};

public:
	~EcoLazy()
	{
		if (_container) {
			assert(!_container->root_use); // should not be in use
			delete _container;
		}
	}

	class Use
	{
		char _placement[sizeof(Container)];
		const EcoLazy &_fields;

		Use() = delete;
		Use(const Use&) = delete;
		Use &operator =(const Use&) = delete;

	public:
		Use(const EcoLazy &fields) : _fields(fields)
		{
			if (!_fields._container) {
				_fields._container = new (&_placement[0]) Container(&_placement[0]);
			} else if (!_fields._container->root_use) {
				_fields._container->root_use = &_placement[0];
			}
		}

		~Use()
		{
			if ((char *)_fields._container == &_placement[0]) {
				// was allocated on stack by *this - check if content is matched to default
				// and if so - forget it, otherwise - allocate and keep heap-backed copy
				if (_fields._container->IsDefault()) { // forget this ephemeral stack instance
					_fields._container->~Container();
					_fields._container = nullptr;
				} else { // allocate copy on heap and remember pointer to it for following uses
					auto *hc = new (std::nothrow) Container(std::move(*_fields._container));
					_fields._container->~Container();
					_fields._container = hc;
					if (LIKELY(hc)) {
						hc->root_use = nullptr;
					} else {
						fprintf(stderr, "EcoLazy: no memory - state lost\n");
					}
				}
			} else if (_fields._container->root_use == &_placement[0]) {
				// was allocated on heap, however *this is last user on backtrace - so again check
				// if content is matched to default and if so - release this heap-backed copy
				if (_fields._container->IsDefault()) {
					delete _fields._container;
					_fields._container = nullptr;
				} else {
					_fields._container->root_use = nullptr;
				}
			}
		}

		FieldsT *operator ->()
		{
			return _fields._container;
		}
	};
};
