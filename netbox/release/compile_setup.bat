SET ISCC="C:\Program Files\Inno Setup 5\ISCC.exe"
if exist %ISCC% (
  %ISCC% /v3 netboxsetup.iss
  if errorlevel 1 echo Error creating distributive & exit 1 /b
)
