## Legacy code
In case you're doing minor changes in existing legacy code - use existing code style as you see in that code.   
Otherwise follow guidelines below.

## Indentation and alignment:
Code block indentation and alignment of hyphenated continuations - tabs.   
Alignment of trailing comments - also tabs.   
All other alignment in the middle of text line (if need) - spaces.   

## Spaces and parenthesis:
``` Examples:
// No space between parenthesis and function name, no space in empty arguments list:
void FunctionWithoutArgs();

// Spaces between arguments, but no spaces between parenthesis and contained argument:
void ConstFunctionWithArgs(int i, std::string s) const;

// Spaces between expression elements and after if, no spaces between parenthesis and contained stuff:
if (condition1 && (condition2 || condition3)) {
}

// Space between try and opening brace, spaces around catch:
try {
} catch (std::exception &) {
}
int arr[] = {1, 2, 3};      // - space both sides of equal sign here
int i{};                    // - no space between variable and list initializer's brace
i = arr[1];                 // - simple assignment - space surrounds both sides of equal sign
i+= arr[2];                 // - incremental assignment - space only on the right
void *ptr_i = (void *)&i;   // - pointer: space between target type and asterisk
int &ref_i = i;             // - reference: space between target type and amperans

// Complex ternary expressions should have parenthesis to segregate things:
int ternary_result1 = simple_condition ? 1 : 2;
int ternary_result2 = (complex && condition) ? 1 : 2;
int ternary_result3 = simple_condition ? ((complex && condition) ? 1 : 2) : ((complex && condition) ? 3 : 4);
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
	SomeCode();
}
```

## Naming:
Use CamelCase for name of enums, namespaces, classes, structures, functions and methods   
Use snake_case for all variables, however:   
&nbsp;&nbsp; Private and protected class's fields - prefix by '\_'   
&nbsp;&nbsp; Static variables - prefix by 's\_'   
&nbsp;&nbsp; Global nonstatic variables - prefix by 'g\_'   
Use UPPER_CASE_WITH_UNDERSCORES for macroses, values of enum-s.   
Additionally values of enums must be prefixes with an abbreviation of corresponding enum's name.   
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
	ClassT TemplateFunction(int arg)
{
	return ClassT(arg + VALUE_T);
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
