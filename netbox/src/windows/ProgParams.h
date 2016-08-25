#pragma once
#define ProgParamsH

#include <Option.h>

class TProgramParams : public TOptions
{
public:
  // static TProgramParams * Instance();

  explicit TProgramParams();
  explicit TProgramParams(const UnicodeString & CmdLine);

private:
  void Init(const UnicodeString & CmdLine);
};

