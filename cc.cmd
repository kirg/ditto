@if "%1" == "" ( set out="d.exe" ) else set out="%1"
gcc -o %out% -mconsole -Wall -DUNICODE *.c -lntdll
