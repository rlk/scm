#include <stdio.h>

int main(int argc, char **argv)
{
    int n = 0;
    int c;

    if (argc > 1)
        printf("const char %s[] = {\n", argv[1]);

    do
    {
        printf("\t");

        while ((c = getchar()) != EOF)
        {
            printf("0x%02x, ", c);
            if (++n % 12 == 0)
                break;
        }

        printf("\n");
    }
    while (c != EOF);

    if (argc > 1)
        printf("};\nconst int %s_len = %d;\n", argv[1], n);

    return 0;
}
