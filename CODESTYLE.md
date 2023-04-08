## Indentation and alignment:
Code block indentation and alignment of hyphenated continuations - tabs.   
Alignment of trailing comments - also tabs.   
All other alignment in the middle of text line (if need) - spaces.   

## Spaces and ellipses:
``` Examples:
void FunctionWithoutArgs();
void ConstFunctionWithArgs(int i, std::string s) const;
if (condition1 && condition2) {
}
try {
} catch (std::exception &) {
}
int i {};
i = 1;
i+= 2;
void *p_i = (void *)&i;
int &ref_i = i;
int ternary_result = (complex && condition) ? 1 : 2;
```

## Line length limit:
110 characters, buts its a soft limit, i.e. prefer better readability more than fiting limit.   
Trailing comments are not affected by this limit at all.   

## Code braces:
Use direct (same line) braces for class-es/struct-s/union-s, functions, namespaces   
Use egyptian braces for if-s, loop-s, try..catch-es, lambda-s   
In case of if..elseif.. with long complex code within block - use empty line to accent end of code block.   
In case code-block-start operator has short own condition and has as child single another code-block-starter, you may put that secondary operator at same line as first one and use single indentation for its code block.   
In case of very short inlined class methods - you may write method definition's code block as single line.   
In all other cases put any nested operator and its code block starting from separate line and with its own indentation level.   
``` Examples:
namespace Foo
{
	struct Bar
	{
		int Short() const { return _result; }

		void Baz() const
		{
			if (short_cond) try {
				SomeCode();
			} catch (...) {
			}

			if (long && complicated && condition) {
				try {
					SomeCode();
				} catch (...) {
				}
			}
		}

		void Qux()
		{
			if (cond1) {
				SomeShortCode();
			} else if (cond2) {
				SomeShortCode();
			} else {
				SomeShortCode();
			}

			if (cond1) {
				Some();
				Long();
				Code();

			} else if (cond2) {
				Some();
				Long();
				Code();

			} else {
				Some();
				Long();
				Code();
			}
		}
	};
};
```

## Hyphenated continuations:
Indent second line of condition using two tabs to separate it from code block.
Put hyphenated operators on new line's beginning.
``` Example:
if (i == 10
		|| i == 20
		|| i == 30) {
}
```

## Naming:
In case you're changing existing code - follow existing naming conventions as you see them in that code. Otherwise:   
Use CamelCase for name of enums, namespaces, classes, structures, functions and methods   
Use snake_case for all variables, however:   
&nbsp;&nbsp; Private and protected class's fields - prefix by '\_'   
&nbsp;&nbsp; Static variables - prefix by 's\_'   
&nbsp;&nbsp; Global nonstatic variables - prefix by 'g\_'   
Use UPPER_CASE_WITH_UNDERSCORES for macroses, values of enum-s.   
Additionally values of enums must be prefixes with a abbreviation of corresponding enum's name.   
Templates:   
&nbsp;&nbsp; For template arguments that represent type name - use CamelCaseT (camel case with T suffix).   
&nbsp;&nbsp; For template arguments that typed constant value - use UPPER_CASE_WITH_UNDERSCORES_T.   
&nbsp;&nbsp; If template function represents 'internal' implementation for some nearby non-templated functions - you may add T suffix for its name to clearly denote this.   
``` Examples:
enum SomeEnum
{
	SE_ZERO = 0,
	SE_ONE  = 1,
	SE_TWO  = 2
};

int g_counter = 0;

class FooBar
{
	int _private_field;

public:
	void CallMe(SomeEnum argument)
	{
		static int s_static_var;
		int local_var = argument + _private_field;
		s_static_var = std::max(local_var, s_static_var);
		++g_counter;
	}
};

template < class ClassT, int VALUE_T >
	VALUE_T TemplateFunction(ClassT arg)
{
	...
}

```

## class/struct:
Use struct if your class exposes some public data fields or/and only public methods otherwise prefer using class.

## class/struct layouts:
```
class FooBar
{
	int _private_field;
	void PrivateMethod();

protected:
	int _protected_field;
	void ProtectedMethod();

public:
	void PublicMethod();
};

struct FooBar
{
	int public_field;
	void PublicMethod();
};
```


## File naming:
Use CamelCase.cpp unless you're adding file into a directory that already have lots of files with different naming convention. In such case follow existing conventions.
