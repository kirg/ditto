@if "%1" == "" ( set out="d.exe" ) else set out="%1"
cl -o %out% -mconsole -Wall -DUNICODE *.c -lntdll

c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin
