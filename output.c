#include <stdio.h>
#include <stdlib.h>
int main() {
char pycode_1[256] = "print('Hello from Python')\n";
FILE *file_script_py_2 = fopen("script.py", "w+");
if (!file_script_py_2) { printf("Error opening file script.py\n"); return 1; }
fprintf(file_script_py_2, "%s\n", pycode_1);
fflush(file_script_py_2);
fclose(file_script_py_2);
printf("Python script generated as script.py\n");
return 0;
}
