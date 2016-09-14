#include <vcl.h>


#include <Common.h>
#include "ProgParams.h"

// unique_ptr-like class
class TProgramParamsOwner : public TObject
{
NB_DISABLE_COPY(TProgramParamsOwner)
public:
  TProgramParamsOwner() :
    FProgramParams(nullptr)
  {
  }

  ~TProgramParamsOwner()
  {
    SAFE_DESTROY(FProgramParams);
  }

  TProgramParams * Get()
  {
    if (FProgramParams == nullptr)
    {
      FProgramParams = new TProgramParams();
    }
    return FProgramParams;
  }

private:
  TProgramParams * FProgramParams;
};

// TProgramParamsOwner ProgramParamsOwner;

// TProgramParams * TProgramParams::Instance()
// {
  // return ProgramParamsOwner.Get();
// }

TProgramParams::TProgramParams()
{
  Init(L"");
}

TProgramParams::TProgramParams(const UnicodeString & CmdLine)
{
  Init(CmdLine);
}

void TProgramParams::Init(const UnicodeString & CmdLine)
{
  UnicodeString CommandLine = CmdLine;

  UnicodeString Param;
  CutToken(CommandLine, Param);
  while (CutToken(CommandLine, Param))
  {
    Add(Param);
  }
}
